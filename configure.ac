# cmusfm - configure.ac
# Copyright (c) 2014 Arkadiusz Bokowy

AC_INIT([cmusfm], [0.2.0], [arkadiusz.bokowy@gmail.com])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([foreign -Wall -Werror])

AC_PROG_CC

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

# support for debugging
AC_ARG_ENABLE(
	[debug],
	AS_HELP_STRING([--enable-debug], [enable debugging support])
)
AS_IF([test "x$enable_debug" = "xyes"],
	[AC_DEFINE([DEBUG], [1], [Define to 1 if the debugging is enabled])]
)

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
