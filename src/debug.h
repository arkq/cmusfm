/*
 * cmusfm - debug.h
 * SPDX-FileCopyrightText: 2014-2024 Arkadiusz Bokowy and contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CMUSFM_DEBUG_H_
#define CMUSFM_DEBUG_H_

#if HAVE_CONFIG_H
# include "../config.h"
#endif

#include <stdio.h>

#if DEBUG
# define debug(M, ARGS...) fprintf(stderr, "DEBUG %s:%d: " M "\n", __FILE__, __LINE__, ## ARGS)
#else
# define debug(M, ARGS...) do {} while (0)
#endif

#endif
