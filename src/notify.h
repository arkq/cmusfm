/*
 * notify.h
 * Copyright (c) 2014-2017 Arkadiusz Bokowy
 *
 * This file is a part of cmusfm.
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

#ifndef CMUSFM_NOTIFY_H_
#define CMUSFM_NOTIFY_H_

#include "libscrobbler2.h"


void cmusfm_notify_initialize();
void cmusfm_notify_free();
void cmusfm_notify_show(const scrobbler_trackinfo_t *sb_tinf, const char *icon);

#endif  /* CMUSFM_NOTIFY_H_ */
