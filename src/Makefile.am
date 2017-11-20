AM_CFLAGS = -O3 -pipe -W -Wall -fno-strict-aliasing

bin_PROGRAMS = muon

muon_SOURCES = \
    conf.c  compress.c  crypto.c  encapsulate.c  log.c  tunif.c  utils.c  vpn.c \
    conf.h  compress.h  crypto.h  encapsulate.h  log.h  tunif.h  utils.h  vpn.h \
    main.c
muon_LDADD = $(LIB_LZ4) $(LIB_MILL) $(LIB_SODIUM)
