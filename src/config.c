/*
 * config.c
 * Copyright (c) 2014-2017 Arkadiusz Bokowy
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

#include "config.h"

#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#if HAVE_SYS_INOTIFY_H
# include <sys/inotify.h>
#endif

#include "cmusfm.h"


/* Return the pointer to the configuration value substring. This function
 * strips all white-spaces and optional quotation marks. */
static char *get_config_value(char *str) {

	char *end;

	/* seek to the beginning of a value */
	if ((str = strchr(str, '=')) == NULL)
		return "";

	str += 1;

	/* trim leading spaces and optional quotation */
	while (isspace(*str)) str++;
	if (*str == '"') str++;

	/* edge case handling */
	if (*str == 0)
		return str;

	/* trim trailing spaces and optional quotation */
	end = str + strlen(str) - 1;
	while (end > str && isspace(*end)) end--;
	if (*end == '"') end--;
	*(end + 1) = 0;

	return str;
}

static char *encode_config_bool(bool value) {
	return value ? "yes" : "no";
}

static bool decode_config_bool(const char *value) {
	return strcmp(value, "yes") == 0;
}

/* Read cmusfm configuration from the file. */
int cmusfm_config_read(const char *fname, struct cmusfm_config *conf) {

	FILE *f;
	char line[128];

	/* initialize configuration defaults */
	memset(conf, 0, sizeof(*conf));
	strcpy(conf->service_api_url, "https://ws.audioscrobbler.com/2.0/");
	strcpy(conf->service_auth_url, "https://www.last.fm/api/auth");
	strcpy(conf->format_localfile, "^(?A.+) - (?T.+)\\.[^.]+$");
	strcpy(conf->format_shoutcast, "^(?A.+) - (?T.+)$");
#if ENABLE_LIBNOTIFY
	/* set the MS Windows name convention as a default - compatible with most
	 * sailors from the pirate bay */
	strcpy(conf->format_coverfile, "^folder\\.jpg$");
#endif
	conf->nowplaying_localfile = true;
	conf->nowplaying_shoutcast = true;
	conf->submit_localfile = true;
	conf->submit_shoutcast = true;

	if ((f = fopen(fname, "r")) == NULL)
		return -1;

	while (fgets(line, sizeof(line), f)) {
		if (strncmp(line, CMCONF_USER_NAME, sizeof(CMCONF_USER_NAME) - 1) == 0)
			strncpy(conf->user_name, get_config_value(line), sizeof(conf->user_name) - 1);
		else if (strncmp(line, CMCONF_SESSION_KEY, sizeof(CMCONF_SESSION_KEY) - 1) == 0)
			strncpy(conf->session_key, get_config_value(line), sizeof(conf->session_key) - 1);
		else if (strncmp(line, CMCONF_FORMAT_LOCALFILE, sizeof(CMCONF_FORMAT_LOCALFILE) - 1) == 0)
			strncpy(conf->format_localfile, get_config_value(line), sizeof(conf->format_localfile) - 1);
		else if (strncmp(line, CMCONF_FORMAT_SHOUTCAST, sizeof(CMCONF_FORMAT_SHOUTCAST) - 1) == 0)
			strncpy(conf->format_shoutcast, get_config_value(line), sizeof(conf->format_shoutcast) - 1);
		else if (strncmp(line, CMCONF_NOWPLAYING_LOCALFILE, sizeof(CMCONF_NOWPLAYING_LOCALFILE) - 1) == 0)
			conf->nowplaying_localfile = decode_config_bool(get_config_value(line));
		else if (strncmp(line, CMCONF_NOWPLAYING_SHOUTCAST, sizeof(CMCONF_NOWPLAYING_SHOUTCAST) - 1) == 0)
			conf->nowplaying_shoutcast = decode_config_bool(get_config_value(line));
		else if (strncmp(line, CMCONF_SUBMIT_LOCALFILE, sizeof(CMCONF_SUBMIT_LOCALFILE) - 1) == 0)
			conf->submit_localfile = decode_config_bool(get_config_value(line));
		else if (strncmp(line, CMCONF_SUBMIT_SHOUTCAST, sizeof(CMCONF_SUBMIT_SHOUTCAST) - 1) == 0)
			conf->submit_shoutcast = decode_config_bool(get_config_value(line));
#if ENABLE_LIBNOTIFY
		else if (strncmp(line, CMCONF_FORMAT_COVERFILE, sizeof(CMCONF_FORMAT_COVERFILE) - 1) == 0)
			strncpy(conf->format_coverfile, get_config_value(line), sizeof(conf->format_coverfile) - 1);
		else if (strncmp(line, CMCONF_NOTIFICATION, sizeof(CMCONF_NOTIFICATION) - 1) == 0)
			conf->notification = decode_config_bool(get_config_value(line));
#endif
		else if (strncmp(line, CMCONF_SERVICE_API_URL, sizeof(CMCONF_SERVICE_API_URL) - 1) == 0)
			strncpy(conf->service_api_url, get_config_value(line), sizeof(conf->service_api_url) - 1);
		else if (strncmp(line, CMCONF_SERVICE_AUTH_URL, sizeof(CMCONF_SERVICE_AUTH_URL) - 1) == 0)
			strncpy(conf->service_auth_url, get_config_value(line), sizeof(conf->service_auth_url) - 1);
	}

	return fclose(f);
}

/* Write cmusfm configuration to the file. */
int cmusfm_config_write(const char *fname, struct cmusfm_config *conf) {

	FILE *f;

	/* create configuration file (truncate previous one) */
	if ((f = fopen(fname, "w")) == NULL)
		return -1;

	/* protect session key from exposure */
	chmod(fname, S_IWUSR | S_IRUSR);

	fprintf(f, "# authentication\n");
	fprintf(f, "%s = \"%s\"\n", CMCONF_USER_NAME, conf->user_name);
	fprintf(f, "%s = \"%s\"\n", CMCONF_SESSION_KEY, conf->session_key);

	fprintf(f, "\n# regular expressions for name parsers\n");
	fprintf(f, "%s = \"%s\"\n", CMCONF_FORMAT_LOCALFILE, conf->format_localfile);
	fprintf(f, "%s = \"%s\"\n", CMCONF_FORMAT_SHOUTCAST, conf->format_shoutcast);
#if ENABLE_LIBNOTIFY
	fprintf(f, "%s = \"%s\"\n", CMCONF_FORMAT_COVERFILE, conf->format_coverfile);
#endif

	fprintf(f, "\n");
	fprintf(f, "%s = \"%s\"\n", CMCONF_NOWPLAYING_LOCALFILE, encode_config_bool(conf->nowplaying_localfile));
	fprintf(f, "%s = \"%s\"\n", CMCONF_NOWPLAYING_SHOUTCAST, encode_config_bool(conf->nowplaying_shoutcast));
	fprintf(f, "%s = \"%s\"\n", CMCONF_SUBMIT_LOCALFILE, encode_config_bool(conf->submit_localfile));
	fprintf(f, "%s = \"%s\"\n", CMCONF_SUBMIT_SHOUTCAST, encode_config_bool(conf->submit_shoutcast));
#if ENABLE_LIBNOTIFY
	fprintf(f, "%s = \"%s\"\n", CMCONF_NOTIFICATION, encode_config_bool(conf->notification));
#endif

	fprintf(f, "\n# scrobbling service\n");
	fprintf(f, "%s = \"%s\"\n", CMCONF_SERVICE_API_URL, conf->service_api_url);
	fprintf(f, "%s = \"%s\"\n", CMCONF_SERVICE_AUTH_URL, conf->service_auth_url);

	return fclose(f);
}

#if HAVE_SYS_INOTIFY_H
/* Add the cmusfm configuration file into the inotify watch stack. File
 * is watched for modification, and after event is triggered it will be
 * automatically removed from the stack. */
int cmusfm_config_add_watch(int fd) {
	return inotify_add_watch(fd, cmusfm_config_file, IN_MODIFY | IN_ONESHOT);
}
#endif

/* Helper function for retrieving cmusfm configuration file. */
char *get_cmusfm_config_file(void) {
	return get_cmus_home_file(CONFIG_FNAME);
}
