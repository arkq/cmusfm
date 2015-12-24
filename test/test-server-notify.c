#include <assert.h>

#define DEBUG_SKIP_HICCUP
#include "test-server.inc"

int main(void) {

	char track_buffer[512];
	struct cmusfm_data_record *track = (struct cmusfm_data_record *)track_buffer;

	track->duration = 35;
	track->off_album = 20;
	track->off_title = 40;
	track->off_location = 60;
	strcpy(((char *)(track + 1)), "The Beatles");
	strcpy(((char *)(track + 1)) + 20, "");
	strcpy(((char *)(track + 1)) + 40, "Yellow Submarine");
	strcpy(((char *)(track + 1)) + 60, "");

	config.nowplaying_localfile = 1;
	config.nowplaying_shoutcast = 1;

	track->status = CMSTATUS_PLAYING;
	cmusfm_server_update_record_checksum(track);

	cmusfm_server_process_data(NULL, track);
	assert(scrobbler_update_now_playing_count == 1);

	config.nowplaying_localfile = 0;

	cmusfm_server_process_data(NULL, track);
	assert(scrobbler_update_now_playing_count == 1);
	assert(strcmp(scrobbler_update_now_playing_sbt.artist, "The Beatles") == 0);
	assert(strcmp(scrobbler_update_now_playing_sbt.track, "Yellow Submarine") == 0);

#if ENABLE_LIBNOTIFY
	config.notification = 1;
#endif

	track->status |= CMSTATUS_SHOUTCASTMASK;
	cmusfm_server_update_record_checksum(track);

	cmusfm_server_process_data(NULL, track);
	assert(scrobbler_update_now_playing_count == 2);
#if ENABLE_LIBNOTIFY
	assert(cmusfm_notify_show_count == 1);
#endif

	return EXIT_SUCCESS;
}
