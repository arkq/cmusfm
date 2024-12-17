/*
 * cmusfm - cache.c
 * SPDX-FileCopyrightText: 2014-2024 Arkadiusz Bokowy and contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#if HAVE_CONFIG_H
# include "../config.h"
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
		record->len_track + record->len_album_artist + record->len_mb_track_id;
}

/* Return the checksum for the length-invariant part of the cache record
 * structure - segmentation-fault-safe checksum. */
static uint8_t get_cache_record_checksum1(const struct cmusfm_cache_record *record) {
	return make_data_hash((unsigned char *)&record->timestamp,
			sizeof(*record) - ((char *)&record->timestamp - (char *)record));
}

/* Return the data checksum of the given cache record structure. If the
 * overall record length is compromised, we might end up dead... */
static uint8_t get_cache_record_checksum2(const struct cmusfm_cache_record *record) {
	return make_data_hash((unsigned char *)&record->timestamp,
			get_cache_record_size(record) - ((char *)&record->timestamp - (char *)record));
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
	if (sb_tinf->album_artist)
		record->len_album_artist = strlen(sb_tinf->album_artist) + 1;
	if (sb_tinf->mb_track_id)
		record->len_mb_track_id = strlen(sb_tinf->mb_track_id) + 1;

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
	if (record->len_album_artist) {
		strcpy(ptr, sb_tinf->album_artist);
		ptr += record->len_album_artist;
	}
	if (record->len_mb_track_id) {
		strcpy(ptr, sb_tinf->mb_track_id);
		ptr += record->len_mb_track_id;
	}

	record->checksum1 = get_cache_record_checksum1(record);
	record->checksum2 = get_cache_record_checksum2(record);

	return record;
}

/* Write data, which should be submitted later, to the cache file. */
void cmusfm_cache_update(const scrobbler_trackinfo_t *sb_tinf) {

	FILE *f;
	struct cmusfm_cache_record *record;
	size_t record_size;

	debug("Cache update: %ld", sb_tinf->timestamp);
	debug("Payload: %s - %s (%s) - %d. %s (%ds)",
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
	record->len_mb_track_id = htons(record->len_mb_track_id);

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

	debug("Cache submit");

	if ((f = fopen(cmusfm_cache_file, "r")) == NULL)
		return;

	/* read file until EOF */
	while (!feof(f)) {
		rd_len = fread(rd_buff, 1, sizeof(rd_buff), f);
		record = (struct cmusfm_cache_record *)rd_buff;

		/* iterate while there is no enough data for full cache record header */
		while (((char *)record - rd_buff) + sizeof(*record) <= rd_len) {

			/* convert "universal" endianness to the host one */
			record->signature = ntohs(record->signature);
			record->timestamp = ntohl(record->timestamp);
			record->track_number = ntohs(record->track_number);
			record->duration = ntohs(record->duration);
			record->len_artist = ntohs(record->len_artist);
			record->len_album = ntohs(record->len_album);
			record->len_track = ntohs(record->len_track);
			record->len_album_artist = ntohs(record->len_album_artist);
			record->len_mb_track_id = ntohs(record->len_mb_track_id);

			/* validate record type and first-stage data integration */
			if (record->signature != CMUSFM_CACHE_SIGNATURE ||
					record->checksum1 != get_cache_record_checksum1(record)) {
				fprintf(stderr, "ERROR: Invalid cache record signature\n");
				debug("Signature: %x, checksum: %x", record->signature, record->checksum1);
				goto return_failure;
			}

			record_size = get_cache_record_size(record);
			debug("Record size: %ld", record_size);

			/* break if current record is truncated */
			if (((char *)record - rd_buff) + record_size > rd_len)
				break;

			/* check for second-stage data integration */
			if (record->checksum2 != get_cache_record_checksum2(record)) {
				fprintf(stderr, "ERROR: Cache record data corrupted\n");
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
			if (record->len_album_artist) {
				sb_tinf.album_artist = ptr;
				ptr += record->len_album_artist;
			}
			if (record->len_mb_track_id) {
				sb_tinf.mb_track_id = ptr;
				ptr += record->len_mb_track_id;
			}

			debug("Cache: %s - %s (%s) - %d. %s (%ds)",
					sb_tinf.artist, sb_tinf.album, sb_tinf.album_artist,
					sb_tinf.track_number, sb_tinf.track, sb_tinf.duration);

			/* submit tracks to Last.fm */
			scrobbler_scrobble(sbs, &sb_tinf);

			/* point to next record */
			record = (struct cmusfm_cache_record *)((char *)record + record_size);
		}

		if ((size_t)((char *)record - rd_buff) != rd_len)
			/* seek to the beginning of the current record, because it is
			 * truncated, so we have to read it one more time */
			fseek(f, ((char *)record - rd_buff) - rd_len, SEEK_CUR);
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
