/*
 * vpn.h
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

#ifndef VPN_H
#define VPN_H

#include <libmill.h>

#include "conf.h"

#define POOL 40


typedef struct {
    int mode;
    int mtu;
    int path_count;
    int running;
    int tun;
    struct {
        char server[64];
        int port_start;
        int port_range;
        int alive;
        udpsock sock;
        ipaddr remote;
        int ports[POOL];
  } paths[PATH_MAX_COUNT];
} ctx_t;


extern int vpn_init(const conf_t *config);
extern int vpn_run(void);
extern void vpn_stop(void);


#endif
