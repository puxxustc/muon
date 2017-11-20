/*
 * compress.c - wrapper of lz4
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
#include <string.h>

#include <lz4.h>

#include "compress.h"
#include "encapsulate.h"
#include "log.h"


void compress(pbuf_t *pbuf)
{
    uint8_t out[2048];
    int outlen = LZ4_compress_default(
        (const char *)(pbuf->payload),
        (char *)out,
        pbuf->len,
        sizeof(out));
    if ((outlen > 0) && (outlen < pbuf->len))
    {
        memcpy(pbuf->payload, out, outlen);
        pbuf->len = outlen;
        pbuf->flag |= FLAG_COMPRESS;
    }
}


void decompress(pbuf_t *pbuf)
{
    if (pbuf->flag & FLAG_COMPRESS)
    {
        uint8_t out[2048];
        int outlen = LZ4_decompress_safe(
            (const char *)(pbuf->payload),
            (char *)out,
            pbuf->len,
            sizeof(out));
        assert(outlen > 0);
        memcpy(pbuf->payload, out, outlen);
        pbuf->len = outlen;
    }
}
