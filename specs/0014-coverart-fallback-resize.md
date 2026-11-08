# 0014: Album art fetcher: MusicBrainz fallback, resize, cover.jpg

- **Status:** Implemented
- **Branch/PR:** `master` (roadmap 2.5, extends spec 0002)

## Problem

`tools/coverart` (spec 0002) only searches iTunes, so albums iTunes
doesn't carry stay without art. It also embeds the downloaded image
as-is: iTunes art is 600x600 and other sources can be much larger or
progressive JPEGs. Rockbox's JPEG decoder can't decode progressive
JPEGs, and large images slow PictureFlow. Some themes read a folder
`cover.jpg` instead of embedded art.

## Requirements

- **R1:** When iTunes has no match (or fails), search MusicBrainz
  release groups. The best match must score at least 90. Then use its
  Cover Art Archive front image (`front-500`). `--no-musicbrainz`
  disables this. Requests send a descriptive User-Agent (no personal
  data) and respect 1 request/second.
- **R2:** `--max-size PX` downscales the longest side to at most PX
  (never upscales) and re-encodes as baseline (non-progressive) JPEG.
  PNG input becomes JPEG.
- **R3:** `--write-cover-file` also writes `cover.jpg` into the album
  folder when the art is JPEG.
- **R4:** The dry-run default and the skip rules from spec 0002 are
  unchanged.

## Acceptance criteria

- **A1:** Unit tests: MusicBrainz URL building, best-match selection
  and threshold, the CAA URL, the `fit_size` geometry, and
  `prepare_image` (passthrough, resize to baseline JPEG, PNG to JPEG).
- **A2:** Live run against real services embeds art via iTunes with
  `--max-size 500 --write-cover-file`. A forced iTunes miss falls back
  to MusicBrainz/CAA. A nonexistent album reports not found.

## Verification results

- **A1:** `tools/coverart/test_fetch_coverart.py`, 39 tests pass.
- **A2:** Live run, 2026-11-08.
  - "Radiohead - OK Computer" (MP3) and "Kid A" (FLAC) were embedded as
    500x500 non-progressive JPEG, and `cover.jpg` was written in both
    folders.
  - With iTunes stubbed to return no results, `find_artwork` returned
    the Cover Art Archive URL for release group `b1392450-…` (OK
    Computer). The 85 KB image downloaded and was resized to 300x300.
  - A nonexistent artist/album returned `(None, None)`.

## Note on the duplicate fetcher branch

The `claude/ipod-rockbox-album-art-nx05vt` branch carries a separate
tool, `tools/albumart_fetcher`, with a web review UI. It also numbers
its spec 0002, so it conflicts with this tree's spec 0002. That branch
is not merged: `tools/coverart` remains the supported tool. The web
review UI is a candidate for a later spec, ported onto `tools/coverart`
rather than kept as a second tool.
