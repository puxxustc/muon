/*
 * encrypt.c - encryption and decryption
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

#include <arpa/inet.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>

#include <sodium.h>

#include "conf.h"
#include "crypto.h"


static uint8_t key[32];


int crypto_init(const void *k)
{
    if (sodium_init() == -1) {
        return -1;
    }
    crypto_generichash_blake2b(key, sizeof(key), k, strlen(k), NULL, 0);
    randombytes_set_implementation(&randombytes_salsa20_implementation);
    randombytes_stir();
    return 0;
}


void hmac(void *out, const void *in, size_t inlen)
{
    crypto_generichash_blake2b(out, 16, in, inlen, key, sizeof(key));
}


static void crypto_hmac(pbuf_t *pbuf)
{
    uint8_t mac[16];
    crypto_generichash_blake2b(mac, sizeof(mac), pbuf->payload, pbuf->len, key, sizeof(key));
    memcpy(&(pbuf->chksum), mac, sizeof(pbuf->chksum));
}


void crypto_encrypt(int token, pbuf_t *pbuf)
{
    // fill nonce
    randombytes_buf(pbuf->nonce, sizeof(pbuf->nonce));

    // calc hash
    crypto_hmac(pbuf);

    int len = CRYPTO_LEN(pbuf);

    pbuf->flag = htons(pbuf->flag);
    pbuf->len = htons(pbuf->len);

    uint8_t onetimekey[32];
    uint8_t factor[2] = { token & 0xffu, (token >> 8) & 0xffu };
    crypto_generichash_blake2b(onetimekey, sizeof(onetimekey), key, sizeof(key), factor, sizeof(factor));

    // encrypt
    crypto_stream_chacha20_xor(
        (void *)CRYPTO_START(pbuf),
        (void *)CRYPTO_START(pbuf),
        len,
        pbuf->nonce,
        onetimekey);
}


int crypto_decrypt(int token, pbuf_t *pbuf, size_t len)
{
    uint8_t onetimekey[32];
    uint8_t factor[2] = { token & 0xff, (token >> 8) & 0xff };
    crypto_generichash_blake2b(onetimekey, sizeof(onetimekey), key, sizeof(key), factor, sizeof(factor));

    // decrypt
    crypto_stream_chacha20_xor(
        (void *)CRYPTO_START(pbuf),
        (void *)CRYPTO_START(pbuf),
        len - CRYPTO_NONCE_LEN,
        pbuf->nonce,
        onetimekey);

    pbuf->flag = ntohs(pbuf->flag);
    pbuf->len = ntohs(pbuf->len);

    // check pbuf->len
    if (pbuf->len > sizeof(pbuf->payload))
    {
        return -1;
    }

    // check if chksum is valid
    uint32_t chksum = pbuf->chksum;
    crypto_hmac(pbuf);
    return (chksum == pbuf->chksum) ? 0 : -1;
}
