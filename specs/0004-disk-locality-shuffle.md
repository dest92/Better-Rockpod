# 0004: Disk-locality-aware shuffle for HDD

- **Status:** Implemented. A1–A4 verified by unit tests (36 checks,
  ASan/UBSan). A5/A6 verified on the ipod6g simulator: with the setting
  on, playback shuffles via the fallback path (control file records
  `S:`), survives a clean shutdown and resumes the same track/order
  after restart. A7 hardware builds pending (no ARM toolchain in the
  implementation environment) — note the disk path itself
  (`HAVE_DISK_SHUFFLE`) only compiles on hardware targets, so A7 is its
  first compile check. On-disk locality effect needs an HDD iPod.
- **Branch/PR:** claude/rockpod-features-roadmap-qg0tc3

## Problem

On spinning drives, shuffle produces a fully random track order, so every
rebuffer reads a handful of tracks scattered across the whole platter:
long seeks, longer spin-ups, worse battery and skip latency. The shuffle
is a plain Fisher-Yates over `playlist->indices[]`
(`randomise_playlist_unlocked()`, `apps/playlist.c:1518`) with no notion
of where files live on disk.

## Requirements

- **R1:** With the new setting on and a spinning disk active, shuffling
  orders tracks so that consecutive playlist neighbors tend to be
  physically near each other: tracks are sorted by disk position, split
  into fixed-size regions, and both the region order and the order
  within each region are shuffled.
- **R2:** The locality key is the file's FAT start cluster, obtained
  from dircache (`firstcluster`, cached per entry) via the playlist's
  existing per-track filerefs — no file opens at shuffle time. Cluster
  number is monotone in physical sector, so cluster order == disk order.
- **R3:** The result is always a valid permutation of the playlist, and
  the RNG discipline matches the existing code (`srand(seed)` + `rand()`),
  so a given (playlist order, keys, seed) reproduces the same result.
- **R4:** Fallback to the existing plain shuffle, transparently: setting
  off; SSD active (`ata_get_ssd_mode()`, `firmware/export/ata.h:226`);
  simulator/hosted builds; dircache off; temp allocation failure; or more
  than 25% of tracks with unresolvable keys.
- **R5:** Resume reproduces the order: disk-shuffle writes its own
  control-file command (`H:<seed>:<first_index>`; existing plain shuffle
  keeps `S:`). Replay sorts the list first (as `S:` does), waits for
  dircache readiness (`dircache_wait()`), regathers keys and re-runs the
  same algorithm. If keys are unavailable at replay (e.g. dircache now
  disabled), fall back to plain seeded shuffle — playback continues, the
  resume order may differ (documented degradation).
- **R6:** The pure ordering algorithm lives in a standalone
  host-testable header with no firmware includes.

## Non-goals

- Fragmentation awareness: only the start cluster is used. Music files
  are rarely fragmented in practice.
- Album/folder grouping (that's the separate album-shuffle roadmap item).
- Any change to plain shuffle, repeat-shuffle semantics, or playlist
  file formats beyond the new `H:` control command.
- Tuning UI for region size: one compile-time constant.

## Design sketch

New pure header `apps/shuffle_locality.h` (pattern:
`apps/gui/skin_engine/aa_color_math.h`):

- `shuffle_locality_order(const long *keys, int *order, int n)` — fills
  `order[]` with a permutation of 0..n-1: stable-sort positions by key
  (`SHUFFLE_KEY_UNKNOWN` sorts last), split into regions of
  `SHUFFLE_REGION_TRACKS` (32; a rebuffer spans ~5–15 tracks, so one
  region covers a few rebuffers), Fisher-Yates the region order, then
  Fisher-Yates within each region, all via `rand()` (caller seeds).

Integration:

- `firmware/common/dircache.c` / `dircache.h`: new accessor
  `dircache_get_fileref_firstcluster()` resolving a
  `struct dircache_fileref` to its entry's `firstcluster` (≥ 0) or a
  negative error, following the `dircache_get_fileref_path()` pattern.
- `apps/playlist.c`: `randomise_playlist_unlocked()` gains the locality
  path: gather keys into a temp `core_alloc` buffer
  (`amount * (sizeof(long) + 2*sizeof(int))` for keys + order + scratch;
  ~160 KB at 10k tracks, freed immediately), run the algorithm, apply
  the permutation to `indices[]` and `dcfrefs[]` together, write the
  `H:` command. Any failure → existing plain path (R4).
- Control replay (`playlist_resume` command parser): `H:` case mirrors
  the `S:` case (sort first) with the locality path.
- Setting: `global_settings.disk_shuffle` bool, `OFFON_SETTING`
  ("disk shuffle", default off), LANG string, menu entry beside Shuffle
  in the playback settings.

## Acceptance criteria

- **A1:** `order[]` is a valid permutation for n in {0, 1, 2, 31, 32,
  33, 1000} (bitmap check), keys clustered/equal/unknown/mixed.
- **A2:** Deterministic: same (keys, seed) twice → identical order;
  different seeds → different order (n large enough).
- **A3:** Locality: for keys drawn from k well-separated clusters, the
  mean |key[i+1] - key[i]| over the result is at least 5x smaller than
  the plain-shuffle baseline on the same input.
- **A4:** Unknown keys land in the trailing region(s) and are shuffled;
  all-unknown input degenerates to a plain full shuffle.
- **A5:** Playlist integration honors R4 fallbacks: with setting on in
  the simulator (no ATA/dircache keys), shuffle behaves exactly as
  before — no crash, valid playback order.
- **A6:** Resume in the simulator (kill + restart + Resume Playback)
  continues from the same track with setting on (fallback path).
- **A7:** Both hardware targets compile.

## Test plan

| Criterion | How verified |
|-----------|--------------|
| A1–A4 | unit tests `tests/test_shuffle_locality.c` (`make -C tests`) |
| A5, A6 | simulator: autopilot run, shuffle-play test tracks, restart, resume |
| A7 | hardware builds — user or CI (`./build-hw.sh`, `./build-hw.sh 5g`) |
| R1 on-disk effect | on-device by user (HDD iPod): listen for reduced seek noise / check spin-up frequency |
