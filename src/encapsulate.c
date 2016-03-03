/*
 * encapsulate.h
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

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include "compress.h"
#include "crypto.h"
#include "encapsulate.h"


// 判断一个包是否重复
#define DUP_LEN 4093
static int is_dup(uint32_t chksum)
{
    static uint32_t hash[DUP_LEN][4];

    int h = (int)(chksum % DUP_LEN);
    int dup =    (hash[h][0] == chksum)
              || (hash[h][1] == chksum)
              || (hash[h][2] == chksum)
              || (hash[h][3] == chksum);

    if (!dup)
    {
        hash[h][3] = hash[h][2];
        hash[h][2] = hash[h][1];
        hash[h][1] = hash[h][0];
        hash[h][0] = chksum;
    }
    return dup;
}


// naïve obfuscation
static void obfuscate(pbuf_t *pbuf, int mtu)
{
    assert(pbuf != NULL);

    // nonce = rand()[0:8]
    for (int i = 0; i < 8; i++)
    {
        pbuf->nonce[i] = (uint8_t)(rand() & 0xff);
    }
    // random padding
    if (pbuf->len < mtu)
    {
        int max = mtu - pbuf->len;
        if (max > 897)
        {
            pbuf->padding = rand() % 554;
        }
        else if (max > 554)
        {
            pbuf->padding = rand() % 342;
        }
        else if (max > 342)
        {
            pbuf->padding = rand() % 211;
        }
        else
        {
            pbuf->padding = rand() % 130;
            if (pbuf->padding > max)
            {
                pbuf->padding = max;
            }
        }
        for (int i = (int)(pbuf->len); i < (int)(pbuf->len) + pbuf->padding; i++)
        {
            pbuf->payload[i] = (uint8_t)(rand() & 0xff);
        }
    }
    else
    {
        pbuf->padding = 0;
    }
}


// 封装
int encapsulate(pbuf_t *pbuf, int mtu)
{
    assert(pbuf != NULL);

    // 压缩
    compress(pbuf);

    // 计算 hash
    crypto_hash(pbuf);

    // 混淆
    pbuf->padding = 0;
    if (!(pbuf->flag & 0x04))
    {
        obfuscate(pbuf, mtu);
    }

    // 加密
    ssize_t n = PAYLOAD_OFFSET + pbuf->len + pbuf->padding;
    crypto_encrypt(pbuf);

    return (int)n;
}


// 解封装
int decapsulate(pbuf_t *pbuf, int n)
{
    assert(pbuf != NULL);

    // 解密
    int invalid = crypto_decrypt(pbuf, n);
    if (invalid)
    {
        return -1;
    }

    // 忽略心跳包
    if (pbuf->len == 0)
    {
        return 0;
    }

    // 忽略重复的包
    if (is_dup(pbuf->chksum))
    {
        return 0;
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
