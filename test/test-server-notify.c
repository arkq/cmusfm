/*
 * cmusfm - test-server-notify.c
 * SPDX-FileCopyrightText: 2015-2024 Arkadiusz Bokowy and contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <check.h>

#define DEBUG_SKIP_HICCUP
#include "test-server.inc"

static char buffer[4096];
static struct cmusfm_data_record * track = (struct cmusfm_data_record *)buffer;

START_TEST(test_notify) {

	track->status = CMSTATUS_PLAYING;
	cmusfm_data_record_update_checksum(track);

	cmusfm_server_process_data(NULL, track);
	ck_assert_int_eq(now_playing_count, 1);

	ck_assert_str_eq(now_playing_sbt.artist, "The Beatles");
	ck_assert_str_eq(now_playing_sbt.track, "Yellow Submarine");

	config.nowplaying_localfile = false;

	cmusfm_server_process_data(NULL, track);
	ck_assert_int_eq(now_playing_count, 1);

#if ENABLE_LIBNOTIFY
	config.notification = true;
#endif

	track->status |= CMSTATUS_SHOUTCASTMASK;
	cmusfm_data_record_update_checksum(track);

	cmusfm_server_process_data(NULL, track);
	ck_assert_int_eq(now_playing_count, 2);
#if ENABLE_LIBNOTIFY
	ck_assert_int_eq(notify_show_count, 1);
#endif

	/* Reshow now playing notification after a long pause. */

	config.nowplaying_localfile = true;

	track->status = CMSTATUS_PLAYING;
	cmusfm_data_record_update_checksum(track);

	cmusfm_server_process_data(NULL, track);
	ck_assert_int_eq(now_playing_count, 3);

	/* Track was played for a few seconds. */
	sleep(10);

	track->status = CMSTATUS_PAUSED;
	cmusfm_data_record_update_checksum(track);

	cmusfm_server_process_data(NULL, track);
	ck_assert_int_eq(now_playing_count, 3);

	/* Track was unpaused after more than 120 seconds. */
	sleep(121);

	track->status = CMSTATUS_PLAYING;
	cmusfm_data_record_update_checksum(track);

	cmusfm_server_process_data(NULL, track);
	ck_assert_int_eq(now_playing_count, 4);
#if ENABLE_LIBNOTIFY
	ck_assert_int_eq(notify_show_count, 3);
#endif

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

	config.nowplaying_localfile = true;
	config.nowplaying_shoutcast = true;

	Suite * s = suite_create(__FILE__);
	TCase * tc = tcase_create(__FILE__);
	SRunner * sr = srunner_create(s);

	suite_add_tcase(s, tc);

	tcase_add_test(tc, test_notify);

	srunner_run_all(sr, CK_ENV);
	int nf = srunner_ntests_failed(sr);

	srunner_free(sr);
	return nf == 0 ? 0 : 1;
}
