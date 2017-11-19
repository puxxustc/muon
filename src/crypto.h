/*
 * crypto.h - encryption and decryption
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

#ifndef CRYPTO_H
#define CRYPTO_H

#include <stddef.h>
#include "encapsulate.h"


extern int crypto_init(const void *psk);
extern void hmac(void *out, const void *in, size_t inlen);
extern void crypto_encrypt(pbuf_t *pbuf);
extern int  crypto_decrypt(pbuf_t *pbuf, size_t len);


#endif // CRYPTO_H
