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
 * Pure integer color math for dynamic album-art colors
 * (specs/0003-dynamic-colors-complementary-accent.md).  Standalone on
 * purpose: no firmware includes, so the host unit tests in
 * tests/test_aa_color_math.c can exercise exactly this logic.
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

#ifndef AA_COLOR_MATH_H
#define AA_COLOR_MATH_H

/* Saturation (0-255) below which a color counts as achromatic and the
 * derived accent stays neutral instead of inventing a hue. */
#define AA_ACHROMATIC_SAT 32

/* Cap for the derived accent's starting saturation, so complements of
 * fully saturated art don't come out neon. */
#define AA_ACCENT_MAX_SAT 240

/* Same weights as the skin engine's luminance: (77, 150, 29) / 256 */
static inline int aa_luminance(int r8, int g8, int b8)
{
    return (r8 * 77 + g8 * 150 + b8 * 29) >> 8;
}

/* RGB (0-255 each) to integer HSV: h 0..359, s/v 0..255 */
static inline void aa_rgb2hsv(int r, int g, int b, int *h, int *s, int *v)
{
    int max = r > g ? (r > b ? r : b) : (g > b ? g : b);
    int min = r < g ? (r < b ? r : b) : (g < b ? g : b);
    int delta = max - min;
    int num, base, hue;

    *v = max;
    *s = max > 0 ? (delta * 255 + max / 2) / max : 0;

    if (delta == 0)
    {
        *h = 0;
        return;
    }

    if (max == r)
    {
        num = g - b;
        base = 0;
    }
    else if (max == g)
    {
        num = b - r;
        base = 120;
    }
    else
    {
        num = r - g;
        base = 240;
    }

    if (num >= 0)
        hue = base + (num * 60 + delta / 2) / delta;
    else
        hue = base - ((-num * 60 + delta / 2) / delta);
    if (hue < 0)
        hue += 360;
    *h = hue;
}

/* Integer HSV back to RGB.  One rounded division per channel
 * (denominator 255*60) keeps the per-channel error at +/-0.5. */
static inline void aa_hsv2rgb(int h, int s, int v, int *r, int *g, int *b)
{
    if (s <= 0)
    {
        *r = *g = *b = v;
        return;
    }

    int sector = (h / 60) % 6;
    int rem = h % 60;
    int p = (v * (255 - s) * 60 + 7650) / 15300;
    int q = (v * (15300 - s * rem) + 7650) / 15300;
    int t = (v * (15300 - s * (60 - rem)) + 7650) / 15300;

    switch (sector)
    {
    case 0:  *r = v; *g = t; *b = p; break;
    case 1:  *r = q; *g = v; *b = p; break;
    case 2:  *r = p; *g = v; *b = t; break;
    case 3:  *r = p; *g = q; *b = v; break;
    case 4:  *r = t; *g = p; *b = v; break;
    default: *r = v; *g = p; *b = q; break;
    }
}

static inline int aa_lum_hsv(int h, int s, int v)
{
    int r, g, b;
    aa_hsv2rgb(h, s, v, &r, &g, &b);
    return aa_luminance(r, g, b);
}

/* Adjust (s, v) at fixed hue until the luminance is >= target (up) or
 * <= target (!up).  Moves v first; desaturates only when v alone
 * cannot get there.  Luminance is monotone in v, and monotone in s at
 * v=255, so both binary searches are sound and the result meets the
 * target by construction (s=0/v=255 is white, v=0 is black). */
static inline void aa_solve_sv(int h, int *s, int *v, int target, int up)
{
    int lo, hi, mid;

    if (up)
    {
        if (aa_lum_hsv(h, *s, 255) < target)
        {
            /* v maxed out: keep as much chroma as still reaches it */
            lo = 0;
            hi = *s;
            while (lo < hi)
            {
                mid = (lo + hi + 1) / 2;
                if (aa_lum_hsv(h, mid, 255) >= target)
                    lo = mid;
                else
                    hi = mid - 1;
            }
            *s = lo;
            *v = 255;
        }
        else
        {
            /* smallest v that reaches the target */
            lo = 0;
            hi = 255;
            while (lo < hi)
            {
                mid = (lo + hi) / 2;
                if (aa_lum_hsv(h, *s, mid) >= target)
                    hi = mid;
                else
                    lo = mid + 1;
            }
            *v = lo;
        }
    }
    else
    {
        /* largest v that stays at or below the target */
        lo = 0;
        hi = 255;
        while (lo < hi)
        {
            mid = (lo + hi + 1) / 2;
            if (aa_lum_hsv(h, *s, mid) <= target)
                lo = mid;
            else
                hi = mid - 1;
        }
        *v = lo;
    }
}

/* Pick the luminance target on the far side of the dominant color and
 * report the direction.  Clamped to the representable range. */
static inline int aa_contrast_target(int dom_lum, int min_contrast, int *up)
{
    int target;

    *up = dom_lum < 128;
    target = *up ? dom_lum + min_contrast : dom_lum - min_contrast;
    if (target < 0)
        target = 0;
    if (target > 255)
        target = 255;
    return target;
}

/* Derive an accent from the dominant color alone: complementary hue,
 * saturation retained (capped), value solved so the luminance delta
 * vs. the dominant is >= min_contrast by construction.  Near-achromatic
 * dominants get a neutral accent at the target luminance ((t,t,t) has
 * luminance exactly t with the 77/150/29 weights). */
static inline void aa_derive_complement(int dom_r, int dom_g, int dom_b,
                                        int min_contrast,
                                        int *acc_r, int *acc_g, int *acc_b)
{
    int h, s, v, up, target;

    aa_rgb2hsv(dom_r, dom_g, dom_b, &h, &s, &v);
    target = aa_contrast_target(aa_luminance(dom_r, dom_g, dom_b),
                                min_contrast, &up);

    if (s < AA_ACHROMATIC_SAT)
    {
        *acc_r = *acc_g = *acc_b = target;
        return;
    }

    h = (h + 180) % 360;
    if (s > AA_ACCENT_MAX_SAT)
        s = AA_ACCENT_MAX_SAT;
    aa_solve_sv(h, &s, &v, target, up);
    aa_hsv2rgb(h, s, v, acc_r, acc_g, acc_b);
}

/* Bring an extracted accent up to contrast against the dominant while
 * preserving its hue: value first, saturation only as a last resort.
 * Accents that already clear the bar pass through untouched. */
static inline void aa_fix_contrast(int acc_r, int acc_g, int acc_b,
                                   int dom_r, int dom_g, int dom_b,
                                   int min_contrast,
                                   int *out_r, int *out_g, int *out_b)
{
    int dom_lum = aa_luminance(dom_r, dom_g, dom_b);
    int acc_lum = aa_luminance(acc_r, acc_g, acc_b);
    int delta = dom_lum > acc_lum ? dom_lum - acc_lum : acc_lum - dom_lum;
    int h, s, v, up, target;

    if (delta >= min_contrast)
    {
        *out_r = acc_r;
        *out_g = acc_g;
        *out_b = acc_b;
        return;
    }

    target = aa_contrast_target(dom_lum, min_contrast, &up);
    aa_rgb2hsv(acc_r, acc_g, acc_b, &h, &s, &v);
    if (s == 0)
    {
        *out_r = *out_g = *out_b = target;
        return;
    }
    aa_solve_sv(h, &s, &v, target, up);
    aa_hsv2rgb(h, s, v, out_r, out_g, out_b);
}

#endif /* AA_COLOR_MATH_H */
