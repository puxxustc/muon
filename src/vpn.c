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
#include "crypto.h"
#include "log.h"
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


static void tun_cb(pbuf_t *pbuf);
static void udp_cb(pbuf_t *pbuf);
static void heartbeat(pbuf_t *pbuf);
static void copypkt(pbuf_t *pbuf, const pbuf_t *src);
static void sendpkt(pbuf_t *buf);
static int  is_dup(uint32_t chksum);


int vpn_init(const conf_t *config)
{
    conf = config;

    LOG("starting sipvpn %s", (conf->mode == server) ? "server" : "client");

    // set crypto key
    crypto_init(conf->key);

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
    if (conf->mode == client)
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

    if (conf->mode == client)
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

    running = 1;
    while (running)
    {
        FD_ZERO(&readset);
        FD_SET(tun, &readset);
        FD_SET(sock, &readset);

        int r;
        if ((conf->mode == client) && (conf->keepalive != 0))
        {
            struct timeval timeout;
            timeout.tv_sec = conf->keepalive;
            timeout.tv_usec = 0;
            r = select((tun>sock?tun:sock) + 1, &readset, NULL, NULL, &timeout);
        }
        else
        {
            r = select((tun>sock?tun:sock) + 1, &readset, NULL, NULL, NULL);
        }
        if (r == 0)
        {
            heartbeat(&pbuf);
        }
        else if (r < 0)
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
    if ((conf->mode == server) && (conf->nat))
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

    if (conf->duplicate)
    {
        static pbuf_t tmp;
        copypkt(&tmp, pbuf);
        sendpkt(&tmp);
    }

    sendpkt(pbuf);
}


static void udp_cb(pbuf_t *pbuf)
{
    // 读取 UDP 包
    struct sockaddr_storage addr;
    socklen_t addrlen;
    ssize_t n;
    if (conf->mode == client)
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
        if (conf->mode == client)
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

    if (pbuf->len == 0)
    {
        if (conf->mode == server)
        {
            // server 接收到一个心跳包，回送一个心跳包
            heartbeat(pbuf);
        }
    }
    else
    {
        if (!is_dup(*(uint32_t *)(pbuf->chksum)))
        {
            n = tun_write(tun, pbuf->payload, pbuf->len);
            if (n < 0)
            {
                ERROR("tun_write");
            }
        }
    }
    // update remote address
    if (conf->mode == server)
    {
        if ((remote.addrlen != addrlen) ||
            (memcmp(&(remote.addr), &addr, addrlen) != 0))
        {
            memcpy(&(remote.addr), &addr, addrlen);
            remote.addrlen = addrlen;
        }
    }
}


// 判断一个包是否重复
#define HASH_LEN 1021U
static int is_dup(uint32_t chksum)
{
    static uint32_t hash[HASH_LEN][2];
    int h = (int)(chksum % HASH_LEN);
    int dup = (hash[h][0] == chksum) || (hash[h][1] == chksum);

    hash[h][1] = hash[h][0];
    hash[h][0] = chksum;
    return dup;
}


// 复制数据包
static void copypkt(pbuf_t *pbuf, const pbuf_t *src)
{
    pbuf->len = src->len;
    memcpy(pbuf->payload, src->payload, src->len);
}


// naive confusion
static void confusion(pbuf_t *pbuf)
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
        if (max > 1000)
        {
            pbuf->padding = rand() % 251;
        }
        else if (max > 500)
        {
            pbuf->padding = rand() % 401 + 99;
        }
        else if (max > 200)
        {
            pbuf->padding = rand() % 151 + 49;
        }
        else
        {
            pbuf->padding = rand() % 199;
            if (pbuf->padding > max)
            {
                pbuf->padding = max;
            }
        }
        pbuf->padding = rand() % (conf->mtu - pbuf->len) / 2;
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
static void sendpkt(pbuf_t *pbuf)
{
    if (remote.addrlen != 0)
    {
        // 混淆
        confusion(pbuf);
        // 加密
        ssize_t n = PAYLOAD_OFFSET + pbuf->len + pbuf->padding;
        crypto_encrypt(pbuf);
        n = sendto(sock, pbuf, n, 0, (struct sockaddr *)&(remote.addr), remote.addrlen);
        if (n < 0)
        {
            ERROR("sendto");
        }
    }
}


// 发送心跳包
static void heartbeat(pbuf_t *pbuf)
{
    srand(time(NULL));
    pbuf->len = 0;
    sendpkt(pbuf);
}
