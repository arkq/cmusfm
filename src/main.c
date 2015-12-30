/*
 * cmusfm - main.c
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

#include "cmusfm.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cache.h"
#include "config.h"
#include "debug.h"
#include "server.h"


/* Last.fm API key for cmusfm */
unsigned char SC_api_key[16] = {0x67, 0x08, 0x2e, 0x45, 0xda, 0xb1,
		0xf6, 0x43, 0x3d, 0xa7, 0x2a, 0x00, 0xe3, 0xbc, 0x03, 0x7a};
unsigned char SC_secret[16] = {0x02, 0xfc, 0xbc, 0x90, 0x34, 0x1a,
		0x01, 0xf2, 0x1c, 0x3b, 0xfc, 0x05, 0xb6, 0x36, 0xe3, 0xae};

/* Global cmusfm file location variables */
const char *cmusfm_cache_file = NULL;
const char *cmusfm_config_file = NULL;
const char *cmusfm_socket_file = NULL;

/* Global configuration structure */
struct cmusfm_config config;


/* Parse status arguments which we've get from the cmus and return track info
 * structure. Upon error NULL is returned and errno is set appropriately. */
static struct cmtrack_info *get_track_info(int argc, char *argv[]) {

	struct cmtrack_info *tinfo;
	int i;

	if ((tinfo = calloc(1, sizeof(*tinfo))) == NULL)
		return NULL;

	tinfo->status = CMSTATUS_UNDEFINED;

	for (i = 1; i + 1 < argc; i += 2) {
		debug("cmus argv: %s %s", argv[i], argv[i + 1]);
		if (strcmp(argv[i], "status") == 0) {
			if (strcmp(argv[i + 1], "playing") == 0)
				tinfo->status = CMSTATUS_PLAYING;
			else if (strcmp(argv[i + 1], "paused") == 0)
				tinfo->status = CMSTATUS_PAUSED;
			else if (strcmp(argv[i + 1], "stopped") == 0)
				tinfo->status = CMSTATUS_STOPPED;
		}
		else if (strcmp(argv[i], "file") == 0)
			tinfo->file = argv[i + 1];
		else if (strcmp(argv[i], "url") == 0)
			tinfo->url = argv[i + 1];
		else if (strcmp(argv[i], "artist") == 0)
			tinfo->artist = argv[i + 1];
		else if (strcmp(argv[i], "album") == 0)
			tinfo->album = argv[i + 1];
		else if (strcmp(argv[i], "tracknumber") == 0)
			tinfo->tracknb = atoi(argv[i + 1]);
		else if (strcmp(argv[i], "title") == 0)
			tinfo->title = argv[i + 1];
		else if (strcmp(argv[i], "duration") == 0)
			tinfo->duration = atoi(argv[i + 1]);
	}

	/* NOTE: cmus always passes status parameter */
	if (tinfo->status == CMSTATUS_UNDEFINED) {
		free(tinfo);
		errno = EINVAL;
		return NULL;
	}

	return tinfo;
}

/* User authorization callback for the initialization process. */
static int user_authorization(const char *url) {
	printf("Open this URL in your favorite web browser and afterwards "
			"press ENTER:\n  %s\n", url);
	getchar();
	return 0;
}

/* Initialization routine. Get Last.fm session key from the scrobbler service
 * and initialize configuration file with default values (if needed). */
static void cmusfm_initialization(void) {

	scrobbler_session_t *sbs;
	struct cmusfm_config conf;
	int fetch_session_key;
	char yesno[8], *ptr;

	fetch_session_key = 1;
	sbs = scrobbler_initialize(SC_api_key, SC_secret);

	/* try to read previous configuration */
	if (cmusfm_config_read(cmusfm_config_file, &conf) == 0) {
		printf("Checking previous session (user: %s) ...", conf.user_name);
		fflush(stdout);
		scrobbler_set_session_key_str(sbs, conf.session_key);
		if (scrobbler_test_session_key(sbs) == 0)
			printf("OK.\n");
		else
			printf("failed.\n");

		printf("Fetch new session key [yes/NO]: ");
		ptr = fgets(yesno, sizeof(yesno), stdin);
		if (ptr != NULL && strncmp(ptr, "yes", 3) != 0)
			fetch_session_key = 0;
	}

	if (fetch_session_key) {  /* fetch new session key */
		if (scrobbler_authentication(sbs, user_authorization) == 0) {
			scrobbler_get_session_key_str(sbs, conf.session_key);
			strncpy(conf.user_name, sbs->user_name, sizeof(conf.user_name) - 1);
		}
		else
			printf("Error: scrobbler authentication failed\n");
	}
	scrobbler_free(sbs);

	if (cmusfm_config_write(cmusfm_config_file, &conf) != 0)
		printf("Error: unable to write file: %s\n", cmusfm_config_file);
}

/* Spawn server process of cmusfm by simply forking ourself and exec with
 * special "server" argument. */
static void cmusfm_spawn_server_process(const char *cmusfm) {

	pid_t pid;

	if ((pid = fork()) == -1) {
		perror("error: fork server");
		exit(EXIT_FAILURE);
	}

	if (pid == 0) {
		execlp(cmusfm, cmusfm, "server", NULL);
		perror("error: exec server");
		exit(EXIT_FAILURE);
	}

}

int main(int argc, char *argv[]) {

	struct cmtrack_info *tinfo;

	/* print initialization help message */
	if (argc == 1) {
		printf("usage: %s [init]\n\n"
"NOTE: Before usage with the cmus you should invoke this program with the\n"
"      `init` argument. Afterwards you can set the status_display_program\n"
"      (for more informations see `man cmus`). Enjoy!\n", argv[0]);
		return EXIT_SUCCESS;
	}

	/* setup global variables - file locations */
	cmusfm_cache_file = get_cmusfm_cache_file();
	cmusfm_config_file = get_cmusfm_config_file();
	cmusfm_socket_file = get_cmusfm_socket_file();

	if (argc == 2 && strcmp(argv[1], "init") == 0) {
		cmusfm_initialization();
		return EXIT_SUCCESS;
	}

	if (cmusfm_config_read(cmusfm_config_file, &config) == -1) {
		perror("error: config read");
		return EXIT_FAILURE;
	}

	if (argc == 2 && strcmp(argv[1], "server") == 0)
		return cmusfm_server_start();

	/* try to parse cmus status display program arguments */
	if ((tinfo = get_track_info(argc, argv)) == NULL) {
		perror("error: get track info");
		return EXIT_FAILURE;
	}

	if (cmusfm_server_check() == 0) {
		cmusfm_spawn_server_process(argv[0]);
		sleep(1);
	}

	if (cmusfm_server_send_track(tinfo)) {
		fprintf(stderr, "error: sending track to server failed\n");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
