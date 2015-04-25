/*
 * utils.c - some util functions
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
#include <fcntl.h>
#include <netdb.h>
#include <pwd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include "utils.h"

int setnonblock(int fd)
{
	int flags;
	flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1)
	{
		return -1;
	}
	if (-1 == fcntl(fd, F_SETFL, flags | O_NONBLOCK))
	{
		return -1;
	}
	return 0;
}

int runas(const char *user)
{
	struct passwd *pw_ent = NULL;

	if (user != NULL)
	{
		pw_ent = getpwnam(user);
	}

	if (pw_ent != NULL)
	{
		if (setegid(pw_ent->pw_gid) != 0)
		{
			return -1;
		}
		if (seteuid(pw_ent->pw_uid) != 0)
		{
			return -1;
		}
	}

	return 0;
}

int daemonize(const char *pidfile, const char *logfile)
{
	pid_t pid;

	pid = fork();
	if (pid < 0)
	{
		fprintf(stderr, "fork: %s\n", strerror(errno));
		return -1;
	}

	if (pid > 0)
	{
		FILE *fp = fopen(pidfile, "w");
		if (fp == NULL)
		{
			fprintf(stderr, "can not open pidfile: %s\n", pidfile);
		}
		else
		{
			fprintf(fp, "%d", pid);
			fclose(fp);
		}
		exit(EXIT_SUCCESS);
	}

	umask(0);

	if (setsid() < 0)
	{
		fprintf(stderr, "setsid: %s\n", strerror(errno));
		return -1;
	}

	fclose(stdin);
	FILE *fp;
	fp = freopen(logfile, "w", stdout);
	if (fp == NULL)
	{
		fprintf(stderr, "freopen: %s\n", strerror(errno));
		return -1;
	}
	fp = freopen(logfile, "w", stderr);
	if (fp == NULL)
	{
		fprintf(stderr, "freopen: %s\n", strerror(errno));
		return -1;
	}

	return 0;
}

int setup_nic(const char *tunif, int mtu, const char *address)
{
	char cmd[128];
	int r;
	sprintf(cmd, "/usr/bin/sh -c \'ip link set %s up\'", tunif);
	r = system(cmd);
	if (r != 0)
	{
		return r;
	}
	sprintf(cmd, "/usr/bin/sh -c \'ip link set %s mtu %d\'", tunif, mtu);
	r = system(cmd);
	if (r != 0)
	{
		return r;
	}
	sprintf(cmd, "/usr/bin/sh -c \'ip addr add %s dev %s\'", address, tunif);
	r = system(cmd);
	if (r != 0)
	{
		return r;
	}
	return 0;
}

int setup_route(const char *tunif, const char *server)
{
	// 解析服务器地址
	struct addrinfo hints;
	struct addrinfo *res;
	bzero(&hints, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;
	if (getaddrinfo(server, "0", &hints, &res) != 0)
	{
		return -1;
	}
	if (res->ai_addr->sa_family == AF_INET)
	{
		char cmd[128];
		char subnet[16];
		int r;
		uint32_t ip = ntohl(((struct sockaddr_in *)(res->ai_addr))->sin_addr.s_addr);
		uint32_t start = 0U;
		int mask = 1;
		while (mask <= 32)
		{
			if ((uint64_t)start + (1U << (32 - mask)) - 1 < ip)
			{
				sprintf(subnet, "%u.%u.%u.%u/%d", start >> 24, (start >> 16) & 0xff, (start >> 8) & 0xff,
				        start & 0xff, mask);
				start += (1U << (32 - mask));
				sprintf(cmd, "/usr/bin/sh -c \'ip route add %s dev %s table 100\'", subnet, tunif);
				r = system(cmd);
				if (r != 0)
				{
					return r;
				}
			}
			else
			{
				mask++;
			}
		}
		uint32_t end = ~0U;
		mask = 1;
		while (mask <= 32)
		{
			if ((uint64_t)end - (1U << (32 - mask)) + 1 > ip)
			{
				end -= (1U << (32 - mask));
				sprintf(subnet, "%u.%u.%u.%u/%d", (end + 1) >> 24, ((end + 1) >> 16) & 0xff, 
				        ((end + 1) >> 8) & 0xff, (end + 1) & 0xff, mask);
				sprintf(cmd, "/usr/bin/sh -c \'ip route add %s dev %s table 100\'", subnet, tunif);
				r = system(cmd);
				if (r != 0)
				{
					return r;
				}
			}
			else
			{
				mask++;
			}
		}
	}
	else
	{

	}
	freeaddrinfo(res);
	return 0;
}

int setup_nat(const char *tunif, int on)
{
	char cmd[128];
	int r;
	if (on)
	{
		strcpy(cmd, "/usr/bin/sh -c \'sysctl -w net.ipv4.ip_forward=1 >/dev/null\'");
		r = system(cmd);
		if (r != 0)
		{
			return r;
		}
		sprintf(cmd, "/usr/bin/sh -c \'iptables -t nat -A POSTROUTING -o %s -j MASQUERADE\'", tunif);
		r = system(cmd);
		if (r != 0)
		{
			return r;
		}
		sprintf(cmd, "/usr/bin/sh -c \'iptables -A FORWARD -o %s -j ACCEPT\'", tunif);
		r = system(cmd);
		if (r != 0)
		{
			return r;
		}
		sprintf(cmd, "/usr/bin/sh -c \'iptables -A FORWARD -i %s -j ACCEPT\'", tunif);
		r = system(cmd);
		if (r != 0)
		{
			return r;
		}
		sprintf(cmd, "iptables -t mangle -A FORWARD -o %s -p tcp -m tcp --tcp-flags SYN,RST SYN -j TCPMSS --clamp-mss-to-pmtu", tunif);
		r = system(cmd);
		if (r != 0)
		{
			return r;
		}
		sprintf(cmd, "iptables -t mangle -A FORWARD -i %s -p tcp -m tcp --tcp-flags SYN,RST SYN -j TCPMSS --clamp-mss-to-pmtu", tunif);
		r = system(cmd);
		if (r != 0)
		{
			return r;
		}
	}
	else
	{
		/*
		strcpy(cmd, "/usr/bin/sh -c \'sysctl -w net.ipv4.ip_forward=0\'");
		r = system(cmd);
		if (r != 0)
		{
			return r;
		}
		*/
		sprintf(cmd, "/usr/bin/sh -c \'iptables -t nat -D POSTROUTING -o %s -j MASQUERADE\'", tunif);
		r = system(cmd);
		if (r != 0)
		{
			return r;
		}
		sprintf(cmd, "/usr/bin/sh -c \'iptables -D FORWARD -o %s -j ACCEPT\'", tunif);
		r = system(cmd);
		if (r != 0)
		{
			return r;
		}
		sprintf(cmd, "/usr/bin/sh -c \'iptables -D FORWARD -i %s -j ACCEPT\'", tunif);
		r = system(cmd);
		if (r != 0)
		{
			return r;
		}
		sprintf(cmd, "iptables -t mangle -D FORWARD -o %s -p tcp -m tcp --tcp-flags SYN,RST SYN -j TCPMSS --clamp-mss-to-pmtu", tunif);
		r = system(cmd);
		if (r != 0)
		{
			return r;
		}
		sprintf(cmd, "iptables -t mangle -D FORWARD -i %s -p tcp -m tcp --tcp-flags SYN,RST SYN -j TCPMSS --clamp-mss-to-pmtu", tunif);
		r = system(cmd);
		if (r != 0)
		{
			return r;
		}
	}
	return 0;
}
