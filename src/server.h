/*
 * cmusfm - server.h
 * SPDX-FileCopyrightText: 2014-2026 Arkadiusz Bokowy and contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CMUSFM_SERVER_H_
#define CMUSFM_SERVER_H_

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdint.h>

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

	uint16_t disc_number;
	uint16_t track_number;
	uint16_t duration;

	uint16_t off_artist;
	uint16_t off_album_artist;
	uint16_t off_album;
	uint16_t off_title;
	uint16_t off_location;

	/* NULL-terminated strings
	 * char mb_track_id[];
	 * char artist[];
	 * char album_artist[];
	 * char album[];
	 * char title[];
	 * char location[];
	 */

};

/**
 * Hook called by the server once it is listening for clients.
 *
 * The default weak implementation does nothing. It is intended to be overridden by tests to
 * signal that the server is ready to accept connections in case of multi-threaded tests. */
void cmusfm_server_ready(void);

int cmusfm_server_check(void);
int cmusfm_server_start(void);
int cmusfm_server_send_track(struct cmtrack_info * tinfo);

#endif
