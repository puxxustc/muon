AM_CFLAGS = -O3 -pipe -W -Wall -fno-strict-aliasing

check_PROGRAMS = test_encapsulate perf

test_encapsulate_LDADD = ../src/crypto.o ../src/compress.o \
                    ../src/log.o ../src/encapsulate.o -llz4 -lsodium
perf_LDADD = ../src/crypto.o ../src/compress.o \
             ../src/log.o ../src/encapsulate.o -llz4 -lsodium

TESTS = test_encapsulate
