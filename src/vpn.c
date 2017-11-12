/*
 * vpn.c
 *
 * Copyright (C) 2014 - 2016, Xiaoxiao <i@pxx.io>
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
#include <libmill.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include "conf.h"
#include "compress.h"
#include "crypto.h"
#include "encapsulate.h"
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

#define POOL 40

static struct {
    udpsock sock;
    ipaddr remote;
    int ports[POOL];
} paths[PATH_MAX_COUNT];


coroutine static void tun_worker(void);
coroutine static void udp_worker(int path, int port, int timeout);
coroutine static void udp_sender(pbuf_t *pbuf, int times);
coroutine static void client_hop(void);
coroutine static void heartbeat(void);

static int otp_port(int path, int offset);


int vpn_init(const conf_t *config)
{
    conf = config;

    LOG("starting muon %s", (conf->mode == MODE_SERVER) ? "server" : "client");

    // 初始化 lzo
    if (compress_init() != 0)
    {
        return -1;
    }

    // 初始化加密
    crypto_init(conf->key);

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
            for (int i = 0; i < conf->path_count; i++)
            {
                if (route(conf->tunif, conf->paths[i].server, conf->address[0], conf->address6[0]) != 0)
                {
                    LOG("failed to setup route");
                }
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
    go(tun_worker());

    // keepalive
    go(heartbeat());

    if (conf->mode == MODE_CLIENT)
    {
        go(client_hop());
    }
    else
    {
        for (int i = 0; i < conf->path_count; i++)
        {
            for (int j = conf->paths[i].port[0]; j <= conf->paths[i].port[1]; j++)
            {
                go(udp_worker(i, j, -1));
            }
        }
    }

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
    while (1)
    {
        for (int i = 0; i < conf->path_count; i++) {
            int port;
            port = otp_port(i, 0);
            go(udp_worker(i, port, 5 * 1000));
        }
        msleep(now() + 500);
    }
}


coroutine static void udp_worker(int path, int port, int timeout)
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
        addr = iplocal(NULL, 0, IPADDR_PREF_IPV6);
        paths[path].remote = ipremote(conf->paths[path].server, port, 0, -1);
    }
    else
    {
        // server
        addr = iplocal(conf->paths[path].server, port, 0);
        paths[path].remote = ipremote(conf->paths[path].server, port, 0, -1);
    }
    udpsock s = udplisten(addr);

    if (s == NULL)
    {
        LOG("failed to bind udp address");
        return;
    }

    paths[path].sock = s;

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
            int port = udpport(s);
            int i;
            for (i = 0; i < POOL; i++)
            {
                if (port == paths[path].ports[i])
                {
                    break;
                }
            }
            if (i >= POOL)
            {
                char buf[IPADDR_MAXSTRLEN];
                ipaddrstr(addr, buf);
                if (strcmp(buf, "127.0.0.1") != 0)
                {
                    LOG("invalid packet from %s:%d", buf, port);
                }
                continue;
            }
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
                ipaddrstr(addr, buf);
                int port = udpport(s);
                LOG("invalid packet from %s:%d", buf, port);
            }
            continue;
        }

        // update active socket, remote address
        if (conf->mode == MODE_SERVER)
        {
            paths[path].sock = s;
            paths[path].remote = addr;
        }

        if (n == 0)
        {
            // heartbeat
            continue;
        }

        // 写入到 tun 设备
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
    for (int path = 0; path < conf->path_count; path++) {
        for (int i = 0; i < POOL; i++)
        {
            paths[path].ports[i] = otp_port(path, i - POOL / 2);
        }
    }

    pbuf_t pbuf;
    while (1)
    {
        msleep(now() + 500);

        for (int path = 0; path < conf->path_count; path++) {
            for (int i = 0; i < POOL; i++)
            {
                paths[path].ports[i] = otp_port(path, i - POOL / 2);
            }

            srand((unsigned)now());
            pbuf.len = 0;
            pbuf.flag = 0;
            go(udp_sender(&pbuf, 1));
        }
    }
}


// 发送数据包
coroutine static void udp_sender(pbuf_t *pbuf, int times)
{
    assert(pbuf != NULL);

    double r = (double)rand() / (double)(RAND_MAX);
    int path;
    for (path = 0; path < conf->path_count; path++)
    {
        r -= conf->paths[path].weight;
        if (r < 0.0)
        {
            break;
        }
    }

    assert(paths[path].sock != NULL);

    pbuf_t copy;
    copy.flag = pbuf->flag;
    copy.len = pbuf->len;
    memcpy(copy.payload, pbuf->payload, pbuf->len);

    int n = encapsulate(&copy, conf->mtu);
    if (conf->delay > 0)
    {
        msleep(now() + rand() % conf->delay);
    }
    udpsend(paths[path].sock, paths[path].remote, &copy, n);
    while (times > 1)
    {
        msleep(now() + 5 + rand() % 10);
        udpsend(paths[path].sock, paths[path].remote, &copy, n);
        times--;
    }
}


static int otp_port(int path, int offset)
{
    int range = conf->paths[path].port[1] - conf->paths[path].port[0];

    if (range == 0)
    {
        return conf->paths[path].port[0];
    }

    char s[17];
    uint8_t d[16];
    struct timeval tv;
    gettimeofday(&tv, NULL);
    sprintf(s, "%016llx", (unsigned long long)(tv.tv_sec * 1000 + tv.tv_usec / 1000) / 500 + offset);
    hmac_md5(d, conf->key, conf->klen, s, 16);
    int port = 0;
    for (int i = 0; i < 16; i++)
    {
        port = (port * 256 + (int)(d[i])) % range;
    }
    return port + conf->paths[path].port[0];
}
