/*
 * md5.c - the MD5 Message-Digest Algorithm (RFC 1321)
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

#include <stdint.h>
#include <string.h>
#include "md5.h"

static inline uint32_t F(uint32_t x, uint32_t y, uint32_t z)
{
	return (x & y) | (~x & z);
}

static inline uint32_t G(uint32_t x, uint32_t y, uint32_t z)
{
	return (x & z) | (y & ~z);
}

static inline uint32_t H(uint32_t x, uint32_t y, uint32_t z)
{
	return x ^ y ^ z;
}

static inline uint32_t I(uint32_t x, uint32_t y, uint32_t z)
{
	return y ^ (x | ~z);
}

static inline uint32_t LEFT_ROTATE(uint32_t x, uint32_t n)
{
	return (x << n) | (x >> (32 - n));
}

#define FF(a, b, c, d, x, s, ac) \
	do {(a) = (b) + LEFT_ROTATE((a) + F((b), (c), (d)) + (x) + (ac), s);} while (0)

#define GG(a, b, c, d, x, s, ac) \
	do {(a) = (b) + LEFT_ROTATE((a) + G((b), (c), (d)) + (x) + (ac), s);} while (0)

#define HH(a, b, c, d, x, s, ac) \
	do {(a) = (b) + LEFT_ROTATE((a) + H((b), (c), (d)) + (x) + (ac), s);} while (0)

#define II(a, b, c, d, x, s, ac) \
	do {(a) = (b) + LEFT_ROTATE((a) + I((b), (c), (d)) + (x) + (ac), s);} while (0)

void md5(void *digest, const void *in, size_t len)
{
	size_t n = len * 8 / 512 + 1;
	uint32_t M[(n + 1) * 16];

	bzero(M, sizeof(M));
	for (size_t i = 0; i < len; i++)
	{
		M[i / 4] |= ((unsigned char *)in)[i] << 8 * ((i % 4));
	}
	M[len / 4] |= 0x80 << 8 * ((len % 4));
	M[n * 16 - 2] = len * 8;

	uint32_t state[4] = {0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476};

	for (size_t i = 0; i < n; i++)
	{
		uint32_t X[16];
		uint32_t a, b, c, d;

		for (int j = 0; j < 16; j++)
		{
			X[j] = M[i * 16 + j];
		}

		a = state[0];
		b = state[1];
		c = state[2];
		d = state[3];

		// 第一轮
		FF(a, b, c, d, X[ 0],  7, 0xd76aa478); // 1
		FF(d, a, b, c, X[ 1], 12, 0xe8c7b756); // 2
		FF(c, d, a, b, X[ 2], 17, 0x242070db); // 3
		FF(b, c, d, a, X[ 3], 22, 0xc1bdceee); // 4
		FF(a, b, c, d, X[ 4],  7, 0xf57c0faf); // 5
		FF(d, a, b, c, X[ 5], 12, 0x4787c62a); // 6
		FF(c, d, a, b, X[ 6], 17, 0xa8304613); // 7
		FF(b, c, d, a, X[ 7], 22, 0xfd469501); // 8
		FF(a, b, c, d, X[ 8],  7, 0x698098d8); // 9
		FF(d, a, b, c, X[ 9], 12, 0x8b44f7af); // 10
		FF(c, d, a, b, X[10], 17, 0xffff5bb1); // 11
		FF(b, c, d, a, X[11], 22, 0x895cd7be); // 12
		FF(a, b, c, d, X[12],  7, 0x6b901122); // 13
		FF(d, a, b, c, X[13], 12, 0xfd987193); // 14
		FF(c, d, a, b, X[14], 17, 0xa679438e); // 15
		FF(b, c, d, a, X[15], 22, 0x49b40821); // 16

		// 第二轮
		GG(a, b, c, d, X[ 1],  5, 0xf61e2562); // 17
		GG(d, a, b, c, X[ 6],  9, 0xc040b340); // 18
		GG(c, d, a, b, X[11], 14, 0x265e5a51); // 19
		GG(b, c, d, a, X[ 0], 20, 0xe9b6c7aa); // 20
		GG(a, b, c, d, X[ 5],  5, 0xd62f105d); // 21
		GG(d, a, b, c, X[10],  9, 0x02441453); // 22
		GG(c, d, a, b, X[15], 14, 0xd8a1e681); // 23
		GG(b, c, d, a, X[ 4], 20, 0xe7d3fbc8); // 24
		GG(a, b, c, d, X[ 9],  5, 0x21e1cde6); // 25
		GG(d, a, b, c, X[14],  9, 0xc33707d6); // 26
		GG(c, d, a, b, X[ 3], 14, 0xf4d50d87); // 27
		GG(b, c, d, a, X[ 8], 20, 0x455a14ed); // 28
		GG(a, b, c, d, X[13],  5, 0xa9e3e905); // 29
		GG(d, a, b, c, X[ 2],  9, 0xfcefa3f8); // 30
		GG(c, d, a, b, X[ 7], 14, 0x676f02d9); // 31
		GG(b, c, d, a, X[12], 20, 0x8d2a4c8a); // 32

		// 第三轮
		HH(a, b, c, d, X[ 5],  4, 0xfffa3942); // 33
		HH(d, a, b, c, X[ 8], 11, 0x8771f681); // 34
		HH(c, d, a, b, X[11], 16, 0x6d9d6122); // 35
		HH(b, c, d, a, X[14], 23, 0xfde5380c); // 36
		HH(a, b, c, d, X[ 1],  4, 0xa4beea44); // 37
		HH(d, a, b, c, X[ 4], 11, 0x4bdecfa9); // 38
		HH(c, d, a, b, X[ 7], 16, 0xf6bb4b60); // 39
		HH(b, c, d, a, X[10], 23, 0xbebfbc70); // 40
		HH(a, b, c, d, X[13],  4, 0x289b7ec6); // 41
		HH(d, a, b, c, X[ 0], 11, 0xeaa127fa); // 42
		HH(c, d, a, b, X[ 3], 16, 0xd4ef3085); // 43
		HH(b, c, d, a, X[ 6], 23, 0x04881d05); // 44
		HH(a, b, c, d, X[ 9],  4, 0xd9d4d039); // 45
		HH(d, a, b, c, X[12], 11, 0xe6db99e5); // 46
		HH(c, d, a, b, X[15], 16, 0x1fa27cf8); // 47
		HH(b, c, d, a, X[ 2], 23, 0xc4ac5665); // 48

		// 第四轮
		II(a, b, c, d, X[ 0],  6, 0xf4292244); // 49
		II(d, a, b, c, X[ 7], 10, 0x432aff97); // 50
		II(c, d, a, b, X[14], 15, 0xab9423a7); // 51
		II(b, c, d, a, X[ 5], 21, 0xfc93a039); // 52
		II(a, b, c, d, X[12],  6, 0x655b59c3); // 53
		II(d, a, b, c, X[ 3], 10, 0x8f0ccc92); // 54
		II(c, d, a, b, X[10], 15, 0xffeff47d); // 55
		II(b, c, d, a, X[ 1], 21, 0x85845dd1); // 56
		II(a, b, c, d, X[ 8],  6, 0x6fa87e4f); // 57
		II(d, a, b, c, X[15], 10, 0xfe2ce6e0); // 58
		II(c, d, a, b, X[ 6], 15, 0xa3014314); // 59
		II(b, c, d, a, X[13], 21, 0x4e0811a1); // 60
		II(a, b, c, d, X[ 4],  6, 0xf7537e82); // 61
		II(d, a, b, c, X[11], 10, 0xbd3af235); // 62
		II(c, d, a, b, X[ 2], 15, 0x2ad7d2bb); // 63
		II(b, c, d, a, X[ 9], 21, 0xeb86d391); // 64

		state[0] += a;
		state[1] += b;
		state[2] += c;
		state[3] += d;
	}

	if (digest != NULL)
	{
		memcpy(digest, state, sizeof(uint32_t) * 4);
	}
}
