/*
 * cmusfm - notify.h
 * SPDX-FileCopyrightText: 2014-2024 Arkadiusz Bokowy and contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CMUSFM_NOTIFY_H_
#define CMUSFM_NOTIFY_H_

#include "libscrobbler2.h"


void cmusfm_notify_initialize();
void cmusfm_notify_free();
void cmusfm_notify_show(const scrobbler_trackinfo_t *sb_tinf, const char *icon);

#endif  /* CMUSFM_NOTIFY_H_ */
