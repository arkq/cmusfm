/*
  cmusfm - libscrobbler.c
  Copyright (c) 2010 Arkadiusz Bokowy

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  If you want to read full version of the GNU General Public License
  see <http://www.gnu.org/licenses/>.

  ** Note: **
  For contact information and the latest version of this program see
  my webpage <http://arkq.awardspace.us/#cmusfm>.

*/

#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <openssl/md5.h>
#include "libscrobbler.h"

char sb_srv_response[512];
char libsb_error_str[CURL_ERROR_SIZE]; //256-bytes
char libsb_error_tag[] = "scrobbler: %s";

typedef struct scrobbler_session* sbs_tp;
struct scrobbler_session {
	char user_name[64];
	unsigned char password_md5[16];

	unsigned char session_id[16*2+1];
	char now_playing_url[128];
	char submission_url[128];

	char *error_str;

	int global_init;
};

static size_t sb_write_callback(void *ptr, size_t size, size_t nmemb,
		void *stream)
{
	bzero(sb_srv_response, sizeof(sb_srv_response));
	strncpy(sb_srv_response, ptr, sizeof(sb_srv_response) - 1);
	return size*nmemb;
}

// Convert buffer containing md5 hash to hex string
char *md5_to_hex(const unsigned char *md5)
{
	static char md5hex[MD5_DIGEST_LENGTH*2 + 1];
	char *ptr = md5hex;
	char hexchars[] = "0123456789abcdef";
	int i;

	bzero(md5hex, sizeof(md5hex));
	for(i = 0; i < MD5_DIGEST_LENGTH; i++){
		*(ptr++) = hexchars[(md5[i] >> 4) & 0x0f];
		*(ptr++) = hexchars[md5[i] & 0x0f];}
	*ptr = 0;
	return md5hex;
}

// NOTE: returned pointer must be freed!
// if submitnb == -1 make string for nowplay_notify
char *make_post_track_string(CURL *curl, scrobbler_trackinf_t *sbn,
		int submitnb)
{
	char len_str[16], tnb_str[16], src_str[16], rat_str[4];
	char *art, *tra, *alb, *mbi, *len, *tnb, *src, *rat;
	char *track_string;
	char zero_len_string[] = "";

	len = len_str;
	tnb = tnb_str;
	src = src_str;
	rat = rat_str;

	// this should be enough for escaped POST data, but if not what then...?
	track_string = (char*)malloc(2048);

	if(track_string != NULL) {
		// replace NULL char* with pointer to ""
		if(sbn->album == NULL) sbn->album = zero_len_string;
		if(sbn->mb_trackid == NULL) sbn->mb_trackid = zero_len_string;

		// create POST data string (escape values)
		art = curl_easy_escape(curl, sbn->artist, 0);
		tra = curl_easy_escape(curl, sbn->track, 0);
		alb = curl_easy_escape(curl, sbn->album, 0);
		mbi = curl_easy_escape(curl, sbn->mb_trackid, 0);

		if(sbn->length == 0) len = zero_len_string;
		else sprintf(len_str, "%d", sbn->length);
		if(sbn->tracknb == 0) tnb = zero_len_string;
		else sprintf(tnb_str, "%d", sbn->tracknb);

		if(submitnb == -1)
			sprintf(track_string, "a=%s&t=%s&b=%s&l=%s&n=%s&m=%s",
					art, tra, alb, len, tnb, mbi);
		else {//make string for submission

			if(sbn->source == 0) src = zero_len_string;
			else sprintf(src_str, "%c", sbn->source);
			if(sbn->rating == 0) rat = zero_len_string;
			else sprintf(rat_str, "%c", sbn->rating);

			if(sbn->source == SCROBBSUBMIT_LASTFM)
				sprintf(src_str, "%c%x", sbn->source, sbn->lastfmid);

			sprintf(track_string, "a[%d]=%s&t[%d]=%s&i[%d]=%ld&o[%d]=%s"
					"&r[%d]=%s&l[%d]=%s&b[%d]=%s&n[%d]=%s&m[%d]=%s",
					submitnb, art, submitnb, tra, submitnb, sbn->started,
					submitnb, src, submitnb, rat, submitnb, len,
					submitnb, alb, submitnb, tnb, submitnb, mbi);
		}

		curl_free(art);
		curl_free(tra);
		curl_free(alb);
		curl_free(mbi);
	}

	return track_string;
}

// ----- Library interface ------

// Dump trackinfos to stdout
void scrobbler_dump_trackinf(scrobbler_trackinf_t (*sbn)[], int size)
{
	int x;

	for(x = 0; x < size; x++)
		printf("trackinf_t nb: %d\n"
"  artist: %s\n  track:  %s\n  album:  %s\n  mb_trackid: %s\n"
"  started: %d\n  rating:  %c\n  source:  %c\n  lastfmid: %x\n",
x + 1, (*sbn)[x].artist, (*sbn)[x].track, (*sbn)[x].album, (*sbn)[x].mb_trackid,
(int)(*sbn)[x].started, (*sbn)[x].rating, (*sbn)[x].source, (*sbn)[x].lastfmid);
}

// Scrobbler submission
int scrobbler_submit(scrobbler_t *sbs, scrobbler_trackinf_t (*sbn)[], int size)
{
	CURL *curl;
	char *post_data, *track_data;
	int x, argerr_count, status;

	if(size > SCROBBSUBMIT_MAX) return SCROBBERR_BADARG;

	// this should be enough for escaped POST data, but if not...
	post_data = (char*)malloc(2048*size);
	if(post_data == NULL) return -1;

	curl = curl_easy_init();
	if(curl == NULL){
		free(post_data);
		return SCROBBERR_CURLINIT;}

	sprintf(post_data, "s=%s", ((sbs_tp)sbs)->session_id);

	// add all trackinfo we've got to post_data buffer
	for(x = 0, argerr_count = 0; x < size; x++) {
		// check required fields
		if((*sbn)[x].artist == NULL || (*sbn)[x].track == NULL){
			argerr_count++; continue;}
		if((*sbn)[x].source == SCROBBSUBMIT_USER && (*sbn)[x].length == 0){
			argerr_count++; continue;}

		track_data = make_post_track_string(curl, &(*sbn)[x], x - argerr_count);
		strcat(post_data, "&");
		strcat(post_data, track_data);
		free(track_data);
	}

	// if there was more then 30% of bad tracks info raise error
	if((argerr_count*100)/size > 30){
		free(post_data);
		curl_easy_cleanup(curl);
		return SCROBBERR_BADARG;}

	// initialize curl for HTTP POST
#ifdef CURLOPT_PROTOCOLS
	curl_easy_setopt(curl, CURLOPT_PROTOCOLS, CURLPROTO_HTTP);
#endif
	curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
	curl_easy_setopt(curl, CURLOPT_POST, 1);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, libsb_error_str);
	curl_easy_setopt(curl, CURLOPT_URL, ((sbs_tp)sbs)->submission_url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, sb_write_callback);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);

	status = curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	free(post_data);

	if(status == 0) {
		if(memcmp(sb_srv_response, "OK", 2) == 0) return 0;
		if(memcmp(sb_srv_response, "BADSESSION", 10) == 0){
			sprintf(libsb_error_str, libsb_error_tag, "bad session ID");
			return SCROBBERR_SESSION;}

		sprintf(libsb_error_str, libsb_error_tag, "hard failure");
		return SCROBBERR_HARD;
	}

	// network transfer failure
	return SCROBBERR_CURLPERF;
}

// Now-playing notification
int scrobbler_nowplay_notify(scrobbler_t *sbs, scrobbler_trackinf_t *sbn)
{
	CURL *curl;
	char *post_data, *track_data;
	int status;

	// this fields are required
	if(sbn->artist == NULL || sbn->track == NULL) return SCROBBERR_BADARG;

	// this should be enough for escaped POST data, but if not...
	post_data = (char*)malloc(2048);
	if(post_data == NULL) return -1;

	curl = curl_easy_init();
	if(curl == NULL){
		free(post_data);
		return SCROBBERR_CURLINIT;}

	track_data = make_post_track_string(curl, sbn, -1);

	sprintf(post_data, "s=%s&%s", ((sbs_tp)sbs)->session_id, track_data);
	free(track_data);

	// initialize curl for HTTP POST
#ifdef CURLOPT_PROTOCOLS
	curl_easy_setopt(curl, CURLOPT_PROTOCOLS, CURLPROTO_HTTP);
#endif
	curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
	curl_easy_setopt(curl, CURLOPT_POST, 1);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, libsb_error_str);
	curl_easy_setopt(curl, CURLOPT_URL, ((sbs_tp)sbs)->now_playing_url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, sb_write_callback);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);

	status = curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	free(post_data);

	if(status == 0) {
		if(memcmp(sb_srv_response, "OK", 2) == 0) return 0;
		if(memcmp(sb_srv_response, "BADSESSION", 10) == 0){
			sprintf(libsb_error_str, libsb_error_tag, "bad session ID");
			return SCROBBERR_SESSION;}

		sprintf(libsb_error_str, libsb_error_tag, "hard failure");
		return SCROBBERR_HARD;
	}

	// network transfer failure
	return SCROBBERR_CURLPERF;
}

// Initialize libscrobbler variables and memory
scrobbler_t *scrobbler_initialize(const char *user, const char *pass_raw,
		const unsigned char pass_md5[16])
{
	struct scrobbler_session *sbs;

	// we need some form of password, right?
	if(pass_raw == NULL && pass_md5 == NULL) return NULL;

	// alloc memory for scrobbler struct
	sbs = (sbs_tp)malloc(sizeof(struct scrobbler_session));
	if(sbs == NULL) return NULL;

	bzero(sbs, sizeof(struct scrobbler_session));

	//TODO: some other way to handle error's strings
	sbs->error_str = libsb_error_str;
	bzero(libsb_error_str, 5);

	sbs->global_init = curl_global_init(CURL_GLOBAL_NOTHING);
	if(sbs->global_init != 0){
		free(sbs);
		return NULL;}

	// save authentications informations (keep password in MD5 format)
	strcpy(sbs->user_name, user);
	if(pass_raw != NULL)
		MD5((unsigned char*)pass_raw, strlen(pass_raw), sbs->password_md5);
	else memcpy(sbs->password_md5, pass_md5, MD5_DIGEST_LENGTH);

	return (scrobbler_t*)sbs;
}

// Make handshake with Scrobbler server
// On error function returns non-zero value
int scrobbler_login(scrobbler_t *sbs)
{
	CURL *curl;
	time_t ts;
	int status;
	char auth_raw[MD5_DIGEST_LENGTH*2*2 + 1];
	unsigned char auth[MD5_DIGEST_LENGTH];
	char get_url[1024];
	char *stat_str, *ptr;

	curl = curl_easy_init();
	if(curl == NULL) return SCROBBERR_CURLINIT;

	// create handshake GET URL
	ts = time(NULL);
	sprintf(auth_raw, "%s%ld", md5_to_hex(((sbs_tp)sbs)->password_md5), ts);
	MD5((unsigned char*)auth_raw, strlen(auth_raw), auth);
	sprintf(get_url, SCROBBLER_POSTHOST "/?hs=true&p=" SCROBBLER_PROTOVER 
			"&c=" SCROBBLER_CLIENTID "&v=" SCROBBLER_CLIENTNB "&u=%s&t=%ld&a=%s",
			((sbs_tp)sbs)->user_name, ts, md5_to_hex(auth));

	// initialize curl for HTTP handshake
#ifdef CURLOPT_PROTOCOLS
	curl_easy_setopt(curl, CURLOPT_PROTOCOLS, CURLPROTO_HTTP);
#endif
	curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
	curl_easy_setopt(curl, CURLOPT_HTTPGET, 1);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, libsb_error_str);
	curl_easy_setopt(curl, CURLOPT_URL, get_url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, sb_write_callback);

	status = curl_easy_perform(curl);
	curl_easy_cleanup(curl);

	if(status == 0) {
		// network transfer performed, so now check scrobbler status
		stat_str = strtok_r(sb_srv_response, "\n", &ptr);

		if(memcmp(stat_str, "OK", 2) == 0) {
			memcpy(((sbs_tp)sbs)->session_id, strtok_r(NULL, "\n", &ptr),
					sizeof(((sbs_tp)sbs)->session_id) - 1);
			strncpy(((sbs_tp)sbs)->now_playing_url, strtok_r(NULL, "\n", &ptr),
					sizeof(((sbs_tp)sbs)->now_playing_url) - 1);
			strncpy(((sbs_tp)sbs)->submission_url, strtok_r(NULL, "\n", &ptr),
					sizeof(((sbs_tp)sbs)->submission_url) - 1);
			return 0;
		}
		if(memcmp(stat_str, "BANNED", 6) == 0){
			sprintf(libsb_error_str, libsb_error_tag,
					"this client version has been banned");
			return SCROBBERR_BANNED;}
		if(memcmp(stat_str, "BADAUTH", 7) == 0){
			sprintf(libsb_error_str, libsb_error_tag, "authentication error");
			return SCROBBERR_AUTH;}
		if(memcmp(stat_str, "BADTIME", 7) == 0){
			sprintf(libsb_error_str, libsb_error_tag, "bad system time");
			return SCROBBERR_TIME;}
		if(memcmp(stat_str, "FAILED", 6) == 0){
			sprintf(libsb_error_str, libsb_error_tag, "temporary server failure");
			return SCROBBERR_TEMPERR;}

		sprintf(libsb_error_str, libsb_error_tag, "hard failure");
		return SCROBBERR_HARD;
	}

	// network transfer failure (curl error)
	return SCROBBERR_CURLPERF;
}

char *scrobbler_get_strerr(scrobbler_t *sbs)
{
	return ((sbs_tp)sbs)->error_str;
}

void scrobbler_get_passmd5(scrobbler_t *sbs, unsigned char pass_md5[16])
{
	memcpy(pass_md5, ((sbs_tp)sbs)->password_md5, MD5_DIGEST_LENGTH);
}

void scrobbler_cleanup(scrobbler_t *sbs)
{
	if(((sbs_tp)sbs)->global_init == 0) curl_global_cleanup();
	free(sbs);
}

