/*
  cmusfm - main.c
  Copyright (c) 2010-2011 Arkadiusz Bokowy

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  If you want to read full version of the GNU General Public License
  see <http://www.gnu.org/licenses/>.

  ** Note: **
  For contact information and the latest version of this program see
  my webpage <http://arkq.awardspace.us/projects/cmusfm.html>.

*/

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>        /* sockaddr_un */
#include "libscrobbler2.h"
#include "cmusfm.h"

unsigned char SC_api_key[16] = {0x67, 0x08, 0x2e, 0x45, 0xda, 0xb1,
		0xf6, 0x43, 0x3d, 0xa7, 0x2a, 0x00, 0xe3, 0xbc, 0x03, 0x7a};
unsigned char SC_secret[16] = {0x02, 0xfc, 0xbc, 0x90, 0x34, 0x1a,
		0x01, 0xf2, 0x1c, 0x3b, 0xfc, 0x05, 0xb6, 0x36, 0xe3, 0xae};

int main(int argc, char *argv[])
{
	struct cmtrack_info ti;

	if(argc == 1){ //print help
		printf("usage: " APP_NAME " [init]\n\n"
"  init\t\tset/change scrobbler settings\n\n"
"NOTE: Before usage with cmus you MUST invoke this program with 'init'\n"
"      argument. After that you can set status_display_program in cmus\n"
"      (for more informations see 'man cmus'). Enjoy!\n");
		return 0;}

	if(argc == 2 && strcmp(argv[1], "init") == 0)
		return cmusfm_initialization();

	memset(&ti, 0, sizeof(ti));
	cmusfm_socket_sanity_check();

	switch(parse_argv(&ti, argc, argv)){
	case 1: return run_server();
	case 2: return send_data_to_server(&ti);}

	printf("Run it again without any arguments ;)\n");
	return 0;
}

// Send track info to server instance.
// If playing from stream guess format: '$ARTIST - $TITLE'
int send_data_to_server(struct cmtrack_info *tinf)
{
	struct cmusfm_config cm_conf;
	struct sock_data_tag *sock_data = (struct sock_data_tag*)sock_buff;
	struct sockaddr_un sock_a;
	char *album, *artist, *title;
	char *ptr;
	int sock;

	// read configurations
	if(read_cmusfm_config(&cm_conf) != 0) return -1;

	// check if server is running
	memset(&sock_a, 0, sizeof(sock_a));
	sprintf(sock_a.sun_path, "%s/" SOCKET_FNAME , get_cmus_home_dir());
	if(access(sock_a.sun_path, R_OK) != 0) return -1;

	// connect to communication socket (no error check - if socket file is
	// created we assumed that everything should be OK)
	sock_a.sun_family = AF_UNIX;
	sock = socket(PF_UNIX, SOCK_STREAM, 0);
	connect(sock, (struct sockaddr*)(&sock_a), sizeof(sock_a));

	memset(sock_buff, 0, sizeof(sock_buff));

	// load data into sock container
	sock_data->status = tinf->status;
	sock_data->tracknb = tinf->tracknb;
	// if no duration time assume 3 min
	sock_data->duration = tinf->duration == 0 ? 180 : tinf->duration;

	// add Shoutcast (stream) flag
	if(tinf->url != NULL) sock_data->status |= CMSTATUS_SHOUTCASTMASK;

	album = (char *)(sock_data + 1);
	if(tinf->album != NULL) strcpy(album, tinf->album);
	artist = &album[strlen(album) + 1];

	if(tinf->url != NULL && tinf->artist == NULL && tinf->title != NULL) {
		// URL: try to fetch artist and track tile form 'title' field
		if((ptr = strstr(tinf->title, " - ")) == NULL)
			goto normal_case;

		*ptr = 0; //terminate artist tag
		strcpy(artist, tinf->title);
		title = &artist[strlen(artist) + 1];
		strcpy(title, &ptr[3]);
	}
	else if(tinf->file != NULL && tinf->artist == NULL
			&& tinf->title == NULL && cm_conf.parse_file_name) {
		// FILE: try to fetch artist and track title from 'file' field

		// strip PATH segment from 'file' field
		if((ptr = strrchr(tinf->file, '/')) != NULL) ptr++;
		else ptr = tinf->file;

		strcpy(artist, ptr);
		if((ptr = strstr(artist, " - ")) == NULL)
			goto normal_case;

		*ptr = 0; //terminate artist tag
		title = &artist[strlen(artist) + 1];
		strcpy(title, &ptr[3]);

		// strip file extension (everything after last dot)
		if((ptr = strrchr(title, '.')) != NULL) *ptr = 0;
	}
	else { //no title and artist guessing
normal_case:
		if(tinf->artist != NULL) strcpy(artist, tinf->artist);
		title = &artist[strlen(artist) + 1];
		if(tinf->title != NULL) strcpy(title, tinf->title);
	}

	// calculate data offsets
	sock_data->artoff = artist - album;
	sock_data->titoff = title - album;

	write(sock, sock_buff, sizeof(struct sock_data_tag)
			+ sock_data->titoff + strlen(title) + 1);
	return close(sock);
}

// Check if behind our socket is server, if not remove socket node.
void cmusfm_socket_sanity_check()
{
	struct sockaddr_un sock_a;
	int sock;

	memset(&sock_a, 0, sizeof(sock_a));
	sock_a.sun_family = AF_UNIX;
	sprintf(sock_a.sun_path, "%s/" SOCKET_FNAME , get_cmus_home_dir());

	// if there is connection failure try to remove socket node
	sock = socket(PF_UNIX, SOCK_STREAM, 0);
	if(connect(sock, (struct sockaddr*)(&sock_a), sizeof(sock_a)) == -1)
		unlink(sock_a.sun_path);

	close(sock);
}

char *get_cmus_home_dir()
{
	static char fname[128];
	sprintf(fname, "%s/.cmus", getenv("HOME"));
	return fname;
}

// Parse arguments which we've get from cmus
int parse_argv(struct cmtrack_info *tinf, int argc, char *argv[])
{
	int x;

	for(x = 1; x + 1 < argc; x += 2) {
		if(strcmp(argv[x], "status") == 0) {
			if(strcmp(argv[x + 1], "playing") == 0)
				tinf->status = CMSTATUS_PLAYING;
			else if(strcmp(argv[x + 1], "paused") == 0)
				tinf->status = CMSTATUS_PAUSED;
			else if(strcmp(argv[x + 1], "stopped") == 0)
				tinf->status = CMSTATUS_STOPPED;
		}
		else if(strcmp(argv[x], "file") == 0)
			tinf->file = argv[x + 1];
		else if(strcmp(argv[x], "url") == 0)
			tinf->url = argv[x + 1];
		else if(strcmp(argv[x], "artist") == 0)
			tinf->artist = argv[x + 1];
		else if(strcmp(argv[x], "album") == 0)
			tinf->album = argv[x + 1];
		else if(strcmp(argv[x], "tracknumber") == 0)
			tinf->tracknb = atoi(argv[x + 1]);
		else if(strcmp(argv[x], "title") == 0)
			tinf->title = argv[x + 1];
		else if(strcmp(argv[x], "duration") == 0)
			tinf->duration = atoi(argv[x + 1]);
//		else if(strcmp(argv[x], "date") == 0)
//			tinf->date = atoi(argv[x + 1]);
	}

	if(tinf->status == 0) return -1;

	// check required fields - verify cmus program invocation
	if(tinf->file != NULL || tinf->url != NULL) return 2;

	// initial run -> start server
	return 1;
}

// Load information from configuration file
int read_cmusfm_config(struct cmusfm_config *cm_conf)
{
	int fd;
	char buff[128], *ptr;

	// read data from configuration file
	sprintf(buff, "%s/" CONFIG_FNAME, get_cmus_home_dir());
	if((fd = open(buff, O_RDONLY)) == -1) return -1;

	read(fd, buff, sizeof(buff));
	strcpy(cm_conf->user_name, strtok_r(buff, "\n", &ptr));
	strcpy(cm_conf->session_key, strtok_r(NULL, "\n", &ptr));
	cm_conf->parse_file_name = atoi(strtok_r(NULL, "\n", &ptr));
	cm_conf->submit_radio = atoi(strtok_r(NULL, "\n", &ptr));
	return close(fd);
}

// Save information to configuration file
int write_cmusfm_config(struct cmusfm_config *cm_conf)
{
	int fd;
	char buff[128];

	// create config file (truncate previous one)
	sprintf(buff, "%s/" CONFIG_FNAME, get_cmus_home_dir());
	if((fd = open(buff, O_CREAT|O_WRONLY|O_TRUNC, S_IWUSR|S_IRUSR)) == -1)
		return -1;

	sprintf(buff, "%s\n%s\n%d\n%d\n", cm_conf->user_name, cm_conf->session_key,
			cm_conf->parse_file_name, cm_conf->submit_radio);
	write(fd, buff, strlen(buff));
	return close(fd);
}

// Initialize cmusfm (get session key, tune some options)
int user_authorization(const char *url) {
	printf("Open this URL in your favorite web browser and then "
			"press ENTER:\n  %s\n", url);
	getchar(); return 0;}
int cmusfm_initialization()
{
	struct cmusfm_config cm_conf;
	scrobbler_session_t *sbs;
	int fetch_session_key;
	char yesno_buf[8];

	fetch_session_key = 1;
	memset(&cm_conf, 0, sizeof(cm_conf));
	sbs = scrobbler_initialize(SC_api_key, SC_secret);

	// try to read previous configuration
	if(read_cmusfm_config(&cm_conf) == 0) {
		printf("Testing previous session key (Last.fm user name: %s) ...",
				cm_conf.user_name); fflush(stdout);
		scrobbler_set_session_key_str(sbs, cm_conf.session_key);
		if(scrobbler_test_session_key(sbs) == 0) printf("OK.\n");
		else printf("Failed.\n");

		printf("Fetch new session key [yes/NO]: ");
		fgets(yesno_buf, sizeof(yesno_buf), stdin);
		if(memcmp(yesno_buf, "yes", 3) == 0) fetch_session_key = 1;
		else fetch_session_key = 0;
	}

	if(fetch_session_key) { //fetch new session key
		if(scrobbler_authentication(sbs, user_authorization) == 0){
			scrobbler_get_session_key_str(sbs, cm_conf.session_key);
			strcpy(cm_conf.user_name, sbs->user_name);}
		else{ //if user_name and/or session_key is NULL -> segfault :)
			strcpy(cm_conf.user_name, "(none)");
			strcpy(cm_conf.session_key, "xxx");
			printf("Error (scobbler authentication failed)\n\n");}
	}
	scrobbler_free(sbs);

	// ask user few questions
	printf("Submit tracks played from radio [yes/NO]: ");
	fgets(yesno_buf, sizeof(yesno_buf), stdin);
	if(memcmp(yesno_buf, "yes", 3) == 0) cm_conf.submit_radio = 1;
	printf("Try to parse file name if tags are not supplied [yes/NO]: ");
	fgets(yesno_buf, sizeof(yesno_buf), stdin);
	if(memcmp(yesno_buf, "yes", 3) == 0) cm_conf.parse_file_name = 1;

	if(write_cmusfm_config(&cm_conf) != 0)
		printf("Error (cannot create configuration file)\n");
	return 0;
}

// Simple and fast "hashing" function
int make_data_hash(const unsigned char *data, int len)
{
	int x, hash;

	for(x = hash = 0; x < len; x++)
		hash += data[x]*(x + 1);
	return hash;
}
