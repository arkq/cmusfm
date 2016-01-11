/*
 * cmusfm - libscrobbler2.h
 * Copyright (c) 2011-2014 Arkadiusz Bokowy
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
 *
 * For more information about scrobbling API see:
 *   http://www.last.fm/api/scrobbling
 */

#ifndef CMUSFM_LIBSCROBBLER2_H_
#define CMUSFM_LIBSCROBBLER2_H_

#include <stdint.h>
#include <time.h>

#define SCROBBLER_URL "http://ws.audioscrobbler.com/2.0/"
#define SCROBBLER_USERAUTH_URL "http://www.last.fm/api/auth/"

typedef struct scrobbler_session_tag {
	uint8_t api_key[16];     /* 128-bit API key */
	uint8_t secret[16];      /* 128-bit secret */

	uint8_t session_key[16]; /* 128-bit session key */
	char user_name[64];

	int error_code;
} scrobbler_session_t;

typedef struct scrobbler_trackinfo_tag {
	char *artist, *album, *album_artist, *track, *mbid;
	unsigned int track_number, duration;
	time_t timestamp;
} scrobbler_trackinfo_t;

/* ----- scrobbler_* error definitions ----- */
#define SCROBBERR_CURLINIT 1 /* curl initialization error */
#define SCROBBERR_CURLPERF 2 /* curl perform error - network issue */
#define SCROBBERR_SBERROR  3 /* scrobbler API error */
#define SCROBBERR_CALLBACK 4 /* callback error (authentication) */
#define SCROBBERR_TRACKINF 5 /* missing required field in trackinfo */

scrobbler_session_t *scrobbler_initialize(uint8_t api_key[16],
		uint8_t secret[16]);
void scrobbler_free(scrobbler_session_t *sbs);

typedef int (*scrobbler_authuser_callback_t)(const char *auth_url);
int scrobbler_authentication(scrobbler_session_t *sbs,
		scrobbler_authuser_callback_t callback);
int scrobbler_test_session_key(scrobbler_session_t *sbs);

char *scrobbler_get_session_key_str(scrobbler_session_t *sbs, char *str);
void scrobbler_set_session_key_str(scrobbler_session_t *sbs, const char *str);

int scrobbler_update_now_playing(scrobbler_session_t *sbs,
		scrobbler_trackinfo_t *sbt);
int scrobbler_scrobble(scrobbler_session_t *sbs, scrobbler_trackinfo_t *sbt);

#endif  /* CMUSFM_LIBSCROBBLER2_H_ */
