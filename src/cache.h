/*
 * cmusfm - cache.h
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

#ifndef __CMUSFM_CACHE_H
#define __CMUSFM_CACHE_H

#include <stdint.h>
#include "libscrobbler2.h"


#define CMUSFM_CACHE_SIGNATURE 0x6643

// cache record header structure
struct __attribute__((__packed__)) cmusfm_cache_record {
	uint32_t signature;
	uint32_t timestamp, track_number, duration;
	uint16_t artist_len, album_len, album_artist_len;
	uint16_t track_len, mbid_len;
	//char artist[];       // NULL-terminated
	//char album[];        // NULL-terminated
	//char album_artist[]; // NULL-terminated
	//char track[];        // NULL-terminated
	//char mbid[];         // NULL-terminated
};


char *get_cmusfm_cache_file(void);
void cmusfm_cache_update(const scrobbler_trackinfo_t *sb_tinf);
void cmusfm_cache_submit(scrobbler_session_t *sbs);

#endif
