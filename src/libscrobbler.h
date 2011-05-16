/*
    cmusfm - libscrobbler.h
    Copyright (c) 2010 Arkadiusz Bokowy

    For more information see:
      http://www.audioscrobbler.net/development/protocol/
*/

#define __LIBSCROBBLER_H

#include <time.h>

#define SCROBBLER_POSTHOST "post.audioscrobbler.com"
#define SCROBBLER_PROTOVER "1.2"
#define SCROBBLER_CLIENTID "cmf"
#define SCROBBLER_CLIENTNB "1.0"

typedef void scrobbler_t;
typedef struct scrobbler_trackinf_tag {
	char *artist,
			*track,
			*album,      //can be NULL
			*mb_trackid; //MusicBrainz Track ID - can be NULL
	int length,      //track length in [s] - if not known set 0
			tracknb;     //if not known set 0

	// submission fields:
	time_t started;  //the time the track started playing
	char rating,     //track rating (not required) - see definition below
			source;      //track source - see definition below
	int lastfmid;    //required only if source = Last.fm

}scrobbler_trackinf_t;

// ----- Submission preferences -----
// track ratings:
#define SCROBBSUBMIT_LOVE 'L'
#define SCROBBSUBMIT_BAN 'B'
#define SCROBBSUBMIT_SKIP 'S'
// track sources:
#define SCROBBSUBMIT_USER 'P'
#define SCROBBSUBMIT_RADIO 'R'   //e.g. Shoutcast, BBC Radio 1
#define SCROBBSUBMIT_PERSON 'E'  //e.g. Pandora, Launchcast
#define SCROBBSUBMIT_LASTFM 'L'
#define SCROBBSUBMIT_UNKNOWN 'U'

// maximum number of tract tu submit at once
#define SCROBBSUBMIT_MAX 50

// ----- Error definitions -----
// [s] means that you can obtain human readable error message
// by calling scrobbler_get_strerr()
// common errors:
#define SCROBBERR_CURLINIT 1 //curl initialization error
#define SCROBBERR_CURLPERF 2 //curl perform error - network issue [s]
#define SCROBBERR_HARD 3     //hard server failure [s]
// login errors:
#define SCROBBERR_BANNED 4   //this client version has been banned [s]
#define SCROBBERR_AUTH 5     //authentication error [s]
#define SCROBBERR_TIME 6     //bad system time -> sync your time [s]
#define SCROBBERR_TEMPERR 7  //temporary failure [s]
// nowplay_notify and submit errors:
#define SCROBBERR_BADARG 8   //bad argument(s) in function call
#define SCROBBERR_SESSION 9  //bad session -> relogin [s]


// On error this function returns NULL, otherwise it returns pointer
// which must be freed with scrobbler_cleanup().
scrobbler_t *scrobbler_initialize(const char *user, const char *pass_raw,
		const unsigned char pass_md5[16]);
void scrobbler_cleanup(scrobbler_t *sbs);

// On successful functions return 0, otherwise see errors defined above.
int scrobbler_login(scrobbler_t *sbs);
int scrobbler_nowplay_notify(scrobbler_t *sbs, scrobbler_trackinf_t *sbn);
int scrobbler_submit(scrobbler_t *sbs, scrobbler_trackinf_t (*sbn)[], int size);

char *scrobbler_get_strerr(scrobbler_t *sbs);
void scrobbler_get_passmd5(scrobbler_t *sbs, unsigned char pass_md5[16]);
void scrobbler_dump_trackinf(scrobbler_trackinf_t (*sbn)[], int size);

