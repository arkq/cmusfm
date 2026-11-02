/*
 * cmusfm - test-server-submit01.c
 * SPDX-FileCopyrightText: 2015-2024 Arkadiusz Bokowy and contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <assert.h>
#include <stdlib.h>

#define DEBUG_SKIP_HICCUP
#include "test-server.inc"

static char track_buffer[512];
static struct cmusfm_data_record * track = (struct cmusfm_data_record *)track_buffer;

static void test_30_seconds_threshold(void) {

	scrobble_count = 0;

	track->status = CMSTATUS_PLAYING;
	track->duration = 30;
	cmusfm_data_record_update_checksum(track);

	cmusfm_server_process_data(NULL, track);

	/* Whole track was played but its duration isn't longer than 30 seconds. */
	sleep(track->duration);

	cmusfm_server_process_data(NULL, track);
	assert(scrobble_count == 0);

	/* It was overplayed (due to seeking) but it's still 30-seconds track. */
	sleep(track->duration + 10);

	track->status = CMSTATUS_STOPPED;
	cmusfm_data_record_update_checksum(track);

	cmusfm_server_process_data(NULL, track);
	assert(scrobble_count == 0);

}

static void test_4_minutes_threshold(void) {

	scrobble_count = 0;

	track->status = CMSTATUS_PLAYING;
	track->duration = 4 * 60 * 5;
	cmusfm_data_record_update_checksum(track);

	cmusfm_server_process_data(NULL, track);

	/* Track was played for more than 4 minutes but less than half its duration. */
	virtual_sleep(4 * 60 + 1);

	track->status = CMSTATUS_STOPPED;
	cmusfm_data_record_update_checksum(track);

	cmusfm_server_process_data(NULL, track);
	assert(scrobble_count == 1);

}

static void test_half_duration_threshold(void) {

	scrobble_count = 0;

	track->status = CMSTATUS_PLAYING;
	track->duration = 35;
	cmusfm_data_record_update_checksum(track);

	cmusfm_server_process_data(NULL, track);

	/* Track was played for more than half its duration. */
	sleep(track->duration / 2 + 1);

	track->status = CMSTATUS_PLAYING;
	track->duration = 35;
	strcpy(((char *)(track + 1)) + track->off_title, "For No One");
	cmusfm_data_record_update_checksum(track);

	cmusfm_server_process_data(NULL, track);
	assert(scrobble_count == 1);

	/* Track was played for less than half its duration. */
	sleep(track->duration / 2 - 1);

	track->status = CMSTATUS_STOPPED;
	cmusfm_data_record_update_checksum(track);

	cmusfm_server_process_data(NULL, track);
	assert(scrobble_count == 1);

}

int main(void) {

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

	test_30_seconds_threshold();
	test_4_minutes_threshold();
	test_half_duration_threshold();

	return EXIT_SUCCESS;
}
