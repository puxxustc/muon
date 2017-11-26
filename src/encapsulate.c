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

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#include <sodium.h>

#include "compress.h"
#include "crypto.h"
#include "encapsulate.h"


// naïve obfuscation
static void obfuscate(pbuf_t *pbuf, int mtu)
{
    assert(pbuf != NULL);

    // random padding
    if (pbuf->len < mtu)
    {
        int max = mtu - pbuf->len;
        if (max > 897)
        {
            pbuf->padding = randombytes_uniform(554);
        }
        else if (max > 554)
        {
            pbuf->padding = randombytes_uniform(342);
        }
        else if (max > 342)
        {
            pbuf->padding = randombytes_uniform(211);
        }
        else
        {
            pbuf->padding = randombytes_uniform(130);
            if (pbuf->padding > max)
            {
                pbuf->padding = max;
            }
        }
        randombytes_buf(pbuf->payload + pbuf->len, pbuf->padding);
    }
    else
    {
        pbuf->padding = 0;
    }
}


// 封装
int encapsulate(int token, pbuf_t *pbuf, int mtu)
{
    assert(pbuf != NULL);

    // 压缩
    compress(pbuf);

    // 混淆
    pbuf->padding = 0;
    if (!(pbuf->flag & FLAG_COMPRESS))
    {
        obfuscate(pbuf, mtu);
    }

    // 加密
    ssize_t n = PAYLOAD_OFFSET + pbuf->len + pbuf->padding;
    crypto_encrypt(token, pbuf);

    return (int)n;
}


// 解封装
int decapsulate(int token, pbuf_t *pbuf, int n)
{
    assert(pbuf != NULL);

    // 解密
    int invalid = crypto_decrypt(token, pbuf, n);
    if (invalid)
    {
        return -1;
    }

    if (pbuf->len == 0)
    {
        // 忽略心跳包
        return 0;
    }
    else if (pbuf->len > sizeof(pbuf->payload))
    {
        // 忽略错误的长度
        return -1;
    }

    // 解压缩
    decompress(pbuf);

    // 忽略 ack 包
    if (pbuf->flag & 0x0002)
    {
        return 0;
    }

    if (pbuf->flag & 0x0001)
    {
        // 暂不处理 ack flag
    }

    return (int)(pbuf->len);
}
