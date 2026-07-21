/* Unit tests for apps/plugins/lib/battcurve.h — the pure on-device
 * battery discharge-curve computation, mirroring tools/battcal
 * (specs/0008-ondevice-battery-calibration.md, A1-A4).
 */

#include "rbtest.h"
#include "battcurve.h"

TEST(parse_line_basic)
{
    int secs, mv, ma;
    /* battery_bench row without current column */
    CHECK(battcurve_parse_line(
        "00:01:00,  00060,     090%,     00:00,         4068", &secs, &mv, &ma));
    CHECK_EQ(secs, 60);
    CHECK_EQ(mv, 4068);
    CHECK_EQ(ma, -1);
}

TEST(parse_line_with_current)
{
    int secs, mv, ma;
    CHECK(battcurve_parse_line(
        "00:01:00,  00060,     090%,     00:00,         4068,         0110",
        &secs, &mv, &ma));
    CHECK_EQ(mv, 4068);
    CHECK_EQ(ma, 110);
}

TEST(parse_line_rejects_junk)
{
    int secs, mv, ma;
    CHECK(!battcurve_parse_line("# comment", &secs, &mv, &ma));
    CHECK(!battcurve_parse_line("", &secs, &mv, &ma));
    CHECK(!battcurve_parse_line("too, few, fields", &secs, &mv, &ma));
}

TEST(compute_linear_time)
{
    int n = 101;
    static int secs[101], mv[101], ma[101];
    int v_full = 4120, v_empty = 3600;
    for (int i = 0; i < n; i++) {
        secs[i] = i * 60;
        mv[i] = v_full - (v_full - v_empty) * i / 100;
        ma[i] = -1;
    }
    unsigned short out[BATTCURVE_POINTS];
    int used_charge = -1;
    static long axis[101];
    int rc = battcurve_compute(secs, mv, ma, n, 0, axis, 101, out, &used_charge);
    CHECK_EQ(rc, BATTCURVE_OK);
    CHECK_EQ(used_charge, 0);
    for (int i = 0; i < BATTCURVE_POINTS; i++) {
        int exp = v_empty + (v_full - v_empty) * i / 10;
        CHECK(out[i] >= exp - 1 && out[i] <= exp + 1);
    }
    /* ascending, empty..full */
    for (int i = 0; i < BATTCURVE_POINTS - 1; i++)
        CHECK(out[i] < out[i + 1]);
}

TEST(compute_charge_axis_differs)
{
    int n = 101;
    static int secs[101], mv[101], ma[101];
    int v_full = 4120, v_empty = 3600;
    for (int i = 0; i < n; i++) {
        secs[i] = i * 60;
        mv[i] = v_full - (v_full - v_empty) * i / 100;
        ma[i] = (i < 50) ? 300 : 60;   /* front-loaded draw */
    }
    unsigned short curve_c[BATTCURVE_POINTS], curve_t[BATTCURVE_POINTS];
    int uc = -1, ut = -1;
    static long axis[101];
    CHECK_EQ(battcurve_compute(secs, mv, ma, n, 0, axis, 101, curve_c, &uc), BATTCURVE_OK);
    CHECK_EQ(battcurve_compute(secs, mv, ma, n, 1, axis, 101, curve_t, &ut), BATTCURVE_OK);
    CHECK_EQ(uc, 1);   /* auto-selected charge */
    CHECK_EQ(ut, 0);   /* forced time */
    int differ = 0;
    for (int i = 0; i < BATTCURVE_POINTS; i++)
        if (curve_c[i] != curve_t[i]) differ = 1;
    CHECK(differ);
}

TEST(compute_rejects_bad)
{
    unsigned short out[BATTCURVE_POINTS];
    int uc;
    long axis[16];
    /* too short */
    static int s1[3] = {0, 60, 120}, v1[3] = {4120, 4000, 3600}, m1[3] = {-1,-1,-1};
    CHECK_EQ(battcurve_compute(s1, v1, m1, 3, 0, axis, 16, out, &uc), BATTCURVE_TOO_SHORT);

    /* non-monotonic (noisy) discharge over enough samples */
    static int s2[12], v2[12], m2[12];
    int noisy[12] = {4120, 3600, 4100, 3650, 4080, 3700,
                     4050, 3720, 4000, 3750, 3900, 3600};
    for (int i = 0; i < 12; i++) { s2[i] = i*60; v2[i] = noisy[i]; m2[i] = -1; }
    CHECK_EQ(battcurve_compute(s2, v2, m2, 12, 0, axis, 16, out, &uc),
             BATTCURVE_NONMONOTONIC);
}

int main(void)
{
    RUN_TEST(parse_line_basic);
    RUN_TEST(parse_line_with_current);
    RUN_TEST(parse_line_rejects_junk);
    RUN_TEST(compute_linear_time);
    RUN_TEST(compute_charge_axis_differs);
    RUN_TEST(compute_rejects_bad);
    return rbtest_report();
}
