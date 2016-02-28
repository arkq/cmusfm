# cmusfm - configure.ac
# Copyright (c) 2014-2016 Arkadiusz Bokowy

AC_INIT([cmusfm], [0.3.0], [arkadiusz.bokowy@gmail.com])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([foreign subdir-objects -Wall -Werror])

AC_PROG_CC
AM_PROG_CC_C_O

# support for debugging
AC_ARG_ENABLE(
	[debug],
	AS_HELP_STRING([--enable-debug], [enable debugging support])
)
AS_IF([test "x$enable_debug" = "xyes"],
	[AC_DEFINE([DEBUG], [1], [Define to 1 if the debugging is enabled])]
)

AC_CHECK_HEADERS(
	[poll.h],
	[], [AC_MSG_ERROR([poll.h header not found])]
)

# support for configuration reload
AC_CHECK_HEADERS([sys/inotify.h])

PKG_CHECK_MODULES([LIBCURL], [libcurl])
PKG_CHECK_MODULES([LIBCRYPTO], [libcrypto])

# support for desktop notifications
AC_ARG_ENABLE(
	[libnotify],
	AS_HELP_STRING([--enable-libnotify], [enable libnotify support])
)
AM_CONDITIONAL([ENABLE_LIBNOTIFY], [test "x$enable_libnotify" = "xyes"])
AS_IF([test "x$enable_libnotify" = "xyes"], [
	PKG_CHECK_MODULES(
		[LIBNOTIFY], [libnotify >= 0.7],
		[AC_DEFINE([ENABLE_LIBNOTIFY], [1], [Define to 1 if the libnotify is enabled])]
	)
])

AC_CONFIG_FILES([Makefile src/Makefile test/Makefile])
AC_OUTPUT
