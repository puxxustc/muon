/*
 * md5.c - test md5
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
#include <string.h>


extern void md5(void *digest, const void *in, size_t ilen);


int main()
{
    char * tests[][2] = {
        {"", "d41d8cd98f00b204e9800998ecf8427e"},
        {"a", "0cc175b9c0f1b6a831c399e269772661"},
        {"abc", "900150983cd24fb0d6963f7d28e17f72"},
        {"message digest", "f96b697d7cb7938d525a2f31aaf161d0"},
        {"abcdefghijklmnopqrstuvwxyz", "c3fcd3d76192e4007dfb496cca67e13b"},
        {"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789", "d174ab98d277d9f5a5611c2c9f419d9f"},
        {"12345678901234567890123456789012345678901234567890123456789012345678901234567890", "57edf4a22be3c955ac49da2e2107b67a"},
    };
    unsigned char digest[16];
    char hex[33];
    for (int i = 0; i < (int)(sizeof(tests) / sizeof(char *[2])); i++)
    {
        md5(digest, tests[i][0], strlen(tests[i][0]));
        for (int j = 0; j < 16; j++)
        {
            sprintf(hex + j * 2, "%02x", digest[j] & 0xffu);
        }
        hex[32] = '\0';
        printf("%s\n%s\n\n", hex, tests[i][1]);
        fflush(stdout);
        assert(memcmp(hex, tests[i][1], 32) == 0);
    }
    return 0;
}
