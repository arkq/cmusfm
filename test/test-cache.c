#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "../src/cache.c"
#include "../src/utils.c"

/* global variable used in the cache code */
const char *cmusfm_cache_file;

/* library function used by the cache code */
int scrobbler_scrobble_count = 0;
scrobbler_status_t scrobbler_scrobble(scrobbler_session_t *sbs, scrobbler_trackinfo_t *sbt) {
	scrobbler_scrobble_count++;
	return SCROBBLER_STATUS_OK;
}

int main(void) {

	FILE *f;
	size_t size;
	char buffer[512];
	int i;

	cmusfm_cache_file = tempnam(NULL, NULL);

	scrobbler_trackinfo_t track_null = { 0 };
	scrobbler_trackinfo_t track_empty = {
		.artist = "",
		.album = "",
		.album_artist = "",
		.track = "",
		.mbid = "",
	};
	scrobbler_trackinfo_t track_full = {
		.artist = "The Beatles",
		.album = "Revolver",
		.album_artist = "The Beatles",
		.track = "Yellow Submarine",
		.mbid = "b2181aae-5cba-496c-bb0c-b4cc0109ebf8",
		.track_number = 6,
		.duration = 161,
		.timestamp = 1444444444,
	};

	cmusfm_cache_update(&track_null);
	cmusfm_cache_update(&track_empty);
	cmusfm_cache_update(&track_full);

	assert((f = fopen(cmusfm_cache_file, "r")) != NULL);
	/* make sure the structure of the cache file is not changed */
	assert((size = fread(buffer, 1, sizeof(buffer), f)) == 107);
	assert(make_data_hash(buffer, size) == 371117);
	fclose(f);

	cmusfm_cache_submit(NULL);
	assert(scrobbler_scrobble_count == 3);

	/* cache file should have been removed after the submission */
	assert(fopen(cmusfm_cache_file, "r") == NULL);

	/* test for big cache file - multiple read calls */

	for (i = 500; i != 0; i--)
		cmusfm_cache_update(&track_full);

	cmusfm_cache_submit(NULL);
	assert(scrobbler_scrobble_count == 503);

	return EXIT_SUCCESS;
}
