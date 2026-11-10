First Better-Rockpod build. It is based on Rockpod v5.5+ and upstream
Rockbox `dd21a1d1d`, plus the fixes and features below.

**Beta:** every change is unit-tested, simulator-tested and built in
CI, but several haven't been confirmed on real hardware yet. See
`docs/on-device-tests.md`, and please report results.

## Install

1. Download the zip for your iPod:
   - `ipod6g`: iPod Classic 6G/7G;
   - `ipodvideo`: iPod Video 5G/5.5G.
2. Extract it to the root of the iPod (disk mode), replacing the
   existing `.rockbox` folder contents. The Rockbox bootloader must
   already be installed.
3. **Replace all plugins.** The plugin API changed, so `.rock` files
   from an older build won't load.

`SHA256SUMS` lists the checksums of both zips.

## New

- **Aero7 theme** (Frutiger Aero / Windows 7 glass) with a transparent
  menu list, glass Now Playing card and dock. It ships in the zip; pick it
  under Theme Settings.
- **Boost While Navigating** (Settings → General → System): the CPU runs
  at full speed while you scroll and enter menus, and drops back a
  second after the last input.
- **Cover Flow stage:** a lit gradient background, a glass edge and
  shadowed titles.

- **Shuffle Albums** (Settings → Playback): random album order, each
  album in track order. Resume keeps the order.
- **[By album, random order]** in the database, like the stock iPod.
- **Multiselection** in the file browser, database and playlist
  viewer: play next, add to a playlist, remove or move a range.
- **Playlist viewer** "CD & Track num & Title & Album" display, and the
  Track display setting works again.
- **Hardware H.264 video** on the iPod Classic 6G (`.mp4/.m4v/.mov`,
  Baseline ≤ 640x480).
- **Storage Mode** (auto/HDD/SSD) now works on the iPod Video.
- **PictureFlow** reads embedded cover art.
- Long-press PLAY in the WPS opens PictureFlow only when "WPS select
  action" is Cover Flow; otherwise it stops playback.

## Fixes

- 6G/7G: possible fix for playback stalling when switching between
  tracks of different sample rates (rockpod#13, #17).
- 6G: the drive is no longer powered down before its write cache is
  flushed (upstream).
- Dynamic colors: no mixed colors within a frame during fades.
- More upstream fixes: oversized FLAC frames, the DC offset in the ARM
  EQ filter, and database crashes during a commit.
- 39 upstream Rockbox fixes: FLAC, ALAC, JPEG album art, metadata,
  playlist and database robustness, and click-wheel recovery on the 5G.
- Low-battery shutdown debounce, battery calibration, disk-locality
  shuffle and Repeat Shuffle anti-repeat (from the roadmap milestone).

## PC tools

`tools/coverart` fetches missing album art (iTunes → MusicBrainz),
resizes it to baseline JPEG and can write `cover.jpg`.
