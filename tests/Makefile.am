AM_CFLAGS = -pipe -W -Wall -fno-strict-aliasing

check_PROGRAMS = md5 hmac_md5 rc4

md5_LDADD = ../src/md5.o
hmac_md5_LDADD = ../src/md5.o
rc4_LDADD = ../src/crypto.o ../src/md5.o

TESTS = $(check_PROGRAMS)
