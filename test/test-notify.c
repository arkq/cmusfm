/*
 * cmusfm - test-notify.c
 * SPDX-FileCopyrightText: 2015-2026 Arkadiusz Bokowy and contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../src/notify.h"

int main(int argc, char * argv[]) {

	/* The delay between notifications. This can be customized via the command
	 * line argument in order to see all notifications clearly (notifications
	 * are displayed on the screen if the notification daemon is running). */
	unsigned int delay = 0;

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

	scrobbler_trackinfo_t track_null = { 0 };
	scrobbler_trackinfo_t track_empty = {
		.mb_track_id = "",
		.artist = "",
		.album_artist = "",
		.album = "",
		.track = "",
	};
	scrobbler_trackinfo_t track_full = {
		.mb_track_id = "b2181aae-5cba-496c-bb0c-b4cc0109ebf8",
		.artist = "The Beatles",
		.album_artist = "The Beatles",
		.album = "Revolver",
		.track_number = 6,
		.track = "Yellow Submarine",
		.duration = 161,
		.timestamp = 1444444444,
	};
	scrobbler_trackinfo_t track_no_track = {
		.album_artist = "The Beatles",
		.album = "Revolver",
	};

	cmusfm_notify_show(&track_null, NULL);
	sleep(delay);
	cmusfm_notify_show(&track_empty, NULL);
	sleep(delay);
	cmusfm_notify_show(&track_full, NULL);
	sleep(delay);
	cmusfm_notify_show(&track_no_track, NULL);
	sleep(delay);

	cmusfm_notify_free();
	return EXIT_SUCCESS;
}
