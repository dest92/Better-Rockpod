# 0016: Multiselection in the file browser, database and playlist viewer

- **Status:** Implemented
- **Branch/PR:** `master` (roadmap 3.4, port of rockpod#2 by Paul Sauro / Olsro)

## Problem

Lists in Rockbox act on one item at a time. Building a mix from
several files, or removing or moving a run of tracks in the playlist
viewer, means repeating the same context-menu action per item.
rockpod#2 adds range multiselection.

## Requirements

- **R1:** The context menu of the file browser, database lists and
  playlist viewer offers "Enable/Disable Multiselection". While it's
  enabled, moving the cursor extends a contiguous range from the
  anchor item, in either direction, and the whole range is
  highlighted.
- **R2:** SELECT on a multi-item range opens the context menu for the
  range:
  - file browser and database: Playing Next… / Add to Playlist…;
  - playlist viewer: additionally Remove and Move.
- **R3:** The selection range always covers exactly anchor..cursor,
  inclusive.
- **R4:** Plugins built against the previous API are rejected (the
  `gui_synclist_init()` signature changed): `PLUGIN_API_VERSION` 282.

## Design / port notes

- The PR applied without textual conflicts on this tree.
- Changes from the PR as submitted:
  - The new plugin API entry `onplay_show_playlist_menu_get_choice`
    moved to the end of `struct plugin_api`, as the struct's rules
    require, and the version was bumped to 282 with an explanation.
  - The four new language phrases moved to the end of `english.lang`,
    so existing IDs are unchanged.
  - The signed-size encoding (n > 0: n items from the cursor; -k: k+1
    items ending at the cursor) was duplicated in `tree.c`,
    `bitmap/list.c` and `playlist_viewer.c`. It now lives in the
    host-tested `apps/gui/multiselect.h` (`multiselect_size`,
    `multiselect_range`). The move code in the playlist viewer keeps
    its own adjustment, since it shifts two indices at once.
- The touchscreen and voice paths are as submitted. The author flags
  them as untested, and neither applies to the iPod targets.

## Acceptance criteria

- **A1:** Unit tests: for every anchor/cursor pair in 0..11 the range
  is exactly anchor..cursor. Disabled means one item; size 0 means one
  item.
- **A2:** Simulator: a file-browser range plays exactly that range, a
  playlist-viewer range removes exactly that range, and an upward range
  in a database list plays exactly that range.
- **A3:** `ipod6g` and `ipodvideo` build, plugins included.

## Verification results

- **A1:** `tests/test_multiselect.c`, 293 checks, green.
- **A2:** ipod6g simulator, folder `Music/Multi` with 6 tracks.
  - Files: enabled multiselection on 02, moved down two (02-04
    highlighted), then SELECT → Playing Next… → Play. The playlist was
    Song 2, Song 3, Song 4.
  - Playlist viewer: on Song 3, enabled multiselection, moved down one,
    then SELECT → Remove. Song 3 and Song 4 were removed; Song 3 was
    the current track, so playback stopped.
  - Database (`[By album, random order]` list): enabled on Song 5,
    moved up two (Song 3-5, a negative-size range), then Playing Next…
    → Play. The playlist was Song 3, Song 4, Song 5.
- **A3:** Both hardware targets build and zip.
- **Not exercised:** Move of a range in the playlist viewer, and
  Add to Playlist… (catalog) with a range.
