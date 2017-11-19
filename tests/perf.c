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

#include <sodium.h>

#include "../src/compress.h"
#include "../src/crypto.h"
#include "../src/encapsulate.h"


const int count = 50000;


int main()
{
    if (compress_init() != 0)
    {
        return -1;
    }

    const char *key = "8556085d7ff5655a5e09a385c152ea2a";
    if (crypto_init(key) != 0)
    {
        return -1;
    }

    const int mtu = 1452;


    printf("encapsulate/decapsulate, %d packets\n", count);
    printf("      |     time(ms)\n"
           " size | random | ascending\n"
           "------+--------+----------\n");

    pbuf_t pbuf;
    for (int len = 0; len <= mtu; len += 363)
    {
        for (int random = 1; random >= 0; random--)
        {
            for (int j = 0; j < len; j++)
            {
                if (random == 0)
                {
                    pbuf.payload[j] = (uint8_t)(randombytes_uniform(256));
                }
                else if (random == 1)
                {
                    pbuf.payload[j] = (uint8_t)(j & 0xffU);
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

            struct timeval tv;
            gettimeofday(&tv, NULL);
            int64_t start = tv.tv_sec * 1000 + tv.tv_usec / 1000;

            for (int k = 0; k < count; k++)
            {
                int n = encapsulate(&copy, mtu);
                decapsulate(&copy, n);
            }

            gettimeofday(&tv, NULL);
            int64_t end = tv.tv_sec * 1000 + tv.tv_usec / 1000;
            if (random)
            {
                printf("%5d |%6d", len, (int)(end - start));
            }
            else
            {
                printf("  |%7d\n", (int)(end - start));
            }
            if ((len == mtu) && (random == 0))
            {
                double speed = (double)len * count * 1000 / (double)(end - start) / 1024.0 / 1024.0;
                printf("Speed: %.3lfMB/s\n", speed);
            }
        }
    }
    return 0;
}
