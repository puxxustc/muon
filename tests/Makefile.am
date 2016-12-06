AM_CFLAGS = -O3 -pipe -W -Wall -fno-strict-aliasing

check_PROGRAMS = test_md5 test_hmac_md5 test_rc4 test_encapsulate perf

test_md5_LDADD = ../src/md5.o
test_hmac_md5_LDADD = ../src/md5.o
test_rc4_LDADD = ../src/crypto.o ../src/md5.o
test_encapsulate_LDADD = ../src/crypto.o ../src/compress.o ../src/md5.o \
                    ../src/log.o ../src/encapsulate.o ../src/3rd/minilzo.o
perf_LDADD = ../src/crypto.o ../src/compress.o ../src/md5.o \
             ../src/log.o ../src/encapsulate.o ../src/3rd/minilzo.o

TESTS = test_md5 test_hmac_md5 test_rc4 test_encapsulate
