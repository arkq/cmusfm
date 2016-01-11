/*
 * cmusfm - cache.h
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
	uint16_t len_mbid;

	/* NULL-terminated strings
	char artist[];
	char album[];
	char album_artist[];
	char track[];
	char mbid[];
	*/

};


void cmusfm_cache_update(const scrobbler_trackinfo_t *sb_tinf);
void cmusfm_cache_submit(scrobbler_session_t *sbs);
char *get_cmusfm_cache_file(void);

#endif  /* CMUSFM_CACHE_H_ */
