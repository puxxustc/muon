/*
 * compress.c - wrap of lzo
 *
 * Copyright (C) 2014 - 2016, Xiaoxiao <i@xiaoxiao.im>
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

#include <string.h>
#include "compress.h"
#include "log.h"
#include "minilzo.h"


// lzo 算法
static void lzo(pbuf_t *pbuf)
{
    uint8_t wrkmem[LZO1X_1_MEM_COMPRESS];
    uint8_t out[2048+2048/16+64+3];
    lzo_uint olen;
    lzo1x_1_compress(pbuf->payload, pbuf->len, out, &olen, wrkmem);
    if (olen < pbuf->len)
    {
        memcpy(pbuf->payload, out, olen);
        pbuf->flag |= 0x04;
        pbuf->len = olen;
    }
}


static void unlzo(pbuf_t *pbuf)
{
    uint8_t out[2048];
    lzo_uint olen;
    lzo1x_decompress(pbuf->payload, pbuf->len, out, &olen, NULL);
    memcpy(pbuf->payload, out, olen);
    pbuf->len = olen;
}


int compress_init(void)
{
    // 初始化 lzo
    if (lzo_init() != LZO_E_OK)
    {
        LOG("failed to initialize LZO library");
        return -1;
    }
    return 0;
}



// 压缩 pbuf
void compress(pbuf_t *pbuf)
{
    lzo(pbuf);
}


// 解压缩 pbuf
void decompress(pbuf_t *pbuf)
{
    if (pbuf->flag & 0x04)
    {
        unlzo(pbuf);
    }
}
