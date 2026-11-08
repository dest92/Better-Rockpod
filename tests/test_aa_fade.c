/* Unit tests for apps/gui/skin_engine/aa_fade.h
 * (specs/0015-dynamic-colors-frame-latched-fade.md, A1).
 */

#include <stdbool.h>
#include "rbtest.h"
#include "aa_fade.h"

#define DUR 25

TEST(progress_clamped_and_monotonic)
{
    CHECK_EQ(aa_fade_progress(-5, DUR), 0);
    CHECK_EQ(aa_fade_progress(0, DUR), 0);
    CHECK_EQ(aa_fade_progress(DUR, DUR), 256);
    CHECK_EQ(aa_fade_progress(DUR * 10, DUR), 256);
    int prev = 0;
    for (long e = 0; e <= DUR + 2; e++)
    {
        int p = aa_fade_progress(e, DUR);
        CHECK(p >= prev && p >= 0 && p <= 256);
        prev = p;
    }
    CHECK_EQ(aa_fade_progress(5, 0), 256);
}

TEST(lerp_endpoints_and_bounds)
{
    for (int a = 0; a < 256; a += 17)
        for (int b = 0; b < 256; b += 13)
        {
            CHECK_EQ(aa_fade_lerp8(a, b, 0), a);
            CHECK_EQ(aa_fade_lerp8(a, b, 256), b);
            for (int t = 0; t <= 256; t += 32)
            {
                int v = aa_fade_lerp8(a, b, t);
                int lo = a < b ? a : b, hi = a < b ? b : a;
                CHECK(v >= lo && v <= hi);
            }
        }
}

TEST(pick_tick_latch)
{
    CHECK_EQ(aa_fade_pick_tick(100, 100, 2), 100);
    CHECK_EQ(aa_fade_pick_tick(100, 102, 2), 100);
    CHECK_EQ(aa_fade_pick_tick(100, 103, 2), 103);
    CHECK_EQ(aa_fade_pick_tick(105, 100, 2), 100);  /* latch in future */
    /* tick wrap-around */
    CHECK_EQ(aa_fade_pick_tick(0x7fffffffL, (long)0x80000000UL + 1, 2),
             0x7fffffffL);
}

TEST(frame_spanning_tick_is_consistent)
{
    long start = 1000, frame = 1010;
    int first = aa_fade_progress(aa_fade_pick_tick(frame, 1010, 2) - start, DUR);
    int later = aa_fade_progress(aa_fade_pick_tick(frame, 1011, 2) - start, DUR);
    CHECK_EQ(first, later);
}

int main(void)
{
    RUN_TEST(progress_clamped_and_monotonic);
    RUN_TEST(lerp_endpoints_and_bounds);
    RUN_TEST(pick_tick_latch);
    RUN_TEST(frame_spanning_tick_is_consistent);
    return rbtest_report();
}
