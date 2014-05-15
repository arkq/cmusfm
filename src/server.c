/*
 * cmusfm - server.c
 * Copyright (c) 2010-2014 Arkadiusz Bokowy
 *
 * This file is a part of a cmusfm.
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

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include <libgen.h>
#include "cmusfm.h"
#include "server.h"
#include "cache.h"
#include "debug.h"


void set_trackinfo(scrobbler_trackinfo_t *sbt, const struct sock_data_tag *dt)
{
	sbt->duration = dt->duration;
	sbt->track_number = dt->tracknb;
	sbt->artist = (char *)(dt + 1);
	sbt->album = &((char *)(dt + 1))[dt->alboff];
	sbt->track = &((char *)(dt + 1))[dt->titoff];
}

// Simple and fast "hashing" function.
int make_data_hash(const unsigned char *data, int len)
{
	int x, hash;

	for(x = hash = 0; x < len; x++)
		hash += data[x] * (x + 1);
	return hash;
}

// Process real server task - Last.fm submission.
void cmusfm_server_process_data(int fd, scrobbler_session_t *sbs)
{
	static char saved_data[CMSOCKET_BUFFER_SIZE], saved_is_radio = 0;
	char buffer[CMSOCKET_BUFFER_SIZE];
	struct sock_data_tag *sock_data = (struct sock_data_tag*)buffer;
	ssize_t rd_len;
	// scrobbler stuff
	static time_t scrobbler_fail_time = 1; //0 -> login OK
	static time_t started = 0, playtime = 0, fulltime = 10, reinitime = 0;
	static int prev_hash = 0, paused = 0;
	scrobbler_trackinfo_t sb_tinf;
	int new_hash;
	char raw_status;

	memset(&sb_tinf, 0, sizeof(sb_tinf));

	rd_len = read(fd, buffer, sizeof(buffer));
	debug("rdlen: %ld, status: %d", rd_len, sock_data->status);

	if(rd_len < (ssize_t)sizeof(struct sock_data_tag))
		return;  // something was wrong...

	debug("payload: %s - %s - %d. %s (%ds)",
			(char *)(sock_data + 1), &((char *)(sock_data + 1))[sock_data->alboff],
			sock_data->tracknb, &((char *)(sock_data + 1))[sock_data->titoff],
			sock_data->duration);

#ifdef DEBUG
	// simulate server "hiccup" (e.g. internet connection issue)
	debug("server hiccup test (5s)");
	sleep(5);
#endif

	// make data hash without status field
	new_hash = make_data_hash((unsigned char*)&buffer[1], rd_len - 1);

	raw_status = sock_data->status & ~CMSTATUS_SHOUTCASTMASK;

	// test connection to server (on failure try again in some time)
	if(scrobbler_fail_time != 0 &&
			time(NULL) - scrobbler_fail_time > SERVICE_RETRY_DELAY) {
		if(scrobbler_test_session_key(sbs) == 0) {  // everything should be OK now
			scrobbler_fail_time = 0;

			// if there is something in cache submit it
			cmusfm_cache_submit(sbs);
		}
		else
			scrobbler_fail_time = time(NULL);
	}

	if(new_hash != prev_hash) {  // maybe it's time to submit :)
		prev_hash = new_hash;
submission_place:
		playtime += time(NULL) - reinitime;
		if(started != 0 && (playtime*100/fulltime > 50 || playtime > 240)) {
			// playing duration is OK so submit track
			set_trackinfo(&sb_tinf, (struct sock_data_tag*)saved_data);
			sb_tinf.timestamp = started;

			if((saved_is_radio && !config.submit_shoutcast) ||
					(!saved_is_radio && !config.submit_localfile)) {
				// skip submission if we don't want it
				debug("submission not enabled");
				goto submission_skip;
			}

			if(scrobbler_fail_time == 0) {
				if(scrobbler_scrobble(sbs, &sb_tinf) != 0) {
					scrobbler_fail_time = 1;
					goto submission_failed;
				}
			}
			else  // write data to cache
submission_failed:
				cmusfm_cache_update(&sb_tinf);
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
				// update now-playing indicator if we want it
				if((saved_is_radio && config.nowplaying_shoutcast) ||
						(!saved_is_radio && config.nowplaying_localfile)) {
					set_trackinfo(&sb_tinf, sock_data);
					if(scrobbler_update_now_playing(sbs, &sb_tinf) != 0)
						scrobbler_fail_time = 1;
				}
#if DEBUG
				else
					debug("now playing not enabled");
#endif
			}
		}
	}
	else {  // new_hash == prev_hash
		if(raw_status == CMSTATUS_STOPPED)
			goto submission_place;

		if(raw_status == CMSTATUS_PAUSED) {
			playtime += time(NULL) - reinitime;
			paused = 1;
		}

		//NOTE: There is no possibility to distinguish between replayed track
		//      and unpaused. We assumed that if track was paused before this
		//      indicate that track is continued to playing in other case
		//      track is played again so we should submit previous playing.
		if(raw_status == CMSTATUS_PLAYING) {
			if(paused) {
				reinitime = time(NULL);
				paused = 0;
			}
			else
				goto submission_place;
		}
	}
}

// server shutdown stuff
static int server_on = 1;
static void cmusfm_server_stop(int sig) {
	debug("stopping cmusfm server");
	server_on = 0;
}

// Run server instance (fork to background) and manage connections to it.
void cmusfm_server_start()
{
	struct sigaction sigact;
	struct sockaddr_un sock_a;
	int sock, peer_fd;
	fd_set rd_fds;
	scrobbler_session_t *sbs;

	debug("starting cmusfm server");

	memset(&sock_a, 0, sizeof(sock_a));
	sock_a.sun_family = AF_UNIX;
	strcpy(sock_a.sun_path, get_cmusfm_socket_file());
	sock = socket(PF_UNIX, SOCK_STREAM, 0);

	// check if behind the socket there is already an active server instance
	if(connect(sock, (struct sockaddr*)(&sock_a), sizeof(sock_a)) == 0)
		return;

	// initialize scrobbling library
	sbs = scrobbler_initialize(SC_api_key, SC_secret);
	scrobbler_set_session_key_str(sbs, config.session_key);

	// catch signals which are used to quit server
	memset(&sigact, 0, sizeof(sigact));
	sigact.sa_handler = cmusfm_server_stop;
	sigaction(SIGHUP, &sigact, NULL);
	sigaction(SIGTERM, &sigact, NULL);
	sigaction(SIGINT, &sigact, NULL);

	// create server communication socket (no error check)
	unlink(sock_a.sun_path);
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
		if(FD_ISSET(peer_fd, &rd_fds)) {
			cmusfm_server_process_data(peer_fd, sbs);
			close(peer_fd);
			peer_fd = -1;
		}
	}

	close(sock);
	scrobbler_free(sbs);
	unlink(sock_a.sun_path);
}

// Send track info to server instance.
int cmusfm_server_send_track(struct cmtrack_info *tinfo)
{
	char buffer[CMSOCKET_BUFFER_SIZE];
	struct sock_data_tag *sock_data = (struct sock_data_tag*)buffer;
	char *artist = (char *)(sock_data + 1);
	char *album = (char *)(sock_data + 1);
	char *title = (char *)(sock_data + 1);
	struct format_match *match, *matches;
	struct sockaddr_un sock_a;
	int sock;

	debug("sending track to cmusfm server");

	memset(buffer, 0, sizeof(buffer));

	// load data into the sock container
	sock_data->status = tinfo->status;
	sock_data->tracknb = tinfo->tracknb;
	// if no duration time assume 3 min
	sock_data->duration = tinfo->duration == 0 ? 180 : tinfo->duration;

	// add Shoutcast (stream) flag
	if(tinfo->url != NULL)
		sock_data->status |= CMSTATUS_SHOUTCASTMASK;

	if((tinfo->url != NULL && tinfo->artist == NULL && tinfo->title != NULL) ||
			(tinfo->file != NULL && tinfo->artist == NULL && tinfo->title == NULL)) {
		// NOTE: Automatic format detection mode.
		// When title and artist was not specified but URL or file is available.
		debug("regular expression matching mode");

		if(tinfo->url != NULL) {
			// URL: try to fetch artist and track tile form the 'title' field

			matches = get_regexp_format_matches(tinfo->title, config.format_shoutcast);
			if(matches == NULL) {
				fprintf(stderr, "error: shoutcast format match failed\n");
				return -1;
			}
		}
		else {
			// FILE: try to fetch artist and track title from the 'file' field

			tinfo->file = basename(tinfo->file);
			matches = get_regexp_format_matches(tinfo->file, config.format_localfile);
			if(matches == NULL) {
				fprintf(stderr, "error: localfile format match failed\n");
				return -1;
			}
		}

		match = get_regexp_match(matches, CMFORMAT_ARTIST);
		strncpy(artist, match->data, match->len);
		album = &artist[strlen(artist) + 1];
		match = get_regexp_match(matches, CMFORMAT_ALBUM);
		strncpy(album, match->data, match->len);
		title = &album[strlen(album) + 1];
		match = get_regexp_match(matches, CMFORMAT_TITLE);
		strncpy(title, match->data, match->len);

		free(matches);

	}
	else {

		if(tinfo->artist != NULL) strcpy(artist, tinfo->artist);
		album = &artist[strlen(artist) + 1];
		if(tinfo->album != NULL) strcpy(album, tinfo->album);
		title = &album[strlen(album) + 1];
		if(tinfo->title != NULL) strcpy(title, tinfo->title);

	}

	// calculate data offsets
	sock_data->alboff = album - artist;
	sock_data->titoff = title - artist;

	// connect to the communication socket
	memset(&sock_a, 0, sizeof(sock_a));
	strcpy(sock_a.sun_path, get_cmusfm_socket_file());
	sock_a.sun_family = AF_UNIX;
	sock = socket(PF_UNIX, SOCK_STREAM, 0);
	if(connect(sock, (struct sockaddr*)(&sock_a), sizeof(sock_a)) == -1) {
		close(sock);
		return -1;
	}

	debug("socket wrlen: %ld", sizeof(struct sock_data_tag) +
			sock_data->titoff + strlen(title) + 1);
	write(sock, buffer, sizeof(struct sock_data_tag) + sock_data->titoff +
			strlen(title) + 1);
	return close(sock);
}

// Helper function for retrieving cmusfm server socket file.
char *get_cmusfm_socket_file()
{
	static char fname[128];
	sprintf(fname, "%s/" SOCKET_FNAME, get_cmus_home_dir());
	return fname;
}
