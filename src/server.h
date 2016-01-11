/*
 * cmusfm - server.h
 * Copyright (c) 2014-2015 Arkadiusz Bokowy
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

#ifndef CMUSFM_SERVER_H_
#define CMUSFM_SERVER_H_

#include "cmusfm.h"


/* communication socket buffer size */
#define CMSOCKET_BUFFER_SIZE 1024

/* shoutcast/stream flag for the status field */
#define CMSTATUS_SHOUTCASTMASK 0xF0

/* message queue record structure */
struct cmusfm_data_record {

	/* record header */
	uint8_t checksum1;
	uint8_t checksum2;

	/* NOTE: This field has to be defined before any other fields,
	 *       because the hashing logic relies on this assumption. */
	enum cmstatus status;

	uint16_t track_number;
	uint16_t duration;

	uint16_t off_album;
	uint16_t off_title;
	uint16_t off_location;

	/* NULL-terminated strings
	char artist[];
	char album[];
	char title[];
	char location[];
	*/

};


int cmusfm_server_check(void);
int cmusfm_server_start(void);
int cmusfm_server_send_track(struct cmtrack_info *tinfo);
char *get_cmusfm_socket_file(void);

#endif  /* CMUSFM_SERVER_H_ */
