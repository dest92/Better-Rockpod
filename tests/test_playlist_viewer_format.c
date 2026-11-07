/* Unit tests for apps/playlist_viewer_format.h
 * (specs/0011-playlist-viewer-disc-track-format.md, A1).
 */

#include <stdbool.h>
#include "rbtest.h"
#include "playlist_viewer_format.h"

#define UNTAGGED "<Untagged>"

static char buf[260];

static bool fmt(const char *ds, int dn, const char *ts, int tn,
                const char *title, const char *album, bool nums, bool alb)
{
    return pv_format_tag_line(buf, sizeof(buf), ds, dn, ts, tn, title,
                              album, UNTAGGED, nums, alb);
}

TEST(title_only)
{
    CHECK(fmt(NULL, 1, NULL, 3, "Song", "Album", false, false));
    CHECK_STR_EQ(buf, "Song");
}

TEST(title_and_album)
{
    CHECK(fmt(NULL, 0, NULL, 0, "Song", "Album", false, true));
    CHECK_STR_EQ(buf, "Song - Album");
}

TEST(numbers_from_ints)
{
    CHECK(fmt(NULL, 1, NULL, 3, "Song", "Album", true, true));
    CHECK_STR_EQ(buf, "1.03. Song - Album");
    CHECK(fmt(NULL, 2, NULL, 12, "Song", "Album", true, true));
    CHECK_STR_EQ(buf, "2.12. Song - Album");
}

TEST(numbers_from_strings_win)
{
    CHECK(fmt("2/3", 2, "5/12", 5, "Song", "Album", true, true));
    CHECK_STR_EQ(buf, "2/3.5/12. Song - Album");
}

TEST(missing_disc_or_track)
{
    CHECK(fmt(NULL, 0, NULL, 7, "Song", "Album", true, true));
    CHECK_STR_EQ(buf, "07. Song - Album");
    CHECK(fmt(NULL, 1, NULL, 0, "Song", "Album", true, true));
    CHECK_STR_EQ(buf, "1. Song - Album");
    CHECK(fmt(NULL, 0, NULL, 0, "Song", "Album", true, true));
    CHECK_STR_EQ(buf, "Song - Album");
    CHECK(fmt("", 0, "", 0, "Song", "Album", true, true));
    CHECK_STR_EQ(buf, "Song - Album");
}

TEST(missing_album_uses_untagged)
{
    CHECK(fmt(NULL, 1, NULL, 1, "Song", NULL, true, true));
    CHECK_STR_EQ(buf, "1.01. Song - <Untagged>");
    CHECK(fmt(NULL, 0, NULL, 0, "Song", "", false, true));
    CHECK_STR_EQ(buf, "Song - <Untagged>");
}

TEST(empty_title_fails)
{
    CHECK(!fmt(NULL, 1, NULL, 1, NULL, "Album", true, true));
    CHECK(!fmt(NULL, 1, NULL, 1, "", "Album", true, true));
}

TEST(truncates_without_overflow)
{
    char small[8];
    for (size_t sz = 1; sz <= sizeof(small); sz++)
    {
        memset(small, 'X', sizeof(small));
        pv_format_tag_line(small, sz, NULL, 1, NULL, 3, "LongTitle",
                           "LongAlbum", UNTAGGED, true, true);
        CHECK(strlen(small) == sz - 1);
        CHECK(memcmp(small, "1.03. LongTitle", sz - 1) == 0);
    }
    CHECK(!pv_format_tag_line(small, 0, NULL, 1, NULL, 3, "T", "A",
                              UNTAGGED, true, true));
}

int main(void)
{
    RUN_TEST(title_only);
    RUN_TEST(title_and_album);
    RUN_TEST(numbers_from_ints);
    RUN_TEST(numbers_from_strings_win);
    RUN_TEST(missing_disc_or_track);
    RUN_TEST(missing_album_uses_untagged);
    RUN_TEST(empty_title_fails);
    RUN_TEST(truncates_without_overflow);
    return rbtest_report();
}
