/* Unit tests for apps/plugins/pictureflow/aa_source.h — the pure
 * source-selection logic behind PictureFlow's embedded-album-art
 * support (specs/0001-pictureflow-embedded-albumart.md, A4).
 */

#include "rbtest.h"
#include "aa_source.h"

TEST(select_source_disabled_always_none)
{
    /* AA_OFF: no source regardless of what's available */
    CHECK_EQ(pf_aa_select_source(false, false, false, false), AA_SOURCE_NONE);
    CHECK_EQ(pf_aa_select_source(false, false, true, true), AA_SOURCE_NONE);
    CHECK_EQ(pf_aa_select_source(false, true, true, true), AA_SOURCE_NONE);
}

TEST(select_source_prefer_embedded)
{
    /* prefer_file_first = false: embedded wins whenever present */
    CHECK_EQ(pf_aa_select_source(true, false, false, false), AA_SOURCE_NONE);
    CHECK_EQ(pf_aa_select_source(true, false, true, false), AA_SOURCE_FILE);
    CHECK_EQ(pf_aa_select_source(true, false, false, true), AA_SOURCE_EMBEDDED);
    CHECK_EQ(pf_aa_select_source(true, false, true, true), AA_SOURCE_EMBEDDED);
}

TEST(select_source_prefer_image_file)
{
    /* prefer_file_first = true: file wins whenever present */
    CHECK_EQ(pf_aa_select_source(true, true, false, false), AA_SOURCE_NONE);
    CHECK_EQ(pf_aa_select_source(true, true, true, false), AA_SOURCE_FILE);
    CHECK_EQ(pf_aa_select_source(true, true, false, true), AA_SOURCE_EMBEDDED);
    CHECK_EQ(pf_aa_select_source(true, true, true, true), AA_SOURCE_FILE);
}

TEST(needs_file_search_prefer_image_file_always_searches)
{
    /* File could outrank embedded either way — always need the search */
    CHECK(pf_aa_needs_file_search(true, false));
    CHECK(pf_aa_needs_file_search(true, true));
}

TEST(needs_file_search_prefer_embedded_skips_when_embedded_available)
{
    /* The performance-critical case: default setting + embedded art
     * present must NOT require a filesystem search. */
    CHECK(!pf_aa_needs_file_search(false, true));
    /* No embedded art: must fall back to searching for a file. */
    CHECK(pf_aa_needs_file_search(false, false));
}

int main(void)
{
    RUN_TEST(select_source_disabled_always_none);
    RUN_TEST(select_source_prefer_embedded);
    RUN_TEST(select_source_prefer_image_file);
    RUN_TEST(needs_file_search_prefer_image_file_always_searches);
    RUN_TEST(needs_file_search_prefer_embedded_skips_when_embedded_available);
    return rbtest_report();
}
