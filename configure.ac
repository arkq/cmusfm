# cmusfm - configure.ac
# Copyright (c) 2014-2017 Arkadiusz Bokowy

AC_PREREQ([2.59])
AC_INIT([cmusfm], [0.3.2], [arkadiusz.bokowy@gmail.com])
AM_INIT_AUTOMAKE([foreign subdir-objects -Wall -Werror])

AC_CONFIG_HEADERS([config.h])

AC_PROG_CC
AC_PROG_INSTALL
AM_PROG_CC_C_O

# support for debugging
AC_ARG_ENABLE([debug],
	AS_HELP_STRING([--enable-debug], [enable debugging support]))
AM_CONDITIONAL([ENABLE_DEBUG], [test "x$enable_debug" = "xyes"])
AM_COND_IF([ENABLE_DEBUG], [
	AC_DEFINE([DEBUG], [1], [Define to 1 if the debugging is enabled.])
])

AC_CHECK_HEADERS([poll.h],
	[], [AC_MSG_ERROR([poll.h header not found])])

# support for configuration reload
AC_CHECK_HEADERS([sys/inotify.h])

# support for system-wide MD5
LIBCRYPTO_CFLAGS=
LIBCRYPTO_LIBS=
AC_CHECK_HEADERS([openssl/md5.h], [
	PKG_CHECK_MODULES([LIBCRYPTO], [libcrypto])
])

PKG_CHECK_MODULES([LIBCURL], [libcurl])

# support for desktop notifications
AC_ARG_ENABLE([libnotify],
	AS_HELP_STRING([--enable-libnotify], [enable libnotify support]))
AM_CONDITIONAL([ENABLE_LIBNOTIFY], [test "x$enable_libnotify" = "xyes"])
AM_COND_IF([ENABLE_LIBNOTIFY], [
	PKG_CHECK_MODULES([LIBNOTIFY], [libnotify >= 0.7])
	AC_DEFINE([ENABLE_LIBNOTIFY], [1], [Define to 1 if libnotify is enabled.])
])

AC_CONFIG_FILES([Makefile src/Makefile test/Makefile])
AC_OUTPUT
