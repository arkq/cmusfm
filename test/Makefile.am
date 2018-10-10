# cmusfm - Makefile.am
# Copyright (c) 2015-2018 Arkadiusz Bokowy

TESTS = \
	test-cache \
	test-server-notify \
	test-server-submit01 \
	test-server-submit02 \
	test-server-submit03

check_PROGRAMS = \
	test-cache \
	test-server-notify \
	test-server-submit01 \
	test-server-submit02 \
	test-server-submit03

test_server_submit03_LDADD = -lpthread

if ENABLE_LIBNOTIFY
TESTS += test-notify
check_PROGRAMS += test-notify
test_notify_CFLAGS = @LIBNOTIFY_CFLAGS@
test_notify_LDADD = @LIBNOTIFY_LIBS@
endif
