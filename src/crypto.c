/*
 * encrypt.c - encryption and decryption
 *
 * Copyright (C) 2014 - 2015, Xiaoxiao <i@xiaoxiao.im>
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
#include <stdint.h>
#include <string.h>
#include "conf.h"
#include "crypto.h"
#include "md5.h"

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#define SWAP(x, y) do {register uint8_t tmp = (x); (x) = (y); (y) = tmp; } while (0)

static void rc4(void *stream, size_t len, const void *key)
{
	register int i, j;
	uint8_t s[256];
	for (i = 0; i < 256; i++)
	{
		s[i] = (uint8_t)i;
	}
	for (i = 0, j = 0; i < 256; i++)
	{
		j = (j + s[i] + ((uint8_t *)key)[i % 16]) & 255;
		SWAP(s[i], s[j]);
	}

#if defined(__GNUC__) && defined(TARGET_LINUX)
#  if defined(__amd64__) || defined(__x86_64__)
#    define RC4_ASM 1
	assert(((uintptr_t)stream & 0x07) == 0);
	__asm__ __volatile__ (
		// 每次处理 8 字节
		"lea 8(%[stream]), %%r8\n\t"
		"cmpq %%r8, %[end]\n\t"
		"jbe 2f\n\t"
		"1:\n\t"
		// i = (i + 1) & 255
		"incl %[i]\n\t"
		"movzbl %b[i], %[i]\n\t"
		// j = (j + s[i] ) & 255
		"movzbl (%[s], %q[i]), %%ecx\n\t"
		"addb %%cl, %b[j]\n\t"
		// SWAP(s[i], s[j])
		// r8 ^= s[(s[i] + s[j]) & 255]
		"movzbl (%[s], %q[j]), %%edx\n\t"
		"movb %%dl, (%[s], %q[i])\n\t"
		"addl %%ecx, %%edx\n\t"
		"movb %%cl, (%[s], %q[j])\n\t"
		"movzbl %%dl, %%edx\n\t"
		"shl $8, %%r8\n\t"
		"movb (%[s], %%rdx), %%r8b\n\t"
		// stream++
		"incq %[stream]\n\t"
		"testq $7, %[stream]\n\t"
		"jne 1b\n\t"
		"bswap %%r8\n\t"
		"xorq %%r8, -8(%[stream])\n\t"
		"lea 8(%[stream]), %%r8\n\t"
		"cmpq %%r8, %[end]\n\t"
		"jg 1b\n\t"
		"2:\n\t"

		// 末尾未对齐的部分，每次处理 1 字节
		"cmpq %[stream], %[end]\n\t"
		"jbe 4f\n\t"
		"3:\n\t"
		// i = (i + 1) & 255
		"incl %[i]\n\t"
		"movzbl %b[i], %[i]\n\t"
		// j = (j + s[i] ) & 255
		"movzbl (%[s], %q[i]), %%ecx\n\t"
		"addb %%cl, %b[j]\n\t"
		// SWAP(s[i], s[j])
		// *((uint8_t *)stream) ^= s[(s[i] + s[j]) & 255]
		"movzbl (%[s], %q[j]), %%edx\n\t"
		"movb %%dl, (%[s], %q[i])\n\t"
		"addl %%ecx, %%edx\n\t"
		"movb %%cl, (%[s], %q[j])\n\t"
		"movzbl %%dl, %%edx\n\t"
		"movb (%[s], %%rdx), %%cl\n\t"
		"xorb %%cl, (%[stream])\n\t"
		// stream++
		"incq %[stream]\n\t"
		"cmpq %[stream], %[end]\n\t"
		"jg 3b\n\t"
		"4:\n\t"
		:
		: [stream] "r" (stream),
		  [end] "g" (stream + len),
		  [s] "r" (s),
		  [i] "a" (0),
		  [j] "b" (0)
		: "memory", "rcx", "rdx", "r8"
	);
#  elif defined(__i386__)
#    define RC4_ASM 1
	__asm__ __volatile__ (
		"cmpl %[stream], %[end]\n\t"
		"je 2f\n\t"
		"1:\n\t"
		/* i = (i + 1) & 255; */
		"incl %[i]\n\t"
		"movzbl %b[i], %[i]\n\t"
		/* j = (j + s[i] ) & 255 */
		"movzbl (%[s], %[i]), %%ecx\n\t"
		"addb %%cl, %b[j]\n\t"
		/* SWAP(s[i], s[j]); */
		/* *((uint8_t *)stream) ^= s[(s[i] + s[j]) & 255]; */
		"movzbl (%[s], %[j]), %%edx\n\t"
		"movb %%dl, (%[s], %[i])\n\t"
		"addl %%ecx, %%edx\n\t"
		"movb %%cl, (%[s], %[j])\n\t"
		"movzbl %%dl, %%edx\n\t"
		"movb (%[s], %%edx), %%cl\n\t"
		"xorb %%cl, (%[stream])\n\t"
		/* stream++ */
		"incl %[stream]\n\t"
		"cmpl %[stream], %[end]\n\t"
		"jne 1b\n\t"
		"2:\n\t"
		:
		: [stream] "r" (stream),
		  [end] "g" (stream + len),
		  [s] "r" (s),
		  [i] "a" (0),
		  [j] "b" (0)
		: "memory", "ecx", "edx"
	);
#  elif defined(__arm__)
#    define RC4_ASM 1
	__asm__ __volatile__ (
		"cmp %[stream], %[end]\n\t"
		"bcs 2f\n\t"
		"1:\n\t"
		/* i = (i + 1) & 255; */
		"add %[i], %[i], #1\n\t"
		"and %[i], %[i], #255\n\t"
		/* j = (j + s[i] ) & 255 */
		"ldrb r4, [%[s], %[i]]\n\t"
		"add %[j], %[j], r4\n\t"
		"and %[j], %[j], #255\n\t"
		/* SWAP(s[i], s[j]); */
		/* *((uint8_t *)stream) ^= s[(s[i] + s[j]) & 255]; */
		"ldrb r5, [%[s], %[j]]\n\t"
		"strb r5, [%[s], %[i]]\n\t"
		"ldrb r6, [%[stream]]\n\t"
		"add r5, r5, r4\n\t"
		"strb r4, [%[s], %[j]]\n\t"
		"and r5, r5, #255\n\t"
		"ldrb r7, [%[s], r5]\n\t"
		"eor r6, r6, r7\n\t"
		"strb r6, [%[stream]], #1\n\t"
		"cmp %[stream], %[end]\n\t"
		"bne 1b\n\t"
		"2:\n\t"
		:
		: [stream] "r"(stream),
		  [end] "r"(stream + len),
		  [s] "r"(s),
		  [i] "r" (0),
		  [j] "r" (0)
		: "memory", "r4", "r5", "r6", "r7"
	);
#  endif
#endif

#ifndef RC4_ASM
	i = 0;
	j = 0;
	register uint8_t *end = (uint8_t *)stream + len;
	for (; (uint8_t *)stream < end; stream++)
	{
		i = (i + 1) & 255;
		j = (j + s[i]) & 255;
		SWAP(s[i], s[j]);
		*((uint8_t *)stream) ^= s[(s[i] + s[j]) & 255];
	}
#endif
}

static char key[32];

void crypto_init(const void *_key)
{
	memcpy(key + 16, _key, 16);
}

void crypto_encrypt(void *buf, size_t len)
{
	char enc_key[16];
	// IV = md5(tun)
	md5(buf, buf + IV_LEN, len);
	// enc_key = md5(IV + key)
	memcpy(key, buf, IV_LEN);
	md5(enc_key, key, IV_LEN + 16);
	// rc4
	rc4(buf + IV_LEN, len, enc_key);
}

int crypto_decrypt(void *buf, size_t len)
{
	char dec_key[16];
	// dec_key = md5(IV + key)
	memcpy(key, buf, IV_LEN);
	md5(dec_key, key, IV_LEN + 16);
	// rc4
	rc4(buf + IV_LEN, len - IV_LEN, dec_key);
	// check if IV == md5(tun)
	md5(dec_key, buf + IV_LEN, len - IV_LEN);
	if (memcmp(dec_key, buf, 16) == 0)
	{
		return 0;
	}
	else
	{
		return -1;
	}
}
