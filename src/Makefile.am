AM_CFLAGS = -O3 -pipe -W -Wall -fno-strict-aliasing

bin_PROGRAMS = muon

muon_SOURCES = \
    conf.c  compress.c  crypto.c  encapsulate.c  log.c  md5.c  tunif.c  utils.c  vpn.c  minilzo.c  main.c \
    conf.h  compress.h  crypto.h  encapsulate.h  log.h  md5.h  tunif.h  utils.h  vpn.h  minilzo.h  lzoconf.h  lzodefs.h
muon_LDADD = $(LIB_MILL)
