/*
    cmusfm - config.h
    Copyright (c) 2010-2014 Arkadiusz Bokowy
*/

#ifndef __CMUSFM_CONFIG_H
#define __CMUSFM_CONFIG_H

// Configuration file key definitions
#define CMCONF_USER_NAME "user"
#define CMCONF_SESSION_KEY "key"
#define CMCONF_FORMAT_LOCALFILE "format-localfile"
#define CMCONF_FORMAT_SHOUTCAST "format-shoutcast"
#define CMCONF_NOWPLAYING_LOCALFILE "now-playing-localfile"
#define CMCONF_NOWPLAYING_SHOUTCAST "now-playing-shoutcast"
#define CMCONF_SUBMIT_LOCALFILE "submit-localfile"
#define CMCONF_SUBMIT_SHOUTCAST "submit-shoutcast"

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
};


char *get_cmusfm_config_file();
int cmusfm_config_read(const char *fname, struct cmusfm_config *conf);
int cmusfm_config_write(const char *fname, struct cmusfm_config *conf);

#endif
