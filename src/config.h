/*
 * cmusfm - config.h
 * Copyright (c) 2014 Arkadiusz Bokowy
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

#ifndef __CMUSFM_CONFIG_H
#define __CMUSFM_CONFIG_H

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif


// Configuration file key definitions
#define CMCONF_USER_NAME "user"
#define CMCONF_SESSION_KEY "key"
#define CMCONF_FORMAT_LOCALFILE "format-localfile"
#define CMCONF_FORMAT_SHOUTCAST "format-shoutcast"
#define CMCONF_NOWPLAYING_LOCALFILE "now-playing-localfile"
#define CMCONF_NOWPLAYING_SHOUTCAST "now-playing-shoutcast"
#define CMCONF_SUBMIT_LOCALFILE "submit-localfile"
#define CMCONF_SUBMIT_SHOUTCAST "submit-shoutcast"
#define CMCONF_NOTIFICATION "notification"

struct cmusfm_config {
	char user_name[64];
	char session_key[16 * 2 + 1];

	// regular expressions for name parsers
	char format_localfile[64];
	char format_shoutcast[64];

	unsigned int nowplaying_localfile : 1;
	unsigned int nowplaying_shoutcast : 1;
	unsigned int submit_localfile : 1;
	unsigned int submit_shoutcast : 1;
#ifdef ENABLE_LIBNOTIFY
	unsigned int notification : 1;
#endif
};


char *get_cmusfm_config_file();
int cmusfm_config_read(const char *fname, struct cmusfm_config *conf);
int cmusfm_config_write(const char *fname, struct cmusfm_config *conf);
#if HAVE_SYS_INOTIFY_H
int cmusfm_config_add_watch(int fd);
#endif

#endif
