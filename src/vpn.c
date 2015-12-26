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

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <libmill.h>
#include "conf.h"
#include "compress.h"
#include "crypto.h"
#include "log.h"
#include "md5.h"
#include "tunif.h"
#include "utils.h"
#include "vpn.h"

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

static const conf_t *conf;

static volatile int running;
static int tun;
static udpsock sock;
ipaddr remote;


coroutine static void tun_worker(void);
coroutine static void udp_worker(int port, int timeout);
coroutine static void udp_sender(pbuf_t *pbuf, int times);
coroutine static void client_hop(void);
coroutine static void heartbeat(void);

static int encapsulate(pbuf_t *pbuf);
static int decapsulate(pbuf_t *pbuf, int n);
static int otp_port(int offset);


int vpn_init(const conf_t *config)
{
    conf = config;

    LOG("starting muon %s", (conf->mode == MODE_SERVER) ? "server" : "client");

    // 初始化 lzo
    if (compress_init() != 0)
    {
        return -1;
    }

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
    if (conf->mode == MODE_CLIENT)
    {
        go(client_hop());
    }
    else
    {
        for (int i = conf->port[0]; i <= conf->port[1]; i++)
        {
            go(udp_worker(i, -1));
        }
    }

    go(tun_worker());

    // keepalive
    go(heartbeat());

    running = 1;
    while (running)
    {
        msleep(now() + 100);
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


coroutine static void tun_worker(void)
{
    pbuf_t pbuf;
    int events;
    ssize_t n;
    while (1)
    {
        events = fdwait(tun, FDW_IN, -1);
        if(events & FDW_IN)
        {
            // 从 tun 设备读取 IP 包
            n = tun_read(tun, pbuf.payload, conf->mtu);
            if (n <= 0)
            {
                ERROR("tun_read");
                return;
            }
            pbuf.len = (uint16_t)n;
            pbuf.flag = 0x0000;

            // 发送到 remote
            go(udp_sender(&pbuf, conf->duplicate));
        }
    }
}


coroutine static void client_hop(void)
{
    int port;
    while (1)
    {
        port = otp_port(0);
        go(udp_worker(port, 5 * 1000));
        msleep(now() + 500);
    }
}


coroutine static void udp_worker(int port, int timeout)
{
    int64_t deadline;
    if (timeout < 0)
    {
        deadline = -1;
    }
    else
    {
        deadline = now() + timeout;
    }

    ipaddr addr;
    if (conf->mode == MODE_CLIENT)
    {
        // client
        addr = iplocal("0.0.0.0", 0, 0);
        remote = ipremote(conf->server, port, 0, 0);
    }
    else
    {
        // server
        addr = iplocal(conf->server, port, 0);
        remote = ipremote(conf->server, port, 0, 0);
    }
    udpsock s = udplisten(addr);

    if (s == NULL)
    {
        LOG("failed to bind udp address");
        return;
    }

    sock = s;

    pbuf_t pbuf;
    ssize_t n;
    while (1)
    {
        if (conf->mode == MODE_CLIENT)
        {
            // client
            n = udprecv(s, NULL, &pbuf, conf->mtu + PAYLOAD_OFFSET, deadline);
        }
        else
        {
            // server
            n = udprecv(s, &addr, &pbuf, conf->mtu + PAYLOAD_OFFSET, deadline);
        }
        if (errno == ETIMEDOUT)
        {
            break;
        }
        else if (errno != 0)
        {
            ERROR("udprecv");
            continue;
        }
        if (n < PAYLOAD_OFFSET)
        {
            continue;
        }

        // decrypt, decompress
        n = decapsulate(&pbuf, n);
        if (n < 0)
        {
            // invalid packet
            if (conf->mode == MODE_CLIENT)
            {
                LOG("invalid packet, drop");
            }
            else
            {
                char buf[IPADDR_MAXSTRLEN];
                LOG("invalid packet from %s, drop", ipaddrstr(addr, buf));
            }
            continue;
        }

        // update active socket, remote address
        if (conf->mode == MODE_SERVER)
        {
            sock = s;
            remote = addr;
        }

        if (n == 0)
        {
            // heartbeat
            continue;
        }

        // 写入到 tun 设备
        assert(pbuf.len < sizeof(pbuf.payload));
        n = tun_write(tun, pbuf.payload, pbuf.len);
        if (n < 0)
        {
            ERROR("tun_write");
        }
    }
    udpclose(s);
}


// 发送心跳包
coroutine static void heartbeat(void)
{
    pbuf_t pbuf;
    while (1)
    {
        msleep(now() + 500 + rand() % 500);
        srand((unsigned)now());
        pbuf.len = 0;
        go(udp_sender(&pbuf, 1));
    }
}


// 发送数据包
coroutine static void udp_sender(pbuf_t *pbuf, int times)
{
    assert(pbuf != NULL);
    assert(sock != NULL);

    pbuf_t copy;
    int n = encapsulate(pbuf);
    memcpy(&copy, pbuf, n);
    if (conf->delay > 0)
    {
        msleep(now() + rand() % conf->delay);
    }
    udpsend(sock, remote, &copy, n);
    while (times > 1)
    {
        msleep(now() + 5 + rand() % 10);
        udpsend(sock, remote, &copy, n);
        times--;
    }
}


// 判断一个包是否重复
#define DUP_LEN 4093
static int is_dup(uint32_t chksum)
{
    static uint32_t hash[DUP_LEN][4];

    int h = (int)(chksum % DUP_LEN);
    int dup =    (hash[h][0] == chksum)
              || (hash[h][1] == chksum)
              || (hash[h][2] == chksum)
              || (hash[h][3] == chksum);

    if (!dup)
    {
        hash[h][3] = hash[h][2];
        hash[h][2] = hash[h][1];
        hash[h][1] = hash[h][0];
        hash[h][0] = chksum;
    }
    return dup;
}


// naïve obfuscation
static void obfuscate(pbuf_t *pbuf)
{
    assert(pbuf != NULL);

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


// 封装
static int encapsulate(pbuf_t *pbuf)
{
    assert(pbuf != NULL);

    // 压缩
    compress(pbuf);

    // 计算 hash
    crypto_hash(pbuf);

    // 混淆
    if (!(pbuf->flag & 0x04))
    {
        obfuscate(pbuf);
    }

    // 加密
    ssize_t n = PAYLOAD_OFFSET + pbuf->len + pbuf->padding;
    assert(n < (ssize_t)sizeof(pbuf->payload));
    crypto_encrypt(pbuf);

    return (int)n;
}


// 解封装
static int decapsulate(pbuf_t *pbuf, int n)
{
    assert(pbuf != NULL);
    assert(n < (int)sizeof(pbuf->payload));

    // 解密
    int invalid = crypto_decrypt(pbuf, n);
    if (invalid)
    {
        return -1;
    }

    // 忽略心跳包
    if (pbuf->len == 0)
    {
        return 0;
    }

    // 忽略重复的包
    if (is_dup(pbuf->chksum))
    {
        return 0;
    }

    // 解压缩
    decompress(pbuf);

    // 忽略 ack 包
    if (pbuf->flag & 0x0002)
    {
        return 0;
    }

    if (pbuf->flag & 0x0001)
    {
        // 暂不处理 ack flag
    }

    assert(pbuf->len < sizeof(pbuf->payload));
    return (int)(pbuf->len);
}


static int otp_port(int offset)
{
    int range = conf->port[1] - conf->port[0];

    if (range == 0)
    {
        return conf->port[0];
    }

    char s[17];
    uint8_t d[8];
    sprintf(s, "%016llx", (unsigned long long)now() / 500 + offset);
    hmac_md5(d, conf->key, conf->klen, s, 16);
    int port = 0;
    for (int i = 0; i < 8; i++)
    {
        port = (port * 256 + (int)(d[i])) % range;
    }
    return port + conf->port[0];
}
