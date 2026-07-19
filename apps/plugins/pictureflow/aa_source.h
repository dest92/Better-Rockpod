/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Pure decision logic for choosing an album art source (embedded tag
 * picture vs. separate image file) while building PictureFlow's
 * album art cache.
 *
 * Deliberately free of I/O and rockbox headers (stdbool.h only) so it
 * stays host-testable: see tests/test_pf_aa_source.c. The actual
 * get_metadata()/search_albumart_files() calls live in pictureflow.c.
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

#ifndef PICTUREFLOW_AA_SOURCE_H
#define PICTUREFLOW_AA_SOURCE_H

#include <stdbool.h>

enum aa_source
{
    AA_SOURCE_NONE = 0,
    AA_SOURCE_FILE,
    AA_SOURCE_EMBEDDED,
};

/* Whether the on-disk cover file search is worth doing at all.
 *
 * "prefer image file" mode always needs it: a file can outrank
 * already-available embedded art. "prefer embedded" mode (the
 * default) only needs the search as a fallback when there is no
 * usable embedded art, since embedded art always wins over a file
 * when both are present. Skipping the search in that common case
 * avoids several stat()-like file_exists() calls per album.
 */
static inline bool
pf_aa_needs_file_search(bool prefer_file_first, bool have_embedded_jpg)
{
    return prefer_file_first || !have_embedded_jpg;
}

/* Final source choice for one album. Pass have_embedded_jpg = false to
 * disable embedded art altogether (e.g. the "album art" setting is off).
 *
 * `have_file` may be false either because no cover file exists, or
 * because pf_aa_needs_file_search() said the search could be skipped.
 * Both are safe: the search is only skipped when the result cannot
 * depend on it (prefer-embedded with embedded art already available).
 */
static inline enum aa_source
pf_aa_select_source(bool prefer_file_first,
                     bool have_file, bool have_embedded_jpg)
{
    if (prefer_file_first)
    {
        if (have_file)
            return AA_SOURCE_FILE;
        return have_embedded_jpg ? AA_SOURCE_EMBEDDED : AA_SOURCE_NONE;
    }

    if (have_embedded_jpg)
        return AA_SOURCE_EMBEDDED;
    return have_file ? AA_SOURCE_FILE : AA_SOURCE_NONE;
}

#endif /* PICTUREFLOW_AA_SOURCE_H */
