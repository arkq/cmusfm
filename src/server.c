/*
 * server.c
 * Copyright (c) 2010-2021 Arkadiusz Bokowy
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

#if HAVE_CONFIG_H
# include "../config.h"
#endif

#include "server.h"

#include <errno.h>
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
# include <sys/inotify.h>
#endif

#include "cache.h"
#include "cmusfm.h"
#include "config.h"
#include "debug.h"
#if ENABLE_LIBNOTIFY
# include "notify.h"
#endif


/* Helper function for MB track ID retrieval. */
static char *get_record_mb_track_id(const struct cmusfm_data_record *r) {
	return (char *)(r + 1);
}

/* Helper function for artist retrieval. */
static char *get_record_artist(const struct cmusfm_data_record *r) {
	return &((char *)(r + 1))[r->off_artist];
}

/* Helper function for album artist retrieval. */
static char *get_record_album_artist(const struct cmusfm_data_record *r) {
	return &((char *)(r + 1))[r->off_album_artist];
}

/* Helper function for album name retrieval. */
static char *get_record_album(const struct cmusfm_data_record *r) {
	return &((char *)(r + 1))[r->off_album];
}

/* Helper function for track title retrieval. */
static char *get_record_title(const struct cmusfm_data_record *r) {
	return &((char *)(r + 1))[r->off_title];
}

/* Helper function for location retrieval. */
static char *get_record_location(const struct cmusfm_data_record *r) {
	return &((char *)(r + 1))[r->off_location];
}

/* Return the checksum for the length-invariant part of the record. */
static uint8_t make_record_checksum1(const struct cmusfm_data_record *r) {
	return make_data_hash((unsigned char *)&r->status,
			sizeof(*r) - ((char *)&r->status - (char *)r));
}

/* Return the data checksum of the given record structure. This checksum
 * does not include status field, so it can be used for data comparison. */
static uint8_t make_record_checksum2(const struct cmusfm_data_record *r) {
	return make_data_hash((unsigned char *)&r->disc_number,
			sizeof(*r) - ((char *)&r->disc_number - (char *)r) +
			r->off_location + strlen(get_record_location(r)));
}

/* Copy data from the message into the scrobbler structure. */
static void set_trackinfo(scrobbler_trackinfo_t *sbt,
		const struct cmusfm_data_record *record) {

	char *tmp;

	tmp = get_record_mb_track_id(record);
	sbt->mb_track_id = tmp[0] != '\0' ? tmp : NULL;
	tmp = get_record_artist(record);
	sbt->artist = tmp[0] != '\0' ? tmp : NULL;
	tmp = get_record_album_artist(record);
	sbt->album_artist = tmp[0] != '\0' ? tmp : NULL;
	tmp = get_record_album(record);
	sbt->album = tmp[0] != '\0' ? tmp : NULL;
	sbt->track_number = record->track_number;
	tmp = get_record_title(record);
	sbt->track = tmp[0] != '\0' ? tmp : NULL;
	sbt->duration = record->duration;
	sbt->timestamp = 0;

}

/* Process real server task - Last.fm submission. */
static void cmusfm_server_process_data(scrobbler_session_t *sbs,
		const struct cmusfm_data_record *record) {

	static char saved_data[CMSOCKET_BUFFER_SIZE];
	static char saved_is_radio = 0;
	struct cmusfm_data_record *saved_record = (struct cmusfm_data_record *)saved_data;

	/* scrobbler stuff */
	static time_t scrobbler_fail_time = 1;
	static time_t started = 0, paused = 0, unpaused = 0;
	static time_t playtime = 0, fulltime = 10;
	scrobbler_trackinfo_t sb_tinf;
	unsigned char status;
	time_t pausedtime;
	int checksum2;

	/* check for data integrity */
	if (make_record_checksum1(record) != record->checksum1 ||
			(checksum2 = make_record_checksum2(record)) != record->checksum2)
		return;

	debug("Payload: %s - %s - %d. %s (%ds)",
			get_record_artist(record), get_record_album(record),
			record->track_number, get_record_title(record),
			record->duration);
	debug("Location: %s", get_record_location(record));

#if DEBUG && !defined(DEBUG_SKIP_HICCUP)
	/* simulate server "hiccup" (e.g. internet connection issue) */
	debug("Server hiccup test (5s)");
	sleep(5);
#endif

	status = record->status & ~CMSTATUS_SHOUTCASTMASK;

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

	/* User is playing a new track or the status has changed for the previous
	 * one. In both cases we should check if the track should be submitted. */
	if (checksum2 != saved_record->checksum2) {
action_submit:
		playtime += time(NULL) - unpaused;

		/* Track should be submitted if it is longer than 30 seconds and it has
		 * been played for at least half its duration (play time is greater than
		 * 15 seconds or 50% of the track duration respectively). Also the track
		 * should be submitted if the play time is greater than 4 minutes. */
		if (started != 0 && (playtime > fulltime - playtime || playtime > 240)) {

			/* playing duration is OK so submit track */
			set_trackinfo(&sb_tinf, saved_record);
			sb_tinf.timestamp = started;

			if (sb_tinf.duration <= 30)
				goto action_submit_skip;

			if ((saved_is_radio && !config.submit_shoutcast) ||
					(!saved_is_radio && !config.submit_localfile)) {
				/* skip submission if we don't want it */
				debug("Submission not enabled");
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
		if (status == CMSTATUS_STOPPED)
			started = 0;
		else {
			/* reinitialize variables, save track info in save_data */
			started = unpaused = time(NULL);
			playtime = paused = 0;

			if ((record->status & CMSTATUS_SHOUTCASTMASK) != 0)
				/* you have to listen radio min 90s (50% of 180) */
				fulltime = 180;  /* overrun DEVBYZERO in URL mode :) */
			else
				fulltime = record->duration;

			/* save information for later submission purpose */
			memcpy(saved_data, record, sizeof(saved_data));
			saved_is_radio = record->status & CMSTATUS_SHOUTCASTMASK;

			if (status == CMSTATUS_PLAYING) {
action_nowplaying:
				set_trackinfo(&sb_tinf, record);

#if ENABLE_LIBNOTIFY
				if (config.notification)
					cmusfm_notify_show(&sb_tinf, get_album_cover_file(
								get_record_location(record), config.format_coverfile));
				else
					debug("Notification not enabled");
#endif

				/* update now-playing indicator */
				if (scrobbler_fail_time == 0) {
					if ((saved_is_radio && config.nowplaying_shoutcast) ||
							(!saved_is_radio && config.nowplaying_localfile)) {
						if (scrobbler_update_now_playing(sbs, &sb_tinf) != 0)
							scrobbler_fail_time = 1;
					}
					else
						debug("Now playing not enabled");
				}

			}
		}
	}
	else {  /* old checksum2 == new checksum2 */
		if (status == CMSTATUS_STOPPED)
			goto action_submit;

		if (status == CMSTATUS_PAUSED) {
			paused = time(NULL);
			playtime += paused - unpaused;
		}

		/* NOTE: There is no possibility to distinguish between replayed track
		 *       and unpaused. We assumed that if track was paused before, this
		 *       indicates that track is continued to play (unpaused). In other
		 *       case track is played again, so we should submit previous play. */
		if (status == CMSTATUS_PLAYING) {
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

	struct sockaddr_un saddr = { .sun_family = AF_UNIX };
	int fd;

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
static bool server_on = true;
static void cmusfm_server_stop(int sig) {
	(void)sig;
	debug("Stopping server");
	server_on = false;
}

/* Start server instance. This function hangs until server is stopped.
 * Upon error -1 is returned. */
int cmusfm_server_start(void) {

	char buffer[CMSOCKET_BUFFER_SIZE];
	scrobbler_session_t *sbs;
#if HAVE_SYS_INOTIFY_H
	struct inotify_event inot_even;
#endif
	size_t rd_len;
	int retval;

	debug("Starting server");

	/* setup poll structure for data reading */
	struct pollfd pfds[3] = {
		{ -1, POLLIN, 0 },  /* server */
		{ -1, POLLIN, 0 },  /* client */
		{ -1, POLLIN, 0 },  /* inotify */
	};

	struct sockaddr_un saddr = { .sun_family = AF_UNIX };
	strcpy(saddr.sun_path, cmusfm_socket_file);

	if ((pfds[0].fd = socket(PF_UNIX, SOCK_STREAM, 0)) == -1)
		return -1;

	/* initialize scrobbling library */
	sbs = scrobbler_initialize(config.service_api_url,
			config.service_auth_url, SC_api_key, SC_secret);
	scrobbler_set_session_key(sbs, config.session_key);

	/* catch signals which are used to quit server */
	struct sigaction sigact = { .sa_handler = cmusfm_server_stop };
	sigaction(SIGTERM, &sigact, NULL);
	sigaction(SIGHUP, &sigact, NULL);
	sigaction(SIGINT, &sigact, NULL);

	/* create server communication socket */
	unlink(saddr.sun_path);
	if (bind(pfds[0].fd, (struct sockaddr *)(&saddr), sizeof(saddr)) == -1)
		goto fail;
	if (listen(pfds[0].fd, 2) == -1)
		goto fail;

#if HAVE_SYS_INOTIFY_H
	/* initialize inode notification to watch changes in the config file */
	pfds[2].fd = inotify_init();
	cmusfm_config_add_watch(pfds[2].fd);
#endif

	debug("Entering server main loop");
	while (server_on) {

		if (poll(pfds, 3, -1) == -1)
			break;  /* signal interruption */

		if (pfds[0].revents & POLLIN) {
			pfds[1].fd = accept(pfds[0].fd, NULL, NULL);
			debug("New client accepted: %d", pfds[1].fd);
		}

		if (pfds[1].revents & POLLIN && pfds[1].fd != -1) {
			rd_len = read(pfds[1].fd, buffer, sizeof(buffer));
			close(pfds[1].fd);
			pfds[1].fd = -1;
			if (rd_len >= sizeof(struct cmusfm_data_record))
				cmusfm_server_process_data(sbs, (struct cmusfm_data_record *)buffer);
		}

#if HAVE_SYS_INOTIFY_H
		if (pfds[2].revents & POLLIN) {
			/* We're watching only one file, so the result is of no importance
			 * to us, simply read out the inotify file descriptor. */
			read(pfds[2].fd, &inot_even, sizeof(inot_even));
			debug("Inotify event occurred: %x", inot_even.mask);
			cmusfm_config_read(cmusfm_config_file, &config);
			cmusfm_config_add_watch(pfds[2].fd);
		}
#endif
	}

	retval = 0;
	goto final;

fail:
	retval = -1;

final:

#if HAVE_SYS_INOTIFY_H
	close(pfds[2].fd);
#endif
#if ENABLE_LIBNOTIFY
	cmusfm_notify_free();
#endif
	scrobbler_free(sbs);
	close(pfds[0].fd);
	unlink(saddr.sun_path);

	return retval;
}

/* Send track info to server instance. */
int cmusfm_server_send_track(struct cmtrack_info *tinfo) {

	char buffer[CMSOCKET_BUFFER_SIZE] = { 0 };
	struct cmusfm_data_record *record = (struct cmusfm_data_record *)buffer;
	struct format_match *match, *matches;
	int err, sock;

	/* helper accessors for dynamic fields */
	char *mb_track_id = &buffer[sizeof(*record)];
	char *artist, *album_artist, *album, *title, *location;
	size_t size = sizeof(buffer) - (mb_track_id - buffer) - 6;

	debug("Sending track to server");

	record->status = tinfo->status;
	record->disc_number = tinfo->disc_number;
	record->track_number = tinfo->track_number;
	/* if no duration time assume 3 min */
	record->duration = tinfo->duration == 0 ? 180 : tinfo->duration;

	if (tinfo->mb_track_id != NULL)
		strncpy(mb_track_id, tinfo->mb_track_id, size);
	artist = &mb_track_id[strlen(mb_track_id) + 1];
	size -= artist - mb_track_id - 1;

	/* add Shoutcast (stream) flag */
	if (tinfo->url != NULL)
		record->status |= CMSTATUS_SHOUTCASTMASK;

	/* use album artist as a fall-back if artist is missing */
	if (tinfo->artist == NULL && tinfo->album_artist != NULL)
		tinfo->artist = tinfo->album_artist;

	if ((tinfo->url != NULL && tinfo->artist == NULL && tinfo->title != NULL) ||
			(tinfo->file != NULL && !(tinfo->artist != NULL && tinfo->title != NULL))) {
		debug("Regular expression matching mode");

		if (tinfo->url != NULL) {
			/* URL: try to fetch artist and track tile form the 'title' field */

			matches = get_regexp_format_matches(tinfo->title, config.format_shoutcast);
			if (matches == NULL) {
				fprintf(stderr, "INFO: Title does not match format-shoutcast\n");
				return 0;
			}
		}
		else {
			/* FILE: try to fetch artist and track title from the 'file' field */

			tinfo->file = basename(tinfo->file);
			matches = get_regexp_format_matches(tinfo->file, config.format_localfile);
			if (matches == NULL) {
				fprintf(stderr, "INFO: File name does not match format-localfile\n");
				return 0;
			}
		}

		match = get_regexp_match(matches, CMFORMAT_ARTIST);
		strncpy(artist, match->data, size < match->len ? size : match->len);
		album_artist = &artist[strlen(artist) + 1];
		album = &album_artist[strlen(album_artist) + 1];
		size -= album - artist - 2;

		match = get_regexp_match(matches, CMFORMAT_ALBUM);
		strncpy(album, match->data, size < match->len ? size : match->len);
		title = &album[strlen(album) + 1];
		size -= title - album - 1;

		match = get_regexp_match(matches, CMFORMAT_TITLE);
		strncpy(title, match->data, size < match->len ? size : match->len);

		free(matches);

	}
	else {

		if (tinfo->artist != NULL)
			strncpy(artist, tinfo->artist, size);
		album_artist = &artist[strlen(artist) + 1];
		size -= album_artist - artist - 1;

		if (tinfo->album_artist != NULL)
			strncpy(album_artist, tinfo->album_artist, size);
		album = &album_artist[strlen(album_artist) + 1];
		size -= album - album_artist - 1;

		if (tinfo->album != NULL)
			strncpy(album, tinfo->album, size);
		title = &album[strlen(album) + 1];
		size -= title - album - 1;

		if (tinfo->title != NULL)
			strncpy(title, tinfo->title, size);

	}

	/* update track location (localfile or shoutcast) */
	location = &title[strlen(title) + 1];
	size -= location - title - 1;
	if (tinfo->file != NULL)
		strncpy(location, tinfo->file, size);
	else if (tinfo->url != NULL)
		strncpy(location, tinfo->url, size);

	/* calculate data offsets */
	record->off_artist = artist - mb_track_id;
	record->off_album_artist = album_artist - mb_track_id;
	record->off_album = album - mb_track_id;
	record->off_title = title - mb_track_id;
	record->off_location = location - mb_track_id;

	/* calculate checksums - used for data integrity check */
	record->checksum1 = make_record_checksum1(record);
	record->checksum2 = make_record_checksum2(record);

	/* connect to the communication socket */
	struct sockaddr_un saddr = { .sun_family = AF_UNIX };
	strcpy(saddr.sun_path, cmusfm_socket_file);
	if ((sock = socket(PF_UNIX, SOCK_STREAM, 0)) == -1)
		goto fail;
	if (connect(sock, (struct sockaddr *)(&saddr), sizeof(saddr)) == -1)
		goto fail;

	ssize_t len = sizeof(struct cmusfm_data_record) +
		record->off_location + strlen(location) + 1;
	debug("Record length: %zd", len);
	if (write(sock, buffer, len) != len)
		goto fail;

	return close(sock);

fail:
	err = errno;
	close(sock);
	errno = err;
	return -1;
}

/* Helper function for retrieving server socket file. */
char *get_cmusfm_socket_file(void) {
	return get_cmus_home_file(SOCKET_FNAME);
}
