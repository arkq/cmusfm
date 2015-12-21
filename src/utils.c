/*
 * cmusfm - utils.c
 * Copyright (c) 2014-2015 Arkadiusz Bokowy
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

#if HAVE_CONFIG_H
#include "../config.h"
#endif

#include "cmusfm.h"

#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if ENABLE_LIBNOTIFY
#include <dirent.h>
#include <libgen.h>
#endif

#include "debug.h"


/* Helper function for retrieving cmus configuration home path. */
char *get_cmus_home_dir(void) {

	const char cmusdir[] = "/cmus";
	char *fullpath;
	char *tmp;

	/* get our file path in the XDG configuration directory */
	if ((tmp = getenv("XDG_CONFIG_HOME")) != NULL) {
		fullpath = malloc(strlen(tmp) + sizeof(cmusdir));
		sprintf(fullpath, "%s%s", tmp, cmusdir);
		return fullpath;
	}

	if ((tmp = getenv("HOME")) != NULL) {
		fullpath = malloc(strlen(tmp) + 8 + sizeof(cmusdir));
		sprintf(fullpath, "%s/.config%s", tmp, cmusdir);
		return fullpath;
	}

	/* semi failproof return */
	return strdup(cmusdir);
}

/* Helper function for retrieving a path for given file inside the cmus
 * configuration home path. */
char *get_cmus_home_file(const char *file) {

	char *home;
	char *fullpath;

	home = get_cmus_home_dir();
	fullpath = malloc(strlen(home) + 1 + strlen(file) + 1);
	sprintf(fullpath, "%s/%s", home, file);

	free(home);
	return fullpath;
}

/* Simple and fast "hashing" function. */
int make_data_hash(const unsigned char *data, int len) {
	int x, hash;
	for (x = hash = 0; x < len; x++)
		hash += data[x] * (x + 1);
	return hash;
}

#if ENABLE_LIBNOTIFY
/* Return an album cover file based on the current location. Location should
 * be either a local file name or an URL. When cover file can not be found,
 * NULL is returned (URL case, or when coverfile ERE match failed). In case
 * of wild-card match, the first one is returned. */
char *get_album_cover_file(const char *location, const char *format) {

	static char fname[256];

	DIR *dir;
	struct dirent *dp;
	regex_t regex;
	char *tmp;

	if (location == NULL)
		return NULL;

	/* NOTE: We can support absolute paths only. The reason for this, is, that
	 *       cmus might report file name according to its current directory,
	 *       which is not known. Hence this obstruction. */
	if (location[0] != '/')
		return NULL;

	tmp = strdup(location);
	strcpy(fname, dirname(tmp));
	free(tmp);

	if ((dir = opendir(fname)) == NULL)
		return NULL;

	if (regcomp(&regex, format, REG_EXTENDED | REG_ICASE | REG_NOSUB)) {
		closedir(dir);
		return NULL;
	}

	/* scan given directory for cover file name pattern */
	while ((dp = readdir(dir)) != NULL) {
		debug("cover lookup: %s", dp->d_name);
		if (!regexec(&regex, dp->d_name, 0, NULL, 0)) {
			strcat(strcat(fname, "/"), dp->d_name);
			break;
		}
	}

	closedir(dir);
	regfree(&regex);

	if (dp == NULL)
		return NULL;

	debug("cover: %s", fname);
	return fname;
}
#endif

/* Get track information substrings from the given string. Matching is done
 * according to the provided format, which is a ERE pattern with customized
 * placeholders. Placeholder is defined as a marked subexpression with the
 * `?X` marker, where X can be one the following characters:
 *   A - artist, B - album, T - title, N - track number
 *   e.g.: ^(?A.+) - (?N[:digits:]+)\. (?T.+)$
 * In order to get a single match structure, one should use `get_regexp_match`
 * function. When matches are not longer needed, is should be freed by the
 * standard `free` function. When something goes wrong, NULL is returned. */
struct format_match *get_regexp_format_matches(const char *str, const char *format) {
#define MATCHES_SIZE FORMAT_MATCH_TYPE_COUNT + 1

	struct format_match *matches;
	const char *p = format;
	char *regexp;
	int status, i = 0;
	regex_t regex;
	regmatch_t regmatch[MATCHES_SIZE];

	debug("matching: %s: %s", format, str);

	/* allocate memory for up to FORMAT_MATCH_TYPE_COUNT matches
	 * with one extra always empty terminating structure */
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

/* Return pointer to the single format match structure with given
 * match type. */
struct format_match *get_regexp_match(struct format_match *matches,
		enum format_match_type type) {
	while (matches->type) {
		if (matches->type == type)
			break;
		matches++;
	}
	return matches;
}
