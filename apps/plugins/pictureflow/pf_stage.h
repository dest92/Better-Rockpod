/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Cover Flow "stage" background shape: how much of the theme accent each
 * row gets. No firmware includes, so tests/test_pf_stage.c runs on the
 * host.
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

#ifndef PF_STAGE_H
#define PF_STAGE_H

#define PF_STAGE_PEAK 72   /* accent weight (of 256) behind the covers */
#define PF_STAGE_TOP  24   /* accent weight at the top row */

/* Accent weight 0..256 for row y. It rises from the top to a soft light
 * band at glow_y, then falls to 0 at floor_y; the floor (reflections)
 * is the plain background, so reflections fading to it stay seamless. */
static inline int pf_stage_weight(int y, int glow_y, int floor_y)
{
    if (y >= floor_y || glow_y >= floor_y)
        return 0;
    if (y <= 0)
        return PF_STAGE_TOP;
    if (y <= glow_y)
        return PF_STAGE_TOP + (PF_STAGE_PEAK - PF_STAGE_TOP) * y / glow_y;
    /* smooth fall: quadratic ease-out to the floor */
    int span = floor_y - glow_y;
    int d = floor_y - y;
    return PF_STAGE_PEAK * d * d / (span * span);
}

#endif /* PF_STAGE_H */
