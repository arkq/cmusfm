/*
 * debug.h
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
