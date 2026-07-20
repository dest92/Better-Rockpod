/* Unit tests for apps/shuffle_locality.h — the pure ordering algorithm
 * behind disk-locality-aware shuffle
 * (specs/0004-disk-locality-shuffle.md, A1-A4).
 */

#include <stdlib.h>
#include "rbtest.h"
#include "shuffle_locality.h"

#define MAXN 2000
static struct shuffle_loc_ent ents[MAXN], aux[MAXN];
static int region_perm[MAXN / SHUFFLE_REGION_TRACKS + 2];
static int seen[MAXN];

static void fill(long *keys, int n)
{
    for (int i = 0; i < n; i++)
    {
        ents[i].key = keys[i];
        ents[i].idx = i;
    }
}

static int check_permutation(int n)
{
    memset(seen, 0, sizeof(seen));
    for (int i = 0; i < n; i++)
    {
        int idx = ents[i].idx;
        if (idx < 0 || idx >= n || seen[idx])
            return 0;
        seen[idx] = 1;
    }
    return 1;
}

/* A1: valid permutation across sizes and key shapes */
TEST(valid_permutation)
{
    static long keys[MAXN];
    static const int sizes[] = { 0, 1, 2, 31, 32, 33, 1000 };
    for (unsigned s = 0; s < sizeof(sizes)/sizeof(sizes[0]); s++)
    {
        int n = sizes[s];
        for (int shape = 0; shape < 4; shape++)
        {
            for (int i = 0; i < n; i++)
            {
                switch (shape)
                {
                case 0: keys[i] = i * 100; break;              /* spread */
                case 1: keys[i] = 42; break;                   /* equal */
                case 2: keys[i] = SHUFFLE_KEY_UNKNOWN; break;  /* unknown */
                default: keys[i] = (i % 3 == 0)
                        ? SHUFFLE_KEY_UNKNOWN : i; break;      /* mixed */
                }
            }
            fill(keys, n);
            srand(123 + shape);
            shuffle_locality_order(ents, aux, region_perm, n);
            CHECK(check_permutation(n));
        }
    }
}

/* A2: deterministic per seed, different across seeds */
TEST(deterministic)
{
    static long keys[MAXN];
    static int first[MAXN];
    int n = 1000;
    for (int i = 0; i < n; i++)
        keys[i] = (i * 7919) % 100000;

    fill(keys, n);
    srand(42);
    shuffle_locality_order(ents, aux, region_perm, n);
    for (int i = 0; i < n; i++)
        first[i] = ents[i].idx;

    fill(keys, n);
    srand(42);
    shuffle_locality_order(ents, aux, region_perm, n);
    int same = 1;
    for (int i = 0; i < n; i++)
        if (ents[i].idx != first[i])
            same = 0;
    CHECK(same);

    fill(keys, n);
    srand(43);
    shuffle_locality_order(ents, aux, region_perm, n);
    int diff = 0;
    for (int i = 0; i < n; i++)
        if (ents[i].idx != first[i])
            diff = 1;
    CHECK(diff);
}

/* A3: locality — clustered keys stay clustered vs. plain shuffle */
TEST(locality_vs_plain)
{
    static long keys[MAXN];
    int n = 1000;
    /* 4 clusters, 1M clusters apart, tight within */
    for (int i = 0; i < n; i++)
        keys[i] = (long)(i % 4) * 1000000 + (i / 4);

    /* plain-shuffle baseline: Fisher-Yates over positions */
    static int plain[MAXN];
    for (int i = 0; i < n; i++)
        plain[i] = i;
    srand(7);
    for (int i = n - 1; i > 0; i--)
    {
        int j = rand() % (i + 1);
        int t = plain[j]; plain[j] = plain[i]; plain[i] = t;
    }
    long long base = 0;
    for (int i = 0; i + 1 < n; i++)
    {
        long d = keys[plain[i+1]] - keys[plain[i]];
        base += d > 0 ? d : -d;
    }

    fill(keys, n);
    srand(7);
    shuffle_locality_order(ents, aux, region_perm, n);
    long long loc = 0;
    for (int i = 0; i + 1 < n; i++)
    {
        long d = keys[ents[i+1].idx] - keys[ents[i].idx];
        loc += d > 0 ? d : -d;
    }
    CHECK(loc * 5 <= base);
}

/* A4: unknown keys go last and still shuffle; all-unknown acts like a
 * plain full shuffle (single region spanning everything) */
TEST(unknown_keys)
{
    static long keys[MAXN];
    int n = 200;
    /* half known (small keys), half unknown */
    for (int i = 0; i < n; i++)
        keys[i] = (i < n/2) ? i : SHUFFLE_KEY_UNKNOWN;
    fill(keys, n);
    srand(9);
    shuffle_locality_order(ents, aux, region_perm, n);
    CHECK(check_permutation(n));
    /* all known tracks (orig idx < n/2) come before all unknown */
    int last_known = -1, first_unknown = n;
    for (int i = 0; i < n; i++)
    {
        if (ents[i].idx < n/2 && i > last_known)
            last_known = i;
        if (ents[i].idx >= n/2 && i < first_unknown)
            first_unknown = i;
    }
    CHECK(last_known < first_unknown);

    /* all-unknown: one full-span shuffle, must displace something for
     * a large n and stay a permutation */
    for (int i = 0; i < n; i++)
        keys[i] = SHUFFLE_KEY_UNKNOWN;
    fill(keys, n);
    srand(11);
    shuffle_locality_order(ents, aux, region_perm, n);
    CHECK(check_permutation(n));
    int moved = 0;
    for (int i = 0; i < n; i++)
        if (ents[i].idx != i)
            moved = 1;
    CHECK(moved);
    /* and long-range moves happen (beyond one region) — plain shuffle,
     * not per-region shuffle */
    int longrange = 0;
    for (int i = 0; i < n; i++)
    {
        int d = ents[i].idx - i;
        if (d < 0) d = -d;
        if (d > SHUFFLE_REGION_TRACKS)
            longrange = 1;
    }
    CHECK(longrange);
}

int main(void)
{
    RUN_TEST(valid_permutation);
    RUN_TEST(deterministic);
    RUN_TEST(locality_vs_plain);
    RUN_TEST(unknown_keys);
    return rbtest_report();
}
