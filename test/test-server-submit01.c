#include <assert.h>

#define DEBUG_SKIP_HICCUP
#include "test-server.inc"

int main(void) {

	char track_buffer[512];
	struct sock_data_tag *track = (struct sock_data_tag *)track_buffer;

	track->alboff = 20;
	track->titoff = 40;
	track->locoff = 60;
	strcpy(((char *)(track + 1)), "The Beatles");
	strcpy(((char *)(track + 1)) + 20, "");
	strcpy(((char *)(track + 1)) + 40, "Yellow Submarine");
	strcpy(((char *)(track + 1)) + 60, "");

	/* data communication facility */
	int fd[2];
	assert(pipe(fd) != -1);

	config.submit_localfile = 1;
	config.submit_shoutcast = 1;

	track->status = CMSTATUS_PLAYING;
	track->duration = 35;
	write(fd[1], track_buffer, sizeof(track_buffer));
	cmusfm_server_process_data(fd[0], NULL);

	/* track was played for more than half its duration */
	sleep(track->duration / 2 + 1);

	track->status = CMSTATUS_PLAYING;
	track->duration = 35;
	strcpy(((char *)(track + 1)) + 40, "For No One");
	write(fd[1], track_buffer, sizeof(track_buffer));
	cmusfm_server_process_data(fd[0], NULL);
	assert(scrobbler_scrobble_count == 1);

	/* track was played for less than half its duration */
	sleep(track->duration / 2 - 1);

	track->duration = 30;
	strcpy(((char *)(track + 1)) + 40, "Doctor Robert");
	write(fd[1], track_buffer, sizeof(track_buffer));
	cmusfm_server_process_data(fd[0], NULL);
	assert(scrobbler_scrobble_count == 1);

	/* whole track was played but its duration isn't longer than 30 seconds */
	sleep(track->duration);

	write(fd[1], track_buffer, sizeof(track_buffer));
	cmusfm_server_process_data(fd[0], NULL);
	assert(scrobbler_scrobble_count == 1);

	/* short track was overplayed (due to seeking) */
	sleep(track->duration + 10);

	track->status = CMSTATUS_STOPPED;
	write(fd[1], track_buffer, sizeof(track_buffer));
	cmusfm_server_process_data(fd[0], NULL);
	assert(scrobbler_scrobble_count == 1);

	return EXIT_SUCCESS;
}
