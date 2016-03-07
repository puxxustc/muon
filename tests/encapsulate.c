/*
 * encapsulate.c - test encapsulate and decapsulate
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
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "../src/compress.h"
#include "../src/encapsulate.h"


int main()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    srand((int)((tv.tv_sec * 1000 + tv.tv_usec / 1000) % RAND_MAX));

    if (compress_init() != 0)
    {
        return -1;
    }

    const int mtu = 1452;

    pbuf_t pbuf;
    for (int len = 0; len <= mtu; len++)
    {
        for (int data = 0; data <=2; data++)
        {
            for (int j = 0; j < len; j++)
            {
                if (data == 0)
                {
                    pbuf.payload[j] = (uint8_t)(rand() & 0xffU);
                }
                else if (data == 1)
                {
                    pbuf.payload[j] = (uint8_t)(j & 0xffU);
                }
                else
                {
                    pbuf.payload[j] = (uint8_t)(len & 0xffU);
                }
            }
            pbuf.len = len;
            pbuf.flag = 0x0000;
            pbuf.ack = 0;

            pbuf_t copy;
            copy.ack = pbuf.ack;
            copy.flag = pbuf.flag;
            copy.len = pbuf.len;
            memcpy(copy.payload, pbuf.payload, pbuf.len);

            int n = encapsulate(&copy, mtu);
            assert(n <= mtu + PAYLOAD_OFFSET);

            n = decapsulate(&copy, n);
            assert(n >= 0);
            assert(copy.len == pbuf.len);
            assert(memcmp(copy.payload, pbuf.payload, copy.len) == 0);
        }
    }
    return 0;
}
