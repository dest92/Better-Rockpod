/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Rockbox contributors
 *
 * Pure ordering algorithm for disk-locality-aware shuffle
 * (specs/0004-disk-locality-shuffle.md).  Standalone on purpose: no
 * firmware includes, so the host unit tests in
 * tests/test_shuffle_locality.c can exercise exactly this logic.
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

#ifndef SHUFFLE_LOCALITY_H
#define SHUFFLE_LOCALITY_H

#include <stdlib.h>
#include <string.h>
#include <limits.h>

/* Key for tracks whose disk position could not be resolved; sorts last */
#define SHUFFLE_KEY_UNKNOWN LONG_MAX

/* Tracks per disk region.  A rebuffer spans roughly 5-15 tracks, so one
 * region covers a few rebuffers worth of physically-close files. */
#define SHUFFLE_REGION_TRACKS 32

/* Number of int slots the caller must provide as region_perm scratch */
#define SHUFFLE_REGION_SLOTS(n) ((n) / SHUFFLE_REGION_TRACKS + 2)

struct shuffle_loc_ent
{
    long key;   /* disk position (e.g. FAT start cluster) */
    int idx;    /* original playlist position */
};

static int shuffle_loc_cmp(const void *pa, const void *pb)
{
    const struct shuffle_loc_ent *a = pa, *b = pb;
    if (a->key != b->key)
        return a->key < b->key ? -1 : 1;
    return a->idx - b->idx;   /* stable tie-break: deterministic order */
}

static inline void shuffle_loc_fy(struct shuffle_loc_ent *e, int len)
{
    for (int i = len - 1; i > 0; i--)
    {
        int j = rand() % (i + 1);
        struct shuffle_loc_ent t = e[j];
        e[j] = e[i];
        e[i] = t;
    }
}

/* Reorder ents[0..n) so that consecutive entries tend to be near each
 * other on disk: sort by key, split the known-key span into fixed-size
 * regions, shuffle the region order and each region's contents.
 * Unknown-key entries form one full-span region at the end (an
 * all-unknown list therefore gets a plain full shuffle).
 *
 * aux must hold n entries, region_perm SHUFFLE_REGION_SLOTS(n) ints.
 * Caller seeds the RNG (srand) beforehand; the result is deterministic
 * for a given (keys, seed). */
static inline void shuffle_locality_order(struct shuffle_loc_ent *ents,
                                          struct shuffle_loc_ent *aux,
                                          int *region_perm, int n)
{
    if (n <= 1)
        return;

    qsort(ents, n, sizeof(*ents), shuffle_loc_cmp);

    /* known-key span [0, known); unknown tail [known, n) */
    int known = n;
    while (known > 0 && ents[known - 1].key == SHUFFLE_KEY_UNKNOWN)
        known--;

    /* shuffle within each fixed-size region of the known span */
    int nr = 0;
    for (int start = 0; start < known; start += SHUFFLE_REGION_TRACKS)
    {
        int len = known - start;
        if (len > SHUFFLE_REGION_TRACKS)
            len = SHUFFLE_REGION_TRACKS;
        shuffle_loc_fy(ents + start, len);
        region_perm[nr++] = start / SHUFFLE_REGION_TRACKS;
    }

    /* shuffle the region order */
    for (int i = nr - 1; i > 0; i--)
    {
        int j = rand() % (i + 1);
        int t = region_perm[j];
        region_perm[j] = region_perm[i];
        region_perm[i] = t;
    }

    /* concatenate regions in their new order */
    int pos = 0;
    for (int r = 0; r < nr; r++)
    {
        int start = region_perm[r] * SHUFFLE_REGION_TRACKS;
        int len = known - start;
        if (len > SHUFFLE_REGION_TRACKS)
            len = SHUFFLE_REGION_TRACKS;
        memcpy(aux + pos, ents + start, len * sizeof(*ents));
        pos += len;
    }
    memcpy(ents, aux, known * sizeof(*ents));

    /* unknown tail: no locality info, plain shuffle over its full span */
    shuffle_loc_fy(ents + known, n - known);
}

/* Apply the permutation described by ents[] to two parallel arrays in
 * place: after ordering, position i should hold the element that was at
 * position ents[i].idx.  a[] is an unsigned long array (playlist
 * indices); b[] is an array of bsz-byte elements (dircache filerefs),
 * with bscratch pointing at one bsz-byte temporary.  Both arrays are
 * permuted identically.  ents[].idx is consumed (set to -1).
 *
 * Cycle-walking keeps this allocation-free and touches only memory the
 * caller already holds, so it never yields — safe to run while holding
 * buflib pointers.  Exhaustively unit-tested in test_shuffle_locality.c. */
static inline void shuffle_locality_apply2(struct shuffle_loc_ent *ents,
                                           int n, unsigned long *a,
                                           char *b, size_t bsz,
                                           char *bscratch)
{
    for (int i = 0; i < n; i++)
    {
        if (ents[i].idx < 0)
            continue;
        int cur = i;
        unsigned long saved_a = a[i];
        memcpy(bscratch, b + (size_t)i * bsz, bsz);
        while (1)
        {
            int src = ents[cur].idx;
            ents[cur].idx = -1;
            if (src == i)
            {
                a[cur] = saved_a;
                memcpy(b + (size_t)cur * bsz, bscratch, bsz);
                break;
            }
            a[cur] = a[src];
            memcpy(b + (size_t)cur * bsz, b + (size_t)src * bsz, bsz);
            cur = src;
        }
    }
}

/* Max tracks considered "recently played" by the repeat anti-repeat */
#define SHUFFLE_ANTIREPEAT_MAX 10

/* Repeat Shuffle anti-repeat helper
 * (specs/0005-repeat-shuffle-antirepeat.md): count how many of the
 * first k entries of indices[] appear in recent[0..k).  Playlist index
 * values are unique, so raw equality identifies a track. */
static inline int shuffle_repeat_overlap(const unsigned long *indices, int n,
                                         const unsigned long *recent, int k)
{
    int window = k < n ? k : n;
    int overlap = 0;

    for (int i = 0; i < window; i++)
        for (int j = 0; j < k; j++)
            if (indices[i] == recent[j])
            {
                overlap++;
                break;
            }
    return overlap;
}

#endif /* SHUFFLE_LOCALITY_H */
