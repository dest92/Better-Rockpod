/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Pure multiselection range math (specs/0016-multiselect.md). A selection
 * is stored as (selected item, signed size): size n > 0 covers n items
 * starting at the selected one, size -k covers the k + 1 items ending at
 * it. No firmware includes, so tests/test_multiselect.c runs on the host.
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

#ifndef MULTISELECT_H
#define MULTISELECT_H

#include <stdbool.h>

/* Signed size of the range between the anchor and the selected item. */
static inline int multiselect_size(bool enabled, int selected, int anchor)
{
    int diff = selected - anchor;
    if (!enabled || diff == 0)
        return 1;
    return diff < 0 ? -diff + 1 : -diff;
}

/* First item and item count covered by (selected, size). */
static inline void multiselect_range(int selected, int size,
                                     int *first, int *count)
{
    if (size <= 0)
    {
        *first = selected + size;
        *count = -size + 1;
    }
    else
    {
        *first = selected;
        *count = size;
    }
}

#endif /* MULTISELECT_H */
