/* Unit tests for apps/gui/multiselect.h (specs/0016-multiselect.md). */

#include <stdbool.h>
#include "rbtest.h"
#include "multiselect.h"

/* The range always spans anchor..selected inclusive, in either direction. */
TEST(range_covers_anchor_to_selected)
{
    for (int anchor = 0; anchor < 12; anchor++)
        for (int sel = 0; sel < 12; sel++)
        {
            int first, count;
            int size = multiselect_size(true, sel, anchor);
            multiselect_range(sel, size, &first, &count);
            int lo = sel < anchor ? sel : anchor;
            int hi = sel < anchor ? anchor : sel;
            CHECK_EQ(first, lo);
            CHECK_EQ(count, hi - lo + 1);
        }
}

TEST(disabled_is_single)
{
    int first, count;
    CHECK_EQ(multiselect_size(false, 7, 2), 1);
    multiselect_range(7, 1, &first, &count);
    CHECK_EQ(first, 7);
    CHECK_EQ(count, 1);
}

TEST(zero_size_is_one_item)
{
    int first, count;
    multiselect_range(4, 0, &first, &count);
    CHECK_EQ(first, 4);
    CHECK_EQ(count, 1);
}

int main(void)
{
    RUN_TEST(range_covers_anchor_to_selected);
    RUN_TEST(disabled_is_single);
    RUN_TEST(zero_size_is_one_item);
    return rbtest_report();
}
