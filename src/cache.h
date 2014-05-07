/*
    cmusfm - cache.h
    Copyright (c) 2010-2014 Arkadiusz Bokowy
*/

#ifndef __CMUSFM_CACHE_H
#define __CMUSFM_CACHE_H

#include "libscrobbler2.h"

char *get_cmusfm_cache_file();
void cmusfm_cache_update(const scrobbler_trackinfo_t *sb_tinf);
void cmusfm_cache_submit(scrobbler_session_t *sbs);

#endif
