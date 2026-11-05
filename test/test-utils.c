/*
 * cmusfm - test-utils.c
 * SPDX-FileCopyrightText: 2026 Arkadiusz Bokowy and contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>

#include <check.h>

#include "../src/cmusfm.h"
#include "../src/utils.h"

START_TEST(test_get_cmus_home_dir) {
	char * s;
	setenv("HOME", "/home/test", 1);
	ck_assert_str_eq(s = get_cmus_home_dir(), "/home/test/.config/cmus");
	free(s);
}

START_TEST(test_get_cmus_home_dir_xdg) {
	char * s;
	setenv("XDG_CONFIG_HOME", "/config", 1);
	ck_assert_str_eq(s = get_cmus_home_dir(), "/config/cmus");
	free(s);
}

START_TEST(test_get_cmusfm_cache_file) {
	char * s;
	ck_assert_str_eq(s = get_cmusfm_cache_file(), "/cmus/" CACHE_FNAME);
	free(s);
}

START_TEST(test_get_cmusfm_config_file) {
	char * s;
	ck_assert_str_eq(s = get_cmusfm_config_file(), "/cmus/" CONFIG_FNAME);
	free(s);
}

START_TEST(test_get_cmusfm_socket_file) {
	char * s;
	ck_assert_str_eq(s = get_cmusfm_socket_file(), "/cmus/" SOCKET_FNAME);
	free(s);
}

START_TEST(test_mkdirp) {

	/* Check if passing empty string does not crash. */
	ck_assert_int_eq(mkdirp("", 0755), -1);
	ck_assert_int_eq(errno, EINVAL);

	ck_assert_int_eq(mkdirp("tmp/test/dir", 0755), 0);
	/* Check that the directory was created. */
	ck_assert_int_eq(access("tmp/test/dir", F_OK), 0);

	/* Check that function does not fail when the directory already exists. */
	ck_assert_int_eq(mkdirp("tmp/test/dir", 0755), 0);

	ck_assert_int_eq(rmdir("tmp/test/dir"), 0);
	ck_assert_int_eq(rmdir("tmp/test"), 0);
	ck_assert_int_eq(rmdir("tmp"), 0);

}

START_TEST(test_mkdirp_long_path) {

	char path[PATH_MAX + 1];
	memset(path, 'a', sizeof(path) - 1);
	path[sizeof(path) - 1] = '\0';

	ck_assert_int_eq(mkdirp(path, 0755), -1);
	ck_assert_int_eq(errno, ENAMETOOLONG);

}

START_TEST(test_mem2hash) {
	ck_assert_uint_eq(mem2hash("test", 4), 1127);
}

START_TEST(test_mem2hex) {
	char buffer[9];
	ck_assert_str_eq(mem2hex(buffer, "test", 4), "74657374");
}

START_TEST(test_regexp_matches) {

	const char * str = "Queen - 07. Bohemian Rhapsody";
	struct format_match * matches;
	struct format_match * m;

	matches = get_regexp_format_matches(str, "^(?A.+) - (?N[0-9]+)\\. (?T.+)$");
	ck_assert_ptr_nonnull(matches);

	ck_assert_ptr_nonnull(m = get_regexp_match(matches, CMFORMAT_ARTIST));
	ck_assert_mem_eq(m->data, "Queen", 5);
	ck_assert_uint_eq(m->len, 5);

	ck_assert_ptr_nonnull(m = get_regexp_match(matches, CMFORMAT_NUMBER));
	ck_assert_mem_eq(m->data, "07", 2);
	ck_assert_uint_eq(m->len, 2);

	ck_assert_ptr_nonnull(m = get_regexp_match(matches, CMFORMAT_TITLE));
	ck_assert_mem_eq(m->data, "Bohemian Rhapsody", 17);
	ck_assert_uint_eq(m->len, 17);

	/* The format has no album placeholder. */
	ck_assert_ptr_null(get_regexp_match(matches, CMFORMAT_ALBUM));

	free(matches);
}

START_TEST(test_regexp_no_match) {
	/* Invalid regular expression. */
	ck_assert_ptr_null(get_regexp_format_matches("abc", "^(?A.+"));
	/* String does not match the pattern. */
	ck_assert_ptr_null(get_regexp_format_matches("no separator", "^(?A.+) - (?T.+)$"));
}

START_TEST(test_regexp_match_empty_list) {

	const char * str = "abc";
	struct format_match * matches;

	matches = get_regexp_format_matches(str, "^abc$");
	ck_assert_ptr_nonnull(matches);

	ck_assert_ptr_null(get_regexp_match(matches, CMFORMAT_ARTIST));

	free(matches);
}

static void tc_setup(void) {
	unsetenv("HOME");
	unsetenv("XDG_CONFIG_HOME");
}

int main(void) {

	Suite * s = suite_create(__FILE__);
	TCase * tc = tcase_create(__FILE__);
	SRunner * sr = srunner_create(s);

	suite_add_tcase(s, tc);
	tcase_add_checked_fixture(tc, tc_setup, NULL);

	tcase_add_test(tc, test_get_cmus_home_dir);
	tcase_add_test(tc, test_get_cmus_home_dir_xdg);
	tcase_add_test(tc, test_get_cmusfm_cache_file);
	tcase_add_test(tc, test_get_cmusfm_config_file);
	tcase_add_test(tc, test_get_cmusfm_socket_file);
	tcase_add_test(tc, test_mkdirp);
	tcase_add_test(tc, test_mkdirp_long_path);
	tcase_add_test(tc, test_mem2hash);
	tcase_add_test(tc, test_mem2hex);
	tcase_add_test(tc, test_regexp_matches);
	tcase_add_test(tc, test_regexp_no_match);
	tcase_add_test(tc, test_regexp_match_empty_list);

	srunner_run_all(sr, CK_ENV);
	int nf = srunner_ntests_failed(sr);

	srunner_free(sr);
	return nf == 0 ? 0 : 1;
}
