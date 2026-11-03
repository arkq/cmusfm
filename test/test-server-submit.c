/*
 * cmusfm - test-server-submit01.c
 * SPDX-FileCopyrightText: 2015-2024 Arkadiusz Bokowy and contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <check.h>

#define DEBUG_SKIP_HICCUP
#include "test-server.inc"

static char buffer[4096];
static struct cmusfm_data_record * track = (struct cmusfm_data_record *)buffer;

START_TEST(test_30_seconds_threshold) {

	track->status = CMSTATUS_PLAYING;
	track->duration = 30;
	cmusfm_data_record_update_checksum(track);

	cmusfm_server_process_data(NULL, track);

	/* Whole track was played but its duration isn't longer than 30 seconds. */
	sleep(track->duration);

	cmusfm_server_process_data(NULL, track);
	ck_assert_int_eq(scrobble_count, 0);

	/* It was overplayed (due to seeking) but it's still 30-seconds track. */
	sleep(track->duration + 10);

	track->status = CMSTATUS_STOPPED;
	cmusfm_data_record_update_checksum(track);

	cmusfm_server_process_data(NULL, track);
	ck_assert_int_eq(scrobble_count, 0);

}

START_TEST(test_4_minutes_threshold) {

	track->status = CMSTATUS_PLAYING;
	track->duration = 4 * 60 * 5;
	cmusfm_data_record_update_checksum(track);

	cmusfm_server_process_data(NULL, track);

	/* Track was played for more than 4 minutes but less than half its duration. */
	virtual_sleep(4 * 60 + 1);

	track->status = CMSTATUS_STOPPED;
	cmusfm_data_record_update_checksum(track);

	cmusfm_server_process_data(NULL, track);
	ck_assert_int_eq(scrobble_count, 1);

}

START_TEST(test_half_duration_threshold) {

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
	ck_assert_int_eq(scrobble_count, 1);

	/* Track was played for less than half its duration. */
	sleep(track->duration / 2 - 1);

	track->status = CMSTATUS_STOPPED;
	cmusfm_data_record_update_checksum(track);

	cmusfm_server_process_data(NULL, track);
	ck_assert_int_eq(scrobble_count, 1);

}

static void tc_setup(void) { scrobble_count = 0; }

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

	Suite * s = suite_create(__FILE__);
	TCase * tc = tcase_create(__FILE__);
	SRunner * sr = srunner_create(s);

	suite_add_tcase(s, tc);
	tcase_add_checked_fixture(tc, tc_setup, NULL);

	tcase_add_test(tc, test_30_seconds_threshold);
	tcase_add_test(tc, test_4_minutes_threshold);
	tcase_add_test(tc, test_half_duration_threshold);

	srunner_run_all(sr, CK_ENV);
	int nf = srunner_ntests_failed(sr);

	srunner_free(sr);
	return nf == 0 ? 0 : 1;
}
