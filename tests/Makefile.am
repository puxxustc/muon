AM_CFLAGS = -O3 -pipe -W -Wall -fno-strict-aliasing

check_PROGRAMS = md5 hmac_md5 rc4 encapsulate perf

md5_LDADD = ../src/md5.o
hmac_md5_LDADD = ../src/md5.o
rc4_LDADD = ../src/crypto.o ../src/md5.o
encapsulate_LDADD = ../src/crypto.o ../src/compress.o ../src/md5.o \
                    ../src/log.o ../src/encapsulate.o ../src/3rd/minilzo.o
perf_LDADD = ../src/crypto.o ../src/compress.o ../src/md5.o \
             ../src/log.o ../src/encapsulate.o ../src/3rd/minilzo.o

TESTS = $(check_PROGRAMS)
