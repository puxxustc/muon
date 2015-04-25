/*
 * tunif.c - tun interface
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

#include <fcntl.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "tunif.h"

int tun_new(const char *dev)
{
	struct ifreq ifr;
	int fd, err;

	fd = open("/dev/net/tun", O_RDWR);
	if (fd < 0)
	{
		return -1;
	}

	bzero(&ifr, sizeof(struct ifreq));

	ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
	if (*dev != '\0')
	{
		strncpy(ifr.ifr_name, dev, IFNAMSIZ);
	}

	err = ioctl(fd, TUNSETIFF, (void *)&ifr);
	if (err < 0)
	{
		return err;
	}
	return fd;
}

void tun_close(int tun)
{
	close(tun);
}
