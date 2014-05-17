/*
 * cmusfm - cache.c
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

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include "cmusfm.h"
#include "cache.h"
#include "debug.h"


// Write data to cache file which should be submitted later.
// Cache file record structure:
// record_size(int)|scrobbler_trackinf_t|artist(NULL-term string)
// track(NULL-term string)|album(NULL-term string)
void cmusfm_cache_update(const scrobbler_trackinfo_t *sb_tinf)
{
	int fd;
	int artlen, tralen, alblen;
	size_t data_size;

	debug("cache update: %ld", sb_tinf->timestamp);
	debug("payload: %s - %s (%s) - %d. %s (%ds)",
			sb_tinf->artist, sb_tinf->album, sb_tinf->album_artist,
			sb_tinf->track_number, sb_tinf->track, sb_tinf->duration);

	fd = open(get_cmusfm_cache_file(), O_CREAT | O_APPEND | O_WRONLY, 00644);
	if(fd == -1) return;

	artlen = strlen(sb_tinf->artist);
	tralen = strlen(sb_tinf->track);
	alblen = strlen(sb_tinf->album);

	// calculate record size
	data_size = sizeof(data_size) + sizeof(scrobbler_trackinfo_t) +
			artlen + 1 + tralen + 1 + alblen + 1;

	write(fd, &data_size, sizeof(data_size));
	write(fd, sb_tinf, sizeof(scrobbler_trackinfo_t));
	write(fd, sb_tinf->artist, artlen + 1);
	write(fd, sb_tinf->track, tralen + 1);
	write(fd, sb_tinf->album, alblen + 1);

	close(fd);
}

// Submit tracks saved in the cache file.
void cmusfm_cache_submit(scrobbler_session_t *sbs)
{
	char rd_buff[4096];
	char *fname;
	int fd;
	scrobbler_trackinfo_t sb_tinf;
	void *record;
	ssize_t rd_len;
	size_t rec_size;

	debug("cache submit");

	fname = get_cmusfm_cache_file();
	if((fd = open(fname, O_RDONLY)) == -1)
		return;  // no cache file -> nothing to submit :)

	// read file until EOF
	while((rd_len = read(fd, rd_buff, sizeof(rd_buff))) != 0) {
		if(rd_len == -1) break;

		for(record = rd_buff;;) {
			rec_size = *((size_t*)record);
			debug("record size: %ld", rec_size);

			// break if current record is truncated
			if(record - (void*)rd_buff + rec_size > (size_t)rd_len)
				break;

			memcpy(&sb_tinf, record + sizeof(rec_size), sizeof(sb_tinf));
			sb_tinf.artist = record + sizeof(rec_size) + sizeof(sb_tinf);
			sb_tinf.track = &sb_tinf.artist[strlen(sb_tinf.artist) + 1];
			sb_tinf.album = &sb_tinf.track[strlen(sb_tinf.track) + 1];

			// submit tracks to Last.fm
			scrobbler_scrobble(sbs, &sb_tinf);

			// point to next record
			record += rec_size;

			// break if there is no enough data to obtain the record size
			if(record - (void*)rd_buff + sizeof(rec_size) > (size_t)rd_len)
				break;
		}

		if(record - (void*)rd_buff != rd_len)
			// seek to the beginning of current record, because
			// it is truncated, so we have to read it one more time
			lseek(fd, record - (void*)rd_buff - rd_len, SEEK_CUR);
	}

	close(fd);

	// remove cache file
	unlink(fname);
}

// Helper function for retrieving cmusfm cache file.
char *get_cmusfm_cache_file()
{
	static char fname[128];
	sprintf(fname, "%s/" CACHE_FNAME, get_cmus_home_dir());
	return fname;
}
