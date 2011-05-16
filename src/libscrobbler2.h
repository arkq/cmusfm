/*
  cmusfm - libscrobbler2.h
  Copyright (c) 2011 Arkadiusz Bokowy

  For more information see:
    http://www.last.fm/api/scrobbling
*/

#define __LIBSCROBBLER20_H

#include <stdint.h>
#include <time.h>

#define SCROBBLER_URL "http://ws.audioscrobbler.com/2.0/"
#define SCROBBLER_USERAUTH_URL "http://www.last.fm/api/auth/"

typedef struct scrobbler_session_tag {
	uint8_t api_key[16];     //128-bit API key
	uint8_t secret[16];      //128-bit secter

	uint8_t session_key[16]; //128-bit session key (authentication)
	char user_name[64];

	int error_code;
} scrobbler_session_t;

typedef struct scrobbler_trackinfo_tag {
	char *artist, *album, *album_artist, *track, *mbid;
	unsigned int track_number, duration;
	time_t timestamp;
} scrobbler_trackinfo_t;

// ----- scrobbler_* error types definitions -----
#define SCROBBERR_CURLINIT 1 //curl initialization error
#define SCROBBERR_CURLPERF 2 //curl perform error - network issue
#define SCROBBERR_SBERROR  3 //scrobbler API error
#define SCROBBERR_CALLBACK 4 //callback error (authentication)
#define SCROBBERR_TRACKINF 5 //missing required field(s) in trackinfo structure

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
