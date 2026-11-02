/*
 * cmusfm - utils.h
 * SPDX-FileCopyrightText: 2026 Arkadiusz Bokowy and contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CMUSFM_UTILS_H_
#define CMUSFM_UTILS_H_

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include <sys/types.h>
#include <time.h>

/* The number of defined match types. */
#define FORMAT_MATCH_TYPE_COUNT 4

/**
 * Format match type. */
enum format_match_type {
	CMFORMAT_NUMBER = 'N',
	CMFORMAT_ARTIST = 'A',
	CMFORMAT_ALBUM = 'B',
	CMFORMAT_TITLE = 'T',
};

/**
 * Structure representing a single format match. */
struct format_match {
	enum format_match_type type;
	const char * data;
	size_t len;
};

/* Get track information substrings from the given string.
 *
 * Matching is done using the provided format string, which should be an ERE-based pattern
 * with customized placeholders. A placeholder is defined as a marked subexpression with the
 * ?X marker, where the X can be one the following characters:
 *   A - artist
 *   B - album
 *   T - title
 *   N - track number
 *
 * Example:
 *   ^(?A.+) - (?N[:digits:]+)\. (?T.+)$
 *
 * In order to get a single match, one should use the get_regexp_match() function. In case of
 * an error, NULL is returned. Memory for matches is obtained with malloc(), and shall be freed
 * with the free() function. */
struct format_match * get_regexp_format_matches(const char * str, const char * format);

/**
 * Get a pointer to a single format match with the given match type.
 *
 * If the format used for matching did not contain a placeholder of the given type, NULL
 * is returned. */
struct format_match * get_regexp_match(struct format_match * matches, enum format_match_type type);

/**
 * Get cmus configuration home directory. */
char * get_cmus_home_dir(void);

/**
 * Get a path for the cmusfm configuration file. */
char * get_cmusfm_config_file(void);

/**
 * Get a path for the cmusfm cache file. */
char * get_cmusfm_cache_file(void);

/**
 * Get a path for the cmusfm socket file. */
char * get_cmusfm_socket_file(void);

/**
 * Get the current time as a Unix timestamp.
 *
 * This function has a weak definition, so it can be overridden by tests to provide a way to
 * control the flow of time. */
time_t get_current_unix_time(void);

/**
 * Extension for the standard mkdir() function.
 *
 * This function tries to recursively create given directory structure in the same fashion
 * as the `mkdir -p` command. On success it returns zero, or -1 if an error occurred. In the
 * latter case, the state of the directory structure is undefined. */
int mkdirp(const char * dir, mode_t mode);

/**
 * Simple and fast "hashing" function. */
int make_data_hash(const void * data, size_t len);

#if ENABLE_LIBNOTIFY
/**
 * Return an album cover file based on the current location.
 *
 * Location should be either a local file name or an URL. When cover file can not be found,
 * NULL is returned (URL case, or when cover file ERE match failed). In case of wild-card
 * match, the first match is returned. */
char * get_album_cover_file(const char * location, const char * format);
#endif

#endif
