/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Pure line formatting for the playlist viewer's tag-based track display
 * (specs/0011-playlist-viewer-disc-track-format.md). No firmware includes,
 * so tests/test_playlist_viewer_format.c can exercise it on the host.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef PLAYLIST_VIEWER_FORMAT_H
#define PLAYLIST_VIEWER_FORMAT_H

#include <stdbool.h>
#include <stdio.h>
#include <stddef.h>

static inline void pv_append(char *buf, size_t size, size_t *pos,
                             const char *fmt, const char *s, int n)
{
    if (*pos >= size)
        return;
    int r = s ? snprintf(buf + *pos, size - *pos, fmt, s)
              : snprintf(buf + *pos, size - *pos, fmt, n);
    if (r > 0)
        *pos += (size_t)r < size - *pos ? (size_t)r : size - *pos - 1;
}

/* Formats "[D.][TT. ]Title[ - Album]" into buf; false if the title is empty. */
static inline bool pv_format_tag_line(char *buf, size_t size,
                                      const char *disc_string, int discnum,
                                      const char *track_string, int tracknum,
                                      const char *title, const char *album,
                                      const char *untagged,
                                      bool with_numbers, bool with_album)
{
    size_t pos = 0;
    if (size == 0)
        return false;
    buf[0] = '\0';
    if (!title || !title[0])
        return false;

    if (with_numbers)
    {
        if (disc_string && disc_string[0])
            pv_append(buf, size, &pos, "%s.", disc_string, 0);
        else if (discnum)
            pv_append(buf, size, &pos, "%d.", NULL, discnum);
        if (track_string && track_string[0])
            pv_append(buf, size, &pos, "%s.", track_string, 0);
        else if (tracknum)
            pv_append(buf, size, &pos, "%02d.", NULL, tracknum);
        if (pos > 0)
            pv_append(buf, size, &pos, "%s", " ", 0);
    }
    pv_append(buf, size, &pos, "%s", title, 0);
    if (with_album)
    {
        pv_append(buf, size, &pos, "%s", " - ", 0);
        pv_append(buf, size, &pos, "%s",
                  album && album[0] ? album : untagged, 0);
    }
    return true;
}

#endif /* PLAYLIST_VIEWER_FORMAT_H */
