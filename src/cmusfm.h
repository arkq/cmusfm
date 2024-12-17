/*
 * cmusfm - cmusfm.h
 * SPDX-FileCopyrightText: 2010-2024 Arkadiusz Bokowy and contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CMUSFM_CMUSFM_H_
#define CMUSFM_CMUSFM_H_

#include <sys/stat.h>
#include <sys/types.h>

#include "config.h"
#include "libscrobbler2.h"


#define CONFIG_FNAME "cmusfm.conf"
#define SOCKET_FNAME "cmusfm.socket"
#define CACHE_FNAME  "cmusfm.cache"


/* time delay (in seconds) between login attempts to the Last.fm
 * scrobbling service after a submit failure */
#define SERVICE_RETRY_DELAY 60 * 30


/* global variable definitions */
extern unsigned char SC_api_key[16];
extern unsigned char SC_secret[16];
extern const char *cmusfm_cache_file;
extern const char *cmusfm_config_file;
extern const char *cmusfm_socket_file;
extern struct cmusfm_config config;


enum cmstatus {
	CMSTATUS_UNDEFINED = 0,
	CMSTATUS_PLAYING,
	CMSTATUS_PAUSED,
	CMSTATUS_STOPPED
};

/* structure for data obtained from cmus */
struct cmtrack_info {

	enum cmstatus status;

	char *file;
	char *url;

	/* the MusicBrainz track ID */
	char *mb_track_id;

	char *artist;
	char *album_artist;
	char *album;
	int disc_number;
	int track_number;
	char *title;
	char *date;
	int duration;

};


enum format_match_type {
	CMFORMAT_NUMBER = 'N',
	CMFORMAT_ARTIST = 'A',
	CMFORMAT_ALBUM = 'B',
	CMFORMAT_TITLE = 'T'
};
#define FORMAT_MATCH_TYPE_COUNT 4

struct format_match {
	enum format_match_type type;
	const char *data;
	size_t len;
};


char *get_cmus_home_dir(void);
char *get_cmus_home_file(const char *file);
int mkdirp(const char *dir, mode_t mode);
int make_data_hash(const unsigned char *data, int len);
#if ENABLE_LIBNOTIFY
char *get_album_cover_file(const char *location, const char *format);
#endif
struct format_match *get_regexp_format_matches(const char *str, const char *format);
struct format_match *get_regexp_match(struct format_match *matches, enum format_match_type type);

#endif  /* CMUSFM_CMUSFM_H_ */
