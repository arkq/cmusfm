/*
 * cmusfm - server.c
 * Copyright (c) 2010-2015 Arkadiusz Bokowy
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

#if HAVE_CONFIG_H
#include "../config.h"
#endif

#include "server.h"

#include <fcntl.h>
#include <libgen.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#if HAVE_SYS_INOTIFY_H
#include <sys/inotify.h>
#endif

#include "cache.h"
#include "cmusfm.h"
#include "config.h"
#include "debug.h"
#if ENABLE_LIBNOTIFY
#include "notify.h"
#endif


/* Helper function for artist name retrieval. */
static char *get_sock_data_artist(struct sock_data_tag *dt) {
	return (char *)(dt + 1);
}

/* Helper function for album name retrieval. */
static char *get_sock_data_album(struct sock_data_tag *dt) {
	return &((char *)(dt + 1))[dt->alboff];
}

/* Helper function for track name retrieval. */
static char *get_sock_data_track(struct sock_data_tag *dt) {
	return &((char *)(dt + 1))[dt->titoff];
}

/* Helper function for location retrieval. */
static char *get_sock_data_location(struct sock_data_tag *dt) {
	return &((char *)(dt + 1))[dt->locoff];
}

/* Copy data from the socket data into the scrobbler structure. */
static void set_trackinfo(scrobbler_trackinfo_t *sbt, struct sock_data_tag *dt) {
	memset(sbt, 0, sizeof(*sbt));
	sbt->duration = dt->duration;
	sbt->track_number = dt->tracknb;
	sbt->artist = get_sock_data_artist(dt);
	sbt->album = get_sock_data_album(dt);
	sbt->track = get_sock_data_track(dt);
}

/* Process real server task - Last.fm submission. */
static void cmusfm_server_process_data(int fd, scrobbler_session_t *sbs) {

	static char saved_data[CMSOCKET_BUFFER_SIZE];
	static char saved_is_radio = 0;

	char buffer[CMSOCKET_BUFFER_SIZE];
	struct sock_data_tag *sock_data = (struct sock_data_tag *)buffer;
	ssize_t rd_len;

	/* scrobbler stuff */
	static time_t scrobbler_fail_time = 1;
	static time_t started = 0, paused = 0, unpaused = 0;
	static time_t playtime = 0, fulltime = 10;
	static int prev_hash = 0;
	scrobbler_trackinfo_t sb_tinf;
	time_t pausedtime;
	int new_hash;
	char raw_status;

	rd_len = read(fd, buffer, sizeof(buffer));
	debug("rdlen: %ld", rd_len);

	if (rd_len < (ssize_t)sizeof(struct sock_data_tag))
		return;  /* server check or something went wrong */

	debug("payload: %s - %s - %d. %s (%ds)",
			get_sock_data_artist(sock_data), get_sock_data_album(sock_data),
			sock_data->tracknb, get_sock_data_track(sock_data),
			sock_data->duration);
	debug("location: %s", get_sock_data_location(sock_data));

#if DEBUG
	/* simulate server "hiccup" (e.g. internet connection issue) */
	debug("server hiccup test (5s)");
	sleep(5);
#endif

	/* make data hash without status field */
	new_hash = make_data_hash((unsigned char*)&buffer[1], rd_len - 1);

	raw_status = sock_data->status & ~CMSTATUS_SHOUTCASTMASK;

	/* test connection to server (on failure try again in some time) */
	if (scrobbler_fail_time != 0 &&
			time(NULL) - scrobbler_fail_time > SERVICE_RETRY_DELAY) {
		if (scrobbler_test_session_key(sbs) == 0) {  /* everything should be OK now */
			scrobbler_fail_time = 0;

			/* if there is something in cache submit it */
			cmusfm_cache_submit(sbs);
		}
		else
			scrobbler_fail_time = time(NULL);
	}

	if (new_hash != prev_hash) {  /* maybe it's time to submit :) */
		prev_hash = new_hash;
action_submit:
		playtime += time(NULL) - unpaused;
		if (started != 0 && (playtime * 100 / fulltime > 50 || playtime > 240)) {
			/* playing duration is OK so submit track */
			set_trackinfo(&sb_tinf, (struct sock_data_tag*)saved_data);
			sb_tinf.timestamp = started;

			if ((saved_is_radio && !config.submit_shoutcast) ||
					(!saved_is_radio && !config.submit_localfile)) {
				/* skip submission if we don't want it */
				debug("submission not enabled");
				goto action_submit_skip;
			}

			if (scrobbler_fail_time == 0) {
				if (scrobbler_scrobble(sbs, &sb_tinf) != 0) {
					scrobbler_fail_time = 1;
					goto action_submit_failed;
				}
			}
			else  /* write data to cache */
action_submit_failed:
				cmusfm_cache_update(&sb_tinf);
		}

action_submit_skip:
		if (raw_status == CMSTATUS_STOPPED)
			started = 0;
		else {
			/* reinitialize variables, save track info in save_data */
			started = unpaused = time(NULL);
			playtime = paused = 0;

			if ((sock_data->status & CMSTATUS_SHOUTCASTMASK) != 0)
				/* you have to listen radio min 90s (50% of 180) */
				fulltime = 180;  /* overrun DEVBYZERO in URL mode :) */
			else
				fulltime = sock_data->duration;

			/* save information for later submission purpose */
			memcpy(saved_data, sock_data, rd_len);
			saved_is_radio = sock_data->status & CMSTATUS_SHOUTCASTMASK;

			if (raw_status == CMSTATUS_PLAYING) {
action_nowplaying:
				set_trackinfo(&sb_tinf, sock_data);

#if ENABLE_LIBNOTIFY
				if (config.notification)
					cmusfm_notify_show(&sb_tinf, get_album_cover_file(
								get_sock_data_location(sock_data), config.format_coverfile));
				else
					debug("notification not enabled");
#endif

				/* update now-playing indicator */
				if (scrobbler_fail_time == 0) {
					if ((saved_is_radio && config.nowplaying_shoutcast) ||
							(!saved_is_radio && config.nowplaying_localfile)) {
						if (scrobbler_update_now_playing(sbs, &sb_tinf) != 0)
							scrobbler_fail_time = 1;
					}
					else
						debug("now playing not enabled");
				}

			}
		}
	}
	else {  /* new_hash == prev_hash */
		if (raw_status == CMSTATUS_STOPPED)
			goto action_submit;

		if (raw_status == CMSTATUS_PAUSED) {
			paused = time(NULL);
			playtime += paused - unpaused;
		}

		/* NOTE: There is no possibility to distinguish between replayed track
		 *       and unpaused. We assumed that if track was paused before, this
		 *       indicates that track is continued to play (unpaused). In other
		 *       case track is played again, so we should submit previous play. */
		if (raw_status == CMSTATUS_PLAYING) {
			if (paused) {
				unpaused = time(NULL);
				pausedtime = unpaused - paused;
				paused = 0;
				if (pausedtime > 120)
					/* If playing was paused for more then 120 seconds, reinitialize
					 * now playing notification (scrobbler and libnotify). */
					goto action_nowplaying;
			}
			else
				goto action_submit;
		}
	}
}

/* Check if server instance is running. If yes, this function returns 1,
 * otherwise 0 is returned. On error -1 is returned and errno is set
 * appropriately. */
int cmusfm_server_check(void) {

	struct sockaddr_un saddr;
	int fd;

	memset(&saddr, 0, sizeof(saddr));
	saddr.sun_family = AF_UNIX;
	strcpy(saddr.sun_path, cmusfm_socket_file);

	if ((fd = socket(PF_UNIX, SOCK_STREAM, 0)) == -1)
		return -1;

	/* check if behind the socket there is an active server instance */
	if (connect(fd, (struct sockaddr *)(&saddr), sizeof(saddr)) == 0) {
		close(fd);
		return 1;
	}

	return 0;
}

/* server shutdown stuff */
static int server_on = 1;
static void cmusfm_server_stop(int sig) {
	(void)sig;
	debug("stopping cmusfm server");
	server_on = 0;
}

/* Start server instance. This function hangs until server is stopped.
 * Upon error -1 is returned. */
int cmusfm_server_start(void) {

	scrobbler_session_t *sbs;
	struct sigaction sigact;
	struct sockaddr_un saddr;
	struct pollfd pfds[3];
#if HAVE_SYS_INOTIFY_H
	struct inotify_event inot_even;
#endif
	int retval;

	debug("starting cmusfm server");

	/* setup poll structure for data reading */
	pfds[0].events = POLLIN;  /* server */
	pfds[1].events = POLLIN;  /* client */
	pfds[2].events = POLLIN;  /* inotify */
	pfds[1].fd = -1;
	pfds[2].fd = -1;

	memset(&saddr, 0, sizeof(saddr));
	saddr.sun_family = AF_UNIX;
	strcpy(saddr.sun_path, cmusfm_socket_file);
	if ((pfds[0].fd = socket(PF_UNIX, SOCK_STREAM, 0)) == -1)
		return -1;

	/* initialize scrobbling library */
	sbs = scrobbler_initialize(SC_api_key, SC_secret);
	scrobbler_set_session_key_str(sbs, config.session_key);

#if ENABLE_LIBNOTIFY
	/* initialize notification library */
	cmusfm_notify_initialize();
#endif

	/* catch signals which are used to quit server */
	memset(&sigact, 0, sizeof(sigact));
	sigact.sa_handler = cmusfm_server_stop;
	sigaction(SIGHUP, &sigact, NULL);
	sigaction(SIGTERM, &sigact, NULL);
	sigaction(SIGINT, &sigact, NULL);

	/* create server communication socket */
	unlink(saddr.sun_path);
	if (bind(pfds[0].fd, (struct sockaddr *)(&saddr), sizeof(saddr)) == -1 ||
			listen(pfds[0].fd, 2) == -1)
		goto return_failure;

#if HAVE_SYS_INOTIFY_H
	/* initialize inode notification to watch changes in the config file */
	pfds[2].fd = inotify_init();
	cmusfm_config_add_watch(pfds[2].fd);
#endif

	debug("entering server main loop");
	while (server_on) {

		if (poll(pfds, 3, -1) == -1)
			break;  /* signal interruption */

		if (pfds[0].revents & POLLIN) {
			pfds[1].fd = accept(pfds[0].fd, NULL, NULL);
			debug("new client accepted: %d", pfds[1].fd);
		}

		if (pfds[1].revents & POLLIN && pfds[1].fd != -1) {
			cmusfm_server_process_data(pfds[1].fd, sbs);
			close(pfds[1].fd);
			pfds[1].fd = -1;
		}

#if HAVE_SYS_INOTIFY_H
		if (pfds[2].revents & POLLIN) {
			/* We're watching only one file, so the result if of no importance
			 * to us, simply read out the inotify file descriptor. */
			read(pfds[2].fd, &inot_even, sizeof(inot_even));
			debug("inotify event occurred: %x", inot_even.mask);
			cmusfm_config_read(cmusfm_config_file, &config);
			cmusfm_config_add_watch(pfds[2].fd);
		}
#endif
	}

	retval = 0;
	goto return_success;

return_failure:
	retval = -1;

return_success:

	close(pfds[0].fd);
#if HAVE_SYS_INOTIFY_H
	close(pfds[2].fd);
#endif
#if ENABLE_LIBNOTIFY
	cmusfm_notify_free();
#endif
	scrobbler_free(sbs);
	unlink(saddr.sun_path);

	return retval;
}

/* Send track info to server instance. */
int cmusfm_server_send_track(struct cmtrack_info *tinfo) {

	char buffer[CMSOCKET_BUFFER_SIZE];
	struct sock_data_tag *sock_data = (struct sock_data_tag *)buffer;
	char *artist = (char *)(sock_data + 1);
	char *album = (char *)(sock_data + 1);
	char *title = (char *)(sock_data + 1);
	char *location = (char *)(sock_data + 1);
	struct format_match *match, *matches;
	struct sockaddr_un saddr;
	int sock;

	debug("sending track to cmusfm server");

	memset(buffer, 0, sizeof(buffer));

	/* load data into the sock container */
	sock_data->status = tinfo->status;
	sock_data->tracknb = tinfo->tracknb;
	/* if no duration time assume 3 min */
	sock_data->duration = tinfo->duration == 0 ? 180 : tinfo->duration;

	/* add Shoutcast (stream) flag */
	if (tinfo->url != NULL)
		sock_data->status |= CMSTATUS_SHOUTCASTMASK;

	if ((tinfo->url != NULL && tinfo->artist == NULL && tinfo->title != NULL) ||
			(tinfo->file != NULL && tinfo->artist == NULL && tinfo->title == NULL)) {
		debug("regular expression matching mode");

		if (tinfo->url != NULL) {
			/* URL: try to fetch artist and track tile form the 'title' field */

			matches = get_regexp_format_matches(tinfo->title, config.format_shoutcast);
			if (matches == NULL) {
				fprintf(stderr, "error: shoutcast format match failed\n");
				return -1;
			}
		}
		else {
			/* FILE: try to fetch artist and track title from the 'file' field */

			tinfo->file = basename(tinfo->file);
			matches = get_regexp_format_matches(tinfo->file, config.format_localfile);
			if (matches == NULL) {
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

		if (tinfo->artist != NULL) strcpy(artist, tinfo->artist);
		album = &artist[strlen(artist) + 1];
		if (tinfo->album != NULL) strcpy(album, tinfo->album);
		title = &album[strlen(album) + 1];
		if (tinfo->title != NULL) strcpy(title, tinfo->title);

	}

	/* update track location (localfile or shoutcast) */
	location = &title[strlen(title) + 1];
	if (tinfo->file != NULL)
		strcpy(location, tinfo->file);
	else if (tinfo->url != NULL)
		strcpy(location, tinfo->url);

	/* calculate data offsets */
	sock_data->alboff = album - artist;
	sock_data->titoff = title - artist;
	sock_data->locoff = location - artist;

	/* connect to the communication socket */
	memset(&saddr, 0, sizeof(saddr));
	strcpy(saddr.sun_path, cmusfm_socket_file);
	saddr.sun_family = AF_UNIX;
	sock = socket(PF_UNIX, SOCK_STREAM, 0);
	if (connect(sock, (struct sockaddr *)(&saddr), sizeof(saddr)) == -1) {
		close(sock);
		return -1;
	}

	debug("socket wrlen: %ld", sizeof(struct sock_data_tag) +
			sock_data->titoff + sock_data->locoff + strlen(location) + 1);
	write(sock, buffer, sizeof(struct sock_data_tag) +
			sock_data->titoff + sock_data->locoff + strlen(location) + 1);
	return close(sock);
}

/* Helper function for retrieving cmusfm server socket file. */
char *get_cmusfm_socket_file(void) {
	return get_cmus_home_file(SOCKET_FNAME);
}
