/*
 * cmusfm - test-cache.c
 * SPDX-FileCopyrightText: 2015-2026 Arkadiusz Bokowy and contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <check.h>

#include "../src/cache.h"
#include "../src/utils.h"

/* Global variable used in the cache code. */
const char * cmusfm_cache_file;

static int scrobble_count = 0;
/* Library function used by the cache code. */
scrobbler_status_t scrobbler_scrobble(scrobbler_session_t * sbs, scrobbler_trackinfo_t * sbt) {
	(void)sbs;
	(void)sbt;
	scrobble_count++;
	return SCROBBLER_STATUS_OK;
}

START_TEST(test_cache) {

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

	cmusfm_cache_update(&track_null);
	cmusfm_cache_update(&track_empty);
	cmusfm_cache_update(&track_full);

	FILE * f;
	char buffer[512];
	size_t size;

	ck_assert_ptr_nonnull(f = fopen(cmusfm_cache_file, "r"));
	/* Make sure the structure of the cache file is not changed. */
	ck_assert_int_eq(size = fread(buffer, 1, sizeof(buffer), f), 158);
	ck_assert_uint_eq(mem2hash(buffer, size), 856388);
	fclose(f);

	cmusfm_cache_submit(NULL);
	ck_assert_int_eq(scrobble_count, 3);

	/* Cache file should have been removed after the submission. */
	ck_assert_ptr_null(fopen(cmusfm_cache_file, "r"));
	ck_assert_int_eq(errno, ENOENT);

	/* Calling submit cache without cache file should not crash. */
	cmusfm_cache_submit(NULL);
	ck_assert_int_eq(scrobble_count, 3);

}

START_TEST(test_cache_big) {

	scrobbler_trackinfo_t info = { .track = "The House of the Rising Sun" };

	/* Check big cache file - multiple read calls. */
	for (size_t i = 0; i < 1000; i++)
		cmusfm_cache_update(&info);

	cmusfm_cache_submit(NULL);
	ck_assert_int_eq(scrobble_count, 1000);

}

static void tc_setup(void) { scrobble_count = 0; }

int main(void) {

	cmusfm_cache_file = tempnam(".", "tmp-");

	Suite * s = suite_create(__FILE__);
	TCase * tc = tcase_create(__FILE__);
	SRunner * sr = srunner_create(s);

	suite_add_tcase(s, tc);
	tcase_add_checked_fixture(tc, tc_setup, NULL);

	tcase_add_test(tc, test_cache);
	tcase_add_test(tc, test_cache_big);

	srunner_run_all(sr, CK_ENV);
	int nf = srunner_ntests_failed(sr);

	srunner_free(sr);
	return nf == 0 ? 0 : 1;
}
