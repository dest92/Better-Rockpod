/* Unit tests for apps/plugins/pictureflow/pf_stage.h */

#include "rbtest.h"
#include "pf_stage.h"

TEST(floor_is_plain_background)
{
    for (int y = 150; y < 240; y++)
        CHECK_EQ(pf_stage_weight(y, 90, 150), 0);
}

TEST(peak_at_glow_and_bounded)
{
    CHECK_EQ(pf_stage_weight(90, 90, 150), PF_STAGE_PEAK);
    CHECK_EQ(pf_stage_weight(0, 90, 150), PF_STAGE_TOP);
    for (int y = -5; y < 260; y++)
    {
        int w = pf_stage_weight(y, 90, 150);
        CHECK(w >= 0 && w <= PF_STAGE_PEAK);
    }
}

TEST(monotonic_on_each_side)
{
    for (int y = 1; y <= 90; y++)
        CHECK(pf_stage_weight(y, 90, 150) >= pf_stage_weight(y - 1, 90, 150));
    for (int y = 91; y <= 150; y++)
        CHECK(pf_stage_weight(y, 90, 150) <= pf_stage_weight(y - 1, 90, 150));
}

TEST(degenerate_layout)
{
    CHECK_EQ(pf_stage_weight(10, 150, 150), 0);
    CHECK_EQ(pf_stage_weight(10, 200, 150), 0);
}

int main(void)
{
    RUN_TEST(floor_is_plain_background);
    RUN_TEST(peak_at_glow_and_bounded);
    RUN_TEST(monotonic_on_each_side);
    RUN_TEST(degenerate_layout);
    return rbtest_report();
}
