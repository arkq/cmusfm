/*
 * notify.c
 * Copyright (c) 2014-2018 Arkadiusz Bokowy
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

#include "notify.h"

#include <stdlib.h>
#include <string.h>
#include <libnotify/notify.h>

#include "debug.h"


/* global notification handler */
static NotifyNotification *cmus_notify;


/* Show track information via the notification system. */
void cmusfm_notify_show(const scrobbler_trackinfo_t *sb_tinf, const char *icon) {

	/* initialize the notification subsystem if needed */
	if (!notify_is_initted())
		cmusfm_notify_initialize();

	if (cmus_notify) {
		/* forcefully close previous notification */
		notify_notification_close(cmus_notify, NULL);
		g_object_unref(G_OBJECT(cmus_notify));
	}

	GError *error = NULL;
	const char *track = sb_tinf->track;
	char *body = NULL;

	const char *artist = "";
	if (sb_tinf->artist != NULL)
		artist = sb_tinf->artist;
	else if (sb_tinf->album_artist != NULL)
		artist = sb_tinf->album_artist;

	const char *album = "";
	if (sb_tinf->album != NULL)
		album = sb_tinf->album;

	size_t art_len = strlen(artist);
	size_t alb_len = strlen(album);

	/* concatenate artist (or album artist) and album */
	if (art_len > 0 || alb_len > 0) {
		body = (char *)malloc(art_len + alb_len + 3 + 1);
		if (art_len > 0) {
			strcpy(body, artist);
			if (alb_len > 0)
				sprintf(&body[art_len], " (%s)", album);
		}
		else
			strcpy(body, album);
	}

	if (track == NULL || strlen(track) == 0) {
		if (body == NULL)
			/* do not display "empty" notification */
			return;
		cmus_notify = notify_notification_new(body, NULL, icon);
	}
	else
		cmus_notify = notify_notification_new(track, body, icon);

	if (!notify_notification_show(cmus_notify, &error)) {
		debug("Desktop notify error: %s", error->message);
		g_error_free(error);
		/* NOTE: Free the notification subsystem upon show failure. This action
		 *       allows us to recover from the D-Bus connection failure, which
		 *       might be caused by the notification daemon restart. */
		cmusfm_notify_free();
	}

	if (body != NULL)
		free(body);
}

/* Initialize notification system. */
void cmusfm_notify_initialize() {
	cmus_notify = NULL;
	notify_init("cmusfm");
}

/* Free notification system resources. */
void cmusfm_notify_free() {
	if (cmus_notify) {
		g_object_unref(G_OBJECT(cmus_notify));
		cmus_notify = NULL;
	}
	notify_uninit();
}
