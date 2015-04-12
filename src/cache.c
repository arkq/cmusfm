/*
 * cmusfm - cache.c
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

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include "cache.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cmusfm.h"
#include "debug.h"


// Return the actual size of given cache record structure.
static size_t get_cache_record_size(const struct cmusfm_cache_record *record) {
	return sizeof(*record) + record->artist_len + record->album_len +
		record->album_artist_len + record->track_len + record->mbid_len;
}

/* Copy scrobbler track info into the cache record structure. Returned
 * pointer has to be freed by the `free` function. */
struct cmusfm_cache_record *get_cache_record(const scrobbler_trackinfo_t *sb_tinf) {

	struct cmusfm_cache_record *cr;
	char *ptr;

	/* allocate memory for initial record data (header) */
	cr = (struct cmusfm_cache_record *)calloc(1, sizeof(*cr));

	cr->signature = CMUSFM_CACHE_SIGNATURE;
	cr->timestamp = sb_tinf->timestamp;
	cr->track_number = sb_tinf->track_number;
	cr->duration = sb_tinf->duration;
	if (sb_tinf->artist)
		cr->artist_len = strlen(sb_tinf->artist) + 1;
	if (sb_tinf->album)
		cr->album_len = strlen(sb_tinf->album) + 1;
//	if (sb_tinf->album_artist)
//		cr->album_artist_len = strlen(sb_tinf->album_artist) + 1;
	if (sb_tinf->track)
		cr->track_len = strlen(sb_tinf->track) + 1;
//	if (sb_tinf->mbid)
//		cr->mbid_len = strlen(sb_tinf->mbid) + 1;

	/* enlarge allocated memory for string data payload */
	cr = (struct cmusfm_cache_record *)realloc(cr, get_cache_record_size(cr));
	ptr = (char *)&cr[1];

	if (cr->artist_len) {
		strcpy(ptr, sb_tinf->artist);
		ptr += cr->artist_len;
	}
	if (cr->album_len) {
		strcpy(ptr, sb_tinf->album);
		ptr += cr->album_len;
	}
//	if (cr->album_artist_len) {
//		strcpy(ptr, sb_tinf->album_artist);
//		ptr += cr->album_artist_len;
//	}
	if (cr->track_len) {
		strcpy(ptr, sb_tinf->track);
		ptr += cr->track_len;
	}
//	if (cr->mbid_len) {
//		strcpy(ptr, sb_tinf->mbid);
//		ptr += cr->mbid_len;
//	}

	return cr;
}

// Write data, which should be submitted later, to the cache file.
void cmusfm_cache_update(const scrobbler_trackinfo_t *sb_tinf) {

	FILE *f;
	struct cmusfm_cache_record *record;

	debug("cache update: %ld", sb_tinf->timestamp);
	debug("payload: %s - %s (%s) - %d. %s (%ds)",
			sb_tinf->artist, sb_tinf->album, sb_tinf->album_artist,
			sb_tinf->track_number, sb_tinf->track, sb_tinf->duration);

	if ((f = fopen(cmusfm_cache_file, "a")) == NULL)
		return;

	record = get_cache_record(sb_tinf);
	fwrite(record, get_cache_record_size(record), 1, f);

	free(record);
	fclose(f);
}

// Submit tracks saved in the cache file.
void cmusfm_cache_submit(scrobbler_session_t *sbs) {

	char rd_buff[4096];
	FILE *f;
	scrobbler_trackinfo_t sb_tinf;
	struct cmusfm_cache_record *record;
	size_t rd_len, record_size;
	char *ptr;

	debug("cache submit");

	if ((f = fopen(cmusfm_cache_file, "r")) == NULL)
		return;

	// read file until EOF
	while (!feof(f)) {
		rd_len = fread(rd_buff, 1, sizeof(rd_buff), f);
		record = (struct cmusfm_cache_record*)rd_buff;

		// iterate while there is no enough data for full cache record header
		while ((void*)record - (void*)rd_buff + sizeof(*record) <= rd_len) {

			if (record->signature != CMUSFM_CACHE_SIGNATURE) {
				debug("invalid cache record signature: %x", record->signature);
				fclose(f);
				return;
			}

			record_size = get_cache_record_size(record);
			debug("record size: %ld", record_size);

			// break if current record is truncated
			if ((void*)record - (void*)rd_buff + record_size > rd_len)
				break;

			// restore scrobbler track info structure from cache
			memset(&sb_tinf, 0, sizeof(sb_tinf));
			sb_tinf.timestamp = record->timestamp;
			sb_tinf.track_number = record->track_number;
			sb_tinf.duration = record->duration;
			ptr = (char*)&record[1];

			if (record->artist_len) {
				sb_tinf.artist = ptr;
				ptr += record->artist_len;
			}
			if (record->album_len) {
				sb_tinf.album = ptr;
				ptr += record->album_len;
			}
//			if (record->album_artist_len) {
//				sb_tinf.album_artist = ptr;
//				ptr += record->album_artist_len;
//			}
			if (record->track_len) {
				sb_tinf.track = ptr;
				ptr += record->track_len;
			}
//			if (record->mbid_len) {
//				sb_tinf.mbid = ptr;
//				ptr += record->mbid_len;
//			}

			debug("cache: %s - %s (%s) - %d. %s (%ds)",
					sb_tinf.artist, sb_tinf.album, sb_tinf.album_artist,
					sb_tinf.track_number, sb_tinf.track, sb_tinf.duration);

			// submit tracks to Last.fm
			scrobbler_scrobble(sbs, &sb_tinf);

			// point to next record
			record = (struct cmusfm_cache_record*)((char*)record + record_size);
		}

		if ((unsigned)((void*)record - (void*)rd_buff) != rd_len)
			// seek to the beginning of current record, because
			// it is truncated, so we have to read it one more time
			fseek(f, (void*)record - (void*)rd_buff - rd_len, SEEK_CUR);
	}

	fclose(f);

	// remove cache file
	unlink(cmusfm_cache_file);
}

/* Helper function for retrieving cmusfm cache file. */
char *get_cmusfm_cache_file(void) {
	return get_cmus_home_file(CACHE_FNAME);
}
