/*
    cmusfm - server.c
    Copyright (c) 2010-2012 Arkadiusz Bokowy
*/

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>        /* sockaddr_un */
#include <signal.h>
#include "libscrobbler2.h"
#include "cmusfm.h"

#ifdef DEBUG
void dump_scrobbler_(const char *info, const scrobbler_trackinfo_t *sb_tinf)
{
	printf("--= %s =--\n", info);
	printf(" timestamp: %d duration: %ds MbID: %s\n",
			(int)sb_tinf->timestamp, sb_tinf->duration, sb_tinf->mbid);
	printf(" %s - %s (%s) - %02d. %s\n", sb_tinf->artist, sb_tinf->album,
			sb_tinf->album_artist, sb_tinf->track_number, sb_tinf->track);
	fflush(stdout);
}
#endif

// Write data to cache file which should be submitted later.
// Cache file record structure:
// record_size(int)|scrobbler_trackinf_t|artist(NULL-term string)
// track(NULL-term string)|album(NULL-term string)
void update_cache(const scrobbler_trackinfo_t *sb_tinf)
{
	char cache_fname[128];
	int fd, data_size;
	int artlen, tralen, alblen;

	sprintf(cache_fname, "%s/" CACHE_FNAME, get_cmus_home_dir());
	fd = open(cache_fname, O_CREAT|O_APPEND|O_WRONLY, 00644);
	if(fd == -1) return;

	artlen = strlen(sb_tinf->artist);
	tralen = strlen(sb_tinf->track);
	alblen = strlen(sb_tinf->album);

	// calculate record size
	data_size = sizeof(int) + sizeof(scrobbler_trackinfo_t)
			+ artlen + 1 + tralen + 1 + alblen + 1;

	write(fd, &data_size, sizeof(int));
	write(fd, sb_tinf, sizeof(scrobbler_trackinfo_t));
	write(fd, sb_tinf->artist, artlen + 1);
	write(fd, sb_tinf->track, tralen + 1);
	write(fd, sb_tinf->album, alblen + 1);

	close(fd);
}

// Submit saved in cache track to Last.fm
void submit_cache(scrobbler_session_t *sbs)
{
	char cache_fname[128];
	char rd_buff[4096];
	int fd, rd_len;
	scrobbler_trackinfo_t sb_tinf;
	void *record;
	int rec_size;

	sprintf(cache_fname, "%s/" CACHE_FNAME, get_cmus_home_dir());

	// no cache file -> nothing to submit :)
	if((fd = open(cache_fname, O_RDONLY)) == -1) return;

	// read file until EOF
	while((rd_len = read(fd, rd_buff, sizeof(rd_buff))) != 0) {
		if(rd_len == -1) break;

		for(record = rd_buff;;) {
			rec_size = *((int*)record);

			// break if current record is truncated
			if(record - (void*)rd_buff + rec_size > rd_len) break;

			memcpy(&sb_tinf, record + sizeof(int), sizeof(sb_tinf));
			sb_tinf.artist = record + sizeof(int) + sizeof(sb_tinf);
			sb_tinf.track = &sb_tinf.artist[strlen(sb_tinf.artist) + 1];
			sb_tinf.album = &sb_tinf.track[strlen(sb_tinf.track) + 1];

			// submit tracks to Last.fm
#ifndef DEBUG
			scrobbler_scrobble(sbs, &sb_tinf);
#else
			dump_scrobbler_("submit_cache", &sb_tinf);
#endif

			// point to next record
			record += rec_size;

			// break if there is no enough data to obtain the record size
			if(record - (void*)rd_buff + sizeof(int) > rd_len) break;
		}

		if(record - (void*)rd_buff != rd_len)
			// seek to the beginning of current record, because
			// it is truncated, so we have to read it one more time
			lseek(fd, record - (void*)rd_buff - rd_len, SEEK_CUR);
	}

	close(fd);

	// remove cache file
	unlink(cache_fname);
}

// Process real server task - Last.fm submission
void process_server_data(int fd, scrobbler_session_t *sbs, int submit_radio)
{
	static char saved_data[sizeof(sock_buff)], saved_is_radio = 0;
	struct sock_data_tag *sock_data = (struct sock_data_tag*)sock_buff;
	int rd_len;
	// scrobbler stuff
	static time_t scrobbler_fail_time = 1; //0 -> login OK
	static time_t started = 0, playtime = 0, fulltime = 10, reinitime = 0;
	static int prev_hash = 0, paused = 0;
	scrobbler_trackinfo_t sb_tinf;
	int new_hash;
	char raw_status;

	memset(&sb_tinf, 0, sizeof(sb_tinf));

	rd_len = read(fd, sock_buff, sizeof(sock_buff));

	// at least that amount of data we should receive
	if(rd_len < sizeof(struct sock_data_tag)) return;

#ifdef DEBUG
	printf("sockrdlen: %d status: 0x%02x duration: %ds\n %s - %s - %02d. %s\n",
			rd_len, sock_data->status, sock_data->duration,
			&((char *)(sock_data + 1))[sock_data->artoff], (char *)(sock_data + 1),
			sock_data->tracknb, &((char *)(sock_data + 1))[sock_data->titoff]);
	fflush(stdout);
#endif

	// make data hash without status field
	new_hash = make_data_hash((unsigned char*)&sock_buff[1], rd_len - 1);

	raw_status = sock_data->status & ~CMSTATUS_SHOUTCASTMASK;

	// test connection to server (on failure try again in some time)
	if(scrobbler_fail_time != 0 &&
			time(NULL) - scrobbler_fail_time > LOGIN_RETRY_DELAY) {
		if(scrobbler_test_session_key(sbs) == 0) { //everything should be OK now
			scrobbler_fail_time = 0;

			// if there is something in cache submit it
			submit_cache(sbs);
		}
		else scrobbler_fail_time = time(NULL);
	}

	if(new_hash != prev_hash) {//maybe it's time to submit :)
		prev_hash = new_hash;
submission_place:
		playtime += time(NULL) - reinitime;
		if(started != 0 && (playtime*100/fulltime > 50 || playtime > 240)) {
			// playing duration is OK so submit track
			fill_trackinfo(&sb_tinf, (struct sock_data_tag*)saved_data);
			sb_tinf.timestamp = started;

			// skip radio submission if we don't want it
			if(saved_is_radio && !submit_radio) goto submission_skip;

			if(scrobbler_fail_time == 0) {
#ifndef DEBUG
				if(scrobbler_scrobble(sbs, &sb_tinf) != 0){
					scrobbler_fail_time = 1;
					// scrobbler fail -> write data to cache
					update_cache(&sb_tinf);}
#else
				dump_scrobbler_("submission", &sb_tinf);
#endif
			}
			else //write data to cache
				update_cache(&sb_tinf);
		}

submission_skip:
		if(raw_status == CMSTATUS_STOPPED)
			started = 0;
		else {
			// reinitialize variables, save track info in save_data
			started = reinitime = time(NULL);
			playtime = paused = 0;

			if((sock_data->status & CMSTATUS_SHOUTCASTMASK) != 0)
				// you have to listen radio min 90s (50% of 180)
				fulltime = 180; //overrun DEVBYZERO in URL mode :)
			else
				fulltime = sock_data->duration;

			// save information for later submission purpose
			memcpy(saved_data, sock_data, rd_len);
			saved_is_radio = sock_data->status & CMSTATUS_SHOUTCASTMASK;

			if(raw_status == CMSTATUS_PLAYING && scrobbler_fail_time == 0) {
				fill_trackinfo(&sb_tinf, sock_data);
#ifndef DEBUG
				if(scrobbler_update_now_playing(sbs, &sb_tinf) != 0)
					scrobbler_fail_time = 1;
#else
				dump_scrobbler_("update_now_playing", &sb_tinf);
#endif
			}
		}
	}
	else {// new_hash == prev_hash
		if(raw_status == CMSTATUS_STOPPED)
			goto submission_place;

		if(raw_status == CMSTATUS_PAUSED){
			playtime += time(NULL) - reinitime;
			paused = 1;}

		//NOTE: There is no possibility to distinguish between replayed track
		//      and unpaused. We assumed that if track was paused before this
		//      indicate that track is continued to playing in other case
		//      track is played again so we should submit previous playing.
		if(raw_status == CMSTATUS_PLAYING) {
			if(paused){
				reinitime = time(NULL);
				paused = 0;}
			else
				goto submission_place;
		}
	}
}

// server shutdown stuff
static int server_on = 1;
static void stop_server(int sig){server_on = 0;}

// Run server instance (fork to background) and manage connections to it
int run_server()
{
	struct sigaction sigact;
	struct sockaddr_un sock_a;
	int sock, peer_fd;
	fd_set rd_fds;
	// scrobbler stuff
	struct cmusfm_config *cm_conf;
	scrobbler_session_t *sbs;
	int submit_radio;

	// check previous instance of process
	memset(&sock_a, 0, sizeof(sock_a));
	sprintf(sock_a.sun_path, "%s/" SOCKET_FNAME , get_cmus_home_dir());
	if(access(sock_a.sun_path, R_OK) == 0) return 0;

	// get session key from configuration file
	cm_conf = malloc(sizeof(*cm_conf));
	if(read_cmusfm_config(cm_conf) != 0){
		free(cm_conf); return -1;}

	// initialize scrobbling library
	sbs = scrobbler_initialize(SC_api_key, SC_secret);
	scrobbler_set_session_key_str(sbs, cm_conf->session_key);
	submit_radio = cm_conf->submit_radio;
	free(cm_conf);

#ifndef DEBUG
	// fork server into background
	if(fork() != 0) return 0; //run only child process
#endif

	// catch signals which are used to quit server
	memset(&sigact, 0, sizeof(sigact));
	sigact.sa_handler = stop_server;
	sigaction(SIGHUP, &sigact, NULL);
	sigaction(SIGTERM, &sigact, NULL);
	sigaction(SIGINT, &sigact, NULL);

	// create server communication socket (no error check)
	sock_a.sun_family = AF_UNIX;
	sock = socket(PF_UNIX, SOCK_STREAM, 0);
	bind(sock, (struct sockaddr*)(&sock_a), sizeof(sock_a));
	listen(sock, 2);

	// server loop mode :)
	for(peer_fd = -1; server_on;) {
		FD_ZERO(&rd_fds);
		FD_SET(sock, &rd_fds);
		if(peer_fd != -1) FD_SET(peer_fd, &rd_fds);
		if(select(sock > peer_fd ? sock + 1 : peer_fd + 1, &rd_fds,
				NULL, NULL, NULL) == -1) break;

		// we've got some data in our server sockets, hmm...
		if(FD_ISSET(sock, &rd_fds)) peer_fd = accept(sock, NULL, NULL);
		if(FD_ISSET(peer_fd, &rd_fds)){
			process_server_data(peer_fd, sbs, submit_radio);
			close(peer_fd); peer_fd = -1;}
	}

	close(sock);
	scrobbler_free(sbs);
	return unlink(sock_a.sun_path);
}

void fill_trackinfo(scrobbler_trackinfo_t *sbt, const struct sock_data_tag *dt)
{
	sbt->artist = &((char *)(dt + 1))[dt->artoff];
	sbt->track = &((char *)(dt + 1))[dt->titoff];
	sbt->album = (char *)(dt + 1);
	sbt->track_number = dt->tracknb;
	sbt->duration = dt->duration;
}
