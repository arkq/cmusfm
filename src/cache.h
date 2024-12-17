/*
 * cmusfm - cache.h
 * SPDX-FileCopyrightText: 2014-2024 Arkadiusz Bokowy and contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CMUSFM_CACHE_H_
#define CMUSFM_CACHE_H_

#include <stdint.h>
#include "libscrobbler2.h"


/* "Cr" string (big-endian) at the beginning of the record */
#define CMUSFM_CACHE_SIGNATURE 0x4372

/* cache record header structure */
struct __attribute__((__packed__)) cmusfm_cache_record {

	/* record header */
	uint16_t signature;
	uint8_t checksum1;
	uint8_t checksum2;

	uint32_t timestamp;
	uint16_t track_number;
	uint16_t duration;

	uint16_t len_artist;
	uint16_t len_album;
	uint16_t len_track;
	uint16_t len_album_artist;
	uint16_t len_mb_track_id;

	/* NULL-terminated strings
	char artist[];
	char album[];
	char album_artist[];
	char track[];
	char mb_track_id[];
	*/

};


void cmusfm_cache_update(const scrobbler_trackinfo_t *sb_tinf);
void cmusfm_cache_submit(scrobbler_session_t *sbs);
char *get_cmusfm_cache_file(void);

#endif  /* CMUSFM_CACHE_H_ */
