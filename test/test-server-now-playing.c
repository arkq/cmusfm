/*
 * cmusfm - test-server-now-playing.c
 * SPDX-FileCopyrightText: 2015-2026 Arkadiusz Bokowy and contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <pthread.h>
#include <stdlib.h>

#include <check.h>

#define DEBUG_SKIP_HICCUP
#include "test-server.inc"

START_TEST(test_server_check) {
	ck_assert_int_eq(cmusfm_server_check(), 1);
}

START_TEST(test_track_minimum) {

	struct cmtrack_info track = { .status = CMSTATUS_PLAYING };

	ck_assert_int_eq(cmusfm_server_send_track(&track), 0);
	ck_assert_int_eq(wait_for_now_playing_update(5), 0);
	ck_assert_uint_eq(now_playing_count, 1);

	ck_assert_ptr_null(now_playing_sbt.mb_track_id);
	ck_assert_ptr_null(now_playing_sbt.artist);
	ck_assert_ptr_null(now_playing_sbt.album_artist);
	ck_assert_ptr_null(now_playing_sbt.album);
	ck_assert_int_eq(now_playing_sbt.track_number, 0);
	ck_assert_ptr_null(now_playing_sbt.track);
	ck_assert_int_eq(now_playing_sbt.duration, 180);

	/* Check case where artist is present. */
	track.artist = "The Beatles";
	track.album_artist = NULL;
	track.title = "Yellow Submarine";

	ck_assert_int_eq(cmusfm_server_send_track(&track), 0);
	ck_assert_int_eq(wait_for_now_playing_update(5), 0);
	ck_assert_uint_eq(now_playing_count, 2);

	ck_assert_ptr_null(now_playing_sbt.mb_track_id);
	ck_assert_str_eq(now_playing_sbt.artist, "The Beatles");
	ck_assert_ptr_null(now_playing_sbt.album_artist);
	ck_assert_ptr_null(now_playing_sbt.album);
	ck_assert_int_eq(now_playing_sbt.track_number, 0);
	ck_assert_str_eq(now_playing_sbt.track, "Yellow Submarine");

	/* Check case where artist is missing but album artist is present. */
	track.artist = NULL;
	track.album_artist = "The Beatles";
	track.title = "Yellow Submarine";

	ck_assert_int_eq(cmusfm_server_send_track(&track), 0);
	ck_assert_int_eq(wait_for_now_playing_update(5), 0);
	ck_assert_uint_eq(now_playing_count, 3);

	ck_assert_ptr_null(now_playing_sbt.mb_track_id);
	ck_assert_str_eq(now_playing_sbt.artist, "The Beatles");
	ck_assert_str_eq(now_playing_sbt.album_artist, "The Beatles");
	ck_assert_ptr_null(now_playing_sbt.album);
	ck_assert_int_eq(now_playing_sbt.track_number, 0);
	ck_assert_str_eq(now_playing_sbt.track, "Yellow Submarine");

}

START_TEST(test_track_full) {

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

	ck_assert_int_eq(cmusfm_server_send_track(&track), 0);
	ck_assert_int_eq(wait_for_now_playing_update(5), 0);
	ck_assert_uint_eq(now_playing_count, 1);

	ck_assert_str_eq(now_playing_sbt.mb_track_id, track.mb_track_id);
	ck_assert_str_eq(now_playing_sbt.artist, track.artist);
	ck_assert_str_eq(now_playing_sbt.album_artist, track.album_artist);
	ck_assert_str_eq(now_playing_sbt.album, track.album);
	ck_assert_int_eq(now_playing_sbt.track_number, track.track_number);
	ck_assert_str_eq(now_playing_sbt.track, track.title);
	ck_assert_int_eq(now_playing_sbt.duration, track.duration);

}

START_TEST(test_track_parse_file_name) {

	struct cmtrack_info track = {
		.status = CMSTATUS_PLAYING,
		.file = "Unknown - Submarine.ogg",
	};

	ck_assert_int_eq(cmusfm_server_send_track(&track), 0);
	ck_assert_int_eq(wait_for_now_playing_update(5), 0);
	ck_assert_uint_eq(now_playing_count, 1);

	ck_assert_ptr_null(now_playing_sbt.mb_track_id);
	ck_assert_str_eq(now_playing_sbt.artist, "Unknown");
	ck_assert_ptr_null(now_playing_sbt.album_artist);
	ck_assert_ptr_null(now_playing_sbt.album);
	ck_assert_int_eq(now_playing_sbt.track_number, 0);
	ck_assert_str_eq(now_playing_sbt.track, "Submarine");
	ck_assert_int_eq(now_playing_sbt.duration, 180);

}

START_TEST(test_out_of_bounds_write) {

	char buffer[CMSOCKET_BUFFER_SIZE / 3] = { 0 };

	/* Fill buffer with printable characters. */
	for (size_t i = 0; i < sizeof(buffer) - 1; i++)
		buffer[i] = '0' + (i % ('z' - '0'));

	struct cmtrack_info track = {
		.status = CMSTATUS_PLAYING,
		.artist = buffer,
		.album = buffer,
		.title = buffer,
	};

	ck_assert_int_eq(cmusfm_server_send_track(&track), 0);
	ck_assert_int_eq(wait_for_now_playing_update(5), 0);
	ck_assert_uint_eq(now_playing_count, 1);

	ck_assert_int_eq(strlen(now_playing_sbt.artist), 340 /* sizeof(buffer) - 1 */);
	ck_assert_int_eq(strlen(now_playing_sbt.album), 340 /* sizeof(buffer) - 1 */);
	ck_assert_int_eq(strlen(now_playing_sbt.track), 314);

	char tmp[CMSOCKET_BUFFER_SIZE] = { 0 };
	snprintf(tmp, sizeof(tmp), "%s%s - %s", buffer, buffer, buffer);

	/* Check overrun for long URL. */
	track.url = "example.com";
	track.artist = NULL;
	track.title = tmp;

	ck_assert_int_eq(cmusfm_server_send_track(&track), 0);
	ck_assert_int_eq(wait_for_now_playing_update(5), 0);
	ck_assert_uint_eq(now_playing_count, 2);

	ck_assert_int_eq(strlen(now_playing_sbt.artist), 680 /* 2 * (sizeof(buffer) - 1) */);
	ck_assert_int_eq(strlen(now_playing_sbt.track), 314);

}

static pthread_t server_thread;

static void * server_worker(void * arg) {
	(void)arg;
	ck_assert_int_eq(cmusfm_server_start(), 0);
	return NULL;
}

static void tc_setup_server(void) {

	now_playing_wait_count = 0;
	now_playing_count = 0;

	server_on = true;
	pthread_create(&server_thread, NULL, server_worker, NULL);
	ck_assert_int_eq(wait_for_cmusfm_server(5), 0);

}

static void tc_teardown_server(void) {
	pthread_kill(server_thread, SIGTERM);
	pthread_join(server_thread, NULL);
	server_ready = false;
}

int main(void) {

	/* Place communication socket in the current directory. */
	cmusfm_socket_file = tempnam(".", "tmp-");
	/* Use "%artist - %title[.ext]" format for name parser. */
	strcpy(config.format_localfile, "^(?A.+) - (?T.+)\\.[^.]+$");
	strcpy(config.format_shoutcast, "^(?A.+) - (?T.+)$");

	config.nowplaying_localfile = true;
	config.nowplaying_shoutcast = true;

	Suite * s = suite_create(__FILE__);
	TCase * tc = tcase_create(__FILE__);
	SRunner * sr = srunner_create(s);

	suite_add_tcase(s, tc);
	tcase_add_checked_fixture(tc, tc_setup_server, tc_teardown_server);

	tcase_add_test(tc, test_server_check);
	tcase_add_test(tc, test_track_minimum);
	tcase_add_test(tc, test_track_full);
	tcase_add_test(tc, test_track_parse_file_name);
	tcase_add_test(tc, test_out_of_bounds_write);

	srunner_run_all(sr, CK_ENV);
	int nf = srunner_ntests_failed(sr);

	srunner_free(sr);
	return nf == 0 ? 0 : 1;
}
