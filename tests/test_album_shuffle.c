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

int main(void)
{
    RUN_TEST(deterministic);
    RUN_TEST(seed_changes_order);
    RUN_TEST(cmp_sign_correct);
    RUN_TEST(distribution);
    return rbtest_report();
}
