/*
 * cmusfm - test-server.inc
 * vim: ft=c
 *
 * SPDX-FileCopyrightText: 2015-2026 Arkadiusz Bokowy and contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include <pthread.h>
#include <time.h>

#include "../src/cmusfm.h"
#include "../src/server.c"

/* global variables used in the server code */
unsigned char SC_api_key[16] = { 0 };
unsigned char SC_secret[16] = { 0 };
struct cmusfm_config config = { 0 };
const char * cmusfm_config_file = NULL;
const char * cmusfm_socket_file = NULL;

/**
 * Variable for tracking the current virtual time. */
static time_t virtual_clock_now = SERVICE_RETRY_DELAY + 42;

/**
 * Non-weak implementation to get the current virtual time. */
time_t get_current_unix_time(void) { return virtual_clock_now; }

/**
 * Sleep for the given number of seconds in virtual time. */
void virtual_sleep(unsigned int sec) { virtual_clock_now += sec; }

/**
 * Redirect standard sleep to the virtual clock sleep. */
#define sleep(s) virtual_sleep(s)

static pthread_mutex_t server_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t server_cond = PTHREAD_COND_INITIALIZER;
static bool server_ready = false;

/**
 * Non-weak implementation to notify that the server is ready. */
void cmusfm_server_ready(void) {
	pthread_mutex_lock(&server_mutex);
	server_ready = true;
	pthread_mutex_unlock(&server_mutex);
	pthread_cond_broadcast(&server_cond);
}

static scrobbler_trackinfo_t scrobble_sbt = { 0 };
static unsigned int scrobble_count = 0;

/**
 * Mock subscription subsystem - with the invocation counter. */
scrobbler_status_t scrobbler_scrobble(scrobbler_session_t * sbs, scrobbler_trackinfo_t * sbt) {
	(void)sbs;
	pthread_mutex_lock(&server_mutex);
	memcpy(&scrobble_sbt, sbt, sizeof(scrobble_sbt));
	scrobble_count++;
	pthread_mutex_unlock(&server_mutex);
	pthread_cond_broadcast(&server_cond);
	return SCROBBLER_STATUS_OK;
}

static scrobbler_trackinfo_t now_playing_sbt = { 0 };
static unsigned int now_playing_count = 0;

/**
 * Mock now-playing subsystem - with the invocation counter. */
scrobbler_status_t scrobbler_update_now_playing(scrobbler_session_t * sbs,
		scrobbler_trackinfo_t * sbt) {
	(void)sbs;
	pthread_mutex_lock(&server_mutex);
	memcpy(&now_playing_sbt, sbt, sizeof(now_playing_sbt));
	now_playing_count++;
	pthread_mutex_unlock(&server_mutex);
	pthread_cond_broadcast(&server_cond);
	return SCROBBLER_STATUS_OK;
}

static scrobbler_trackinfo_t notify_show_sbt = { 0 };
static unsigned int notify_show_count = 0;

/**
 * Mock notification subsystem - with the invocation counter. */
void cmusfm_notify_show(const scrobbler_trackinfo_t * sbt, const char * icon) {
	(void)icon;
	pthread_mutex_lock(&server_mutex);
	memcpy(&notify_show_sbt, sbt, sizeof(notify_show_sbt));
	notify_show_count++;
	pthread_mutex_unlock(&server_mutex);
	pthread_cond_broadcast(&server_cond);
}

/**
 * Wait until the server is ready to accept connections. */
int wait_for_cmusfm_server(unsigned int timeout) {

	struct timespec ts;
	int rv = 0;

	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec += timeout;

	pthread_mutex_lock(&server_mutex);
	while (!server_ready && rv == 0)
		rv = pthread_cond_timedwait(&server_cond, &server_mutex, &ts);
	pthread_mutex_unlock(&server_mutex);

	return rv == 0 ? 0 : -1;
}

static unsigned int now_playing_wait_count = 0;

/**
 * Wait until the now-playing mock was invoked or the timeout expired. */
int wait_for_now_playing_update(unsigned int timeout) {

	struct timespec ts;
	int rv = 0;

	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec += timeout;

	pthread_mutex_lock(&server_mutex);
	while (now_playing_count <= now_playing_wait_count && rv == 0)
		rv = pthread_cond_timedwait(&server_cond, &server_mutex, &ts);
	pthread_mutex_unlock(&server_mutex);

	if (rv == 0)
		now_playing_wait_count = now_playing_count;

	return rv == 0 ? 0 : -1;
}

/* Helper function for updating data record check sums. */
void cmusfm_data_record_update_checksum(struct cmusfm_data_record * record) {
	record->checksum1 = make_record_checksum1(record);
	record->checksum2 = make_record_checksum2(record);
}

/* Other (irrelevant) functions used by the server code. */

void cmusfm_cache_update(const scrobbler_trackinfo_t * sbt) { (void)sbt; }

void cmusfm_cache_submit(scrobbler_session_t * sbs) { (void)sbs; }

int cmusfm_config_read(const char * fname, struct cmusfm_config * conf) {
	(void)fname; (void)conf;
	return 0;
}

int cmusfm_config_add_watch(int fd) {
	(void)fd;
	return 0;
}

void cmusfm_notify_initialize() {}

void cmusfm_notify_free() {}

scrobbler_session_t * scrobbler_initialize(const char * api_url, const char * auth_url,
		uint8_t api_key[16], uint8_t secret[16]) {
	(void)api_url; (void)auth_url; (void)api_key; (void)secret;
	return NULL;
}

void scrobbler_free(scrobbler_session_t * sbs) { (void)sbs; }

void scrobbler_set_session_key(scrobbler_session_t * sbs, const char * str) {
	(void)sbs; (void)str;
}

scrobbler_status_t scrobbler_test_session_key(scrobbler_session_t * sbs) {
	(void)sbs;
	return 0;
}
