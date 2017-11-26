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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <libmill.h>
#include <sodium.h>

#include "conf.h"
#include "crypto.h"
#include "encapsulate.h"
#include "log.h"
#include "totp.h"
#include "tunif.h"
#include "utils.h"

#include "vpn.h"

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

static const conf_t *conf;

static ctx_t ctx;


coroutine static void tun_worker(void);
coroutine static void udp_worker(int path, int port, int timeout);
coroutine static void udp_sender(pbuf_t *pbuf);
coroutine static void client_hop(void);
coroutine static void heartbeat(void);

static int otp_port(int path, int offset);


int vpn_init(const conf_t *config)
{
    conf = config;

    ctx.mode = conf->mode;
    ctx.mtu = conf->mtu;
    ctx.path_count = conf->path_count;
    for (int i = 0; i < ctx.path_count; i++)
    {
        strcpy(ctx.paths[i].server, conf->paths[i].server);
        ctx.paths[i].port_start = conf->paths[i].port[0];
        ctx.paths[i].port_range = conf->paths[i].port[1] - conf->paths[i].port[0];
    }

    LOG("starting muon %s", (ctx.mode == MODE_SERVER) ? "server" : "client");

    if (crypto_init(conf->key) != 0)
    {
        return -1;
    }

    // create tun device
    ctx.tun = tun_new(conf->tunif);
    if (ctx.tun < 0)
    {
        LOG("failed to init tun device");
        return -1;
    }
    LOG("using tun device: %s", conf->tunif);

    // set IP address
#ifdef TARGET_LINUX
    if (ifconfig(conf->tunif, ctx.mtu, conf->address, conf->address6) != 0)
    {
        LOG("failed to add address on tun device");
    }
#endif
#ifdef TARGET_DARWIN
    if (ifconfig(conf->tunif, ctx.mtu, conf->address, conf->peer, conf->address6) != 0)
    {
        LOG("failed to add address on tun device");
    }
#endif

    if (ctx.mode == MODE_CLIENT)
    {
        if (conf->route)
        {
            // set route table
            for (int i = 0; i < ctx.path_count; i++)
            {
                if (route(conf->tunif, ctx.paths[i].server, conf->address[0], conf->address6[0]) != 0)
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
            // turn on NAT
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
    if (ctx.mode == MODE_CLIENT)
    {
        go(client_hop());
    }
    else
    {
        for (int i = 0; i < ctx.path_count; i++)
        {
            for (int j = 0; j <= ctx.paths[i].port_range; j++)
            {
                int port = ctx.paths[i].port_start + j;
                go(udp_worker(i, port, -1));
            }
        }
    }

    go(tun_worker());

    // keepalive
    go(heartbeat());

    ctx.running = 1;
    while (ctx.running)
    {
        msleep(now() + 50);
    }

    // turn off nat
#ifdef TARGET_LINUX
    if ((ctx.mode == MODE_SERVER) && (conf->nat))
    {
        // regain root privilege
        if (conf->user[0] != '\0')
        {
            if (runas("root") != 0)
            {
                ERROR("runas");
            }
        }

        if (nat(conf->address, 0))
        {
            LOG("failed to turn off NAT");
        }
    }
#endif

    // clean up
    tun_close(ctx.tun);
    LOG("close tun device");

    LOG("exit");
    return (ctx.running == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}


void vpn_stop(void)
{
    ctx.running = 0;
}


coroutine static void tun_worker(void)
{
    pbuf_t pbuf;
    int events;
    ssize_t n;
    while (1)
    {
        events = fdwait(ctx.tun, FDW_IN, -1);
        if(events & FDW_IN)
        {
            // 从 tun 设备读取 IP 包
            n = tun_read(ctx.tun, pbuf.payload, ctx.mtu);
            if (n <= 0)
            {
                ERROR("tun_read");
                return;
            }
            pbuf.len = (uint16_t)n;
            pbuf.flag = 0x0000;

            // 发送到 remote
            go(udp_sender(&pbuf));
        }
    }
}


coroutine static void client_hop(void)
{
    while (1)
    {
        for (int i = 0; i < ctx.path_count; i++) {
            int port;
            port = otp_port(i, 0);
            go(udp_worker(i, port, 5 * 1000));
        }
        msleep(now() + TOTP_STEP);
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
    if (ctx.mode == MODE_CLIENT)
    {
        // client
        addr = iplocal(NULL, 0, IPADDR_PREF_IPV6);
        ctx.paths[path].remote = ipremote(ctx.paths[path].server, port, 0, -1);
        ctx.paths[path].alive = 1;
    }
    else
    {
        // server
        addr = iplocal(ctx.paths[path].server, port, 0);
        ctx.paths[path].alive = 0;
    }
    udpsock s = udplisten(addr);

    if (s == NULL)
    {
        LOG("failed to bind udp address");
        return;
    }

    ctx.paths[path].sock = s;

    pbuf_t pbuf;
    ssize_t n;
    while (1)
    {
        if (ctx.mode == MODE_CLIENT)
        {
            // client
            n = udprecv(s, NULL, &pbuf, ctx.mtu + PAYLOAD_OFFSET, deadline);
        }
        else
        {
            // server
            n = udprecv(s, &addr, &pbuf, ctx.mtu + PAYLOAD_OFFSET, deadline);
            int port = udpport(s);
            int i;
            for (i = 0; i < POOL; i++)
            {
                if (port == ctx.paths[path].ports[i])
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
            if (ctx.mode == MODE_CLIENT)
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
        if (ctx.mode == MODE_SERVER)
        {
            ctx.paths[path].sock = s;
            ctx.paths[path].remote = addr;
            ctx.paths[path].alive = 1;
        }

        if (n == 0)
        {
            // heartbeat
            continue;
        }

        // 写入到 tun 设备
        n = tun_write(ctx.tun, pbuf.payload, pbuf.len);
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
    for (int path = 0; path < ctx.path_count; path++) {
        for (int i = 0; i < POOL; i++)
        {
            ctx.paths[path].ports[i] = otp_port(path, i - POOL / 2);
        }
    }

    pbuf_t pbuf;
    while (1)
    {
        msleep(now() + TOTP_STEP);

        for (int path = 0; path < ctx.path_count; path++) {
            for (int i = 0; i < POOL; i++)
            {
                ctx.paths[path].ports[i] = otp_port(path, i - POOL / 2);
            }

            pbuf.len = 0;
            pbuf.flag = 0;
            go(udp_sender(&pbuf));
        }
    }
}


// 发送数据包
coroutine static void udp_sender(pbuf_t *pbuf)
{
    assert(pbuf != NULL);

    static int path = 0;
    if (ctx.path_count > 0)
    {
        path = (path + 1) % ctx.path_count;
    }

    assert(ctx.paths[path].sock != NULL);

    if (!ctx.paths[path].alive)
    {
        return;
    }

    int n = encapsulate(pbuf, ctx.mtu);
    udpsend(ctx.paths[path].sock, ctx.paths[path].remote, pbuf, n);
}


static int otp_port(int path, int offset)
{
    int range = ctx.paths[path].port_range;

    int port = ctx.paths[path].port_start;
    if (range > 0)
    {
        port += totp(range + 1, offset);
    }

    return port;
}
