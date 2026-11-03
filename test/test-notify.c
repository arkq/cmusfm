/*
 * cmusfm - test-notify.c
 * SPDX-FileCopyrightText: 2015-2026 Arkadiusz Bokowy and contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <check.h>

#include "../src/notify.h"

START_TEST(test_notify_show_null) {
	scrobbler_trackinfo_t info = { 0 };
	cmusfm_notify_show(&info, NULL);
}

START_TEST(test_notify_show_empty) {
	scrobbler_trackinfo_t info = {
		.mb_track_id = "",
		.artist = "",
		.album_artist = "",
		.album = "",
		.track = "",
	};
	cmusfm_notify_show(&info, NULL);
}

START_TEST(test_notify_show_full) {
	scrobbler_trackinfo_t info = {
		.mb_track_id = "b2181aae-5cba-496c-bb0c-b4cc0109ebf8",
		.artist = "The Beatles",
		.album_artist = "The Beatles",
		.album = "Revolver",
		.track_number = 6,
		.track = "Yellow Submarine",
		.duration = 161,
		.timestamp = 1444444444,
	};
	cmusfm_notify_show(&info, NULL);
}

START_TEST(test_notify_show_no_track_info) {
	scrobbler_trackinfo_t info = {
		.album_artist = "The Beatles",
		.album = "Revolver",
	};
	cmusfm_notify_show(&info, NULL);
}

/* The delay between notifications. This can be customized via the command
 * line argument in order to see all notifications clearly (notifications
 * are displayed on the screen if the notification daemon is running). */
static unsigned int delay = 0;

static void tc_setup_delay(void) { sleep(delay); }

static void tc_teardown_notify(void) { cmusfm_notify_free(); }

int main(int argc, char * argv[]) {

	int opt;
	const char * opts = "hd:";
	struct option longopts[] = {
		{ "help", no_argument, NULL, 'h' },
		{ "delay", required_argument, NULL, 'd' },
		{ 0 },
	};

	while ((opt = getopt_long(argc, argv, opts, longopts, NULL)) != -1)
		switch (opt) {
		case 'h' /* --help */:
			printf("Usage: %s [--delay <seconds>]\n", argv[0]);
			return EXIT_SUCCESS;
		case 'd' /* --delay */:
			delay = atoi(optarg);
			break;
		default:
			fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
			return EXIT_FAILURE;
		}

	Suite * s = suite_create(__FILE__);
	TCase * tc = tcase_create(__FILE__);
	SRunner * sr = srunner_create(s);

	suite_add_tcase(s, tc);
	tcase_add_checked_fixture(tc, tc_setup_delay, tc_teardown_notify);

	tcase_add_test(tc, test_notify_show_null);
	tcase_add_test(tc, test_notify_show_empty);
	tcase_add_test(tc, test_notify_show_full);
	tcase_add_test(tc, test_notify_show_no_track_info);

	srunner_run_all(sr, CK_ENV);
	int nf = srunner_ntests_failed(sr);

	srunner_free(sr);
	return nf == 0 ? 0 : 1;
}
