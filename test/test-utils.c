/*
 * cmusfm - test-utils.c
 * SPDX-FileCopyrightText: 2026 Arkadiusz Bokowy and contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "../src/utils.h"

static void test_regexp_matches(void) {

	const char * str = "Queen - 07. Bohemian Rhapsody";
	struct format_match * matches;
	struct format_match * m;

	matches = get_regexp_format_matches(str, "^(?A.+) - (?N[0-9]+)\\. (?T.+)$");
	assert(matches != NULL);

	m = get_regexp_match(matches, CMFORMAT_ARTIST);
	assert(m != NULL && m->len == 5 && strncmp(m->data, "Queen", m->len) == 0);

	m = get_regexp_match(matches, CMFORMAT_NUMBER);
	assert(m != NULL && m->len == 2 && strncmp(m->data, "07", m->len) == 0);

	m = get_regexp_match(matches, CMFORMAT_TITLE);
	assert(m != NULL && m->len == 17 && strncmp(m->data, "Bohemian Rhapsody", m->len) == 0);

	/* The format has no album placeholder. */
	assert(get_regexp_match(matches, CMFORMAT_ALBUM) == NULL);

	free(matches);

}

static void test_regexp_no_match(void) {
	/* Invalid regular expression. */
	assert(get_regexp_format_matches("abc", "^(?A.+") == NULL);
	/* String does not match the pattern. */
	assert(get_regexp_format_matches("no separator", "^(?A.+) - (?T.+)$") == NULL);
}

static void test_regexp_match_empty_list(void) {

	const char * str = "abc";
	struct format_match * matches;

	matches = get_regexp_format_matches(str, "^abc$");
	assert(matches != NULL);

	assert(get_regexp_match(matches, CMFORMAT_ARTIST) == NULL);

	free(matches);
}

int main(void) {
	test_regexp_matches();
	test_regexp_no_match();
	test_regexp_match_empty_list();
	return EXIT_SUCCESS;
}
