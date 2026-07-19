# 0002: Automatic album art fetcher (PC-side tool)

- **Status:** Draft
- **Branch/PR:**

## Problem

Even with PictureFlow now reading embedded album art (spec 0001), many
tracks in a user's library have neither an embedded picture nor a
folder-level cover file at all — so PictureFlow still shows "?" for
those albums. Finding and tagging each one by hand is tedious. This is
a companion PC-side tool, not a firmware/plugin change: it runs on the
user's computer against the iPod mounted as a USB disk, finds tracks
missing art, and embeds a downloaded cover into them so PictureFlow
(and the WPS) picks it up automatically afterward.

## Requirements

- **R1:** Recursively scan a given directory (the iPod's music
  folder) for audio files: MP3, M4A/AAC, FLAC, Ogg Vorbis.
- **R2:** Group tracks into albums by (album, album_artist or
  artist). For each album, determine whether it already "has art":
  any track has an embedded picture tag, OR a cover-style file
  (`cover.jpg/jpeg/png`, `folder.jpg/jpeg/png`, `album.jpg/jpeg/png`,
  case-insensitive) exists in the album's directory. Albums with
  existing art are left alone entirely (R2 is the same kind of check
  Rockbox itself does, simplified for PC use — not required to match
  `search_albumart_files()` byte-for-byte).
- **R3:** For albums missing art, query the iTunes Search API
  (`https://itunes.apple.com/search?term=...&media=music&entity=album`)
  with `artist + album`. Take the top result's artwork URL, upsized
  from the default 100x100 to 600x600 (standard `.../100x100bb.jpg` →
  `.../600x600bb.jpg` URL substitution).
- **R4:** Default mode is dry-run: print a report (album, whether a
  match was found, matched title/artist for sanity-checking, artwork
  URL) without downloading or modifying anything.
- **R5:** `--apply` mode: download the matched artwork and embed it as
  a picture tag into **every track of the album** that's missing art
  (not just one — so the WPS shows art regardless of which track is
  playing, and behavior doesn't depend on which track Rockbox's
  tagcache happens to treat as the album's "first").
- **R6:** Never touch a track/album that already has art (embedded or
  folder-level) — R2's detection is also the skip condition for R5.
- **R7:** iTunes API calls are rate-limited (a small delay between
  requests) and tolerate a no-match or network error for one album
  without aborting the whole run — reported as "not found" / "error"
  and the script continues.
- **R8:** Clear per-album progress output: found / not found / error /
  skipped (already has art), plus a final summary count.

## Non-goals

- No MusicBrainz/other source fallback in this version (iTunes only
  for now; the detection/query logic should stay decoupled enough
  that a second source could be added later without a rewrite).
- No GUI, no "watch for iPod connection" automation — the user runs
  the script manually, pointed at a path (the mounted iPod drive).
- No changes to Rockbox firmware or the PictureFlow plugin — this is
  a standalone PC tool.
- No attempt to fix/improve existing embedded or folder art (only
  fills in *missing* art).

## Design sketch

New Python script under `tools/coverart/fetch_coverart.py` (a new
`tools/` subfolder, matching the existing convention of `tools/`
holding developer-facing scripts, distinct from firmware code).
Dependencies: `mutagen` (read/write tags across MP3/M4A/FLAC/Ogg —
already used ad-hoc during spec 0001's verification) and `requests`.

- `scan_library(root) -> list[Album]`: walks the directory, groups
  files into `Album` records (path, artist, album, track file list).
- `album_has_art(album) -> bool`: pure-ish function (only touches the
  filesystem/tags, no network) — checked first against R2's rule.
  This is the piece most worth unit-testing without mocking anything,
  since it only needs a temp directory with fixture files.
- `build_itunes_query(artist, album) -> str`: pure function building
  the query string/URL — fully unit-testable without network.
- `fetch_artwork_url(artist, album) -> str | None`: the actual iTunes
  API call (network — not unit tested, manually verified).
- `apply_artwork(album, image_bytes)`: embeds via mutagen
  (`EasyID3`/`ID3` APIC for MP3, `MP4Cover` for M4A, `Picture` for
  FLAC/Ogg) into every track in the album.
- `main()`: CLI with `--apply` flag (default dry-run per R4), takes
  the library path as a positional argument.

## Acceptance criteria

- **A1:** Dry-run against a test folder with a mix of (a) an album
  with embedded art, (b) an album with a folder cover, (c) an album
  with neither, produces a report listing only (c) as needing art,
  with no files modified.
- **A2:** `--apply` embeds the fetched cover into every track of an
  album that had none, verified by reading the tag back afterward.
- **A3:** Albums from (a) and (b) are untouched byte-for-byte after a
  run (mtimes/hashes unchanged).
- **A4:** An album with no iTunes match is reported as "not found" and
  does not stop the rest of the scan from completing.
- **A5:** Re-running after a successful `--apply` treats the
  now-tagged album as "has art" and skips it (no duplicate work, no
  re-download).

## Test plan

| Criterion | How verified |
|-----------|--------------|
| A1, A3, A5 | unit tests for `album_has_art()` against fixture directories (temp dirs with real small media files, no network) |
| A4 | unit test for `build_itunes_query()`, plus a manual run against a made-up artist/album to confirm graceful "not found" handling |
| A2 | manual end-to-end run against a small real test library (reusing fixtures from spec 0001 verification), confirming the embedded tag round-trips via mutagen |
