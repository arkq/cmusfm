/*
 * cmusfm - test-utils.c
 * SPDX-FileCopyrightText: 2026 Arkadiusz Bokowy and contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stdlib.h>

#include <check.h>

#include "../src/utils.h"

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

int main(void) {

	Suite * s = suite_create(__FILE__);
	TCase * tc = tcase_create(__FILE__);
	SRunner * sr = srunner_create(s);

	suite_add_tcase(s, tc);

	tcase_add_test(tc, test_regexp_matches);
	tcase_add_test(tc, test_regexp_no_match);
	tcase_add_test(tc, test_regexp_match_empty_list);

	srunner_run_all(sr, CK_ENV);
	int nf = srunner_ntests_failed(sr);

	srunner_free(sr);
	return nf == 0 ? 0 : 1;
}
