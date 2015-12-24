#include <assert.h>

#define DEBUG_SKIP_HICCUP
#include "test-server.inc"

int main(void) {

	char track_buffer[512];
	struct cmusfm_data_record *track = (struct cmusfm_data_record *)track_buffer;

	track->off_album = 20;
	track->off_title = 40;
	track->off_location = 60;
	strcpy(((char *)(track + 1)), "The Beatles");
	strcpy(((char *)(track + 1)) + 20, "");
	strcpy(((char *)(track + 1)) + 40, "Yellow Submarine");
	strcpy(((char *)(track + 1)) + 60, "");

	config.submit_localfile = 1;
	config.submit_shoutcast = 1;

	track->status = CMSTATUS_PLAYING;
	track->duration = 4 * 60 * 5;
	cmusfm_server_update_record_checksum(track);

	cmusfm_server_process_data(NULL, track);

	/* track was played for more than 4 minutes but less than half its duration */
	sleep(4 * 60 + 1);

	track->status = CMSTATUS_STOPPED;
	cmusfm_server_update_record_checksum(track);

	cmusfm_server_process_data(NULL, track);
	assert(scrobbler_scrobble_count == 1);

	return EXIT_SUCCESS;
}
