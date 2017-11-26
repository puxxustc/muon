/*
 * totp.c
 *
 * Copyright (C) 2014 - 2017, Xiaoxiao <i@pxx.io>
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

#include <stdio.h>
#include <sys/time.h>

#include "crypto.h"

#include "totp.h"


int totp(int range, int offset)
{
    if (range == 0)
    {
        return 0;
    }

    struct timeval tv;
    gettimeofday(&tv, NULL);
    unsigned long long t;
    t = (unsigned long long)(tv.tv_sec * 1000 + tv.tv_usec / 1000) / TOTP_STEP + offset;
    char s[17];
    sprintf(s, "%016llx", t);
    uint8_t d[16];
    hmac(d, s, 16);
    int token = 0;
    for (int i = 0; i < 16; i++)
    {
        token = (token * 256 + (int)(d[i])) % (range + 1);
    }
    return token;
}
