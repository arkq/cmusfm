/*
 * libscrobbler2.c
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
 */

#include "libscrobbler2.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#include "debug.h"
#include "md5.c"


static char *mem2hex(char *dest, const unsigned char *mem, size_t n);


/* used as a buffer for GET/POST server response */
struct sb_response_data {
	char *data;
	int size;
};

/* used for quick URL and signature creation process */
struct sb_getpost_data {
	char *name;
	char data_format;
	void *data;
};


/* CURL write callback function. */
static size_t sb_curl_write_callback(char *ptr, size_t size, size_t nmemb,
		void *data) {

	struct sb_response_data *rd = (struct sb_response_data *)data;
	size *= nmemb;

	debug("Read: size: %zu, body: %s", size, ptr);

	/* XXX: Passing a zero bytes data to this callback is not en error,
	 *      however memory allocation fail is. */
	void *tmp = rd->data;
	if (!size || (rd->data = realloc(rd->data, rd->size + size + 1)) == NULL) {
		free(tmp);
		return 0;
	}

	memcpy(&rd->data[rd->size], ptr, size);
	rd->size += size;
	rd->data[rd->size] = '\0';

	return size;
}

/* Initialize CURL handler for internal usage. */
static CURL *sb_curl_init(CURLoption method, struct sb_response_data *response) {

	CURL *curl;

	if ((curl = curl_easy_init()) == NULL)
		return NULL;

	/* do not hang "forever" during connection and data transfer */
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5);

#if LIBCURL_VERSION_NUM >= 0x071101 /* 7.17.1 */
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5);
	curl_easy_setopt(curl, CURLOPT_POSTREDIR, CURL_REDIR_POST_ALL);
#endif

#if LIBCURL_VERSION_NUM >= 0x071304 /* 7.19.4 */
	curl_easy_setopt(curl, CURLOPT_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS);
	curl_easy_setopt(curl, CURLOPT_REDIR_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS);
#endif

	curl_easy_setopt(curl, method, 1);
	curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);

	memset(response, 0, sizeof(*response));
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, sb_curl_write_callback);

	return curl;
}

/* Cleanup CURL handler and free allocated response buffer. */
static void sb_curl_cleanup(CURL *curl, struct sb_response_data *response) {
	curl_easy_cleanup(curl);
	free(response->data);
}

/* Check scrobble API response status (and curl itself). */
static scrobbler_status_t sb_check_response(struct sb_response_data *response,
		int curl_status, scrobbler_session_t *sbs) {

	debug("Check: status: %d, body: %s", curl_status, response->data);

	/* network transfer failure (curl error) */
	if (curl_status != 0) {
		sbs->errornum = curl_status;
		return sbs->status = SCROBBLER_STATUS_ERR_CURLPERF;
	}

	/* curl write callback was not called, something was mighty wrong... */
	if (response->data == NULL) {
		sbs->errornum = CURLE_GOT_NOTHING;
		return sbs->status = SCROBBLER_STATUS_ERR_CURLPERF;
	}

	/* scrobbler service failure */
	if (strstr(response->data, "<lfm status=\"ok\"") == NULL) {
		char *error = strstr(response->data, "<error code=");
		if (error != NULL)
			sbs->errornum = atoi(error + 13);
		else
			/* error code was not found in the response, so maybe we are calling
			 * wrong service... set error code as value not used by the Last.fm */
			sbs->errornum = 1;
		return sbs->status = SCROBBLER_STATUS_ERR_SCROBAPI;
	}

	return sbs->status = SCROBBLER_STATUS_OK;
}

/* Generate MD5 signature for API call. */
static void sb_generate_method_signature(
		const scrobbler_session_t *sbs,
		const struct sb_getpost_data *sb_data,
		size_t sb_data_elements,
		uint8_t sign[MD5_DIGEST_LENGTH]) {

	char secret_hex[16 * 2 + 1];
	char tmp_str[2048], format[8];
	size_t x, offset;

	for (x = offset = 0; x < sb_data_elements; x++) {

		/* it means that if numerical data is zero it is also discarded */
		if (sb_data[x].data == NULL)
			continue;

		sprintf(format, "%%s%%%c", sb_data[x].data_format);
		offset += sprintf(tmp_str + offset, format, sb_data[x].name, sb_data[x].data);
	}

	strcat(tmp_str, mem2hex(secret_hex, sbs->secret, 16));
	MD5((unsigned char *)tmp_str, strlen(tmp_str), sign);

#if DEBUG
	char *tmp;
	if ((tmp = strstr(tmp_str, sbs->session_key)) != NULL)
		memset(tmp, 'x', strlen(sbs->session_key));
	debug("Signature data: %s", tmp_str);
#endif

}

/* Make curl GET/POST request string (escape data). */
static char *sb_make_curl_getpost_string(
		const scrobbler_session_t *sbs,
		const struct sb_getpost_data *sb_data,
		size_t sb_data_elements,
		char *dest,
		size_t dest_size,
		CURL *curl) {

	char *escaped_data, format[8];
	size_t x, offset;

	for (x = offset = 0; x < sb_data_elements; x++) {

		/* it means that if numerical data is zero it is also discarded */
		if (sb_data[x].data == NULL)
			continue;

		sprintf(format, "%%s=%%%c&", sb_data[x].data_format);

		/* escape for string data */
		if (sb_data[x].data_format == 's') {
			escaped_data = curl_easy_escape(curl, sb_data[x].data, 0);
			offset += snprintf(dest + offset, dest_size - offset, format,
					sb_data[x].name, escaped_data);
			curl_free(escaped_data);
		}
		else
			/* non-string content - no need for escaping */
			offset += snprintf(dest + offset, dest_size - offset, format,
					sb_data[x].name, sb_data[x].data);
	}

	/* strip '&' from the end of the string */
	dest[offset - 1] = '\0';

#if DEBUG
	char data[2048] = {};
	char *tmp = strncpy(data, dest, sizeof(data) - 1);
	if ((tmp = strstr(tmp, sbs->session_key)) != NULL)
		memset(tmp, 'x', strlen(sbs->session_key));
	debug("Request: %s", data);
#endif

	return dest;
}

/* Scrobble a track. */
scrobbler_status_t scrobbler_scrobble(scrobbler_session_t *sbs,
		scrobbler_trackinfo_t *sbt) {

	CURL *curl;
	uint8_t sign[MD5_DIGEST_LENGTH];
	char api_key_hex[sizeof(sbs->api_key) * 2 + 1];
	char sign_hex[sizeof(sign) * 2 + 1];
	char post_data[2048];
	struct sb_response_data response;

	/* data in alphabetical order sorted by name field (except api_sig) */
	struct sb_getpost_data sb_data[] = {
		{"album", 's', sbt->album},
		{"albumArtist", 's', sbt->album_artist},
		{"api_key", 's', api_key_hex},
		{"artist", 's', sbt->artist},
		/* {"context", 's', NULL}, */
		{"duration", 'd', (void *)(long)sbt->duration},
		{"mbid", 's', sbt->mb_track_id},
		{"method", 's', "track.scrobble"},
		{"sk", 's', sbs->session_key},
		{"timestamp", 'd', (void *)sbt->timestamp},
		{"track", 's', sbt->track},
		{"trackNumber", 'd', (void *)(long)sbt->track_number},
		/* {"streamId", 's', NULL}, */
		{"api_sig", 's', sign_hex},
	};

	debug("Scrobble: %ld", sbt->timestamp);
	debug("Payload: %s - %s (%s) - %d. %s (%ds)",
			sbt->artist, sbt->album, sbt->album_artist,
			sbt->track_number, sbt->track, sbt->duration);

	if (sbt->artist == NULL || sbt->track == NULL || sbt->timestamp == 0)
		return sbs->status = SCROBBLER_STATUS_ERR_TRACKINF;

	if ((curl = sb_curl_init(CURLOPT_POST, &response)) == NULL)
		return sbs->status = SCROBBLER_STATUS_ERR_CURLINIT;

	mem2hex(api_key_hex, sbs->api_key, sizeof(sbs->api_key));

	/* make signature for track.scrobble API call */
	sb_generate_method_signature(sbs, sb_data, 11, sign);
	mem2hex(sign_hex, sign, sizeof(sign));

	/* make track.scrobble POST request */
	sb_make_curl_getpost_string(sbs, sb_data, 12,
			post_data, sizeof(post_data), curl);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
	curl_easy_setopt(curl, CURLOPT_URL, sbs->api_url);

	sb_check_response(&response, curl_easy_perform(curl), sbs);
	debug("Scrobble status: %d", sbs->status);

	sb_curl_cleanup(curl, &response);
	return sbs->status;
}

/* Notify Last.fm that a user has started listening to a track. This
 * is an engine function (without a check for required arguments). */
static scrobbler_status_t sb_update_now_playing(scrobbler_session_t *sbs,
		scrobbler_trackinfo_t *sbt) {

	CURL *curl;
	uint8_t sign[MD5_DIGEST_LENGTH];
	char api_key_hex[sizeof(sbs->api_key) * 2 + 1];
	char sign_hex[sizeof(sign) * 2 + 1];
	char post_data[2048];
	struct sb_response_data response;

	/* data in alphabetical order sorted by name field (except api_sig) */
	struct sb_getpost_data sb_data[] = {
		{"album", 's', sbt->album},
		{"albumArtist", 's', sbt->album_artist},
		{"api_key", 's', api_key_hex},
		{"artist", 's', sbt->artist},
		/* {"context", 's', NULL}, */
		{"duration", 'd', (void *)(long)sbt->duration},
		{"mbid", 's', sbt->mb_track_id},
		{"method", 's', "track.updateNowPlaying"},
		{"sk", 's', sbs->session_key},
		{"track", 's', sbt->track},
		{"trackNumber", 'd', (void *)(long)sbt->track_number},
		{"api_sig", 's', sign_hex},
	};

	debug("Now playing: %ld", sbt->timestamp);
	debug("Payload: %s - %s (%s) - %d. %s (%ds)",
			sbt->artist, sbt->album, sbt->album_artist,
			sbt->track_number, sbt->track, sbt->duration);

	if ((curl = sb_curl_init(CURLOPT_POST, &response)) == NULL)
		return sbs->status = SCROBBLER_STATUS_ERR_CURLINIT;

	mem2hex(api_key_hex, sbs->api_key, sizeof(sbs->api_key));

	/* make signature for track.updateNowPlaying API call */
	sb_generate_method_signature(sbs, sb_data, 10, sign);
	mem2hex(sign_hex, sign, sizeof(sign));

	/* make track.updateNowPlaying POST request */
	sb_make_curl_getpost_string(sbs, sb_data, 11,
			post_data, sizeof(post_data), curl);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
	curl_easy_setopt(curl, CURLOPT_URL, sbs->api_url);

	sb_check_response(&response, curl_easy_perform(curl), sbs);
	debug("Now playing status: %d", sbs->status);

	sb_curl_cleanup(curl, &response);
	return sbs->status;
}

/* Update "Now playing" notification. */
scrobbler_status_t scrobbler_update_now_playing(scrobbler_session_t *sbs,
		scrobbler_trackinfo_t *sbt) {
	debug("Now playing wrapper");
	if (sbt->artist == NULL || sbt->track == NULL)
		return sbs->status = SCROBBLER_STATUS_ERR_TRACKINF;
	return sb_update_now_playing(sbs, sbt);
}

/* Hard-codded method for validating session key. This approach uses the
 * updateNotify method call with invalid parameters as a test call. */
scrobbler_status_t scrobbler_test_session_key(scrobbler_session_t *sbs) {
	debug("Session validation wrapper");
	scrobbler_trackinfo_t sbt = { .artist = "", .track = "" };
	sb_update_now_playing(sbs, &sbt);
	/* Because we are using invalid parameters for session key validation,
	 * service might actually return the "Invalid Parameters" error code.
	 * However, it means that the session key itself is valid. */
	if (sbs->status == SCROBBLER_STATUS_ERR_SCROBAPI &&
			sbs->errornum == SCROBBLER_API_ERR_INVALID_PARAMS)
		return SCROBBLER_STATUS_OK;
	return sbs->status;
}

/* Get the session key. */
const char *scrobbler_get_session_key(scrobbler_session_t *sbs) {
	return sbs->session_key;
}

/* Set the session key. */
void scrobbler_set_session_key(scrobbler_session_t *sbs, const char *str) {
	strncpy(sbs->session_key, str, sizeof(sbs->session_key) - 1);
	sbs->session_key[sizeof(sbs->session_key) - 1] = '\0';
}

/* Perform scrobbler service authentication process. */
scrobbler_status_t scrobbler_authentication(scrobbler_session_t *sbs,
		scrobbler_authuser_callback_t callback) {

	CURL *curl;
	scrobbler_status_t status;
	uint8_t sign[MD5_DIGEST_LENGTH];
	char api_key_hex[sizeof(sbs->api_key) * 2 + 1];
	char sign_hex[sizeof(sign) * 2 + 1], token_hex[33];
	char get_url[1024], *ptr;
	struct sb_response_data response;
	size_t len;

	/* data in alphabetical order sorted by name field (except api_sig) */
	struct sb_getpost_data sb_data_token[] = {
		{"api_key", 's', api_key_hex},
		{"method", 's', "auth.getToken"},
		{"api_sig", 's', sign_hex},
	};
	struct sb_getpost_data sb_data_session[] = {
		{"api_key", 's', api_key_hex},
		{"method", 's', "auth.getSession"},
		{"token", 's', token_hex},
		{"api_sig", 's', sign_hex},
	};

	if ((curl = sb_curl_init(CURLOPT_HTTPGET, &response)) == NULL)
		return sbs->status = SCROBBLER_STATUS_ERR_CURLINIT;

	mem2hex(api_key_hex, sbs->api_key, sizeof(sbs->api_key));

	/* make signature for auth.getToken API call */
	sb_generate_method_signature(sbs, sb_data_token, 2, sign);
	mem2hex(sign_hex, sign, sizeof(sign));

	/* make auth.getToken GET request */
	len = snprintf(get_url, sizeof(get_url), "%s?", sbs->api_url);
	sb_make_curl_getpost_string(sbs, sb_data_token, 3,
			get_url + len, sizeof(get_url) - len, curl);
	curl_easy_setopt(curl, CURLOPT_URL, get_url);

	status = sb_check_response(&response, curl_easy_perform(curl), sbs);
	if (status != SCROBBLER_STATUS_OK) {
		sb_curl_cleanup(curl, &response);
		return status;
	}

	memcpy(token_hex, strstr(response.data, "<token>") + 7, 32);
	token_hex[32] = '\0';

	/* perform user authorization (callback function) */
	sprintf(get_url, "%s?api_key=%s&token=%s",
			sbs->auth_url, api_key_hex, token_hex);
	if (callback(get_url) != 0) {
		sb_curl_cleanup(curl, &response);
		return sbs->status = SCROBBLER_STATUS_ERR_CALLBACK;
	}

	/* make signature for auth.getSession API call */
	sb_generate_method_signature(sbs, sb_data_session, 3, sign);
	mem2hex(sign_hex, sign, sizeof(sign));

	/* reinitialize response buffer */
	response.size = 0;

	/* make auth.getSession GET request */
	len = snprintf(get_url, sizeof(get_url), "%s?", sbs->api_url);
	sb_make_curl_getpost_string(sbs, sb_data_session, 4,
			get_url + len, sizeof(get_url) - len, curl);
	curl_easy_setopt(curl, CURLOPT_URL, get_url);

	status = sb_check_response(&response, curl_easy_perform(curl), sbs);
	debug("Authentication status: %d", sbs->status);
	if (status != SCROBBLER_STATUS_OK) {
		sb_curl_cleanup(curl, &response);
		return status;
	}

	/* extract user name from the response */
	strncpy(sbs->user_name, strstr(response.data, "<name>") + 6,
			sizeof(sbs->user_name));
	sbs->user_name[sizeof(sbs->user_name) - 1] = '\0';
	if ((ptr = strchr(sbs->user_name, '<')) != NULL)
		*ptr = 0;

	/* extract session key from the response */
	strncpy(sbs->session_key, strstr(response.data, "<key>") + 5,
			sizeof(sbs->session_key));
	sbs->session_key[sizeof(sbs->session_key) - 1] = '\0';

	sb_curl_cleanup(curl, &response);
	return SCROBBLER_STATUS_OK;
}

/* Initialize scrobbler session. On success this function returns pointer
 * to allocated scrobbler_session structure, which must be freed by call
 * to scrobbler_free. On error NULL is returned. */
scrobbler_session_t *scrobbler_initialize(const char *api_url,
		const char *auth_url, uint8_t api_key[16], uint8_t secret[16]) {

	scrobbler_session_t *sbs;

	/* allocate space for scrobbler session structure */
	if ((sbs = calloc(1, sizeof(scrobbler_session_t))) == NULL)
		return NULL;

	if (curl_global_init(CURL_GLOBAL_DEFAULT) != 0) {
		free(sbs);
		return NULL;
	}

	strncpy(sbs->api_url, api_url, sizeof(sbs->api_url) - 1);
	strncpy(sbs->auth_url, auth_url, sizeof(sbs->auth_url) - 1);
	memcpy(sbs->api_key, api_key, sizeof(sbs->api_key));
	memcpy(sbs->secret, secret, sizeof(sbs->secret));

	return sbs;
}

void scrobbler_free(scrobbler_session_t *sbs) {
	curl_global_cleanup();
	free(sbs);
}

/* Get the human-readable error message. */
const char *scrobbler_strerror(scrobbler_session_t *sbs) {
	switch (sbs->status) {
	case SCROBBLER_STATUS_OK:
		return "Success";
	case SCROBBLER_STATUS_ERR_CURLINIT:
		return "CURL initialization failure";
	case SCROBBLER_STATUS_ERR_CURLPERF:
		return curl_easy_strerror(sbs->errornum);
	case SCROBBLER_STATUS_ERR_SCROBAPI:
		switch ((scrobbler_api_error_t)sbs->errornum) {
		case SCROBBLER_API_ERR_INVALID_SERVICE:
			return "API: Invalid service";
		case SCROBBLER_API_ERR_INVALID_METHOD:
			return "API: Invalid method";
		case SCROBBLER_API_ERR_AUTH_FAILED:
			return "API: Authentication failed";
		case SCROBBLER_API_ERR_INVALID_FORMAT:
			return "API: Invalid format";
		case SCROBBLER_API_ERR_INVALID_PARAMS:
			return "API: Invalid parameters";
		case SCROBBLER_API_ERR_INVALID_RESOURCE:
			return "API: Invalid resource";
		case SCROBBLER_API_ERR_OPERATION_FAILED:
			return "API: Operation failed";
		case SCROBBLER_API_ERR_INVALID_SESSION_KEY:
			return "API: Invalid session key";
		case SCROBBLER_API_ERR_INVALID_API_KEY:
			return "API: Invalid API key";
		case SCROBBLER_API_ERR_SERVICE_OFFLINE:
			return "API: Service temporarily unavailable";
		case SCROBBLER_API_ERR_SUBSCRIBERS_ONLY:
			return "API: Subscribers only";
		case SCROBBLER_API_ERR_INVALID_SIGNATURE:
			return "API: Invalid signature";
		case SCROBBLER_API_ERR_UNAUTHORIZED_TOKEN:
			return "API: Token not authorized";
		case SCROBBLER_API_ERR_ITEM_NOT_AVAILABLE:
			return "API: Token has expired";
		case SCROBBLER_API_ERR_SERVICE_UNAVAILABLE:
			return "API: Service temporarily unavailable";
		case SCROBBLER_API_ERR_LOGIN_REQUIRED:
			return "API: Login required";
		case SCROBBLER_API_ERR_TRIAL_EXPIRED:
			return "API: Trial expired";
		case SCROBBLER_API_ERR_NOT_ENOUGH_CONTENT:
		case SCROBBLER_API_ERR_NOT_ENOUGH_MEMBERS:
		case SCROBBLER_API_ERR_NOT_ENOUGH_FANS:
		case SCROBBLER_API_ERR_NOT_ENOUGH_NEIGHBOURS:
			return "API: Not enough content/members/etc.";
		case SCROBBLER_API_ERR_RADIO_NO_PEAK:
			return "API: Radio no peak usage";
		case SCROBBLER_API_ERR_RADIO_NOT_FOUND:
			return "API: Radio not found";
		case SCROBBLER_API_ERR_API_KEY_SUSPENDED:
			return "API: Suspended API key";
		case SCROBBLER_API_ERR_DEPRECATED:
			return "API: Deprecated request";
		case SCROBBLER_API_ERR_LIMIT_EXCEDED:
			return "API: Rate limit exceeded";
		}
		return "API: Unknown error";
	case SCROBBLER_STATUS_ERR_CALLBACK:
		return "Authentication callback failure";
	case SCROBBLER_STATUS_ERR_TRACKINF:
		return "Missing required data for track";
	default:
		return "Unknown status";
	}
}

/* Dump memory pointed by the mem into the dest string in the hexadecimal
 * format. Note, that the dest has to be big enough to contain 2 * n with
 * termination NULL character. */
char *mem2hex(char *dest, const unsigned char *mem, size_t n) {

	const char hexchars[] = "0123456789abcdef";
	char *ptr = dest;
	size_t i;

	for (i = 0; i < n; i++) {
		*(ptr++) = hexchars[(mem[i] >> 4) & 0x0f];
		*(ptr++) = hexchars[mem[i] & 0x0f];
	}
	*ptr = '\0';

	return dest;
}
