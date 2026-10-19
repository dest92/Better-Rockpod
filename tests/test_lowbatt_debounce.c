/* Unit tests for firmware/export/lowbatt_debounce.h — the pure
 * consecutive-sample gate that keeps a transient SD-wake voltage sag
 * from triggering a premature low-battery shutdown
 * (specs/0007-lowbatt-sag-shutdown-debounce.md, A1-A3).
 */

#include "rbtest.h"
#include "lowbatt_debounce.h"

/* A1: an isolated sag (1 or 2 below-shutoff samples) never triggers */
TEST(sag_rejected)
{
    int consec = 0;
    /* healthy samples */
    CHECK(!lowbatt_shutdown_debounce(false, &consec));
    CHECK(!lowbatt_shutdown_debounce(false, &consec));
    /* one-sample sag */
    CHECK(!lowbatt_shutdown_debounce(true, &consec));
    CHECK(!lowbatt_shutdown_debounce(false, &consec));
    /* two-sample sag */
    CHECK(!lowbatt_shutdown_debounce(true, &consec));
    CHECK(!lowbatt_shutdown_debounce(true, &consec));
    CHECK(!lowbatt_shutdown_debounce(false, &consec));
}

/* A2: N consecutive below-shutoff samples trigger exactly on the Nth */
TEST(sustained_triggers_on_nth)
{
    int consec = 0;
    for (int i = 1; i < LOWBATT_SHUTDOWN_SAMPLES; i++)
        CHECK(!lowbatt_shutdown_debounce(true, &consec));
    /* the Nth consecutive sample fires */
    CHECK(lowbatt_shutdown_debounce(true, &consec));
    /* and stays fired while it remains below */
    CHECK(lowbatt_shutdown_debounce(true, &consec));
}

/* A3: any healthy sample resets the streak */
TEST(reset_on_healthy)
{
    int consec = 0;
    /* almost there... */
    for (int i = 1; i < LOWBATT_SHUTDOWN_SAMPLES; i++)
        CHECK(!lowbatt_shutdown_debounce(true, &consec));
    /* a recovery sample resets */
    CHECK(!lowbatt_shutdown_debounce(false, &consec));
    CHECK_EQ(consec, 0);
    /* now needs a full fresh streak again */
    for (int i = 1; i < LOWBATT_SHUTDOWN_SAMPLES; i++)
        CHECK(!lowbatt_shutdown_debounce(true, &consec));
    CHECK(lowbatt_shutdown_debounce(true, &consec));
}

/* counter is bounded (no unbounded growth while stuck below) */
TEST(counter_bounded)
{
    int consec = 0;
    for (int i = 0; i < 1000; i++)
        lowbatt_shutdown_debounce(true, &consec);
    CHECK(consec <= LOWBATT_SHUTDOWN_SAMPLES);
}

int main(void)
{
    RUN_TEST(sag_rejected);
    RUN_TEST(sustained_triggers_on_nth);
    RUN_TEST(reset_on_healthy);
    RUN_TEST(counter_bounded);
    return rbtest_report();
}
