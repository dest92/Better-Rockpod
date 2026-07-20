# 0005: Repeat Shuffle anti-repeat

- **Status:** Implemented. A1/A2 verified by unit tests (43 total in
  test_shuffle_locality, ASan/UBSan). A3 verified on the ipod6g
  simulator: repeat=shuffle playback wrapped the playlist cleanly and
  the control file recorded exactly one shuffle command per wrap. A4
  hardware builds pending (no ARM toolchain in the implementation
  environment).
- **Branch/PR:** claude/rockpod-features-roadmap-qg0tc3

## Problem

In Repeat Shuffle mode, when the playlist wraps it is re-shuffled with a
fresh seed (`apps/playlist.c`, repeat-shuffle branch calling
`randomise_playlist_unlocked()`). Nothing stops tracks that just played
at the end of the previous pass from landing at the start of the new
one, so a song can repeat back-to-back across the wrap.

## Requirements

- **R1:** When Repeat Shuffle re-shuffles, the first K positions of the
  new order should avoid the last K tracks played in the previous pass,
  with K = min(amount/4, 10).
- **R2:** Resume must reproduce the final order exactly. Therefore the
  mechanism is seed retry, not order surgery: shuffle with seed s; if
  the overlap window has violations, re-sort and re-shuffle with s+1
  (up to 8 attempts); only the final seed is written to the control
  file, so replay (sort + shuffle with that seed) yields the identical
  order.
- **R3:** If no attempt reaches zero overlap, the attempt with the
  fewest violations is used (best effort — R1 is a strong tendency,
  not an absolute guarantee).
- **R4:** Playlists too small to avoid repeats (amount < 3K) skip the
  retry loop entirely.
- **R5:** Works with both plain and disk-locality shuffle (the retry
  simply re-runs whichever algorithm is active).
- **R6:** The overlap check is a pure, host-testable helper.

## Non-goals

- Long-term listening history across sessions; only the wrap boundary
  of the current playlist is considered.
- Changing shuffle behavior outside Repeat Shuffle mode.

## Design sketch

- `apps/shuffle_locality.h` gains
  `shuffle_repeat_overlap(const unsigned long *indices, int n,
  const unsigned long *recent, int k)` — counts how many of the first
  k entries of `indices` appear in `recent[0..k)` (raw value
  equality; playlist index values are unique).
- In the repeat-shuffle branch of `playlist.c`: before sorting, capture
  the last K played entries (positions `first_index-1, first_index-2,
  ...` mod amount) into a small stack array (K ≤ 10). Then loop
  attempts: `sort_playlist_unlocked()` +
  `randomise_playlist_unlocked(seed+i, write=false)`, evaluate
  overlap, remember the best seed. Finally re-sort and re-run with the
  winning seed and `write=true` — the only control-file record.

## Acceptance criteria

- **A1:** `shuffle_repeat_overlap()` counts correctly: disjoint → 0,
  identical window → k, partial overlaps, k=0, n<k edge cases.
- **A2:** Retry loop semantics (unit-level, simulated with the plain
  Fisher-Yates on a scratch array): for a wrapped playlist whose tail
  tracks would land in the head, retrying seeds finds an order with
  zero overlap in ≤ 8 attempts in the common case (statistical test on
  fixed seeds, deterministic).
- **A3:** Simulator: with Repeat set to Shuffle, playback wraps without
  crashing and writes exactly one shuffle command per wrap to the
  control file.
- **A4:** Both hardware targets compile.

## Test plan

| Criterion | How verified |
|-----------|--------------|
| A1, A2 | unit tests in `tests/test_shuffle_locality.c` |
| A3 | simulator: autopilot, repeat=shuffle, wait through a wrap, inspect control file |
| A4 | hardware builds — user or CI |
