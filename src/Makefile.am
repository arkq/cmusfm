# cmusfm - Makefile.am
# Copyright (c) 2014 Arkadiusz Bokowy

AM_CFLAGS = -Wall -Wextra

bin_PROGRAMS = cmusfm
cmusfm_SOURCES = main.c libscrobbler2.c cache.c config.c server.c
cmusfm_CFLAGS =
cmusfm_LDADD =

if ENABLE_LIBNOTIFY
cmusfm_SOURCES += notify.c
cmusfm_CFLAGS += @libnotify_CFLAGS@
cmusfm_LDADD += @libnotify_LIBS@
endif
