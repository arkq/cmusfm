/*
 * cmusfm - test-server-submit01.c
 * SPDX-FileCopyrightText: 2015-2024 Arkadiusz Bokowy and contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <assert.h>

#define DEBUG_SKIP_HICCUP
#include "test-server.inc"

int main(void) {

	char track_buffer[512];
	struct cmusfm_data_record *track = (struct cmusfm_data_record *)track_buffer;

	track->off_artist = 20;
	track->off_album_artist = 40;
	track->off_album = 60;
	track->off_title = 80;
	track->off_location = 100;

	strcpy(((char *)(track + 1)), "");
	strcpy(((char *)(track + 1)) + track->off_artist, "The Beatles");
	strcpy(((char *)(track + 1)) + track->off_album_artist, "");
	strcpy(((char *)(track + 1)) + track->off_album, "");
	strcpy(((char *)(track + 1)) + track->off_title, "Yellow Submarine");
	strcpy(((char *)(track + 1)) + track->off_location, "");

	config.submit_localfile = true;
	config.submit_shoutcast = true;

	track->status = CMSTATUS_PLAYING;
	track->duration = 35;
	cmusfm_server_update_record_checksum(track);

	cmusfm_server_process_data(NULL, track);

	/* track was played for more than half its duration */
	sleep(track->duration / 2 + 1);

	track->status = CMSTATUS_PLAYING;
	track->duration = 35;
	strcpy(((char *)(track + 1)) + track->off_title, "For No One");
	cmusfm_server_update_record_checksum(track);

	cmusfm_server_process_data(NULL, track);
	assert(scrobbler_scrobble_count == 1);

	/* track was played for less than half its duration */
	sleep(track->duration / 2 - 1);

	track->duration = 30;
	strcpy(((char *)(track + 1)) + track->off_title, "Doctor Robert");
	cmusfm_server_update_record_checksum(track);

	cmusfm_server_process_data(NULL, track);
	assert(scrobbler_scrobble_count == 1);

	/* whole track was played but its duration isn't longer than 30 seconds */
	sleep(track->duration);

	cmusfm_server_process_data(NULL, track);
	assert(scrobbler_scrobble_count == 1);

	/* short track was overplayed (due to seeking) */
	sleep(track->duration + 10);

	track->status = CMSTATUS_STOPPED;
	cmusfm_server_update_record_checksum(track);

	cmusfm_server_process_data(NULL, track);
	assert(scrobbler_scrobble_count == 1);

	return EXIT_SUCCESS;
}
