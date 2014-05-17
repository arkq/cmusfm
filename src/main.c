/*
 * cmusfm - main.c
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

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <regex.h>
#include "cmusfm.h"
#include "config.h"
#include "server.h"
#include "debug.h"


// Last.fm API key for cmusfm
unsigned char SC_api_key[16] = {0x67, 0x08, 0x2e, 0x45, 0xda, 0xb1,
		0xf6, 0x43, 0x3d, 0xa7, 0x2a, 0x00, 0xe3, 0xbc, 0x03, 0x7a};
unsigned char SC_secret[16] = {0x02, 0xfc, 0xbc, 0x90, 0x34, 0x1a,
		0x01, 0xf2, 0x1c, 0x3b, 0xfc, 0x05, 0xb6, 0x36, 0xe3, 0xae};

// Global configuration structure
struct cmusfm_config config;


// Helper function for retrieving cmus configuration home path.
char *get_cmus_home_dir()
{
	static char fname[128];
	sprintf(fname, "%s/.cmus", getenv("HOME"));
	return fname;
}

// Get track information substrings from the given string. Matching is
// done according to the provided format, which is a ERE pattern with
// customized placeholders. Placeholder is defined as a marked
// subexpression with the `?X` marker, where X can be one the following
// characters: A - artist, B - album, T - title, N - track number
// E.g.: ^(?A.+) - (?N[:digits:]+)\. (?T.+)$
// In order to get a single match structure, one should use `get_regexp_match`
// function. When matches are not longer needed, is should be freed by the
// standard `free` function. When something goes wrong, NULL is returned.
struct format_match *get_regexp_format_matches(const char *str, const char *format)
{
#define MATCHES_SIZE FORMAT_MATCH_TYPE_COUNT + 1
	struct format_match *matches;
	const char *p = format;
	char *regexp;
	int status, i = 0;
	regex_t regex;
	regmatch_t regmatch[MATCHES_SIZE];

	debug("matching: %s: %s", format, str);

	// allocate memory for up to FORMAT_MATCH_TYPE_COUNT matches
	// with one extra always empty terminating structure
	matches = (struct format_match*)calloc(MATCHES_SIZE, sizeof(*matches));

	regexp = strdup(format);
	while(++i < MATCHES_SIZE && (p = strstr(p, "(?"))) {
		p += 3;
		matches[i - 1].type = p[-1];
		strcpy(&regexp[p - format - i * 2], p);
	}

	debug("regexp: %s", regexp);

	status = regcomp(&regex, regexp, REG_EXTENDED | REG_ICASE);
	free(regexp);
	if(status) {
		free(matches);
		return NULL;
	}

	status = regexec(&regex, str, MATCHES_SIZE, regmatch, 0);
	regfree(&regex);
	if(status) {
		free(matches);
		return NULL;
	}

	for(i = 1; i < MATCHES_SIZE; i++) {
		matches[i - 1].data = &str[regmatch[i].rm_so];
		matches[i - 1].len = regmatch[i].rm_eo - regmatch[i].rm_so;
	}
	return matches;
}

// Return pointer to the single format match structure with given match type.
struct format_match *get_regexp_match(struct format_match *matches, enum format_match_type type) {
	while(matches->type) {
		if(matches->type == type)
			break;
		matches++;
	}
	return matches;
}

// Parse arguments which we've get from the cmus.
int parse_argv(struct cmtrack_info *tinfo, int argc, char *argv[])
{
	int i;

	memset(tinfo, 0, sizeof(*tinfo));
	for(i = 1; i + 1 < argc; i += 2) {
		debug("cmus argv: %s %s", argv[i], argv[i + 1]);
		if(strcmp(argv[i], "status") == 0) {
			if(strcmp(argv[i + 1], "playing") == 0)
				tinfo->status = CMSTATUS_PLAYING;
			else if(strcmp(argv[i + 1], "paused") == 0)
				tinfo->status = CMSTATUS_PAUSED;
			else if(strcmp(argv[i + 1], "stopped") == 0)
				tinfo->status = CMSTATUS_STOPPED;
		}
		else if(strcmp(argv[i], "file") == 0)
			tinfo->file = argv[i + 1];
		else if(strcmp(argv[i], "url") == 0)
			tinfo->url = argv[i + 1];
		else if(strcmp(argv[i], "artist") == 0)
			tinfo->artist = argv[i + 1];
		else if(strcmp(argv[i], "album") == 0)
			tinfo->album = argv[i + 1];
		else if(strcmp(argv[i], "tracknumber") == 0)
			tinfo->tracknb = atoi(argv[i + 1]);
		else if(strcmp(argv[i], "title") == 0)
			tinfo->title = argv[i + 1];
		else if(strcmp(argv[i], "duration") == 0)
			tinfo->duration = atoi(argv[i + 1]);
	}

	// NOTE: cmus always passes status parameter
	if(tinfo->status == CMSTATUS_UNDEFINED)
		return -1;

	// check for required fields
	if(tinfo->file != NULL || tinfo->url != NULL)
		return 0;

	// cmus initialization call
	return 1;
}

// User authorization callback for the initialization process.
int user_authorization(const char *url) {
	printf("Open this URL in your favorite web browser and afterwards "
			"press ENTER:\n  %s\n", url);
	getchar();
	return 0;
}

// Initialization routine. Get Last.fm session key from the scrobbler service
// and initialize configuration file with default values (if needed).
int cmusfm_initialization()
{
	scrobbler_session_t *sbs;
	struct cmusfm_config conf;
	int fetch_session_key;
	char *conf_fname;
	char yesno[8];

	fetch_session_key = 1;
	conf_fname = get_cmusfm_config_file();
	sbs = scrobbler_initialize(SC_api_key, SC_secret);

	// try to read previous configuration
	if(cmusfm_config_read(conf_fname, &conf) == 0) {
		printf("Checking previous session (user: %s) ...", conf.user_name);
		fflush(stdout);
		scrobbler_set_session_key_str(sbs, conf.session_key);
		if(scrobbler_test_session_key(sbs) == 0) printf("OK.\n");
		else printf("failed.\n");

		printf("Fetch new session key [yes/NO]: ");
		fgets(yesno, sizeof(yesno), stdin);
		if(strncmp(yesno, "yes", 3) != 0)
			fetch_session_key = 0;
	}
	else {  // initialize configuration defaults
		strcpy(conf.format_localfile, "^(?A.+) - (?T.+)\\.[^.]+$");
		strcpy(conf.format_shoutcast, "^(?A.+) - (?T.+)$");
		conf.nowplaying_localfile = 1;
		conf.nowplaying_shoutcast = 1;
		conf.submit_localfile = 1;
		conf.submit_shoutcast = 1;
	}

	if(fetch_session_key) {  // fetch new session key
		if(scrobbler_authentication(sbs, user_authorization) == 0) {
			scrobbler_get_session_key_str(sbs, conf.session_key);
			strncpy(conf.user_name, sbs->user_name, sizeof(conf.user_name) - 1);
		}
		else {  // if user_name and/or session_key is NULL -> segfault :)
			strcpy(conf.user_name, "(none)");
			strcpy(conf.session_key, "xxx");
			printf("Error: scrobbler authentication failed\n\n");
		}
	}
	scrobbler_free(sbs);

	if(cmusfm_config_write(conf_fname, &conf) != 0)
		printf("Error: unable to write configuration file\n");

	return 0;
}

int main(int argc, char *argv[])
{
	struct cmtrack_info tinfo;

	if(argc == 1) {  // print initialization help message
		printf("usage: cmusfm [init]\n\n"
"NOTE: Before usage with the cmus you should invoke this program with the\n"
"      `init` argument. Afterwards you can set the status_display_program\n"
"      (for more informations see `man cmus`). Enjoy!\n");
		return EXIT_SUCCESS;
	}

	if(argc == 2 && strcmp(argv[1], "init") == 0)
		return cmusfm_initialization();

	if(cmusfm_config_read(get_cmusfm_config_file(), &config) == -1) {
		perror("error: unable to read config file");
		return EXIT_FAILURE;
	}

	switch(parse_argv(&tinfo, argc, argv)) {
	case -1:
		fprintf(stderr, "error: arguments parsing failed\n");
		return EXIT_FAILURE;
	case 1:
		cmusfm_server_start();
		return EXIT_SUCCESS;
	}

	if(cmusfm_server_send_track(&tinfo)) {
		fprintf(stderr, "error: sending track to server failed\n");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
