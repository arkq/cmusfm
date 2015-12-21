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

#if HAVE_CONFIG_H
#include "../config.h"
#endif

#include "cache.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "cmusfm.h"
#include "debug.h"


/* Return the actual size of the given cache record structure. */
static size_t get_cache_record_size(const struct cmusfm_cache_record *record) {
	return sizeof(*record) + record->len_artist + record->len_album +
		record->len_track + record->len_album_artist + record->len_mbid;
}

/* Return the checksum for the length-invariant part of the cache record
 * structure - segmentation-fault-safe checksum. */
static uint8_t get_cache_record_checksum1(const struct cmusfm_cache_record *record) {
	return make_data_hash((char *)&record->timestamp,
			sizeof(*record) - ((void *)&record->timestamp - (void *)record));
}

/* Return the data checksum of the given cache record structure. If the
 * overall record length is compromised, we might end up dead... */
static uint8_t get_cache_record_checksum2(const struct cmusfm_cache_record *record) {
	return make_data_hash((char *)&record->timestamp,
			get_cache_record_size(record) - ((void *)&record->timestamp - (void *)record));
}

/* Copy scrobbler track info into the cache record structure. Returned
 * pointer has to be freed by the `free` function. */
static struct cmusfm_cache_record *get_cache_record(const scrobbler_trackinfo_t *sb_tinf) {

	struct cmusfm_cache_record *record;
	char *ptr;

	/* allocate memory for the initial record data (header) */
	record = (struct cmusfm_cache_record *)calloc(1, sizeof(*record));

	record->signature = CMUSFM_CACHE_SIGNATURE;
	record->timestamp = sb_tinf->timestamp;
	record->track_number = sb_tinf->track_number;
	record->duration = sb_tinf->duration;

	if (sb_tinf->artist)
		record->len_artist = strlen(sb_tinf->artist) + 1;
	if (sb_tinf->album)
		record->len_album = strlen(sb_tinf->album) + 1;
	if (sb_tinf->track)
		record->len_track = strlen(sb_tinf->track) + 1;
#if 0
	if (sb_tinf->album_artist)
		record->len_album_artist = strlen(sb_tinf->album_artist) + 1;
	if (sb_tinf->mbid)
		record->len_mbid = strlen(sb_tinf->mbid) + 1;
#endif

	/* enlarge allocated memory for string data payload */
	record = (struct cmusfm_cache_record *)realloc(record, get_cache_record_size(record));
	ptr = (char *)&record[1];

	if (record->len_artist) {
		strcpy(ptr, sb_tinf->artist);
		ptr += record->len_artist;
	}
	if (record->len_album) {
		strcpy(ptr, sb_tinf->album);
		ptr += record->len_album;
	}
	if (record->len_track) {
		strcpy(ptr, sb_tinf->track);
		ptr += record->len_track;
	}
#if 0
	if (record->len_album_artist) {
		strcpy(ptr, sb_tinf->album_artist);
		ptr += record->len_album_artist;
	}
	if (record->len_mbid) {
		strcpy(ptr, sb_tinf->mbid);
		ptr += record->len_mbid;
	}
#endif

	record->checksum1 = get_cache_record_checksum1(record);
	record->checksum2 = get_cache_record_checksum2(record);

	return record;
}

/* Write data, which should be submitted later, to the cache file. */
void cmusfm_cache_update(const scrobbler_trackinfo_t *sb_tinf) {

	FILE *f;
	struct cmusfm_cache_record *record;
	size_t record_size;

	debug("cache update: %ld", sb_tinf->timestamp);
	debug("payload: %s - %s (%s) - %d. %s (%ds)",
			sb_tinf->artist, sb_tinf->album, sb_tinf->album_artist,
			sb_tinf->track_number, sb_tinf->track, sb_tinf->duration);

	if ((f = fopen(cmusfm_cache_file, "a")) == NULL)
		return;

	record = get_cache_record(sb_tinf);
	record_size = get_cache_record_size(record);

	/* convert host endianness to the "universal" one */
	record->signature = htons(record->signature);
	record->timestamp = htonl(record->timestamp);
	record->track_number = htons(record->track_number);
	record->duration = htons(record->duration);
	record->len_artist = htons(record->len_artist);
	record->len_album = htons(record->len_album);
	record->len_track = htons(record->len_track);
	record->len_album_artist = htons(record->len_album_artist);
	record->len_mbid = htons(record->len_mbid);

	fwrite(record, record_size, 1, f);

	free(record);
	fclose(f);
}

/* Submit tracks saved in the cache file. */
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

	/* read file until EOF */
	while (!feof(f)) {
		rd_len = fread(rd_buff, 1, sizeof(rd_buff), f);
		record = (struct cmusfm_cache_record *)rd_buff;

		/* iterate while there is no enough data for full cache record header */
		while ((void *)record - (void *)rd_buff + sizeof(*record) <= rd_len) {

			/* convert "universal" endianness to the host one */
			record->signature = ntohs(record->signature);
			record->timestamp = ntohl(record->timestamp);
			record->track_number = ntohs(record->track_number);
			record->duration = ntohs(record->duration);
			record->len_artist = ntohs(record->len_artist);
			record->len_album = ntohs(record->len_album);
			record->len_track = ntohs(record->len_track);
			record->len_album_artist = ntohs(record->len_album_artist);
			record->len_mbid = ntohs(record->len_mbid);

			/* validate record type and first-stage data integration */
			if (record->signature != CMUSFM_CACHE_SIGNATURE ||
					record->checksum1 != get_cache_record_checksum1(record)) {
				fprintf(stderr, "error: invalid cache record signature\n");
				debug("signature: %x, checksum: %x", record->signature, record->checksum1);
				goto return_failure;
			}

			record_size = get_cache_record_size(record);
			debug("record size: %ld", record_size);

			/* break if current record is truncated */
			if ((void *)record - (void *)rd_buff + record_size > rd_len)
				break;

			/* check for second-stage data integration */
			if (record->checksum2 != get_cache_record_checksum2(record)) {
				fprintf(stderr, "error: cache record data corrupted\n");
				goto return_failure;
			}

			/* restore scrobbler track info structure from cache */
			memset(&sb_tinf, 0, sizeof(sb_tinf));
			sb_tinf.timestamp = record->timestamp;
			sb_tinf.track_number = record->track_number;
			sb_tinf.duration = record->duration;
			ptr = (char *)&record[1];

			if (record->len_artist) {
				sb_tinf.artist = ptr;
				ptr += record->len_artist;
			}
			if (record->len_album) {
				sb_tinf.album = ptr;
				ptr += record->len_album;
			}
			if (record->len_track) {
				sb_tinf.track = ptr;
				ptr += record->len_track;
			}
#if 0
			if (record->len_album_artist) {
				sb_tinf.album_artist = ptr;
				ptr += record->len_album_artist;
			}
			if (record->len_mbid) {
				sb_tinf.mbid = ptr;
				ptr += record->len_mbid;
			}
#endif

			debug("cache: %s - %s (%s) - %d. %s (%ds)",
					sb_tinf.artist, sb_tinf.album, sb_tinf.album_artist,
					sb_tinf.track_number, sb_tinf.track, sb_tinf.duration);

			/* submit tracks to Last.fm */
			scrobbler_scrobble(sbs, &sb_tinf);

			/* point to next record */
			record = (struct cmusfm_cache_record *)((char *)record + record_size);
		}

		if ((unsigned)((void *)record - (void *)rd_buff) != rd_len)
			/* seek to the beginning of the current record, because it is
			 * truncated, so we have to read it one more time */
			fseek(f, (void *)record - (void *)rd_buff - rd_len, SEEK_CUR);
	}

return_failure:

	fclose(f);

	/* Remove the cache file, regardless of the submission status. Note, that
	 * keeping invalid file will result in an inability to submit tracks later
	 * - there is no validity check upon cache creation. */
	unlink(cmusfm_cache_file);
}

/* Helper function for retrieving cmusfm cache file. */
char *get_cmusfm_cache_file(void) {
	return get_cmus_home_file(CACHE_FNAME);
}
