/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Per-album random keys for album-order shuffling
 * (specs/0012-tagtree-random-album-order.md). No firmware includes, so
 * tests/test_album_shuffle.c can exercise it on the host.
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

#ifndef ALBUM_SHUFFLE_H
#define ALBUM_SHUFFLE_H

#include <stdint.h>

static inline uint32_t album_shuffle_key(const char *name, uint32_t seed)
{
    uint32_t h = 2166136261u;
    if (name)
    {
        while (*name)
        {
            h ^= (unsigned char)*name++;
            h *= 16777619u;
        }
    }
    h ^= seed;
    h ^= h >> 16;
    h *= 0x85ebca6bu;
    h ^= h >> 13;
    h *= 0xc2b2ae35u;
    h ^= h >> 16;
    return h;
}

static inline int album_shuffle_cmp(uint32_t a, uint32_t b)
{
    return (a > b) - (a < b);
}

#endif /* ALBUM_SHUFFLE_H */
