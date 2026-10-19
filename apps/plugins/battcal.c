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
 * On-device battery calibration: turn a battery_bench.txt log into a
 * calibrated battery_levels.cfg (specs/0008-ondevice-battery-calibration.md).
 * The curve math lives in the pure, host-tested lib/battcurve.h; this
 * plugin only does the file I/O and user interaction.
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

#include "plugin.h"
#include "lib/battcurve.h"

#define BATTERY_LOG   HOME_DIR "/battery_bench.txt"
#define LEVELS_CFG    ROCKBOX_DIR "/battery_levels.cfg"
#define MAX_SAMPLES   4096
#define BATTCAL_LINE_MAX      160

/* Read parsed samples into secs/mv/ma, keeping the most recent `cap`
 * (the low-voltage end of the discharge is what matters).  Writing into a
 * ring makes each line O(1) instead of shifting the whole array; when the
 * log overflows, battcurve_ring_rotate() (pure, host-tested) puts the
 * window back in chronological order. */
static int read_samples(int *secs, int *mv, int *ma, int cap)
{
    int fd = rb->open(BATTERY_LOG, O_RDONLY);
    if (fd < 0)
        return -1;

    char line[BATTCAL_LINE_MAX];
    int total = 0, head = 0;
    while (rb->read_line(fd, line, sizeof(line)) > 0)
    {
        int s, v, a;
        if (!battcurve_parse_line(line, &s, &v, &a))
            continue;
        secs[head] = s;
        mv[head] = v;
        ma[head] = a;
        head = (head + 1) % cap;
        total++;
    }
    rb->close(fd);

    if (total <= cap)
        return total;   /* no wrap: already in order at [0, total) */

    battcurve_ring_rotate(secs, mv, ma, cap, head);
    return cap;
}

static bool write_cfg(const unsigned short *curve, int shutoff, int disksafe)
{
    int fd = rb->open(LEVELS_CFG, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0)
        return false;

    rb->fdprintf(fd,
        "# Generated on-device by the battcal plugin from %s.\n"
        "# Battery voltage(millivolt) at {0%%, 10%%, ..., 100%%} discharging\n"
        "discharge: {", BATTERY_LOG);
    for (int i = 0; i < BATTCURVE_POINTS; i++)
        rb->fdprintf(fd, "%d%s", curve[i],
                     i < BATTCURVE_POINTS - 1 ? ", " : "");
    rb->fdprintf(fd, "}\n\n"
        "# Uncomment to also override the safety thresholds:\n"
        "#shutoff: %d\n#disksafe: %d\n", shutoff, disksafe);
    rb->close(fd);
    return true;
}

enum plugin_status plugin_start(const void *parameter)
{
    (void)parameter;

    size_t bufsize;
    char *buf = rb->plugin_get_buffer(&bufsize);
    /* axis (long) first for alignment, then the three int arrays */
    if (bufsize < (size_t)MAX_SAMPLES * (sizeof(long) + 3 * sizeof(int)))
    {
        rb->splash(2 * HZ, "Not enough memory");
        return PLUGIN_ERROR;
    }
    long *axis = (long *)buf;
    int *secs = (int *)(axis + MAX_SAMPLES);
    int *mv = secs + MAX_SAMPLES;
    int *ma = mv + MAX_SAMPLES;

    int n = read_samples(secs, mv, ma, MAX_SAMPLES);
    if (n < 0)
    {
        rb->splashf(3 * HZ, "No %s", BATTERY_LOG);
        return PLUGIN_ERROR;
    }

    unsigned short curve[BATTCURVE_POINTS];
    int used_charge = 0;
    int rc = battcurve_compute(secs, mv, ma, n, 0, axis, MAX_SAMPLES,
                               curve, &used_charge);

    if (rc == BATTCURVE_TOO_SHORT)
    {
        rb->splashf(3 * HZ, "Log too short (%d samples)", n);
        return PLUGIN_ERROR;
    }
    if (rc == BATTCURVE_NONMONOTONIC)
    {
        rb->splash(3 * HZ, "Noisy/interrupted log - not calibrated");
        return PLUGIN_ERROR;
    }
    if (rc != BATTCURVE_OK)
    {
        rb->splash(3 * HZ, "Could not compute curve");
        return PLUGIN_ERROR;
    }

    if (rb->file_exists(LEVELS_CFG) &&
        !rb->yesno_pop("Overwrite existing battery_levels.cfg?"))
        return PLUGIN_OK;

    /* Suggest thresholds at/above the lowest voltage actually reached
     * (curve[0]); never advise running below what the bench validated. */
    int vmin = curve[0];
    if (!write_cfg(curve, vmin, vmin + 50))
    {
        rb->splash(3 * HZ, "Could not write cfg");
        return PLUGIN_ERROR;
    }

    rb->splashf(4 * HZ, "Calibrated (%s axis, %d-%d mV). Reboot to apply.",
                used_charge ? "charge" : "time", curve[0],
                curve[BATTCURVE_POINTS - 1]);
    return PLUGIN_OK;
}
