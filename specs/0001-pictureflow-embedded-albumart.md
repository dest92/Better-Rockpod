# 0001: PictureFlow reads embedded album art

- **Status:** Agreed
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
