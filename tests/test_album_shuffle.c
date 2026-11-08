/* Unit tests for apps/album_shuffle.h
 * (specs/0012-tagtree-random-album-order.md, A1).
 */

#include <stdbool.h>
#include <stdint.h>
#include "rbtest.h"
#include "album_shuffle.h"

TEST(deterministic)
{
    CHECK_EQ(album_shuffle_key("Abbey Road", 42),
             album_shuffle_key("Abbey Road", 42));
    CHECK(album_shuffle_key("Abbey Road", 42) !=
          album_shuffle_key("Let It Be", 42));
    CHECK(album_shuffle_key(NULL, 7) == album_shuffle_key("", 7));
}

TEST(seed_changes_order)
{
    const char *albums[] = { "A", "B", "C", "D", "E", "F" };
    int changed = 0;
    for (uint32_t seed = 1; seed < 50; seed++)
    {
        bool a_first_0 = album_shuffle_cmp(album_shuffle_key(albums[0], 0),
                                           album_shuffle_key(albums[1], 0)) < 0;
        bool a_first = album_shuffle_cmp(album_shuffle_key(albums[0], seed),
                                         album_shuffle_key(albums[1], seed)) < 0;
        if (a_first != a_first_0)
            changed++;
    }
    CHECK(changed > 10 && changed < 40);
}

TEST(cmp_sign_correct)
{
    CHECK(album_shuffle_cmp(0u, 0xffffffffu) < 0);
    CHECK(album_shuffle_cmp(0xffffffffu, 0u) > 0);
    CHECK(album_shuffle_cmp(1u, 0x80000002u) < 0);
    CHECK(album_shuffle_cmp(0x80000002u, 1u) > 0);
    CHECK_EQ(album_shuffle_cmp(123u, 123u), 0);
}

TEST(distribution)
{
    int high = 0;
    char name[16];
    for (int i = 0; i < 1000; i++)
    {
        snprintf(name, sizeof(name), "album%d", i);
        if (album_shuffle_key(name, 1234) & 0x80000000u)
            high++;
    }
    CHECK(high > 400 && high < 600);
}

static unsigned int lcg_state;
static int lcg(void)
{
    lcg_state = lcg_state * 1103515245u + 12345u;
    return (int)((lcg_state >> 16) & 0x7fff);
}

/* 2.3: groups of a sorted playlist; out[] maps new position -> old */
TEST(order_keeps_groups_and_inner_order)
{
    int starts[] = { 0, 3, 4, 8, 10 };      /* 4 groups, n = 10 */
    int order[4], out[10], seen[10] = {0};
    for (unsigned int seed = 1; seed < 20; seed++)
    {
        lcg_state = seed;
        album_shuffle_order(starts, 4, order, out, lcg);
        memset(seen, 0, sizeof(seen));
        for (int i = 0; i < 10; i++)
        {
            CHECK(out[i] >= 0 && out[i] < 10);
            seen[out[i]]++;
        }
        for (int i = 0; i < 10; i++)
            CHECK_EQ(seen[i], 1);
        /* inside a group, positions stay consecutive and increasing */
        int pos = 0;
        for (int g = 0; g < 4; g++)
        {
            int grp = order[g];
            for (int k = starts[grp]; k < starts[grp + 1]; k++)
                CHECK_EQ(out[pos++], k);
        }
    }
}

TEST(order_varies_with_seed)
{
    int starts[] = { 0, 1, 2, 3, 4, 5, 6 };
    int order[6], out[6], first_seen[6] = {0};
    for (unsigned int seed = 1; seed < 200; seed++)
    {
        lcg_state = seed;
        album_shuffle_order(starts, 6, order, out, lcg);
        first_seen[out[0]]++;
    }
    for (int g = 0; g < 6; g++)
        CHECK(first_seen[g] > 10);
}

TEST(order_single_and_empty)
{
    int starts1[] = { 0, 5 };
    int order[1], out[5];
    lcg_state = 3;
    album_shuffle_order(starts1, 1, order, out, lcg);
    for (int i = 0; i < 5; i++)
        CHECK_EQ(out[i], i);
    int starts0[] = { 0 };
    album_shuffle_order(starts0, 0, order, out, lcg);
}

int main(void)
{
    RUN_TEST(deterministic);
    RUN_TEST(seed_changes_order);
    RUN_TEST(cmp_sign_correct);
    RUN_TEST(distribution);
    RUN_TEST(order_keeps_groups_and_inner_order);
    RUN_TEST(order_varies_with_seed);
    RUN_TEST(order_single_and_empty);
    return rbtest_report();
}
