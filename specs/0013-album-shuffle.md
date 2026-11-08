# 0013: Album shuffle (shuffle album order, keep track order)

- **Status:** Implemented
- **Branch/PR:** `master` (roadmap 2.3)

## Problem

Shuffle randomizes every track. Many listeners want "shuffle albums": a
random album order, with each album playing start to finish. Rockbox has
no such mode. The database view from spec 0012 covers part of this, but
it doesn't work with folder playback, saved playlists, or Repeat
Shuffle.

## Requirements

- **R1:** A "Shuffle Albums" setting (off by default) in Settings →
  Playback, next to "Disk-optimized Shuffle". When it and Shuffle are
  on, a shuffle orders the playlist by album: albums in random order,
  each album's tracks in playlist order.
- **R2:** An album is a run of consecutive tracks (in unshuffled
  playlist order) that share a parent directory. This needs no tag
  reads.
- **R3:** Resume reproduces the same order. The control file records
  `L:<seed>:<first_index>`, and resume replays it with the same seed.
- **R4:** Album shuffle takes precedence over disk-optimized shuffle
  (albums are already disk-local). With it off, shuffle is unchanged:
  `S:` or `H:`.
- **R5:** If scratch memory is unavailable, it falls back to the plain
  shuffle and records `S:`.
- **R6:** Repeat Shuffle reshuffles using the album mode too, and its
  anti-repeat seed retries (spec 0005) still apply.

## Design sketch

- Pure helper `album_shuffle_order()` in `apps/album_shuffle.h`: given
  the group start positions, it Fisher-Yates shuffles the group order
  with the seeded RNG and writes the old position of each new slot.
- `randomise_playlist_unlocked()` takes a shuffle mode (plain, disk,
  album) instead of `bool disk`. Callers use `shuffle_mode_requested()`
  (setting-driven), and resume maps `S`/`H`/`L` to the mode.
- `randomise_playlist_album_unlocked()` sorts the playlist, reads each
  track's directory via `get_track_filename()` (dircache-backed when
  available), builds group starts, and permutes `indices[]` and
  `dcfrefs[]` with one scratch `core_alloc`.

## Acceptance criteria

- **A1:** Unit tests: every position appears once, groups stay
  contiguous and in order, the group order varies with the seed, and
  single/empty inputs are handled.
- **A2:** Simulator: playing 5 albums × 2 tracks with Shuffle + Shuffle
  Albums gives album-grouped random order. Resume restores the
  identical order. With Shuffle Albums off, tracks are mixed (`S:`).
- **A3:** Both hardware targets build.

## Verification results

- **A1:** `tests/test_album_shuffle.c`, 591 checks (including spec 0012's),
  green under ASan/UBSan.
- **A2:** ipod6g simulator, Database → Band → [All tracks] with
  `shuffle: on`, `album shuffle: on`.
  - The playlist viewer showed Alpha 1-2, Echo 1-2, Charlie 1-2, Bravo
    1-2, Delta 1-2.
  - The control file recorded `L:1866:6`. After a restart, Resume
    Playback showed the identical order.
  - With `album shuffle: off`, the control file recorded `S:1863:1` and
    tracks were interleaved across albums.
- **A3:** `ipod6g` and `ipodvideo` build and zip.
- **Not exercised:** Repeat Shuffle's end-of-playlist reshuffle in album
  mode. It goes through the same `randomise_playlist_unlocked()` path
  with the mode argument.
