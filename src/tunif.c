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
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "tunif.h"

#ifdef TARGET_LINUX
#  include <linux/if.h>
#  include <linux/if_tun.h>
#endif

#ifdef TARGET_DARWIN
#  include <sys/kern_control.h>
#  include <net/if_utun.h>
#  include <sys/sys_domain.h>
#  include <netinet/ip.h>
#  include <sys/uio.h>
#endif


#ifdef TARGET_LINUX
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
#endif


#ifdef TARGET_DARWIN
int tun_new(const char *dev)
{
    struct ctl_info ctlInfo;
    struct sockaddr_ctl sc;
    int fd;
    int utunnum;

    if (sscanf(dev, "utun%d", &utunnum) != 1)
    {
        return -1;
    }

    memset(&ctlInfo, 0, sizeof(ctlInfo));
    if (strlcpy(ctlInfo.ctl_name, UTUN_CONTROL_NAME, sizeof(ctlInfo.ctl_name)) >=
        sizeof(ctlInfo.ctl_name))
    {
        return -1;
    }

    fd = socket(PF_SYSTEM, SOCK_DGRAM, SYSPROTO_CONTROL);

    if (fd == -1)
    {
        return -1;
    }

    if (ioctl(fd, CTLIOCGINFO, &ctlInfo) == -1)
    {
        close(fd);
        return -1;
    }

    sc.sc_id = ctlInfo.ctl_id;
    sc.sc_len = sizeof(sc);
    sc.sc_family = AF_SYSTEM;
    sc.ss_sysaddr = AF_SYS_CONTROL;
    sc.sc_unit = utunnum + 1;

    if (connect(fd, (struct sockaddr *) &sc, sizeof(sc)) == -1)
    {
        close(fd);
        return -1;
    }

    return fd;
}


ssize_t tun_read(int tun, void *buf, size_t len)
{
    uint32_t type;
    struct iovec iv[2];

    iv[0].iov_base = &type;
    iv[0].iov_len = sizeof(type);
    iv[1].iov_base = buf;
    iv[1].iov_len = len;

    return readv(tun, iv, 2) - 4;
}


ssize_t tun_write(int tun, void *buf, size_t len)
{
    uint32_t type;
    struct iovec iv[2];

    if (((struct ip *)buf)->ip_v == 6)
    {
        type = htonl(AF_INET6);
    }
    else
    {
        type = htonl(AF_INET);
    }

    iv[0].iov_base = &type;
    iv[0].iov_len = sizeof(type);
    iv[1].iov_base = buf;
    iv[1].iov_len = len;

    return writev(tun, iv, 2) - 4;
}
#endif


void tun_close(int tun)
{
    close(tun);
}
