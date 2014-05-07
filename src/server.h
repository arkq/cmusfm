/*
    cmusfm - server.h
    Copyright (c) 2010-2014 Arkadiusz Bokowy
*/

#ifndef __CMUSFM_SERVER_H
#define __CMUSFM_SERVER_H

#include "cmusfm.h"


// comunication socket read/write buffer size
#define CMSOCKET_BUFFER_SIZE 1024

#define CMSTATUS_SHOUTCASTMASK 0xF0
struct sock_data_tag {
	enum cmstatus status;
	int tracknb, duration;
	int alboff, titoff;
// char artist[];
// char album[];
// char title[];
}__attribute__ ((packed));


char *get_cmusfm_socket_file();
void cmusfm_server_start();
int cmusfm_server_send_track(struct cmtrack_info *tinfo);

#endif
