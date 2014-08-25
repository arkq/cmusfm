/*
 * cmusfm - utils.c
 * Copyright (c) 2014 Arkadiusz Bokowy
 *
 * This file is a part of a cmusfm.
 *
 * cmusfm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * cmusfm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * If you want to read full version of the GNU General Public License
 * see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>

#include "cmusfm.h"
#include "debug.h"


// Helper function for retrieving cmus configuration home path.
char *get_cmus_home_dir(void) {

	static char fname[128];
	char *xdg_config;

	// get XDG config directory or fall-back to the default
	xdg_config = getenv("XDG_CONFIG_HOME");
	if (xdg_config)
		strcpy(fname, xdg_config);
	else
		sprintf(fname, "%s/.config", getenv("HOME"));

	return strcat(fname, "/cmus");
}

// Get track information substrings from the given string. Matching is
// done according to the provided format, which is a ERE pattern with
// customized placeholders. Placeholder is defined as a marked
// subexpression with the `?X` marker, where X can be one the following
// characters: A - artist, B - album, T - title, N - track number
// E.g.: ^(?A.+) - (?N[:digits:]+)\. (?T.+)$
// In order to get a single match structure, one should use `get_regexp_match`
// function. When matches are not longer needed, is should be freed by the
// standard `free` function. When something goes wrong, NULL is returned.
struct format_match *get_regexp_format_matches(const char *str, const char *format) {
#define MATCHES_SIZE FORMAT_MATCH_TYPE_COUNT + 1

	struct format_match *matches;
	const char *p = format;
	char *regexp;
	int status, i = 0;
	regex_t regex;
	regmatch_t regmatch[MATCHES_SIZE];

	debug("matching: %s: %s", format, str);

	// allocate memory for up to FORMAT_MATCH_TYPE_COUNT matches
	// with one extra always empty terminating structure
	matches = (struct format_match *)calloc(MATCHES_SIZE, sizeof(*matches));

	regexp = strdup(format);
	while (++i < MATCHES_SIZE && (p = strstr(p, "(?"))) {
		p += 3;
		matches[i - 1].type = p[-1];
		strcpy(&regexp[p - format - i * 2], p);
	}

	debug("regexp: %s", regexp);

	status = regcomp(&regex, regexp, REG_EXTENDED | REG_ICASE);
	free(regexp);
	if (status) {
		free(matches);
		return NULL;
	}

	status = regexec(&regex, str, MATCHES_SIZE, regmatch, 0);
	regfree(&regex);
	if (status) {
		free(matches);
		return NULL;
	}

	for (i = 1; i < MATCHES_SIZE; i++) {
		matches[i - 1].data = &str[regmatch[i].rm_so];
		matches[i - 1].len = regmatch[i].rm_eo - regmatch[i].rm_so;
	}
	return matches;
}

// Return pointer to the single format match structure with given match type.
struct format_match *get_regexp_match(struct format_match *matches, enum format_match_type type) {
	while (matches->type) {
		if (matches->type == type)
			break;
		matches++;
	}
	return matches;
}
