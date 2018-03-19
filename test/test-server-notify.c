/*
 * test-server-notify.c
 * Copyright (c) 2015-2018 Arkadiusz Bokowy
 *
 * This file is a part of cmusfm.
 *
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

	config.nowplaying_localfile = true;
	config.nowplaying_shoutcast = true;

	track->status = CMSTATUS_PLAYING;
	cmusfm_server_update_record_checksum(track);

	cmusfm_server_process_data(NULL, track);
	assert(scrobbler_update_now_playing_count == 1);
	assert(strcmp(scrobbler_update_now_playing_sbt.artist, "The Beatles") == 0);
	assert(strcmp(scrobbler_update_now_playing_sbt.track, "Yellow Submarine") == 0);

	config.nowplaying_localfile = false;

	cmusfm_server_process_data(NULL, track);
	assert(scrobbler_update_now_playing_count == 1);

#if ENABLE_LIBNOTIFY
	config.notification = true;
#endif

	track->status |= CMSTATUS_SHOUTCASTMASK;
	cmusfm_server_update_record_checksum(track);

	cmusfm_server_process_data(NULL, track);
	assert(scrobbler_update_now_playing_count == 2);
#if ENABLE_LIBNOTIFY
	assert(cmusfm_notify_show_count == 1);
#endif

	/* reshow now playing notification after a long pause */

	config.nowplaying_localfile = true;

	track->status = CMSTATUS_PLAYING;
	cmusfm_server_update_record_checksum(track);

	cmusfm_server_process_data(NULL, track);
	assert(scrobbler_update_now_playing_count == 3);

	/* track was played for a few seconds */
	sleep(10);

	track->status = CMSTATUS_PAUSED;
	cmusfm_server_update_record_checksum(track);

	cmusfm_server_process_data(NULL, track);
	assert(scrobbler_update_now_playing_count == 3);

	/* track was unpaused after more than 120 seconds */
	sleep(121);

	track->status = CMSTATUS_PLAYING;
	cmusfm_server_update_record_checksum(track);

	cmusfm_server_process_data(NULL, track);
	assert(scrobbler_update_now_playing_count == 4);
#if ENABLE_LIBNOTIFY
	assert(cmusfm_notify_show_count == 3);
#endif

	return EXIT_SUCCESS;
}
