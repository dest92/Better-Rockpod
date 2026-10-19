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
 * Pure debounce for the low-battery shutdown decision
 * (specs/0007-lowbatt-sag-shutdown-debounce.md).  A flash-modded iPod's
 * SD/iFlash controller briefly sags the cell voltage when it wakes to
 * refill the audio buffer; near empty that transient must not be
 * mistaken for real depletion and power the device off.  Kept
 * standalone (no firmware includes) so the host unit tests in
 * tests/test_lowbatt_debounce.c exercise exactly this logic.
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

#ifndef LOWBATT_DEBOUNCE_H
#define LOWBATT_DEBOUNCE_H

#include <stdbool.h>

/* Consecutive below-shutoff samples required before actually powering
 * off.  The power thread steps every POWER_THREAD_STEP_TICKS (HZ/2), so
 * 4 samples ≈ 2 s — long enough to ride out an SD-wake sag, negligible
 * against real depletion.  Overridable per target. */
#ifndef LOWBATT_SHUTDOWN_SAMPLES
#define LOWBATT_SHUTDOWN_SAMPLES 4
#endif

/* Advance the debounce with one sample.  *consec counts consecutive
 * below-shutoff samples (reset by any healthy sample).  Returns true
 * only once the streak reaches LOWBATT_SHUTDOWN_SAMPLES, i.e. the low
 * voltage has persisted rather than being a transient sag. */
static inline bool lowbatt_shutdown_debounce(bool below_shutoff, int *consec)
{
    if (!below_shutoff)
    {
        *consec = 0;
        return false;
    }

    if (*consec < LOWBATT_SHUTDOWN_SAMPLES)
        (*consec)++;

    return *consec >= LOWBATT_SHUTDOWN_SAMPLES;
}

#endif /* LOWBATT_DEBOUNCE_H */
