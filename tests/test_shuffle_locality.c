/* Unit tests for apps/shuffle_locality.h — the pure ordering algorithm
 * behind disk-locality-aware shuffle
 * (specs/0004-disk-locality-shuffle.md, A1-A4).
 */

#include <stdlib.h>
#include <stdbool.h>
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

/* specs/0005-repeat-shuffle-antirepeat.md A1: overlap counting */
TEST(repeat_overlap_counts)
{
    unsigned long ind[10] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    unsigned long rec_disjoint[3] = { 100, 101, 102 };
    unsigned long rec_same[3] = { 0, 1, 2 };
    unsigned long rec_partial[3] = { 2, 50, 9 };  /* 9 outside window */

    CHECK_EQ(shuffle_repeat_overlap(ind, 10, rec_disjoint, 3), 0);
    CHECK_EQ(shuffle_repeat_overlap(ind, 10, rec_same, 3), 3);
    CHECK_EQ(shuffle_repeat_overlap(ind, 10, rec_partial, 3), 1);
    CHECK_EQ(shuffle_repeat_overlap(ind, 10, rec_same, 0), 0);
    /* window clamped to n */
    CHECK_EQ(shuffle_repeat_overlap(ind, 2, rec_same, 3), 2);
}

/* A2: seed retry finds a zero-overlap order quickly (deterministic) */
TEST(repeat_retry_converges)
{
    enum { N = 100, K = 10 };
    static unsigned long ind[N];
    unsigned long recent[K];

    /* previous pass: 0..N-1 in order; recent = last K played */
    for (int i = 0; i < K; i++)
        recent[i] = N - 1 - i;

    int best = K + 1;
    unsigned int seed = 12345;
    int tries;
    for (tries = 0; tries < 8; tries++)
    {
        /* re-sort + plain Fisher-Yates, as playlist.c does per attempt */
        for (int i = 0; i < N; i++)
            ind[i] = i;
        srand(seed + tries);
        for (int i = N - 1; i > 0; i--)
        {
            int j = rand() % (i + 1);
            unsigned long t = ind[j]; ind[j] = ind[i]; ind[i] = t;
        }
        int ov = shuffle_repeat_overlap(ind, N, recent, K);
        if (ov < best)
            best = ov;
        if (best == 0)
            break;
    }
    CHECK_EQ(best, 0);
    CHECK(tries < 8);
}

/* --- shuffle_locality_apply2: the in-place two-array permutation used by
 * playlist.c to reorder indices[] and dcfrefs[] together (spec 0004).
 * This is the highest-corruption-risk code, so it is exercised against a
 * reference over every permutation of small n. --- */

static int apply_failures;

static void check_one_perm(const int *perm, int n)
{
    struct shuffle_loc_ent e[8];
    unsigned long a[8];        /* stands in for playlist indices[] */
    unsigned short b[8];       /* stands in for a second parallel array */
    unsigned short bscratch;

    for (int i = 0; i < n; i++)
    {
        e[i].idx = perm[i];
        e[i].key = 0;
        a[i] = (unsigned long)(100 + i);
        b[i] = (unsigned short)(1000 + i);
    }

    shuffle_locality_apply2(e, n, a, (char *)b, sizeof(b[0]),
                            (char *)&bscratch);

    for (int i = 0; i < n; i++)
    {
        /* new[i] must equal old[perm[i]] for both parallel arrays */
        if (a[i] != (unsigned long)(100 + perm[i]) ||
            b[i] != (unsigned short)(1000 + perm[i]))
            apply_failures++;
    }
}

/* recursively enumerate every permutation of {0..n-1} and test each */
static void gen_perms(int *perm, bool *used, int n, int depth)
{
    if (depth == n)
    {
        check_one_perm(perm, n);
        return;
    }
    for (int v = 0; v < n; v++)
    {
        if (used[v])
            continue;
        used[v] = true;
        perm[depth] = v;
        gen_perms(perm, used, n, depth + 1);
        used[v] = false;
    }
}

TEST(apply2_all_permutations)
{
    apply_failures = 0;
    for (int n = 0; n <= 7; n++)
    {
        int perm[8];
        bool used[8] = { false };
        gen_perms(perm, used, n, 0);
    }
    CHECK_EQ(apply_failures, 0);
}

int main(void)
{
    RUN_TEST(valid_permutation);
    RUN_TEST(deterministic);
    RUN_TEST(locality_vs_plain);
    RUN_TEST(unknown_keys);
    RUN_TEST(repeat_overlap_counts);
    RUN_TEST(repeat_retry_converges);
    RUN_TEST(apply2_all_permutations);
    return rbtest_report();
}
