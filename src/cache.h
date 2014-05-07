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

#include "libscrobbler2.h"

char *get_cmusfm_cache_file();
void cmusfm_cache_update(const scrobbler_trackinfo_t *sb_tinf);
void cmusfm_cache_submit(scrobbler_session_t *sbs);

#endif
