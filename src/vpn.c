/*
 * vpn.c
 *
 * Copyright (C) 2014 - 2015, Xiaoxiao <i@xiaoxiao.im>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#include "conf.h"
#include "compress.h"
#include "crypto.h"
#include "uthash.h"
#include "log.h"
#include "timer.h"
#include "tunif.h"
#include "utils.h"
#include "vpn.h"

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

static const conf_t *conf;

static volatile int running;
static int tun;
static int sock;
static struct
{
    struct sockaddr_storage addr;
    socklen_t addrlen;
} remote;


// 待发送的 ack
#define ACK_LEN 256
static struct
{
    int count;
    uint32_t ack[ACK_LEN];
} ack;

// 已发送，未收到 ack
typedef struct sent_t
{
    uint32_t id;    // hash 表的 key
    uint64_t stime; // 最近一次发送时间戳
    pbuf_t pbuf;
    UT_hash_handle hh;
} sent_t;
sent_t *sent = NULL;


// 计算 RTO 的数据
int srtt = 0;
int devrtt = 0;
int rto = 100;


static void tun_cb(pbuf_t *pbuf);
static void udp_cb(pbuf_t *pbuf);
static void heartbeat(void);
static void copypkt(pbuf_t *pbuf, const pbuf_t *src);
static void sendpkt(pbuf_t *buf, int times);
static int  is_dup(uint32_t chksum);
static void flushack(void);
static void acknowledge(uint32_t chksum);
static void retransmit(void);


int vpn_init(const conf_t *config)
{
    conf = config;

    LOG("starting sipvpn %s", (conf->mode == MODE_SERVER) ? "server" : "client");

    // 初始化 lzo
    if (compress_init() != 0)
    {
        return -1;
    }

    // 初始化 UDP socket
    struct addrinfo hints;
    struct addrinfo *res;
    bzero(&hints, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
    if (getaddrinfo(conf->server, conf->port, &hints, &res) != 0)
    {
        ERROR("getaddrinfo");
        return -1;
    }
    sock = socket(res->ai_family, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0)
    {
        ERROR("socket");
        freeaddrinfo(res);
        return -1;
    }
    setnonblock(sock);
    if (conf->mode == MODE_CLIENT)
    {
        // client
        memcpy(&remote.addr, res->ai_addr, res->ai_addrlen);
        remote.addrlen = res->ai_addrlen;
    }
    else
    {
        // server
        if (bind(sock, res->ai_addr, res->ai_addrlen) != 0)
        {
            ERROR("bind");
            close(sock);
            freeaddrinfo(res);
            return -1;
        }
    }
    freeaddrinfo(res);

    // 初始化 tun 设备
    tun = tun_new(conf->tunif);
    if (tun < 0)
    {
        LOG("failed to init tun device");
        return -1;
    }
    LOG("using tun device: %s", conf->tunif);

    // 配置 IP 地址
#ifdef TARGET_LINUX
    if (ifconfig(conf->tunif, conf->mtu, conf->address, conf->address6) != 0)
    {
        LOG("failed to add address on tun device");
    }
#endif
#ifdef TARGET_DARWIN
    if (ifconfig(conf->tunif, conf->mtu, conf->address, conf->peer, conf->address6) != 0)
    {
        LOG("failed to add address on tun device");
    }
#endif

    if (conf->mode == MODE_CLIENT)
    {
        if (conf->route)
        {
            // 配置路由表
            if (route(conf->tunif, conf->server, conf->address[0], conf->address6[0]) != 0)
            {
                LOG("failed to setup route");
            }
        }
    }
    else
    {
#ifdef TARGET_DARWIN
        LOG("server mode is not supported on Mac OS X");
        return -1;
#endif
#ifdef TARGET_LINUX
        if ((conf->nat) && (conf->address[0] != '\0'))
        {
            // 配置 NAT
            if (nat(conf->address, 1))
            {
                LOG("failed to turn on NAT");
            }
        }
#endif
    }

    // drop root privilege
    if (conf->user[0] != '\0')
    {
        if (runas(conf->user) != 0)
        {
            ERROR("runas");
        }
    }

    return 0;
}


int vpn_run(void)
{
    pbuf_t pbuf;
    fd_set readset;

    // keepalive
    if ((conf->mode == MODE_CLIENT) && (conf->keepalive != 0))
    {
        timer_set(heartbeat, conf->keepalive * 1000);
    }

    // ack timer, 5ms
    timer_set(flushack, 5);

    // retransmit timer, 10ms
    timer_set(retransmit, 10);

    running = 1;
    while (running)
    {
        FD_ZERO(&readset);
        FD_SET(tun, &readset);
        FD_SET(sock, &readset);

        // 10 ms
        struct timeval timeout = {.tv_sec = 0, .tv_usec = 10 * 1000};
        int r = select((tun>sock?tun:sock) + 1, &readset, NULL, NULL, &timeout);
        timer_tick();

        if (r == 0)
        {
            continue;
        }

        if (r < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            else
            {
                ERROR("select");
                break;
            }
        }

        if (FD_ISSET(tun, &readset))
        {
            tun_cb(&pbuf);
        }

        if (FD_ISSET(sock, &readset))
        {
            udp_cb(&pbuf);
        }
    }

    // regain root privilege
    if (conf->user[0] != '\0')
    {
        if (runas("root") != 0)
        {
            ERROR("runas");
        }
    }

    // 关闭 NAT
#ifdef TARGET_LINUX
    if ((conf->mode == MODE_SERVER) && (conf->nat))
    {
        if (nat(conf->address, 0))
        {
            LOG("failed to turn off NAT");
        }
    }
#endif

    // 关闭 tun 设备
    tun_close(tun);
    LOG("close tun device");

    LOG("exit");
    return (running == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}


void vpn_stop(void)
{
    running = 0;
}


static void tun_cb(pbuf_t *pbuf)
{
    // 从 tun 设备读取 IP 包
    ssize_t n = tun_read(tun, pbuf->payload, conf->mtu);
    if (n <= 0)
    {
        if (n < 0)
        {
            ERROR("tun_read");
        }
        return;
    }
    pbuf->len = (uint16_t)n;

    // 取一个待发送的 ack
    if (ack.count != 0)
    {
        pbuf->flag = 0x0001;
        pbuf->ack = ack.ack[ack.count - 1];
        ack.count--;
    }
    else
    {
        pbuf->flag = 0x0000;
    }

    // 压缩
    compress(pbuf);

    // 计算 hash
    crypto_hash(pbuf);

    // 添加到 sent 中
    sent_t *s;
    HASH_FIND_INT(sent, &(pbuf->chksum), s);
    if (s == NULL)
    {
        s = (sent_t *)malloc(sizeof(sent_t));
        if (s == NULL)
        {
            ERROR("out of memory");
            return;
        }
        s->id = pbuf->chksum;
        s->stime = timer_now();
        HASH_ADD_INT(sent, id, s);
        copypkt(&(s->pbuf), pbuf);
    }

    // 发送到 remote
    sendpkt(pbuf, conf->duplicate > 1 ? conf->duplicate : 1);
}


static void udp_cb(pbuf_t *pbuf)
{
    // 读取 UDP 包
    struct sockaddr_storage addr;
    socklen_t addrlen;
    ssize_t n;
    if (conf->mode == MODE_CLIENT)
    {
        // client
        n = recvfrom(sock, pbuf, conf->mtu + PAYLOAD_OFFSET, 0, NULL, NULL);
    }
    else
    {
        // server
        addrlen = sizeof(struct sockaddr_storage);
        n = recvfrom(sock, pbuf, conf->mtu + PAYLOAD_OFFSET, 0,
                     (struct sockaddr *)&addr, &addrlen);
    }
    if (n < PAYLOAD_OFFSET)
    {
        if (n < 0)
        {
            ERROR("recvfrom");
        }
        return;
    }

    // 解密
    if (crypto_decrypt(pbuf, n) != 0)
    {
        // invalid packet
        if (conf->mode == MODE_CLIENT)
        {
            LOG("invalid packet, drop");
        }
        else
        {
            char host[INET6_ADDRSTRLEN];
            char port[8];
            if (addr.ss_family == AF_INET)
            {
                inet_ntop(AF_INET, &(((struct sockaddr_in *)&addr)->sin_addr),
                          host, INET_ADDRSTRLEN);
                sprintf(port, "%u", ntohs(((struct sockaddr_in *)&addr)->sin_port));
            }
            else
            {
                inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)&addr)->sin6_addr),
                          host, INET6_ADDRSTRLEN);
                sprintf(port, "%u", ntohs(((struct sockaddr_in6 *)&addr)->sin6_port));
            }
            LOG("invalid packet from %s:%s, drop", host, port);
        }
        return;
    }

    // 心跳包
    if (pbuf->len == 0)
    {
        if (conf->mode == MODE_CLIENT)
        {
            heartbeat();
        }
        return;
    }

    // 过滤重复的包
    if (is_dup(pbuf->chksum))
    {
        return;
    }

    // 解压缩
    decompress(pbuf);

    // ack 包
    if (pbuf->flag & 0x0002)
    {
        uint32_t *chksum = (uint32_t *)(pbuf->payload);
        for (int i = 0; i < (pbuf->len / 4); i++)
        {
            acknowledge(chksum[i]);
        }
        return;
    }

    if (pbuf->flag & 0x0001)
    {
        // ack flag 为 1，说明附带了一个 ack
        acknowledge(pbuf->ack);
    }

    // 添加到 ack 队列
    if (ack.count == ACK_LEN)
    {
        // ack 队列满，发送一个 ack 包并清空 ack 队列
        flushack();
    }
    ack.ack[ack.count] = pbuf->chksum;
    ack.count++;

    // 写入到 tun 设备
    n = tun_write(tun, pbuf->payload, pbuf->len);
    if (n < 0)
    {
        ERROR("tun_write");
    }

    // 更新 remote address
    if (conf->mode == MODE_SERVER)
    {
        memcpy(&(remote.addr), &addr, addrlen);
        remote.addrlen = addrlen;
    }
}


// 判断一个包是否重复
#define DUP_LEN 1021
static int is_dup(uint32_t chksum)
{
    static uint32_t hash[DUP_LEN][2];

    int h = (int)(chksum % DUP_LEN);
    int dup = (hash[h][0] == chksum) || (hash[h][1] == chksum);

    hash[h][1] = hash[h][0];
    hash[h][0] = chksum;
    return dup;
}


// 发送 ack 包
static void flushack(void)
{
    if (ack.count != 0)
    {
        pbuf_t pkt;
        pkt.len = (uint16_t)(ack.count * 4);
        memcpy(pkt.payload, ack.ack, pkt.len);
        pkt.flag = 0x0002;
        compress(&pkt);
        crypto_hash(&pkt);
        sendpkt(&pkt, 3);
    }
    ack.count = 0;
}


// acknowledge
#define ABS(x) ((x) > 0 ? (x) : -(x))
static void acknowledge(uint32_t chksum)
{
    sent_t *s;
    HASH_FIND_INT(sent, &chksum, s);
    if (s != NULL)
    {
        // SRTT = SRTT + α (RTT – SRTT)
        // DevRTT = (1-β)*DevRTT + β*(|RTT-SRTT|)
        // RTO= µ * SRTT + ∂ *DevRTT
        int rtt = (int)(timer_now() - s->stime);
        srtt = srtt + (rtt - srtt) / 8;
        devrtt = devrtt * 3 / 4 + ABS(rtt - srtt) / 4;
        rto = srtt + devrtt * 5 / 4;
        if (rto < 30)
        {
            rto = 30;
        }
        HASH_DEL(sent, s);
        free(s);
    }
}


// retransmit
static void retransmit(void)
{
    uint64_t now = timer_now();
    sent_t *s, *tmp;
    HASH_ITER(hh, sent, s, tmp)
    {
        uint64_t diff = now - s->stime;
        if (diff > (uint64_t)rto)
        {
            sendpkt(&(s->pbuf), 3);
            HASH_DEL(sent, s);
            free(s);
        }
    }
}


// 复制数据包
static void copypkt(pbuf_t *dest, const pbuf_t *src)
{
    dest->chksum = src->chksum;
    dest->ack = src->ack;
    dest->flag = src->flag;
    dest->len = src->len;
    memcpy(dest->payload, src->payload, src->len);
}


// 发送心跳包
static void heartbeat(void)
{
    srand((unsigned)timer_now());
    pbuf_t pkt;
    pkt.len = 0;
    crypto_hash(&pkt);
    sendpkt(&pkt, 1);
}


// naïve obfuscation
static void obfuscate(pbuf_t *pbuf)
{
    // nonce = rand()[0:8]
    for (int i = 0; i < 8; i++)
    {
        pbuf->nonce[i] = (uint8_t)(rand() & 0xff);
    }
    // random padding
    if (pbuf->len < conf->mtu)
    {
        int max = conf->mtu - pbuf->len;
        if (max > 897)
        {
            pbuf->padding = rand() % 554;
        }
        else if (max > 554)
        {
            pbuf->padding = rand() % 342;
        }
        else if (max > 342)
        {
            pbuf->padding = rand() % 211;
        }
        else
        {
            pbuf->padding = rand() % 130;
            if (pbuf->padding > max)
            {
                pbuf->padding = max;
            }
        }
        for (int i = (int)(pbuf->len); i < (int)(pbuf->len) + pbuf->padding; i++)
        {
            pbuf->payload[i] = (uint8_t)(rand() & 0xff);
        }
    }
    else
    {
        pbuf->padding = 0;
    }
}


// 发送数据包
static void _sendpkt(pbuf_t *pbuf)
{
    // 混淆
    obfuscate(pbuf);
    // 加密
    ssize_t n = PAYLOAD_OFFSET + pbuf->len + pbuf->padding;
    crypto_encrypt(pbuf);
    n = sendto(sock, pbuf, n, 0, (struct sockaddr *)&(remote.addr), remote.addrlen);
    if (n < 0)
    {
        ERROR("sendto");
    }
}

static void sendpkt(pbuf_t *pbuf, int times)
{
    if (remote.addrlen != 0)
    {
        if (times > 1)
        {
            pbuf_t tmp;
            while (times > 1)
            {
                copypkt(&tmp, pbuf);
                _sendpkt(&tmp);
                times--;
            }
        }
        _sendpkt(pbuf);
    }
}
