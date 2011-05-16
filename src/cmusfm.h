/*
    cmusfm - cmusfm.h
    Copyright (c) 2010-2011 Arkadiusz Bokowy
*/

#define APP_NAME "cmusfm"
#define APP_VER "0.1.0"

#define __CMUSFM_H

#define CONFIG_FNAME "cmusfm.conf"
#define SOCKET_FNAME "cmusfm.socket"
#define CACHE_FNAME  "cmusfm.cache"

unsigned char SC_api_key[16], SC_secret[16];

struct cmusfm_config {
	char user_name[64];
	char session_key[16*2 + 1];
	char parse_file_name, submit_radio;
};

struct cmtrack_info {
	char status;
	char *file, *url, *artist, *album, *title;
	int tracknb, duration;
};
#define CMSTATUS_PLAYING 1
#define CMSTATUS_PAUSED 2
#define CMSTATUS_STOPPED 3

// socket transmission stuff
char sock_buff[1024];
struct sock_data_tag {
	char status;
	int tracknb, duration;
	int artoff, titoff;
// char album[];
// char artist[];
// char title[];
}__attribute__ ((packed));
#define CMSTATUS_SHOUTCASTMASK 0xf0

// in "server.c"
#define LOGIN_RETRY_DELAY 60*30 //time in seconds
int run_server();
void process_server_data(int fd, scrobbler_session_t *sbs, int submit_radio);
void update_cache(const scrobbler_trackinfo_t *sb_tinf);
void submit_cache(scrobbler_session_t *sbs);
void fill_trackinfo(scrobbler_trackinfo_t *sbt, const struct sock_data_tag *dt);

// in "main.c"
int send_data_to_server(struct cmtrack_info *tinf);
int cmusfm_initialization();
void cmusfm_socket_sanity_check();
int parse_argv(struct cmtrack_info *tinf, int argc, char *argv[]);
int read_cmusfm_config(struct cmusfm_config *cm_conf);
int write_cmusfm_config(struct cmusfm_config *cm_conf);
char *get_cmus_home_dir();
int make_data_hash(const unsigned char *data, int len);
