#include "../src/cmusfm.h"
#include "../src/server.c"
#include "../src/utils.c"

/* global variables used in the server code */
unsigned char SC_api_key[16] = { 0 };
unsigned char SC_secret[16] = { 0 };
struct cmusfm_config config = { 0 };
const char *cmusfm_config_file = NULL;
const char *cmusfm_socket_file = NULL;

/* mock subscription subsystem - with the invocation counter */
scrobbler_trackinfo_t scrobbler_scrobble_sbt = { 0 };
int scrobbler_scrobble_count = 0;
scrobbler_status_t scrobbler_scrobble(scrobbler_session_t *sbs, scrobbler_trackinfo_t *sbt) {
	memcpy(&scrobbler_scrobble_sbt, sbt, sizeof(scrobbler_scrobble_sbt));
	scrobbler_scrobble_count++;
	return SCROBBLER_STATUS_OK;
}

/* mock now-playing subsystem - with the invocation counter */
scrobbler_trackinfo_t scrobbler_update_now_playing_sbt = { 0 };
int scrobbler_update_now_playing_count = 0;
scrobbler_status_t scrobbler_update_now_playing(scrobbler_session_t *sbs, scrobbler_trackinfo_t *sbt) {
	memcpy(&scrobbler_update_now_playing_sbt, sbt, sizeof(scrobbler_update_now_playing_sbt));
	scrobbler_update_now_playing_count++;
	return SCROBBLER_STATUS_OK;
}

/* mock notification subsystem - with the invocation counter */
scrobbler_trackinfo_t cmusfm_notify_show_sbt = { 0 };
int cmusfm_notify_show_count = 0;
void cmusfm_notify_show(const scrobbler_trackinfo_t *sbt, const char *icon) {
	memcpy(&cmusfm_notify_show_sbt, sbt, sizeof(cmusfm_notify_show_sbt));
	cmusfm_notify_show_count++;
}

/* helper function for updating data record checksum fields */
void cmusfm_server_update_record_checksum(struct cmusfm_data_record *record) {
	record->checksum1 = make_record_checksum1(record);
	record->checksum2 = make_record_checksum2(record);
}

/* other (irrelevant) functions used by the server code */
void cmusfm_cache_update(const scrobbler_trackinfo_t *sbt) {}
void cmusfm_cache_submit(scrobbler_session_t *sbs) {}
int cmusfm_config_read(const char *fname, struct cmusfm_config *conf) { return 0; }
int cmusfm_config_add_watch(int fd) { return 0; }
void cmusfm_notify_initialize() {}
void cmusfm_notify_free() {}
scrobbler_session_t *scrobbler_initialize(const char *api_url, const char *auth_url,
	uint8_t api_key[16], uint8_t secret[16]) { return NULL; }
void scrobbler_free(scrobbler_session_t *sbs) {}
void scrobbler_set_session_key(scrobbler_session_t *sbs, const char *str) {}
scrobbler_status_t scrobbler_test_session_key(scrobbler_session_t *sbs) { return 0; }
