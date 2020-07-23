# cmusfm - configure.ac
# Copyright (c) 2014-2020 Arkadiusz Bokowy

AC_PREREQ([2.59])
AC_INIT([cmusfm], [1.0.0], [arkadiusz.bokowy@gmail.com])
AM_INIT_AUTOMAKE([foreign subdir-objects -Wall -Werror])

AC_CONFIG_HEADERS([config.h])

AC_PROG_CC
AC_PROG_INSTALL
AM_PROG_CC_C_O

# testing presence of pkg-config
AC_MSG_CHECKING([pkg-config m4 macros])
if test m4_ifdef([PKG_CHECK_MODULES], [yes], [no]) = "yes"; then
	AC_MSG_RESULT([yes]);
else
	AC_MSG_RESULT([no]);
	AC_MSG_ERROR([pkg-config is required. See pkg-config.freedesktop.org])
fi

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
PKG_CHECK_MODULES([LIBCRYPTO], [libcrypto], [
	AC_CHECK_HEADERS([openssl/md5.h], [
		AC_DEFINE([WITH_SYSTEM_MD5], [1], [Define to 1 if system MD5 is present.])
	])
], [#skip])

PKG_CHECK_MODULES([LIBCURL], [libcurl])
if $PKG_CONFIG --exact-version=7.71.0 libcurl; then
	AC_MSG_ERROR([libcurl == 7.71.0

Because of curl_easy_escape() bug present in the curl library version 7.71.0,
cmusfm will not work correctly. Please, upgrade or downgrade the curl library.

Related curl bug report: https://github.com/curl/curl/issues/5601])
fi

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
