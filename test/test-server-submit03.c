/*
 * test-server-submit03.c
 * Copyright (c) 2015-2018 Arkadiusz Bokowy
 *
 * This file is a part of cmusfm.
 *
 */

#include <assert.h>
#include <pthread.h>

#define DEBUG_SKIP_HICCUP
#include "test-server.inc"

void *cmusfm_server_worker(void *arg) {
	(void)arg;
	cmusfm_server_start();
	return NULL;
}

pthread_t server_thread;
void cmusfm_server_cleanup(int sig) {
	(void)sig;
	pthread_kill(server_thread, SIGTERM);
	sleep(1);
}

int test_track_minimum(void) {

	struct cmtrack_info track = {
		.status = CMSTATUS_PLAYING,
	};

	cmusfm_server_send_track(&track);
	sleep(1); /* allow server to process data */

	assert(scrobbler_update_now_playing_sbt.mb_track_id == NULL);
	assert(scrobbler_update_now_playing_sbt.artist == NULL);
	assert(scrobbler_update_now_playing_sbt.album_artist == NULL);
	assert(scrobbler_update_now_playing_sbt.album == NULL);
	assert(scrobbler_update_now_playing_sbt.track_number == 0);
	assert(scrobbler_update_now_playing_sbt.track == NULL);
	assert(scrobbler_update_now_playing_sbt.duration == 180);

	/* check case where artist is present */
	track.artist = "The Beatles";
	track.album_artist = NULL;
	track.title = "Yellow Submarine";

	cmusfm_server_send_track(&track);
	sleep(1); /* allow server to process data */

	assert(scrobbler_update_now_playing_sbt.mb_track_id == NULL);
	assert(strcmp(scrobbler_update_now_playing_sbt.artist, "The Beatles") == 0);
	assert(scrobbler_update_now_playing_sbt.album_artist == NULL);
	assert(scrobbler_update_now_playing_sbt.album == NULL);
	assert(scrobbler_update_now_playing_sbt.track_number == 0);
	assert(strcmp(scrobbler_update_now_playing_sbt.track, "Yellow Submarine") == 0);

	/* check case where artist is missing but album artist is present */
	track.artist = NULL;
	track.album_artist = "The Beatles";
	track.title = "Yellow Submarine";

	cmusfm_server_send_track(&track);
	sleep(1); /* allow server to process data */

	assert(scrobbler_update_now_playing_sbt.mb_track_id == NULL);
	assert(strcmp(scrobbler_update_now_playing_sbt.artist, "The Beatles") == 0);
	assert(strcmp(scrobbler_update_now_playing_sbt.album_artist, "The Beatles") == 0);
	assert(scrobbler_update_now_playing_sbt.album == NULL);
	assert(scrobbler_update_now_playing_sbt.track_number == 0);
	assert(strcmp(scrobbler_update_now_playing_sbt.track, "Yellow Submarine") == 0);

	return 3;
}

int test_track_full(void) {

	struct cmtrack_info track = {
		.status = CMSTATUS_PLAYING,
		.file = "The Beatles - Yellow Submarine.ogg",
		.mb_track_id = "b2181aae-5cba-496c-bb0c-b4cc0109ebf8",
		.artist = "The Beatles",
		.album_artist = "The Beatles",
		.album = "Revolver",
		.disc_number = 1,
		.track_number = 6,
		.title = "Yellow Submarine",
		.date = "1966",
		.duration = 160,
	};

	cmusfm_server_send_track(&track);
	sleep(1); /* allow server to process data */

	assert(strcmp(scrobbler_update_now_playing_sbt.mb_track_id, track.mb_track_id) == 0);
	assert(strcmp(scrobbler_update_now_playing_sbt.artist, track.artist) == 0);
	assert(strcmp(scrobbler_update_now_playing_sbt.album_artist, track.album_artist) == 0);
	assert(strcmp(scrobbler_update_now_playing_sbt.album, track.album) == 0);
	assert(scrobbler_update_now_playing_sbt.track_number == track.track_number);
	assert(strcmp(scrobbler_update_now_playing_sbt.track, track.title) == 0);
	assert(scrobbler_update_now_playing_sbt.duration == track.duration);

	return 1;
}

int test_track_parse_file_name(void) {

	struct cmtrack_info track = {
		.status = CMSTATUS_PLAYING,
		.file = "Unknown - Submarine.ogg",
	};

	cmusfm_server_send_track(&track);
	sleep(1); /* allow server to process data */

	assert(scrobbler_update_now_playing_sbt.mb_track_id == NULL);
	assert(strcmp(scrobbler_update_now_playing_sbt.artist, "Unknown") == 0);
	assert(scrobbler_update_now_playing_sbt.album_artist == NULL);
	assert(scrobbler_update_now_playing_sbt.album == NULL);
	assert(scrobbler_update_now_playing_sbt.track_number == 0);
	assert(strcmp(scrobbler_update_now_playing_sbt.track, "Submarine") == 0);
	assert(scrobbler_update_now_playing_sbt.duration == 180);

	return 1;
}

int test_out_of_bounds_write(void) {

	char buffer[CMSOCKET_BUFFER_SIZE / 3] = { 0 };
	size_t i;

	/* fill buffer with printable characters */
	for (i = 0; i < sizeof(buffer) - 1; i++)
		buffer[i] = '0' + (i % ('z' - '0'));

	struct cmtrack_info track = {
		.status = CMSTATUS_PLAYING,
		.artist = buffer,
		.album = buffer,
		.title = buffer,
	};

	cmusfm_server_send_track(&track);
	sleep(1); /* allow server to process data */

	assert(strlen(scrobbler_update_now_playing_sbt.artist) == 340 /* sizeof(buffer) - 1 */);
	assert(strlen(scrobbler_update_now_playing_sbt.album) == 340 /* sizeof(buffer) - 1 */);
	assert(strlen(scrobbler_update_now_playing_sbt.track) == 314);

	char tmp[CMSOCKET_BUFFER_SIZE] = { 0 };
	snprintf(tmp, sizeof(tmp), "%s%s - %s", buffer, buffer, buffer);

	/* check overrun for long URL */
	track.url = "example.com";
	track.artist = NULL;
	track.title = tmp;

	cmusfm_server_send_track(&track);
	sleep(1); /* allow server to process data */

	assert(strlen(scrobbler_update_now_playing_sbt.artist) == 680 /* 2 * (sizeof(buffer) - 1) */);
	assert(strlen(scrobbler_update_now_playing_sbt.track) == 314);

	return 2;
}

int main(void) {

	/* place communication socket in the current directory */
	cmusfm_socket_file = tempnam(".", "tmp-");
	/* use "%artist - %title[.ext]" format for name parser */
	strcpy(config.format_localfile, "^(?A.+) - (?T.+)\\.[^.]+$");
	strcpy(config.format_shoutcast, "^(?A.+) - (?T.+)$");
	/* for the sake of speed we will check now-playing events */
	config.nowplaying_localfile = true;
	config.nowplaying_shoutcast = true;

	pthread_create(&server_thread, NULL, cmusfm_server_worker, NULL);
	/* wait for server to start */
	sleep(1);

	/* register cleanup routine - in the case of assert() termination */
	struct sigaction sigact = { .sa_handler = cmusfm_server_cleanup };
	sigaction(SIGABRT, &sigact, NULL);

	int count = 0;

	count += test_track_minimum();
	assert(scrobbler_update_now_playing_count == count);
	count += test_track_full();
	assert(scrobbler_update_now_playing_count == count);
	count += test_track_parse_file_name();
	assert(scrobbler_update_now_playing_count == count);
	count += test_out_of_bounds_write();
	assert(scrobbler_update_now_playing_count == count);

	cmusfm_server_cleanup(0);
	return EXIT_SUCCESS;
}
