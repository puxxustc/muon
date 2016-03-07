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
#include <stdint.h>
#include <string.h>


extern void rc4(void *stream, size_t len, const void *key);

typedef struct
{
    int len;
    char key[16];
    char result[4097];
} test_t;


int main()
{
    test_t tests[] ={
        {
            32,
            {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10},
            "9ac7cc9a609d1ef7b2932899cde41b975248c4959014126a6e8a84f11d1a9e1c"
        },
        {
            2048,
            {0xeb, 0xb4, 0x62, 0x27, 0xc6, 0xcc, 0x8b, 0x37, 0x64, 0x19, 0x10, 0x83, 0x32, 0x22, 0x77, 0x2a},
            "720c94b63edf44e131d950ca211a5a30c366fdeacf9ca80436be7c358424d20b"
            "8d69dc30e575c56b9e35e068e4c8993c74efb1d4073047c19cd87a93ac6a67fd"
            "63d926d29a7fbaa7e820c94f8019ca524499e377cd552bd14954d23d3a13806e"
            "fab261044059b91007f441d89bf511be79e35c0c341da44f3ce73d6587333425"
            "653cfe45fb82623d8f3331fbd47004773bc56e6e84b9b8ee479cb6390b61cd32"
            "91e943c3405759eb1b7272ca47931bb1d5c6fc2c0582a95cd23243a65960fee8"
            "db605d49edb887bdb35704287fed9e5d6822909c3f914078c78c50bacbfa1074"
            "988d9802c9e751447847ac71b1a5fceab3394a40aabf75cba42282ef25a0059f"
            "4847d81da4942dbc249defc48c922b9f39aedcd1029c65655039dd4ca2d9f14a"
            "e8ab8734eedce35ad33f025df7cb064d19f3085755f8beb4e40bdd8a6b921ed4"
            "52280939fdd66a090ff98c5220cdc3c7a48f76211d9a441a911eef902c3d83c0"
            "714c9670e12cb50687eed78a30ce4cf85d0bcd81670cd4862390d39a7e6bd307"
            "f4e4c0146f1b9391d7878b28c2d19c4e86726fe4f5e4084dd735a0193d6ad7c3"
            "d6665d9159f04110b4ef953c9ddea61282c66c09d17c988040d3089067482073"
            "d2eb5867a57b69ba8cf7dee416d1e2fafcbf45874a3b356cc7d1f2322ffc7034"
            "4da724bbd1c3aba87a6935a64cabe26608128c469f275342adda202b2b58da95"
            "970dacef40ad98723bac5d6955b8176114c3ba56b4005b308b0e957fdd995efe"
            "4810ebc4e9b719db61093ce0b85e56e3a461eae0c8765d831bad93c783cffa73"
            "1e943c0edbe6f070bab918ce3cf63b739cafda0315ae5f10d4bd618f0eb74234"
            "cc0af925a2210d120299ab964007dee965df87e9ba3df4387f3d87253460f7dd"
            "3befc2f217eacc7c99897a333b3b85185eeee092b16fc38187c376d74937e0d1"
            "8f619d19138b0be69e0f9174c9c3e392cba03de820c23ce7dcadd2e327dbf549"
            "b2afa4a7cc50fe3fa7c365788ed31c74298ff178e03217b1151671c74789784d"
            "dcc3db5a99d656cecf2a91f5bda663f03cb89993b07b0ced93de13d2a11013ac"
            "ef2d676f1545c2c13dc680a02f4adbfe568be4c9ad29541b6a2543a1b80642a8"
            "0d126edb03794361c42a47fa444824ddc67854f2afed37cf1daff76118575bf1"
            "ecd5ad426186ef01530d38c86a59cd51b368a062a6895878634332b395bc5503"
            "95c89c45f58691fff1932e0b60b4ce2ba1c632b20e5d44686b45a690d3ed3fe4"
            "b29ac98faa3be9d709e4ea659cbefe4239af5af35ae48890eb8b12a50de2750b"
            "37c9e880e98abcee2068dc0dc728bf84f9e268b78d6cdef54a9c30336cbbae54"
            "0b0f83733f996a3af5038390214b403bb864cf28de8efba4f98d0a5bf1d7e6a1"
            "3e0591590e98b091e860040073d78588b60595514f24bc9fe522a6cad7393644"
            "b515a8c5011754f59003058bdb81514e40f692195ee63ffb22ccf20818ba448d"
            "0f998cbf29b5bec46717ba5a9cca851441a8cdf7f8bc275faa4be68007e446f1"
            "26de24f2d13e4c4c0e7a17ca4b2b6c7e2a9fd73133f8e978676e699e4609eb06"
            "2586c542350b66bbcfbe753813d79b921c241af101a6b9470cac3c077ba7ff9f"
            "26984876655e57d59a171a31843bb2806deeeb0594404fd4cfbabc4e5527b104"
            "c743b7572aaa2b9ae085881f1a6410181cdd8cdfa8bd69a9e8d2bfef5f5cc0cd"
            "4af4bc1a70037d0c35c25c665c30a36e0981eb9067b88e4d05fa6c75b87d3c8a"
            "606aceec20de3d0a3a3a6f7480550a4deb60c2b963651deb7fd6de6611034f3b"
            "504734ba3b3fd1ec1e809ce5d3d92a16a8d56424dfca20ae6a942eb90961bc23"
            "bc6c2e8833412848dd4bde6cf3091fd3f1ef79a01fd671554520363272d800bf"
            "90798ce45fc59ba11ee604ac145747c8d1f962a621b6f2c57ce22373b46dedd3"
            "cb93056d5075b8ab7b2f66ffd5b4e25b3aa8b4daf1c858d4c9974c68f9f05b66"
            "d378a5d6403e9de61b4f971b6d88e7c8e8cdae82aa67a0bc625c39a93a86130f"
            "83415443410631ccb2c4dfefee629a1fd084a2ade82b37d69edd753f61899bf7"
            "73c0b7c4dfdacd40e9252ef0550458cb7b44b1778618a7c1fed1704945cdcaff"
            "992c71da6e3b2414c07d2be53b01d5893c70047e8cbc038e3b9820db601da495"
            "1175da6ee756de46a53e2b075660b77073dbdd99f6e77ebd31ccb2ceea8dd31b"
            "e635d452114593ce8e8535b81c4276665ef7547b48ef6e8c81641e0498dd56ca"
            "5cfc72994f5ce492b8d6c80f58dae3fa178f59126d5502d21e8a333bbb0fdebc"
            "b195abfbfd9db4b49bb9b34aad4881a0cda3400300908e91e49b96337bb08d93"
            "ef42d45572a2357611c7dda0d07261fe17e007a49458fd98491c62f650f42869"
            "679a42cda667ec695893e6c6b23af55a5fe91659c082345ede348cc55f93711c"
            "fb52a5c4b847390eda49950bc61f172ef4f87b93a5850df6a12d877dc36c5a6b"
            "299921f6e0403d34dbfbfee16b56e97f102729326b036c14b5eb8c7123645d41"
            "0e7e9e4f3b13836b58bb3521a4ebd4011956029cf1ad36d70a99578f3ea19e0f"
            "cad43d496a3953ecbcb5b84a0bf4636c6b0b75a1a7a20413ee5c984a2d3ce461"
            "c5d1cf34a7700213cea97e0a54dc7b974244b6416345db48a235c6a19d8a7114"
            "af0885fd7d1d5f1ce76b3849706790fd13fb03cc72a9522aea360684fe649e7c"
            "51d7887c600d9dfd3d41a193ea5f6c8a6ecd286ab3eb80ab33723cdd9bd0c42d"
            "8d1883a0a9d12e0d6db2ed9463cf59349e09df6be8af46cef7281eb312e8c168"
            "2fec6f21eff4befbae405002e76054ff4bf5092f27a2f1eb5ef79b95674ef901"
            "56764fcc0aad4d147c0c68ca964b38e500a542bba02111cc2c65b38ebdba587e"
        },
    };
    uint8_t buf[2048];
    char hex[sizeof(buf) * 2 + 1];
    for (int i = 0; i < (int)(sizeof(tests) / sizeof(tests[0])); i++)
    {
        test_t *test = &(tests[i]);
        bzero(buf, test->len);
        rc4(buf, test->len, test->key);
        for (int j = 0; j < test->len; j++)
        {
            hex[j * 2] = "0123456789abcdef"[buf[j] >> 4];
            hex[j * 2 + 1] = "0123456789abcdef"[buf[j] & 0x0fu];
        }
        hex[test->len * 2] = '\0';
        assert(memcmp(hex, test->result, test->len * 2) == 0);
        rc4(buf, test->len, test->key);
        for (int j = 0; j < test->len; j++)
        {
            assert(buf[j] == 0);
        }
    }
    return 0;
}
