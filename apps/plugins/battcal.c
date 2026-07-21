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

static int read_samples(int *secs, int *mv, int *ma, int cap)
{
    int fd = rb->open(BATTERY_LOG, O_RDONLY);
    if (fd < 0)
        return -1;

    char line[BATTCAL_LINE_MAX];
    int n = 0;
    while (rb->read_line(fd, line, sizeof(line)) > 0)
    {
        int s, v, a;
        if (!battcurve_parse_line(line, &s, &v, &a))
            continue;
        if (n >= cap)
        {
            /* keep the most recent samples: drop the oldest */
            for (int i = 1; i < cap; i++)
            {
                secs[i - 1] = secs[i];
                mv[i - 1] = mv[i];
                ma[i - 1] = ma[i];
            }
            n = cap - 1;
        }
        secs[n] = s;
        mv[n] = v;
        ma[n] = a;
        n++;
    }
    rb->close(fd);
    return n;
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
    if (bufsize < (size_t)MAX_SAMPLES * 3 * sizeof(int))
    {
        rb->splash(2 * HZ, "Not enough memory");
        return PLUGIN_ERROR;
    }
    int *secs = (int *)buf;
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
    int rc = battcurve_compute(secs, mv, ma, n, 0, curve, &used_charge);

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

    int vmin = curve[0];
    if (!write_cfg(curve, vmin - 50, vmin))
    {
        rb->splash(3 * HZ, "Could not write cfg");
        return PLUGIN_ERROR;
    }

    rb->splashf(4 * HZ, "Calibrated (%s axis, %d-%d mV). Reboot to apply.",
                used_charge ? "charge" : "time", curve[0],
                curve[BATTCURVE_POINTS - 1]);
    return PLUGIN_OK;
}
