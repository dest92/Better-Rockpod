# 0002: Automatic album art fetcher (PC-side tool)

- **Status:** Implemented
- **Branch/PR:** `claude/picture-flow-album-art-9hzu6g`

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
| A1, A3, A5 | unit tests for `album_needs_art()`/`folder_has_cover_file()`/`group_tracks_into_albums()` (22 tests, `python3 -m unittest test_fetch_coverart`) |
| A4 | unit test for `pick_itunes_artwork()` with an empty-results response, plus a manual dry-run with the iTunes call mocked to return 0 results for one album and a match for another - confirmed the scan continued and both were reported correctly |
| A2 | manual end-to-end run with `fetch_itunes_json`/`download_image` mocked (real network blocked by this sandbox's egress policy - see below), embedding real image bytes into all 3 tracks of a real multi-track MP3 album via the actual `--apply` CLI path, verified via `has_embedded_art()` afterward |

## Verification results

- **Unit tests:** all 22 pass (`python3 -m unittest tools/coverart/test_fetch_coverart -v`).
- **Real-file, no-mock verification:** built a real test library (ffmpeg-generated
  MP3/M4A/FLAC/Ogg files, one plain, one with real embedded art, one with
  a folder `cover.jpg`) and ran the actual `scan_library`/`album_needs_art`/
  `folder_has_cover_file` pipeline against it - correctly identified the one
  album needing art and skipped the other two; re-running after a manual
  embed correctly skipped it too (idempotency, A5). `embed_artwork` verified
  directly against real files for all four formats (MP3/M4A/FLAC/Ogg) -
  the embedded bytes round-trip exactly and `has_embedded_art()` flips
  false→true.
- **Network path:** this sandbox's outbound proxy blocks `itunes.apple.com`
  by organization egress policy (confirmed via the proxy status endpoint,
  not a bug in the tool) - the real HTTP call to the iTunes Search API
  could not be exercised live here. Verified the full `--apply` pipeline
  instead with `fetch_itunes_json`/`download_image` monkey-patched to
  return iTunes-shaped canned data, confirming: the 100x100→600x600 URL
  upsizing is actually used end-to-end, every track in a 3-track album
  gets the artwork embedded, and a 0-result album is reported as
  "not found" without stopping the scan (A4). **The user should do one
  live run against a real album on their own machine** to confirm the
  actual iTunes HTTP call and JSON shape haven't drifted from what's
  assumed here.
