/*
 * cmusfm - server.h
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

#ifndef __CMUSFM_SERVER_H
#define __CMUSFM_SERVER_H

#include "cmusfm.h"


// communication socket read/write buffer size
#define CMSOCKET_BUFFER_SIZE 1024

#define CMSTATUS_SHOUTCASTMASK 0xF0
struct sock_data_tag {
	enum cmstatus status;
	int tracknb, duration;
	int alboff, titoff, locoff;
// char artist[];
// char album[];
// char title[];
// char location[];
}__attribute__ ((packed));


char *get_cmusfm_socket_file(void);
void cmusfm_server_start(void);
int cmusfm_server_send_track(struct cmtrack_info *tinfo);

#endif
