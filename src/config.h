/*
 * cmusfm - config.h
 * SPDX-FileCopyrightText: 2014-2024 Arkadiusz Bokowy and contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CMUSFM_CONFIG_H_
#define CMUSFM_CONFIG_H_

#if HAVE_CONFIG_H
# include "../config.h"
#endif

#include <stdbool.h>


/* Configuration file setting keys. */
#define CMCONF_USER_NAME "user"
#define CMCONF_SESSION_KEY "key"
#define CMCONF_FORMAT_LOCALFILE "format-localfile"
#define CMCONF_FORMAT_SHOUTCAST "format-shoutcast"
#define CMCONF_FORMAT_COVERFILE "format-coverfile"
#define CMCONF_NOWPLAYING_LOCALFILE "now-playing-localfile"
#define CMCONF_NOWPLAYING_SHOUTCAST "now-playing-shoutcast"
#define CMCONF_SUBMIT_LOCALFILE "submit-localfile"
#define CMCONF_SUBMIT_SHOUTCAST "submit-shoutcast"
#define CMCONF_NOTIFICATION "notification"
#define CMCONF_SERVICE_API_URL "service-api-url"
#define CMCONF_SERVICE_AUTH_URL "service-auth-url"


struct cmusfm_config {

	/* service API endpoints */
	char service_api_url[64];
	char service_auth_url[64];

	char user_name[64];
	char session_key[32 + 1];

	/* regular expressions for name parsers */
	char format_localfile[64];
	char format_shoutcast[64];
#if ENABLE_LIBNOTIFY
	char format_coverfile[64];
#endif

	bool nowplaying_localfile : 1;
	bool nowplaying_shoutcast : 1;
	bool submit_localfile : 1;
	bool submit_shoutcast : 1;
#if ENABLE_LIBNOTIFY
	bool notification : 1;
#endif

};


char *get_cmusfm_config_file(void);
int cmusfm_config_read(const char *fname, struct cmusfm_config *conf);
int cmusfm_config_write(const char *fname, struct cmusfm_config *conf);
#if HAVE_SYS_INOTIFY_H
int cmusfm_config_add_watch(int fd);
#endif

#endif  /* CMUSFM_CONFIG_H_ */
