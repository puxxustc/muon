#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_INIT([muon], [0.4.0], [https://github.com/XiaoxiaoPu/muon/issues], [muon], [https://github.com/XiaoxiaoPu/muon])
AC_CONFIG_SRCDIR([muon])
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADERS([config.h])

AC_CONFIG_AUX_DIR([.])
AC_CONFIG_MACRO_DIR([m4])
AC_CANONICAL_HOST
AM_INIT_AUTOMAKE([foreign -Wall -Werror])

# Checks for programs.
: ${CFLAGS=""}
AC_PROG_CC_C99
AC_PROG_INSTALL

# Custom options
AC_ARG_ENABLE(
    [static],
    [AS_HELP_STRING([--enable-static], [build with static linking])],
    [LDFLAGS="$LDFLAGS -static"])
AM_CONDITIONAL(STATIC, test x"$static" = x"true")
AC_ARG_ENABLE(
    [debug],
    [AS_HELP_STRING([--enable-debug], [build with additional debugging code])],
    [CFLAGS="$CFLAGS -g -DDEBUG -O0"])
AM_CONDITIONAL(DEBUG, test x"$debug" = x"true")

# Check endian
AC_C_BIGENDIAN(
    [],
    [CFLAGS="$CFLAGS -DAC_LITTLE_ENDIAN=1"],
    [])

# Checks for libraries.
LIB_MILL="-lmill"
AC_CHECK_LIB([mill], [go], [AC_SUBST(LIB_MILL)])

# Checks for header files.
AC_HEADER_ASSERT
AC_CHECK_HEADERS([arpa/inet.h fcntl.h libmill.h limits.h netdb.h pwd.h stddef.h stdint.h stdio.h stdlib.h string.h sys/ioctl.h sys/socket.h sys/stat.h sys/time.h sys/wait.h unistd.h])

# Check build target
case "$host" in
    *-*-linux*)
        AC_DEFINE([TARGET_LINUX], [1], [Build for Linux])
        AC_CHECK_HEADER([linux/if_tun.h])
        ;;
    *-*-darwin*)
        AC_DEFINE([TARGET_DARWIN], [1], [Build OS X])
        AC_CHECK_HEADER([net/if_utun.h])
        ;;
    *)
        AC_DEFINE([TARGET_UNKNOWN], ["$host"], [Unsupported build target])
esac

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_INT64_T
AC_TYPE_UINT64_T
AC_CHECK_TYPES([ptrdiff_t])

# Checks for library functions.
AC_FUNC_FORK
AC_CHECK_FUNCS([bzero gettimeofday memmove memset sigaction socket strchr strerror])

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 tests/Makefile])
AC_OUTPUT
