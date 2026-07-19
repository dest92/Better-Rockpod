/* Unit tests for lib/fixedpoint — also serves as the harness example.
 * Build and run: make -C tests
 */

#include "rbtest.h"
#include "fixedpoint.h"

/* fp_sincos: phase is 0..0xffffffff for 0..2*pi, output is s0.31 */
TEST(sincos_cardinal_points)
{
    long cos_val;

    CHECK_NEAR(fp_sincos(0, &cos_val), 0, 0x20);  /* sin(0) = 0 */
    CHECK_NEAR(cos_val, 0x7fffffffL, 0x20);       /* cos(0) = 1 */

    fp_sincos(0x40000000UL, &cos_val);            /* pi/2 */
    CHECK_NEAR(cos_val, 0, 0x1000);               /* cos(pi/2) = 0 */

    CHECK_NEAR(fp_sincos(0x80000000UL, &cos_val), 0, 0x1000); /* sin(pi)=0 */
    CHECK_NEAR(cos_val, -0x7fffffffL, 0x1000);    /* cos(pi) = -1 */
}

TEST(sqrt_perfect_squares)
{
    /* integer mode: 0 fracbits */
    CHECK_EQ(fp_sqrt(0, 0), 0);
    CHECK_EQ(fp_sqrt(1, 0), 1);
    CHECK_EQ(fp_sqrt(144, 0), 12);
    CHECK_EQ(fp_sqrt(65536, 0), 256);

    /* 16.16 fixed point: sqrt(4.0) = 2.0 */
    CHECK_NEAR(fp_sqrt(4L << 16, 16), 2L << 16, 1);
}

TEST(ipow_basics)
{
    CHECK_EQ(ipow(2, 10), 1024);
    CHECK_EQ(ipow(10, 0), 1);
    CHECK_EQ(ipow(3, 4), 81);
    CHECK_EQ(ipow(-2, 3), -8);
}

TEST(log10_and_exp10_roundtrip)
{
    /* fp_log10(100.0) = 2.0 in 16.16 */
    CHECK_NEAR(fp_log10(100L << 16, 16), 2L << 16, 0x100);
    /* fp_exp10(2.0) = 100.0 in 16.16 */
    CHECK_NEAR(fp_exp10(2L << 16, 16), 100L << 16, 0x300);
}

int main(void)
{
    RUN_TEST(sincos_cardinal_points);
    RUN_TEST(sqrt_perfect_squares);
    RUN_TEST(ipow_basics);
    RUN_TEST(log10_and_exp10_roundtrip);
    return rbtest_report();
}
