# 0001: PictureFlow reads embedded album art

- **Status:** Implemented
- **Branch/PR:** `claude/picture-flow-album-art-9hzu6g`

## Problem

PictureFlow only finds cover art stored as separate image files
(`cover.jpg`, `<album>.bmp`, ...) via `search_albumart_files()`
(`apps/recorder/albumart.c:121`). Art embedded in the audio files'
tags (ID3 APIC, MP4 covr, FLAC/Vorbis METADATA_BLOCK_PICTURE) is
ignored, so albums show the "?" slide even though the WPS displays
their cover fine. Users must export covers to files just for
PictureFlow.

The core already decodes embedded JPEG art: `clip_jpeg_fd()`
(`apps/recorder/jpeg_load.c:2059`) handles ID3-unsync and
Vorbis-base64 wrapping, and `apps/buffering.c:867` +
`apps/playback.c:1940` use it for the WPS, honoring the `album_art`
setting (off / prefer embedded [default] / prefer image file,
`apps/settings.h:150`). But `clip_jpeg_fd` is not exported in the
plugin API, and PictureFlow's tagcache-RAM path
(`retrieve_id3`, `pictureflow.c:2132`) never fills
`mp3entry.has_embedded_albumart`.

## Requirements

- **R1:** During PictureFlow cache build, an album whose first track
  has embedded JPEG art and no cover file gets a real slide, not "?".
- **R2:** Source order follows the core `album_art` setting:
  `prefer image file` tries files first with embedded as fallback;
  otherwise embedded is tried first with files as fallback.
- **R3:** Embedded art of unsupported type (PNG/BMP) is skipped and
  the existing file search still runs (same criterion as core:
  `(type & AA_CLEAR_FLAGS_MASK) == AA_TYPE_JPG`).
- **R4:** Existing caches rebuild automatically after upgrade
  (CACHE_VERSION bump 5 → 6).
- **R5:** Runtime slide browsing performance is unchanged (only cache
  build reads audio-file headers).

## Non-goals

- Decoding embedded PNG/BMP (core doesn't either; rare in practice).
- Changing WPS/core album art behavior.
- New user-facing settings — the core `album_art` setting is reused.

## Design sketch

1. Export `clip_jpeg_fd` in the plugin API (`apps/plugin.h` end of
   struct + `apps/plugin.c`, guarded `#ifdef HAVE_JPEG`; bump
   `PLUGIN_API_VERSION` 280→281, MIN unchanged).
2. `struct albumart_t` (pictureflow.c:408) gains
   `bool embedded; struct mp3_albumart aa;`.
3. `get_albumart_for_index_from_db()` (pictureflow.c:2151) calls
   `rb->get_metadata()` (real file parse — the tagcache-RAM shortcut
   lacks embedded info) and picks the source per R2/R3. Selection
   order logic is extracted into a pure helper
   (`apps/plugins/pictureflow/aa_source.h`) so it is host-testable.
4. `incremental_albumart_cache()` (pictureflow.c:2362): when
   embedded, `rb->open` the track, `lseek` to `aa.pos`,
   `rb->clip_jpeg_fd(fd, aa.type, aa.size, ..., &format_transposed)`,
   else `read_image_file()` as today; then `save_pfraw()` unchanged.
5. `CACHE_VERSION` 5 → 6 (pictureflow.c:333).

## Acceptance criteria

- **A1:** Album with embedded-JPEG-only tracks shows its cover in
  PictureFlow after cache build.
- **A2:** Album with both `cover.jpg` and embedded art follows the
  `album_art` setting order.
- **A3:** Album with no art at all still shows the "?" slide, no
  crash/regression.
- **A4:** Source-selection helper returns the right order for all
  combinations of setting × has-file × has-embedded-jpg.
- **A5:** Both hardware targets build; sim build runs PictureFlow.

## Test plan

| Criterion | How verified |
|-----------|--------------|
| A4 | unit test `tests/test_pf_aa_source.c` (red first) |
| A1, A2, A3 | simulator: tagged media in `simdisk/`, init database, run PictureFlow with each `album_art` setting |
| A5 | `./build-hw.sh` + `./build-hw.sh 5g` + `./build-sim.sh` |
| R5 | code inspection: no new I/O in slide render path |

## Verification results

- **Unit tests:** `make -C tests` — `test_pf_aa_source` and
  `test_fixedpoint` (unrelated, pre-existing) both pass under
  ASan+UBSan. (Check count changed from 15 to 12 after the post-review
  fix removed the dead `enabled` parameter — see below.)
- **Simulator (A1/A2/A3):** built `ipod6g` sim, generated MP3s with
  embedded ID3v2 APIC JPEGs (ffmpeg + PIL) covering: embedded-only,
  both cover-file-and-embedded, and no-art-at-all. Confirmed via a
  temporary debug splash that `get_metadata()`/`pick_albumart_source()`
  extract the exact correct embedded-picture `pos`/`size`/`type`
  (cross-checked by hand against the raw ID3 bytes). Confirmed
  end-to-end in PictureFlow: albums with embedded-only art show real
  decoded covers (previously "?"), the no-art album still shows "?"
  with no crash, and switching the `album_art` setting to "prefer
  image file" correctly changes lookup order (verified the file-search
  skip/no-skip behavior via `pf_aa_needs_file_search`). One dead end
  worth recording: initial test JPEGs were flat solid colors and
  decoded as flat gray — reproduced identically via the unrelated,
  unmodified `imageviewer` plugin, isolating it as a pre-existing
  decoder limitation with flat/solid-color JPEGs, not a bug in this
  feature. Switching test fixtures to gradient images decoded
  correctly and matched expectations.
- **Hardware builds (A5):** NOT verified — this environment has no
  `arm-elf-eabi` cross-compiler and cannot build one (no network
  access for `tools/rockboxdev.sh` source fetches, and substituting a
  generic `arm-none-eabi-gcc` showed compiler-macro mismatches
  (`ARM_PROFILE`/`ARM_ARCH` undefined) indicating it isn't a safe
  stand-in for Rockbox's patched toolchain). The plugin API change is
  a 2-line, `#ifdef HAVE_JPEG`-gated addition following the exact
  existing pattern of `read_jpeg_fd`/`read_jpeg_file` in the same
  struct/table, and the rest of the diff is target-independent C. Run
  `./build-hw.sh` and `./build-hw.sh 5g` to confirm before shipping.
- **Unrelated fix required to build at all:** `apps/gui/list.c`
  referenced a `callback_draw_margin` struct field that was never
  declared (introduced by an unrelated earlier commit), which broke
  compilation of every target including the simulator. Reverted that
  one dead conditional to its prior working form (see commit) — this
  is unrelated to album art and pre-dates this branch's work. Left an
  in-code comment naming the never-implemented feature it was for, so
  a proper re-implementation (actually declaring the field) has
  something to find.

## Post-review fixes (code-review pass)

A `/code-review` pass found four correctness regressions and one
simplification opportunity, all fixed and re-verified in the simulator:

- **CACHE_VERSION bump defeated by stale-cache reuse**: the per-album
  "keep existing albumart" shortcut in `incremental_albumart_cache()`
  is keyed on `pf_cfg.update_albumart`, independent of `cache_version`,
  so a user upgrading with old `.pfraw` files already on disk (and
  `update_albumart` persisted `true` from a prior "Update cache") would
  have kept pre-embedded-art covers forever. Fixed by forcing
  `update_albumart` off for the one rebuild triggered by a version
  mismatch, then restoring the user's setting
  (`apps/plugins/pictureflow/pictureflow.c`, `init()`). Verified by
  hand-crafting that exact old-firmware state (stale `.pfraw`,
  `cache_version=5`, `update_albumart=1`) and confirming the file gets
  regenerated with the correct embedded cover, while the setting is
  still `1` afterward.
- **`AA_OFF` disabled all art, not just embedded**: `pick_albumart_source`
  now only suppresses *embedded* art when the "album art" setting is
  off; the file-cover search still runs unconditionally, matching
  PictureFlow's behavior from before this feature existed. Verified:
  under `album_art=off`, an album with both a cover file and embedded
  art now shows the file cover (previously showed nothing).
- **No fallback when the embedded picture fails to decode**: if
  `clip_jpeg_fd` returns an error (corrupt/truncated embedded picture),
  `incremental_albumart_cache()` now retries with
  `search_albumart_files()` before giving up, so a coexisting cover
  file still gets used instead of falling back to "?".
- **`retrieve_id3()`'s RAM-tagcache fast path was dropped entirely**:
  `get_albumart_for_index_from_db()` now tries the full
  `get_metadata()` parse (needed to see embedded art) and falls back to
  `retrieve_id3()` if that fails, restoring the old resilience for
  file-based covers when the full parse can't succeed for some reason.
- **Dead `enabled` parameter removed** from `pf_aa_select_source()` in
  `aa_source.h` (its only real call site always passed `true`, since
  `AA_OFF` is now handled by forcing `have_embedded_jpg` false instead)
  — `tests/test_pf_aa_source.c` updated accordingly (12 checks).

Not fixed, by choice: the double file-open for embedded art (one
`get_metadata()` open + one explicit reopen for `clip_jpeg_fd`) and the
duplication of embedded-vs-file selection logic against
`playback.c`/`buffering.c` — both flagged as real but low-severity/low
actionability (the former needs a `get_metadata()` API change to return
a reusable fd; the latter is a structural consequence of plugins not
being able to call core's static functions). Left as documented code
comments instead of risking a larger, riskier refactor.
