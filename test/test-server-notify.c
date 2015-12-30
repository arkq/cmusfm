#include <assert.h>
#include "test-server.inc"

int main(void) {

	char track_buffer[512];
	struct sock_data_tag *track = (struct sock_data_tag *)track_buffer;

	track->duration = 35;
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

	config.nowplaying_localfile = 1;
	config.nowplaying_shoutcast = 1;

	track->status = CMSTATUS_PLAYING;
	write(fd[1], track_buffer, sizeof(track_buffer));
	cmusfm_server_process_data(fd[0], NULL);
	assert(scrobbler_update_now_playing_count == 1);

	config.nowplaying_localfile = 0;

	write(fd[1], track_buffer, sizeof(track_buffer));
	cmusfm_server_process_data(fd[0], NULL);
	assert(scrobbler_update_now_playing_count == 1);
	assert(strcmp(scrobbler_update_now_playing_sbt.artist, "The Beatles") == 0);
	assert(strcmp(scrobbler_update_now_playing_sbt.track, "Yellow Submarine") == 0);

#if ENABLE_LIBNOTIFY
	config.notification = 1;
#endif

	track->status |= CMSTATUS_SHOUTCASTMASK;
	write(fd[1], track_buffer, sizeof(track_buffer));
	cmusfm_server_process_data(fd[0], NULL);
	assert(scrobbler_update_now_playing_count == 2);
#if ENABLE_LIBNOTIFY
	assert(cmusfm_notify_show_count == 1);
#endif

	return EXIT_SUCCESS;
}
