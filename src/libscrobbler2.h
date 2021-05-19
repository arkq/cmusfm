/*
 * libscrobbler2.h
 * Copyright (c) 2011-2021 Arkadiusz Bokowy
 *
 * This file is a part of cmusfm.
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

/* Status definitions. For more comprehensive information about errors,
 * see the errornum variable in the session structure. */
typedef enum scrobbler_status {
	SCROBBLER_STATUS_OK = 0,
	SCROBBLER_STATUS_ERR_CURLINIT,  /* curl initialization error */
	SCROBBLER_STATUS_ERR_CURLPERF,  /* curl perform error - network issue */
	SCROBBLER_STATUS_ERR_SCROBAPI,  /* scrobbler API error */
	SCROBBLER_STATUS_ERR_CALLBACK,  /* callback error (authentication) */
	SCROBBLER_STATUS_ERR_TRACKINF   /* missing required field in trackinfo */
} scrobbler_status_t;

/* Possible error codes returned by scrobbler API. */
typedef enum scrobbler_api_error {
	SCROBBLER_API_ERR_INVALID_SERVICE = 2,
	SCROBBLER_API_ERR_INVALID_METHOD = 3,
	SCROBBLER_API_ERR_AUTH_FAILED = 4,
	SCROBBLER_API_ERR_INVALID_FORMAT = 5,
	SCROBBLER_API_ERR_INVALID_PARAMS = 6,
	SCROBBLER_API_ERR_INVALID_RESOURCE = 7,
	SCROBBLER_API_ERR_OPERATION_FAILED = 8,
	SCROBBLER_API_ERR_INVALID_SESSION_KEY = 9,
	SCROBBLER_API_ERR_INVALID_API_KEY = 10,
	SCROBBLER_API_ERR_SERVICE_OFFLINE = 11,
	SCROBBLER_API_ERR_SUBSCRIBERS_ONLY = 12,
	SCROBBLER_API_ERR_INVALID_SIGNATURE = 13,
	SCROBBLER_API_ERR_UNAUTHORIZED_TOKEN = 14,
	SCROBBLER_API_ERR_ITEM_NOT_AVAILABLE = 15,
	SCROBBLER_API_ERR_SERVICE_UNAVAILABLE = 16,
	SCROBBLER_API_ERR_LOGIN_REQUIRED = 17,
	SCROBBLER_API_ERR_TRIAL_EXPIRED = 18,
	SCROBBLER_API_ERR_NOT_ENOUGH_CONTENT = 20,
	SCROBBLER_API_ERR_NOT_ENOUGH_MEMBERS = 21,
	SCROBBLER_API_ERR_NOT_ENOUGH_FANS = 22,
	SCROBBLER_API_ERR_NOT_ENOUGH_NEIGHBOURS = 23,
	SCROBBLER_API_ERR_RADIO_NO_PEAK = 24,
	SCROBBLER_API_ERR_RADIO_NOT_FOUND = 25,
	SCROBBLER_API_ERR_API_KEY_SUSPENDED = 26,
	SCROBBLER_API_ERR_DEPRECATED = 27,
	SCROBBLER_API_ERR_LIMIT_EXCEDED = 29,
} scrobbler_api_error_t;

typedef struct scrobbler_session {

	/* service API URL */
	char api_url[64];
	char auth_url[64];

	/* 128-bit API key */
	uint8_t api_key[16];
	/* 128-bit secret */
	uint8_t secret[16];

	char user_name[64];
	/* 32-chars (?) session key */
	char session_key[32 + 1];

	scrobbler_status_t status;
	uint8_t errornum;

} scrobbler_session_t;

typedef struct scrobbler_trackinfo {
	/* the time the track started playing */
	time_t timestamp;
	/* the MusicBrainz track ID */
	char *mb_track_id;
	/* the artist name */
	char *artist;
	/* the album artist */
	char *album_artist;
	/* the album name */
	char *album;
	/* the track number on the album */
	int track_number;
	/* the track name */
	char *track;
	/* the length of the track in seconds */
	int duration;
} scrobbler_trackinfo_t;

scrobbler_session_t *scrobbler_initialize(const char *api_url,
		const char *auth_url, uint8_t api_key[16], uint8_t secret[16]);
void scrobbler_free(scrobbler_session_t *sbs);

const char *scrobbler_strerror(scrobbler_session_t *sbs);

typedef int (*scrobbler_authuser_callback_t)(const char *auth_url);
scrobbler_status_t scrobbler_authentication(scrobbler_session_t *sbs,
		scrobbler_authuser_callback_t callback);
scrobbler_status_t scrobbler_test_session_key(scrobbler_session_t *sbs);

const char *scrobbler_get_session_key(scrobbler_session_t *sbs);
void scrobbler_set_session_key(scrobbler_session_t *sbs, const char *str);

scrobbler_status_t scrobbler_update_now_playing(scrobbler_session_t *sbs,
		scrobbler_trackinfo_t *sbt);
scrobbler_status_t scrobbler_scrobble(scrobbler_session_t *sbs,
		scrobbler_trackinfo_t *sbt);

#endif  /* CMUSFM_LIBSCROBBLER2_H_ */
