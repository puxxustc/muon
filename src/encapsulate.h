/*
 * encapsulate.h
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

#ifndef ENCAPSULATE_H
#define ENCAPSULATE_H

#include <stddef.h>
#include <stdint.h>

/*
 +---------+----------+----------+----------+-------------+----------------------+-------------------+
 |  Nonce  |  CHKSUM  |   ACK    |   Flag   | Data length |       Payload        |      Padding      |
 +---------+----------+----------+----------+-------------+----------------------+-------------------+
      8B        4B         4B         2B          2B               0~mtu

 Flag
   bit0 - ACK set
   bit1 - pure ACK payload
   bit2 - lzo compress

*/
typedef struct
{
    uint8_t  nonce[8];
    uint32_t chksum;
    uint32_t ack;
    uint16_t flag;
    uint16_t len;
    uint8_t  payload[2048];
    // not send to network
    int padding;
} pbuf_t;

#define PAYLOAD_OFFSET ((int)(offsetof(pbuf_t, payload)))
#define PAYLOAD_MAX ((int)(sizeof(pbuf_t) - offsetof(pbuf_t, payload)))
#define CRYPTO_START(pbuf) (&((pbuf)->chksum))
#define CRYPTO_LEN(pbuf) (offsetof(pbuf_t, payload) - offsetof(pbuf_t, chksum) + (pbuf)->len + (pbuf)->padding)
#define CRYPTO_NONCE_LEN ((int)(offsetof(pbuf_t, chksum) - offsetof(pbuf_t, nonce)))

extern int encapsulate(pbuf_t *pbuf, int mtu);
extern int decapsulate(pbuf_t *pbuf, int n);

#endif
