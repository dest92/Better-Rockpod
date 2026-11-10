/* Unit tests for apps/ui_boost.h (specs/0020-ui-interaction-boost.md). */

#include <stdbool.h>
#include "rbtest.h"
#include "ui_boost.h"

TEST(not_boosted_passes_timeout_through)
{
    CHECK_EQ(ui_boost_wait(UI_BOOST_TIMEOUT_BLOCK, false, 0), UI_BOOST_TIMEOUT_BLOCK);
    CHECK_EQ(ui_boost_wait(0, false, 0), 0);
    CHECK_EQ(ui_boost_wait(50, false, 0), 50);
}

TEST(boosted_blocking_wait_is_bounded)
{
    CHECK_EQ(ui_boost_wait(UI_BOOST_TIMEOUT_BLOCK, true, 30), 30);
    CHECK_EQ(ui_boost_wait(UI_BOOST_TIMEOUT_BLOCK, true, 1), 1);
}

TEST(boosted_finite_wait)
{
    CHECK_EQ(ui_boost_wait(10, true, 30), 10);   /* caller's timeout comes first */
    CHECK_EQ(ui_boost_wait(50, true, 30), 30);   /* boost deadline comes first */
    CHECK_EQ(ui_boost_wait(0, true, 30), 0);     /* no-block stays no-block */
}

TEST(expired_boost_does_not_wait)
{
    CHECK_EQ(ui_boost_wait(UI_BOOST_TIMEOUT_BLOCK, true, -5), 0);
    CHECK_EQ(ui_boost_wait(50, true, -1), 0);
}

/* fake clock and button queue: events[i] arrives at times[i] */
static long fake_now;
static long ev_time[8], ev_button[8];
static int ev_count, ev_next;
static int boost_count, boost_calls;
static long fake_get(int ticks)
{
    long limit = ticks < 0 ? 1L << 30 : fake_now + ticks;
    if (ev_next < ev_count && ev_time[ev_next] <= limit)
    {
        if (ev_time[ev_next] > fake_now)
            fake_now = ev_time[ev_next];
        return ev_button[ev_next++];
    }
    CHECK(ticks >= 0);      /* a blocking wait with no event would hang */
    if (ticks < 0)
        return 0;
    fake_now = limit;
    return 0;
}
static long fake_clock(void) { return fake_now; }
static void fake_boost(bool on) { boost_count += on ? 1 : -1; boost_calls++; }
static bool fake_is_input(long b) { return b > 0; }
static struct ui_boost_ops ops = { fake_get, fake_clock, fake_boost,
                                   fake_is_input, true, 100 };
static void reset(void)
{
    fake_now = 1000; ev_count = ev_next = 0; boost_count = boost_calls = 0;
}
static void push(long t, long b) { ev_time[ev_count] = t; ev_button[ev_count++] = b; }

TEST(input_boosts_and_idle_releases)
{
    struct ui_boost_state st = { false, 0 };
    reset();
    push(1010, 5);                  /* a button press */
    push(1500, 6);                  /* another, long after the hold */
    CHECK_EQ(ui_boost_get_button(&st, &ops, UI_BOOST_TIMEOUT_BLOCK), 5);
    CHECK(st.boosted);
    CHECK_EQ(boost_count, 1);
    /* blocking wait: wakes at the deadline, releases, keeps waiting */
    CHECK_EQ(ui_boost_get_button(&st, &ops, UI_BOOST_TIMEOUT_BLOCK), 6);
    CHECK_EQ(boost_calls, 3);       /* boost, release at 1110, boost again */
    CHECK_EQ(boost_count, 1);
    CHECK(st.boosted);
}

TEST(finite_timeout_keeps_its_length)
{
    struct ui_boost_state st = { true, 1030 };
    reset();
    boost_count = 1;
    CHECK_EQ(ui_boost_get_button(&st, &ops, 80), 0);
    CHECK_EQ(fake_now, 1080);       /* full 80 ticks waited */
    CHECK(!st.boosted);             /* released at 1030 on the way */
    CHECK_EQ(boost_count, 0);
}

TEST(noblock_returns_immediately)
{
    struct ui_boost_state st = { false, 0 };
    reset();
    CHECK_EQ(ui_boost_get_button(&st, &ops, 0), 0);
    CHECK_EQ(fake_now, 1000);
    CHECK_EQ(boost_calls, 0);
}

TEST(sys_events_do_not_boost)
{
    struct ui_boost_state st = { false, 0 };
    reset();
    push(1005, -3);                 /* not user input */
    CHECK_EQ(ui_boost_get_button(&st, &ops, UI_BOOST_TIMEOUT_BLOCK), -3);
    CHECK(!st.boosted);
    CHECK_EQ(boost_calls, 0);
}

TEST(disabled_setting_releases)
{
    struct ui_boost_state st = { true, 5000 };
    struct ui_boost_ops off = ops;
    reset();
    boost_count = 1;
    off.enabled = false;
    push(1001, 7);
    CHECK_EQ(ui_boost_get_button(&st, &off, UI_BOOST_TIMEOUT_BLOCK), 7);
    CHECK(!st.boosted);
    CHECK_EQ(boost_count, 0);
}

int main(void)
{
    RUN_TEST(not_boosted_passes_timeout_through);
    RUN_TEST(boosted_blocking_wait_is_bounded);
    RUN_TEST(boosted_finite_wait);
    RUN_TEST(expired_boost_does_not_wait);
    RUN_TEST(input_boosts_and_idle_releases);
    RUN_TEST(finite_timeout_keeps_its_length);
    RUN_TEST(noblock_returns_immediately);
    RUN_TEST(sys_events_do_not_boost);
    RUN_TEST(disabled_setting_releases);
    return rbtest_report();
}
