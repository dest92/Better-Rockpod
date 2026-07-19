/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Minimal host-side unit test framework for Better-Rockpod.
 *
 * Usage:
 *     #include "rbtest.h"
 *
 *     TEST(my_case)
 *     {
 *         CHECK_EQ(1 + 1, 2);
 *         CHECK(some_condition);
 *     }
 *
 *     int main(void)
 *     {
 *         RUN_TEST(my_case);
 *         return rbtest_report();
 *     }
 *
 * Each test file is an independent host executable; the Makefile builds
 * and runs them with ASan+UBSan. A failed CHECK reports and continues;
 * the process exits nonzero if any check failed.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 ****************************************************************************/

#ifndef RBTEST_H
#define RBTEST_H

#include <stdio.h>
#include <string.h>

static int rbtest_checks = 0;
static int rbtest_failures = 0;
static const char *rbtest_current = "?";

#define TEST(name) static void rbtest_##name(void)

#define RUN_TEST(name) \
    do { \
        rbtest_current = #name; \
        rbtest_##name(); \
    } while (0)

#define RBTEST_FAIL(fmt, ...) \
    do { \
        rbtest_failures++; \
        printf("FAIL %s (%s:%d): " fmt "\n", rbtest_current, \
               __FILE__, __LINE__, __VA_ARGS__); \
    } while (0)

#define CHECK(cond) \
    do { \
        rbtest_checks++; \
        if (!(cond)) \
            RBTEST_FAIL("%s", #cond); \
    } while (0)

#define CHECK_EQ(a, b) \
    do { \
        rbtest_checks++; \
        long long rbtest_a_ = (long long)(a); \
        long long rbtest_b_ = (long long)(b); \
        if (rbtest_a_ != rbtest_b_) \
            RBTEST_FAIL("%s == %s: %lld != %lld", \
                        #a, #b, rbtest_a_, rbtest_b_); \
    } while (0)

#define CHECK_STR_EQ(a, b) \
    do { \
        rbtest_checks++; \
        const char *rbtest_a_ = (a); \
        const char *rbtest_b_ = (b); \
        if (strcmp(rbtest_a_, rbtest_b_) != 0) \
            RBTEST_FAIL("%s == %s: \"%s\" != \"%s\"", \
                        #a, #b, rbtest_a_, rbtest_b_); \
    } while (0)

/* |a - b| <= tol, for fixed-point approximations */
#define CHECK_NEAR(a, b, tol) \
    do { \
        rbtest_checks++; \
        long long rbtest_a_ = (long long)(a); \
        long long rbtest_b_ = (long long)(b); \
        long long rbtest_d_ = rbtest_a_ - rbtest_b_; \
        if (rbtest_d_ < 0) rbtest_d_ = -rbtest_d_; \
        if (rbtest_d_ > (long long)(tol)) \
            RBTEST_FAIL("%s ~= %s: %lld vs %lld (tol %lld)", #a, #b, \
                        rbtest_a_, rbtest_b_, (long long)(tol)); \
    } while (0)

static inline int rbtest_report(void)
{
    if (rbtest_failures) {
        printf("%d/%d checks FAILED\n", rbtest_failures, rbtest_checks);
        return 1;
    }
    printf("ok: %d checks passed\n", rbtest_checks);
    return 0;
}

#endif /* RBTEST_H */
