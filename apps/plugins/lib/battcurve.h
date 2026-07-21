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
 * Pure battery discharge-curve computation for the on-device
 * calibration plugin (specs/0008-ondevice-battery-calibration.md).
 * Mirrors tools/battcal/battcal.py — keep the two in sync.  No plugin
 * API dependency, so tests/test_battcurve.c exercises it on the host.
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

#ifndef BATTCURVE_H
#define BATTCURVE_H

#include <stdbool.h>

#define BATTCURVE_POINTS 11

enum {
    BATTCURVE_OK = 0,
    BATTCURVE_TOO_SHORT,     /* fewer than 11 samples */
    BATTCURVE_NO_SPAN,       /* zero elapsed time/charge */
    BATTCURVE_NONMONOTONIC   /* noisy/interrupted run */
};

/* Parse one battery_bench.txt data row into seconds / millivolts /
 * milliamps.  *ma is set to -1 when there is no current column.  Returns
 * false for comment (#), blank, or malformed lines.  Fields are
 * comma-separated (see apps/plugins/battery_bench.c); the current column,
 * when present, is the 6th field and is a plain integer (the trailing
 * charger/USB flags are single characters). */
static inline bool battcurve_parse_line(const char *line, int *secs,
                                        int *mv, int *ma)
{
    while (*line == ' ' || *line == '\t')
        line++;
    if (*line == '\0' || *line == '#' || *line == '\n' || *line == '\r')
        return false;

    int field = 0;
    long vals[6];
    bool numeric[6];
    const char *p = line;

    while (field < 6)
    {
        while (*p == ' ' || *p == '\t')
            p++;
        const char *start = p;
        long sign = 1, acc = 0;
        bool any = false;
        if (*p == '-') { sign = -1; p++; }
        while (*p >= '0' && *p <= '9')
        {
            acc = acc * 10 + (*p - '0');
            any = true;
            p++;
        }
        vals[field] = sign * acc;
        numeric[field] = any && (start != p);
        field++;
        /* advance to next comma */
        while (*p != ',' && *p != '\0' && *p != '\n')
            p++;
        if (*p != ',')
            break;
        p++;
    }

    /* need at least: time, seconds, level, timeleft, voltage */
    if (field < 5 || !numeric[1] || !numeric[4])
        return false;

    *secs = (int)vals[1];
    *mv = (int)vals[4];
    *ma = (field >= 6 && numeric[5]) ? (int)vals[5] : -1;
    return true;
}

static inline int battcurve_interp(const long *axis, const int *volt,
                                   int n, long target)
{
    if (target <= axis[0])
        return volt[0];
    if (target >= axis[n - 1])
        return volt[n - 1];
    int lo = 0;
    while (lo < n - 1 && axis[lo + 1] < target)
        lo++;
    long x0 = axis[lo], x1 = axis[lo + 1];
    int v0 = volt[lo], v1 = volt[lo + 1];
    if (x1 == x0)
        return v0;
    return v0 + (int)(((long)(v1 - v0) * (target - x0)) / (x1 - x0));
}

/* Compute the 11-point ascending discharge table (empty..full mV) from a
 * bench sample series.  Percentage axis: integrated current (charge) when
 * every sample has it and force_time is false, else elapsed time.  The
 * log's own level column is deliberately never used.  *used_charge is set
 * to 1/0 for the axis chosen. */
static inline int battcurve_compute(const int *secs, const int *mv,
                                    const int *ma, int n, int force_time,
                                    unsigned short out[BATTCURVE_POINTS],
                                    int *used_charge)
{
    if (n < BATTCURVE_POINTS)
        return BATTCURVE_TOO_SHORT;

    bool have_current = !force_time;
    for (int i = 0; have_current && i < n; i++)
        if (ma[i] < 0)
            have_current = false;

    /* cumulative consumed axis (charge or time), non-decreasing from 0 */
    static long axis[8192];
    if (n > (int)(sizeof(axis) / sizeof(axis[0])))
        n = (int)(sizeof(axis) / sizeof(axis[0]));
    axis[0] = 0;
    for (int i = 1; i < n; i++)
    {
        long dt = secs[i] - secs[i - 1];
        long step;
        if (have_current)
            step = (long)(ma[i] + ma[i - 1]) * dt / 2;  /* mA-seconds */
        else
            step = dt;
        if (step < 0)
            step = 0;
        axis[i] = axis[i - 1] + step;
    }

    long total = axis[n - 1];
    if (total <= 0)
        return BATTCURVE_NO_SPAN;

    /* voltage at each 10% of remaining charge, empty(0%)..full(100%) */
    for (int p = 0; p <= 100; p += 10)
    {
        long target = (long)(100 - p) * total / 100;
        out[p / 10] = (unsigned short)battcurve_interp(axis, mv, n, target);
    }

    for (int i = 0; i < BATTCURVE_POINTS - 1; i++)
        if (out[i] >= out[i + 1])
            return BATTCURVE_NONMONOTONIC;

    if (used_charge)
        *used_charge = have_current ? 1 : 0;
    return BATTCURVE_OK;
}

#endif /* BATTCURVE_H */
