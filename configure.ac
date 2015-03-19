#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_INIT([sipvpn], [0.1.0], [https://github.com/XiaoxiaoPu/sipvpn/issues], [sipvpn], [https://github.com/XiaoxiaoPu/sipvpn])
AC_CONFIG_SRCDIR([sipvpn])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])

AC_CONFIG_AUX_DIR(auto)
AM_INIT_AUTOMAKE([foreign -Wall -Werror])

# Checks for programs.
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
AC_ARG_ENABLE(
    [coverage],
    [AS_HELP_STRING([--enable-coverage], [enable coverage analysis])],
    [CFLAGS="$CFLAGS -fprofile-arcs -ftest-coverage"])

# Checks for header files.
AC_HEADER_ASSERT
AC_CHECK_HEADERS([fcntl.h netdb.h pwd.h stddef.h stdint.h stdio.h stdlib.h string.h sys/ioctl.h sys/select.h sys/socket.h sys/stat.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_FORK
AC_CHECK_FUNCS([bzero select setenv setuid sigaction socket strchr strerror])

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
