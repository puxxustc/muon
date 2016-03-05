/*
 * md5.c - test md5
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
#include <stdio.h>
#include <string.h>


extern void hmac_md5(void *digest, const void *key, size_t klen, const void *in, size_t ilen);


int main()
{
    char * tests[][3] = {
        {
            "\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b",
            "Hi There",
            "9294727a3638bb1c13f48ef8158bfc9d"
        },
        {
            "Jefe",
            "what do ya want for nothing?",
            "750c783e6ab0b503eaa86e310a5db738"
        },
        {
            "\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa",
            "\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd",
            "56be34521d144c88dbb8c733f0e8b3f6"
        },
        {
            "\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19",
            "\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd",
            "697eaf0aca3a3aea3a75164746ffaa79"
        },
        {
            "\x0c\x0c\x0c\x0c\x0c\x0c\x0c\x0c\x0c\x0c\x0c\x0c\x0c\x0c\x0c\x0c",
            "Test With Truncation",
            "56461ef2342edc00f9bab995690efd4c"
        },
        {
            "\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa",
            "Test Using Larger Than Block-Size Key - Hash Key First",
            "6b1ab7fe4bd7bf8f0b62e6ce61b9d0cd"
        },
        {
            "\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa",
            "Test Using Larger Than Block-Size Key and Larger Than One Block-Size Data",
            "6f630fad67cda0ee1fb1f562db3aa53e"
        },
    };
    unsigned char digest[16];
    char hex[33];
    for (int i = 0; i < (int)(sizeof(tests) / sizeof(char *[3])); i++)
    {
        hmac_md5(digest, tests[i][0], strlen(tests[i][0]), tests[i][1], strlen(tests[i][1]));
        for (int j = 0; j < 16; j++)
        {
            sprintf(hex + j * 2, "%02x", digest[j] & 0xffu);
        }
        hex[32] = '\0';
        printf("%d:\n%s\n\n", i, hex);
        assert(memcmp(hex, tests[i][2], 32) == 0);
    }
    return 0;
}
