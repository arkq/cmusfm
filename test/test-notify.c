/*
 * test-notify.c
 * Copyright (c) 2015-2018 Arkadiusz Bokowy
 *
 * This file is a part of cmusfm.
 *
 */

#include "../src/notify.c"

int main(void) {

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
	sleep(1);
	cmusfm_notify_show(&track_empty, NULL);
	sleep(1);
	cmusfm_notify_show(&track_full, NULL);
	sleep(1);
	cmusfm_notify_show(&track_no_track, NULL);
	sleep(1);

	cmusfm_notify_free();
	return EXIT_SUCCESS;
}
