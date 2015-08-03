/*
 * vpn.h
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

#ifndef VPN_H
#define VPN_H

#include <stddef.h>
#include <stdint.h>
#include "conf.h"


/*
 +-----------+-------------+-----------+----------------------+-------------------+
 | IV/CHKSUM | Data Length |   Nonce   |       Payload        |      Padding      |
 +-----------+-------------+-----------+----------------------+-------------------+
      16B          2B           2B               0~mtu
*/
typedef struct
{
    uint8_t iv[16];
    uint16_t len;
    uint16_t nonce;
    uint8_t payload[2048];
    // not send to network
    int padding;
} pbuf_t;

#define IV_LEN ((int)(offsetof(pbuf_t, len)))
#define PAYLOAD_OFFSET ((int)(offsetof(pbuf_t, payload)))
#define PAYLOAD_MAX ((int)(sizeof(pbuf_t) - offsetof(pbuf_t, payload)))

extern int vpn_init(const conf_t *config);
extern int vpn_run(void);
extern void vpn_stop(void);


#endif
