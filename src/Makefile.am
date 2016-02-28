# cmusfm - Makefile.am
# Copyright (c) 2014-2016 Arkadiusz Bokowy

bin_PROGRAMS = cmusfm

cmusfm_SOURCES = \
	cache.c \
	config.c \
	libscrobbler2.c \
	server.c \
	utils.c \
	main.c

cmusfm_CFLAGS = \
	@LIBCURL_CFLAGS@ \
	@LIBCRYPTO_CFLAGS@

cmusfm_LDADD = \
	@LIBCURL_LIBS@ \
	@LIBCRYPTO_LIBS@

if ENABLE_LIBNOTIFY
cmusfm_SOURCES += notify.c
cmusfm_CFLAGS += @LIBNOTIFY_CFLAGS@
cmusfm_LDADD += @LIBNOTIFY_LIBS@
endif
