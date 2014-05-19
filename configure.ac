# cmusfm - configure.ac
# Copyright (c) 2014 Arkadiusz Bokowy

AC_INIT([cmusfm], [0.2.0], [arkadiusz.bokowy@gmail.com])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([foreign -Wall -Werror])

AC_PROG_CC
AM_PROG_CC_C_O

AC_CHECK_HEADERS(
	[curl/curl.h],
	[], [AC_MSG_ERROR([curl.h header not found])]
)
AC_CHECK_LIB(
	[curl], [curl_easy_init],
	[], [AC_MSG_ERROR([curl library not found])]
)
AC_CHECK_HEADERS(
	[openssl/md5.h],
	[], [AC_MSG_ERROR([md5.h header not found])]
)
AC_CHECK_LIB(
	[crypto], [MD5],
	[], [AC_MSG_ERROR([crypto library not found])]
)

# support for configuration reload
AC_CHECK_HEADERS([sys/inotify.h])

# support for debugging
AC_ARG_ENABLE(
	[debug],
	AS_HELP_STRING([--enable-debug], [enable debugging support])
)
AS_IF([test "x$enable_debug" = "xyes"],
	[AC_DEFINE([DEBUG], [1], [Define to 1 if the debugging is enabled])]
)

# support for libnotify
AC_ARG_ENABLE(
	[libnotify],
	AS_HELP_STRING([--enable-libnotify], [enable libnotify support])
)
AM_CONDITIONAL([ENABLE_LIBNOTIFY], [test "x$enable_libnotify" = "xyes"])
AS_IF([test "x$enable_libnotify" = "xyes"], [
	PKG_CHECK_MODULES(
		[libnotify], [libnotify >= 0.7],
		[AC_DEFINE([ENABLE_LIBNOTIFY], [1], [Define to 1 if the libnotify is enabled])]
	)
])

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
