# 0012: Database view "all tracks, albums in random order"

- **Status:** Implemented
- **Branch/PR:** `master` (roadmap 3.3, port of rockpod#3 by Paul Sauro / Olsro)

## Problem

In the database browser, an artist (or any view whose next level is
albums) offers "[All tracks]" and "[All tracks sorted by album]"
(`apps/tagtree.c:1720`). There is no way to play every album of the view
with the albums in random order and the tracks of each album in order.
The stock iPod OS has this, and rockpod#3 ports it. The PR's comparator
also subtracts `uint32_t` keys and returns the difference as `int`,
which gives the wrong sign for keys more than 2^31 apart. That breaks
qsort's ordering contract.

## Requirements

- **R1:** Views that already show "[All tracks sorted by album]" also
  show "[All tracks, random album order]" directly after it.
- **R2:** Entering it lists every track of the view, grouped by album,
  keeping the existing in-album order (disc/track via the album-sorted
  format). The album groups appear in a random order that changes each
  time the view is entered.
- **R3:** The ordering is a valid strict weak ordering: per-album keys
  are compared with a sign-correct three-way comparison, and ties on
  the key fall back to the album name.
- **R4:** Playing from the list queues tracks in the displayed order.

## Design sketch

- Pure helper `apps/album_shuffle.h`:
  - `album_shuffle_key(name, seed)`: FNV-1a over the album name, mixed
    with a per-entry seed (murmur3 finalizer).
  - `album_shuffle_cmp(a, b)`: three-way comparison of the keys.
- `struct tagentry` gets `album_key`. A new
  `TABLE_ALLSUBENTRIES_SORTED_RANDOMLY_BY_ALBUMS` reuses every code path
  of `TABLE_ALLSUBENTRIES_SORTED_BY_ALBUMS`. The seed is `current_tick`,
  taken when the table is loaded.
- `compare_with_albums` compares the keys first; the key is 0 for the
  non-random table, so that table's order is unchanged.

## Acceptance criteria

- **A1:** Unit tests: keys are deterministic for a given name and seed,
  different seeds permute album order, the comparator is antisymmetric
  for extreme values, and the distribution is non-degenerate.
- **A2:** Simulator, with a database of 3+ albums: the new entry
  appears, tracks stay grouped and in order within each album, and the
  album order differs across re-entries.
- **A3:** Both hardware targets build.

## Verification results

- **Unit tests (A1):** `tests/test_album_shuffle.c`, 10 checks, green
  under ASan/UBSan.
- **Simulator (A2):** ipod6g, database built in the sim from 5 albums ×
  2 tracks by one artist. Database → Artist → Band shows `[By album]`,
  `[By album, random order]`, `[All tracks]`, `[Random]`, then the
  albums.
  - Two entries into the new view gave Bravo, Echo, Alpha, Delta,
    Charlie and then Echo, Charlie, Alpha, Bravo, Delta.
  - Each album's tracks stayed grouped and in order (track 1, track 2).
- **R4:** selecting "Echo track 2" played "2 of 10" with "Charlie track
  1" next, matching the displayed order.
- **Hardware builds (A3):** `ipod6g` and `ipodvideo` build and zip.
- The new table value is appended after `TABLE_PLAYTRACK`, so existing
  enum values (and anything that stores them) are unchanged.
