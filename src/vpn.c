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

static uint8_t buf[IV_LEN + MTU_MAX];

static void tun_cb(void);
static void udp_cb(void);
static int is_dup(uint32_t chksum);
static void heartbeat(void);


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
			heartbeat();
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
			tun_cb();
		}

		if (FD_ISSET(sock, &readset))
		{
			udp_cb();
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


static void tun_cb(void)
{
	// 从 tun 设备读取 IP 包
	ssize_t n = tun_read(tun, buf + IV_LEN, conf->mtu);
	if (n <= 0)
	{
		if (n < 0)
		{
			ERROR("tun_read");
		}
		return;
	}

	// 对 TCP 包, UDP 包双倍发包
	int dup = conf->duplicate;
	if (dup)
	{
		if ((buf[IV_LEN] >> 4) == 4)
		{
			// IPv4
			if ((buf[IV_LEN + 9] != 6) && (buf[IV_LEN + 9] != 17))
			{
				// not TCP, not UDP
				dup = 0;
			}
		}
		else if ((buf[IV_LEN] >> 4) == 6)
		{
			// IPv6
			if ((buf[IV_LEN + 6] != 6) && (buf[IV_LEN + 6] != 17))
			{
				// not TCP, not UDP
				dup = 0;
			}
		}
	}

	if (remote.addrlen != 0)
	{
		// 加密
		crypto_encrypt(buf, n);

		// 发送 UDP 包
		n = sendto(sock, buf, n + IV_LEN, 0,
		           (struct sockaddr *)&(remote.addr), remote.addrlen);
		if (n < 0)
		{
			ERROR("sendto");
		}
		else if (dup)
		{
			sendto(sock, buf, n, 0,
		           (struct sockaddr *)&(remote.addr), remote.addrlen);
		}
	}
}


static void udp_cb(void)
{
	// 读取 UDP 包
	struct sockaddr_storage addr;
	socklen_t addrlen;
	ssize_t n;
	if (conf->mode == client)
	{
		// client
		n = recvfrom(sock, buf, conf->mtu + IV_LEN, 0, NULL, NULL);
	}
	else
	{
		// server
		addrlen = sizeof(struct sockaddr_storage);
		n = recvfrom(sock, buf, conf->mtu + IV_LEN, 0,
		             (struct sockaddr *)&addr, &addrlen);
	}
	if (n < IV_LEN)
	{
		if (n < 0)
		{
			ERROR("recvfrom");
		}
		return;
	}

	// 解密
	if (crypto_decrypt(buf, n) != 0)
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
	}
	else
	{
		if (buf[IV_LEN] == 0)
		{
			if (conf->mode == server)
			{
				// server 接收到一个心跳包，回送一个心跳包
				heartbeat();
			}
		}
		else
		{
			if (!is_dup(*(uint32_t *)buf))
			{
				n = tun_write(tun, buf + IV_LEN, n - IV_LEN);
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
}


// 判断一个包是否重复
#define CHKSUM_LEN 64
static int is_dup(uint32_t chksum)
{
	static uint32_t chksums[CHKSUM_LEN];
	static int last;
	for (int i = last; i >= 0; i--)
	{
		if (chksums[i] == chksum)
		{
			return 1;
		}
	}
	for (int i = CHKSUM_LEN - 1; i > last; i--)
	{
		if (chksums[i] == chksum)
		{
			return 1;
		}
	}
	chksums[last] = chksum;
	last++;
	if (last >= CHKSUM_LEN)
	{
		last = 0;
	}
	return 0;
}


// 发送心跳包
static void heartbeat(void)
{
	if (remote.addrlen != 0)
	{
		srand(time(NULL));
		int len = rand() % (conf->mtu - 40) + 40;
		for (int i = 0; i < len; i++)
		{
			buf[IV_LEN + i] = (uint8_t)(rand() & 0xff);
		}
		buf[IV_LEN] = 0;

		crypto_encrypt(buf, len);

		sendto(sock, buf, len + IV_LEN, 0, (struct sockaddr *)&(remote.addr),
		       remote.addrlen);
	}
}
