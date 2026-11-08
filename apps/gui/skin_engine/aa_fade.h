/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Pure fade math for dynamic album-art colors
 * (specs/0015-dynamic-colors-frame-latched-fade.md). No firmware
 * includes, so tests/test_aa_fade.c can exercise it on the host.
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

#ifndef AA_FADE_H
#define AA_FADE_H

/* Fade position 0..256 after `elapsed` ticks of a `duration`-tick fade. */
static inline int aa_fade_progress(long elapsed, long duration)
{
    if (duration <= 0 || elapsed >= duration)
        return 256;
    if (elapsed <= 0)
        return 0;
    return (int)((elapsed * 256) / duration);
}

/* Blend one 8-bit channel: t = 0 gives a, t = 256 gives b. */
static inline int aa_fade_lerp8(int a, int b, int t)
{
    return a + ((b - a) * t) / 256;
}

/* Tick to evaluate a fade at: the frame's latched tick while it is at
 * most max_age ticks old, otherwise now. */
static inline long aa_fade_pick_tick(long frame_tick, long now, long max_age)
{
    unsigned long age = (unsigned long)now - (unsigned long)frame_tick;
    return age <= (unsigned long)max_age ? frame_tick : now;
}

#endif /* AA_FADE_H */
