# 0002: Host-side album art fetcher with web review UI

- **Status:** Implemented
- **Branch/PR:** `claude/ipod-rockbox-album-art-nx05vt`

## Problem

Tracks on the iPod without album art show the "?" slide in
PictureFlow and no cover in the WPS. Rockbox finds art either
embedded in tags (preferred by the fork's default `album_art`
setting, and read by PictureFlow since spec 0001) or as image files
next to the tracks (`apps/recorder/albumart.c:105` documents the
search order: `cover.jpg` / `cover.bmp` in the album directory,
etc.). The device itself has no network, so missing art can only be
fixed from a PC — today that means hunting images and tagging files
by hand, album by album.

Additional firmware constraint: embedded art must be **baseline
JPEG** — the core decoder rejects progressive DCT
(`apps/recorder/jpeg_load.c:1070`) and subsampling factors above 2x2
(`apps/recorder/jpeg_load.c:1048`), and embedded PNG/BMP is never
decoded (`AA_TYPE_JPG` check, spec 0001 R3). Random internet images
frequently violate this, so a fetcher must transcode, not just copy.

## Requirements

- **R1:** A host-side tool (`tools/albumart_fetcher/`, Python 3)
  scans a music directory (the USB-mounted iPod, or any path), groups
  tracks into albums by (albumartist|artist, album) tags, and
  reports each album as having art or missing it.
- **R2:** An album counts as "has art" if any track has embedded art
  of any type, or an art file exists per the `albumart.c` search
  order (at minimum `cover.{jpeg,jpg,bmp}` and
  `<albumname>.{jpeg,jpg,bmp}` in the track directory and its
  parent). Detection must not rewrite any file.
- **R3:** For albums missing art, the tool queries online sources
  and collects candidate images with their pixel size and source:
  iTunes Search API and MusicBrainz + Cover Art Archive (both
  keyless; MusicBrainz gets a proper User-Agent and 1 req/s
  throttling).
- **R4:** A local web UI (default `http://127.0.0.1:8000`, stdlib
  `http.server`, single HTML page, no external JS) lists albums
  missing art with their candidates, and lets the user per album:
  accept a candidate, paste an image URL, upload a local file, or
  skip. Nothing is written before the user confirms.
- **R5:** Applying art embeds it into every track of the album
  (ID3v2 APIC, FLAC picture block, MP4 `covr`, Vorbis/Opus
  `METADATA_BLOCK_PICTURE`) via mutagen. With `--cover-file` it also
  writes `cover.jpg` into the album directory.
- **R6:** Every applied image is normalized first: decoded with
  Pillow, downscaled so the longest side is ≤ 600 px (configurable),
  re-encoded as baseline (non-progressive) JPEG, 4:2:0 or 4:4:4
  subsampling — guaranteeing R6a: the bytes written are always
  decodable by `jpeg_load.c`.
- **R7:** Supported track formats: mp3, flac, m4a/alac, ogg, opus.
  Other files are ignored and reported as skipped.
- **R8:** The tool is idempotent and resumable: re-running the scan
  after applying art finds those albums complete; a failure on one
  album (network, corrupt file) is reported and does not abort the
  run.

## Non-goals

- No firmware/simulator changes — Rockbox already picks up embedded
  art and `cover.jpg` on its own.
- No fully automatic mode that writes art without review (can be a
  later spec once match quality is proven).
- No tag fixing (artist/album normalization) beyond reading tags.
- No embedding of PNG/BMP (firmware never decodes them embedded).
- No handling of the iTunes-synced (hidden `F00`) music layout —
  the tool targets drag-and-drop file trees Rockbox plays directly.

## Design sketch

New directory `tools/albumart_fetcher/` (Python ≥ 3.9, deps:
`mutagen`, `Pillow`, `requests` — declared in a `requirements.txt`):

- `scanner.py` — walks the tree, reads tags with mutagen, builds
  `Album` records. Pure decision helpers (`group_key(tags)`,
  `has_art(embedded_types, dir_listing, album, parent_listing)`)
  take plain values so they unit-test without files, mirroring the
  fork's "extract decision logic into pure helpers" rule.
- `sources.py` — `search_itunes(artist, album)`,
  `search_caa(artist, album)` returning `Candidate(url, size,
  source, thumb_url)`. Pure helper builds query strings/scoring.
- `imaging.py` — `normalize(img_bytes, max_side) -> jpeg_bytes`
  (Pillow: convert to RGB, resize, save baseline JPEG) and
  `is_rockbox_safe_jpeg(jpeg_bytes)` — a small SOF-marker parser
  that re-checks R6a exactly as `jpeg_load.c` does (baseline SOF0,
  sampling ≤ 2x2); used both as post-write assertion and unit-test
  oracle.
- `apply.py` — embeds per container via mutagen; writes
  `cover.jpg` when asked; fsyncs before reporting success (FAT on
  USB).
- `server.py` + `static/index.html` — stdlib `ThreadingHTTPServer`
  serving JSON endpoints (`/albums`, `/candidates/<id>`,
  `/apply/<id>`, `/upload/<id>`) and one self-contained page.
  Image thumbnails proxied through the server so the page needs no
  external origins.
- `__main__.py` — CLI: `python -m albumart_fetcher /Volumes/IPOD
  [--port 8000] [--cover-file] [--max-side 600] [--dry-run]`.

## Acceptance criteria

- **A1:** On a fixture tree (albums with: embedded art / cover.jpg
  only / no art), scan classifies each album correctly and modifies
  nothing.
- **A2:** For a real album missing art, the web UI shows ≥ 1
  candidate from the online sources, and accepting one embeds art
  in every track; a rescan then reports the album complete.
- **A3:** Every written image (embedded and `cover.jpg`) passes
  `is_rockbox_safe_jpeg` — including when the source image was
  progressive JPEG or PNG.
- **A4:** Files processed by the tool display their art in the
  simulator: WPS shows the cover and PictureFlow builds a real
  slide (both embedded-only and `--cover-file` variants).
- **A5:** A manual URL and a manual file upload apply exactly like
  a fetched candidate (normalized per R6).

## Test plan

The tool is Python, so it gets its own host tests in
`tools/albumart_fetcher/tests/` run with `python3 -m unittest`
(the C harness in `tests/` stays firmware-only; its Makefile is not
touched).

| Criterion | How verified |
|-----------|--------------|
| A1 | unit tests: `group_key`/`has_art` decision tables; integration test on a generated fixture tree with mutagen-crafted files |
| A2 | unit test with mocked HTTP for source parsing/scoring; live-network path manually on the fixture tree |
| A3 | unit test: feed progressive JPEG + PNG fixtures through `normalize`, assert `is_rockbox_safe_jpeg`; assert a progressive fixture *fails* the checker (oracle sanity) |
| A4 | simulator: copy processed fixture album into `simdisk/`, init database, check WPS cover and PictureFlow slide |
| A5 | unit test: upload/URL path shares the same apply code path (assert same bytes written) |
