/* Unit tests for apps/gui/skin_engine/aa_color_math.h — the pure
 * color math behind the dynamic-colors complementary accent
 * (specs/0003-dynamic-colors-complementary-accent.md, A1-A4).
 */

#include "rbtest.h"
#include "aa_color_math.h"

#define MIN_CONTRAST 100

static int hue_dist(int a, int b)
{
    int d = a > b ? a - b : b - a;
    return d > 180 ? 360 - d : d;
}

/* A1: HSV round-trip within +/-2 per channel.  Stride 17 keeps hues on
 * exact degree boundaries (ratios k/15 -> 4k degrees), so this checks
 * conversion accuracy rather than hue quantization. */
TEST(hsv_roundtrip)
{
    int failures = 0;
    for (int r = 0; r < 256; r += 17)
        for (int g = 0; g < 256; g += 17)
            for (int b = 0; b < 256; b += 17)
            {
                int h, s, v, r2, g2, b2;
                aa_rgb2hsv(r, g, b, &h, &s, &v);
                aa_hsv2rgb(h, s, v, &r2, &g2, &b2);
                if (r2 < r - 2 || r2 > r + 2 ||
                    g2 < g - 2 || g2 > g + 2 ||
                    b2 < b - 2 || b2 > b + 2)
                {
                    if (failures++ < 5)
                        printf("  roundtrip (%d,%d,%d) -> (%d,%d,%d)\n",
                               r, g, b, r2, g2, b2);
                }
            }
    CHECK_EQ(failures, 0);
}

TEST(hsv_ranges)
{
    int h, s, v;
    aa_rgb2hsv(255, 0, 0, &h, &s, &v);
    CHECK_EQ(h, 0);
    CHECK_EQ(s, 255);
    CHECK_EQ(v, 255);
    aa_rgb2hsv(0, 255, 0, &h, &s, &v);
    CHECK_EQ(h, 120);
    aa_rgb2hsv(0, 0, 255, &h, &s, &v);
    CHECK_EQ(h, 240);
    aa_rgb2hsv(128, 128, 128, &h, &s, &v);
    CHECK_EQ(s, 0);
    CHECK_EQ(v, 128);
}

/* A2: derived complement always clears the contrast bar, over an
 * exhaustive grid of dominant colors (every 8th value per channel). */
TEST(complement_guaranteed_contrast)
{
    int failures = 0;
    for (int r = 0; r < 256; r += 8)
        for (int g = 0; g < 256; g += 8)
            for (int b = 0; b < 256; b += 8)
            {
                int ar, ag, ab;
                aa_derive_complement(r, g, b, MIN_CONTRAST,
                                     &ar, &ag, &ab);
                int dl = aa_luminance(r, g, b);
                int al = aa_luminance(ar, ag, ab);
                int delta = dl > al ? dl - al : al - dl;
                if (delta < MIN_CONTRAST)
                {
                    if (failures++ < 5)
                        printf("  dom (%d,%d,%d) lum %d -> "
                               "acc (%d,%d,%d) lum %d delta %d\n",
                               r, g, b, dl, ar, ag, ab, al, delta);
                }
            }
    CHECK_EQ(failures, 0);
}

/* A3: saturated dominants get a complementary-hue accent... */
TEST(complement_hue_is_complementary)
{
    /* Dominants whose complement can stay saturated at the target
     * luminance: dark saturated colors -> bright complements. */
    static const int doms[][3] = {
        { 120,  20,  20 },   /* dark red -> cyan family */
        {  20,  90,  20 },   /* dark green -> magenta family */
        {  40,  40, 140 },   /* dark blue -> yellow family */
        { 100,  60,  20 },   /* brown/orange -> blue family */
    };
    for (unsigned i = 0; i < sizeof(doms)/sizeof(doms[0]); i++)
    {
        int dh, ds, dv, ar, ag, ab, ah, as, av;
        aa_rgb2hsv(doms[i][0], doms[i][1], doms[i][2], &dh, &ds, &dv);
        CHECK(ds >= 64);   /* premise: dominant is saturated */
        aa_derive_complement(doms[i][0], doms[i][1], doms[i][2],
                             MIN_CONTRAST, &ar, &ag, &ab);
        aa_rgb2hsv(ar, ag, ab, &ah, &as, &av);
        if (as >= 32)   /* hue only meaningful if accent kept chroma */
            CHECK(hue_dist(ah, (dh + 180) % 360) <= 20);
        else
            CHECK(as < 32); /* desaturated to reach target: acceptable */
    }
}

/* ...and near-achromatic dominants get a neutral accent (R2). */
TEST(complement_achromatic_stays_neutral)
{
    static const int doms[][3] = {
        {   0,   0,   0 },
        { 255, 255, 255 },
        { 128, 128, 128 },
        { 120, 124, 118 },   /* slightly off-gray */
    };
    for (unsigned i = 0; i < sizeof(doms)/sizeof(doms[0]); i++)
    {
        int ar, ag, ab, ah, as, av;
        aa_derive_complement(doms[i][0], doms[i][1], doms[i][2],
                             MIN_CONTRAST, &ar, &ag, &ab);
        aa_rgb2hsv(ar, ag, ab, &ah, &as, &av);
        CHECK_EQ(as, 0);
    }
}

/* A4: contrast fix reaches the target; hue preserved while the
 * adjustment stays in the value axis. */
TEST(fix_contrast_reaches_target)
{
    static const int grid[4] = { 0, 85, 170, 255 };
    int failures = 0;
    for (int i = 0; i < 64; i++)
        for (int j = 0; j < 64; j++)
        {
            int dr = grid[(i >> 4) & 3], dg = grid[(i >> 2) & 3],
                db = grid[i & 3];
            int ar = grid[(j >> 4) & 3], ag = grid[(j >> 2) & 3],
                ab = grid[j & 3];
            int fr, fg, fb;
            aa_fix_contrast(ar, ag, ab, dr, dg, db, MIN_CONTRAST,
                            &fr, &fg, &fb);
            int dl = aa_luminance(dr, dg, db);
            int fl = aa_luminance(fr, fg, fb);
            int delta = dl > fl ? dl - fl : fl - dl;
            if (delta < MIN_CONTRAST)
            {
                if (failures++ < 5)
                    printf("  acc (%d,%d,%d) vs dom (%d,%d,%d) -> "
                           "(%d,%d,%d) delta %d\n",
                           ar, ag, ab, dr, dg, db, fr, fg, fb, delta);
            }
        }
    CHECK_EQ(failures, 0);
}

TEST(fix_contrast_preserves_passing_accent)
{
    /* Accent already clears the bar: returned unchanged (R5). */
    int fr, fg, fb;
    aa_fix_contrast(255, 255, 255, 20, 20, 20, MIN_CONTRAST,
                    &fr, &fg, &fb);
    CHECK_EQ(fr, 255);
    CHECK_EQ(fg, 255);
    CHECK_EQ(fb, 255);

    /* (220,80,80) has luminance 122 vs dominant 10: delta 112 >= 100 */
    aa_fix_contrast(220, 80, 80, 10, 10, 10, MIN_CONTRAST,
                    &fr, &fg, &fb);
    CHECK_EQ(fr, 220);
    CHECK_EQ(fg, 80);
    CHECK_EQ(fb, 80);
}

TEST(fix_contrast_preserves_hue)
{
    /* Saturated accent too close to the dominant in luminance: the
     * fix must move value, not hue. */
    static const int accs[][3] = {
        { 180,  60,  60 },   /* red family */
        {  60, 140,  60 },   /* green family */
        {  70,  70, 200 },   /* blue family */
        { 160, 140,  40 },   /* yellow family */
    };
    for (unsigned i = 0; i < sizeof(accs)/sizeof(accs[0]); i++)
    {
        int ah, as, av, fh, fs, fv, fr, fg, fb;
        aa_rgb2hsv(accs[i][0], accs[i][1], accs[i][2], &ah, &as, &av);
        /* dominant with similar luminance to force a fix */
        int lum = aa_luminance(accs[i][0], accs[i][1], accs[i][2]);
        int dom = lum > 255 ? 255 : lum;
        aa_fix_contrast(accs[i][0], accs[i][1], accs[i][2],
                        dom, dom, dom, MIN_CONTRAST, &fr, &fg, &fb);
        aa_rgb2hsv(fr, fg, fb, &fh, &fs, &fv);
        if (fs >= 32)
            CHECK(hue_dist(fh, ah) <= 2);
    }
}

int main(void)
{
    RUN_TEST(hsv_roundtrip);
    RUN_TEST(hsv_ranges);
    RUN_TEST(complement_guaranteed_contrast);
    RUN_TEST(complement_hue_is_complementary);
    RUN_TEST(complement_achromatic_stays_neutral);
    RUN_TEST(fix_contrast_reaches_target);
    RUN_TEST(fix_contrast_preserves_hue);
    RUN_TEST(fix_contrast_preserves_passing_accent);
    return rbtest_report();
}
