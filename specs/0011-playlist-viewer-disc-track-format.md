# 0011: Playlist viewer shows disc + track number + title + album

- **Status:** Implemented
- **Branch/PR:** `master` (roadmap 3.2, port of rockpod#4 by Paul Sauro / Olsro)

## Problem

The playlist viewer's "Track display" setting
(`playlist_viewer_track_display`, `apps/settings_list.c:1539`) is
ignored. `format_line()` (`apps/playlist_viewer.c:523`) always reads
the tags and shows only the title ("iPod Classic 6G custom"), so
"Track name", "Full path" and "Title and album" do nothing in the list
view. There is also no way to see whether a playlist follows album
order (disc/track), which rockpod#4 asks for to make bulk edits easier,
for example deleting or moving one CD of a multi-disc album.

## Requirements

- **R1:** A fifth "Track display" choice, "CD & track num & title &
  album from tags", formats each entry as `D.TT. Title - Album`.
  - The disc part is `disc_string` if present, else `discnum` if
    non-zero; omitted when neither exists.
  - The track part is `track_string` if present, else `tracknum`
    zero-padded to 2 digits if non-zero; omitted when neither exists.
  - The space after the prefix is written only when a disc or track
    part was written.
  - A missing album shows the "<Untagged>" string (`LANG_TAGNAVI_UNTAGGED`).
- **R2:** The setting is honored again for all choices:
  - filename and full path read no tags;
  - "title" shows the title;
  - "title and album" shows `Title - Album`;
  - the new choice follows R1.
  - If tag retrieval fails or the title is empty, the entry falls back
    to the file name (existing behavior).
- **R3:** The default becomes "title from tags" (3), so the list looks
  the same as today for users who never changed the setting.
- **R4:** Output never overflows the per-entry buffer (`MAX_PATH`).

## Non-goals

- No French or other language strings beyond `english.lang`, since
  other languages fall back to English.
- No change to voice output beyond treating the new choice like the
  other tag-based choices (speak the file name).

## Design sketch

- New header-only pure helper `apps/playlist_viewer_format.h`, with no
  firmware includes (same pattern as `apps/shuffle_locality.h`):
  - `pv_format_tag_line(buf, size, disc_string, discnum, track_string,
    tracknum, title, album, with_numbers, with_album)` builds the line
    with bounded `snprintf` appends and returns false when the title is
    empty.
- `format_line()` calls it for the three tag-based choices and only
  retrieves tags for those choices.
- Settings: add enum value 4, the choice string, the language phrase,
  and default 3.
- This replaces the PR's hand-rolled per-character copy loops.

## Acceptance criteria

- **A1:** Unit tests cover the formatting:
  - disc/track strings vs numbers;
  - missing parts;
  - missing album;
  - empty title;
  - truncation at small buffer sizes (no overflow under ASan).
- **A2:** Simulator: with the new choice, a playlist of tagged files
  shows `1.03. Title - Album`. With "Track name", it shows file names.
  With the default, it shows titles.
- **A3:** Both hardware targets build.

## Test plan

| Criterion | How verified |
|-----------|--------------|
| A1 | `tests/test_playlist_viewer_format.c` (`make -C tests`) |
| A2 | simulator, scripted (Xvfb + xdotool), screenshots of the playlist viewer per setting |
| A3 | `build-hw-ipod6g`, `build-hw-ipodvideo` |

## Verification results

- **Unit tests (A1):** `tests/test_playlist_viewer_format.c`, 41 checks,
  green under ASan/UBSan. Truncation is exercised at every buffer size
  from 0 to 8.
- **Simulator (A2):** ipod6g, headless (Xvfb + xdotool, SDL dummy
  audio), 3 MP3s tagged with disc/track (1/1, 1/2, 2/1) in one album.
  Played from the file browser, then WPS → hold SELECT → Current
  Playlist → View Current Playlist.
  - New choice: `1. 1.1. First Song - Test Album`, `2. 1.2. Second Song
    - Test Album`, `3. 2.1. Third Song - Test Album`. The leading `N.`
    is the playlist index. The track is not zero-padded because ffmpeg
    writes TRCK as the string "1", and the string wins per R1.
  - Default (no `config.cfg`): `1. First Song` and so on, unchanged
    from before (R3).
  - "Track name": `1. 01`, `2. 02`, `3. 03` (file names). The setting is
    honored again (R2).
- **Hardware builds (A3):** `ipod6g` and `ipodvideo` build and zip
  cleanly, with no new warnings.
