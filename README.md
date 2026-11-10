<p align="center">
  <h1 align="center">Better-Rockpod</h1>
  <p align="center">
    A fork of <a href="https://github.com/nuxcodes/rockpod">Rockpod</a>, custom Rockbox firmware
    for iPod Classic (6G/7G) and iPod Video (5G/5.5G).
  </p>
</p>

---

## About this fork

Better-Rockpod builds on [Rockpod](https://github.com/nuxcodes/rockpod), a
[Rockbox](https://www.rockbox.org) fork with MFi digital audio output, a
rewritten Cover Flow, dynamic album art colors and SSD-aware power
management for the iPod Classic and iPod Video. It tracks Rockpod
(currently v5.5+) and upstream Rockbox fixes.

On top of that it adds the features and fixes below, a new theme, PC-side
tools and a test-driven, spec-first development workflow. Each change has
a spec in [`specs/`](specs/), host unit tests where the logic allows,
simulator verification, and CI builds for both iPods.

> **Beta:** items marked (*) are verified in the simulator and in CI
> builds but still need confirmation on real hardware. See
> [`docs/on-device-tests.md`](docs/on-device-tests.md).

---

## Features

### Playback and library

- **Shuffle Albums** (Settings → Playback): albums play in random order,
  each album in track order. Resume restores the same order. *(spec 0013)*
- **[By album, random order]** in the database: every track of an
  artist, grouped by album, with the album order shuffled each time, like
  the stock iPod OS. *(spec 0012)*
- **Disk-optimized Shuffle** for hard drives: the shuffle order follows
  the disk layout so the drive seeks less. It turns itself off on flash
  storage. *(spec 0004)*
- **Repeat Shuffle without repeats**: a reshuffle doesn't bring back the
  tracks you just heard. *(spec 0005)*
- **Multiselection** in the file browser, database and playlist viewer.
  Select a range to play next, add to a playlist, or, in the playlist
  viewer, remove or move it. *(spec 0016)*
- **Playlist viewer**: a "CD & Track num & Title & Album" display, and
  the "Track display" setting works again. *(spec 0011)*
- **WPS stop**: holding PLAY in the Now Playing screen stops playback.
  It opens Cover Flow only when "WPS select action" is set to Cover Flow.
  *(spec 0010)*

### Cover Flow and album art

- **Embedded album art in Cover Flow**: covers stored inside the files
  are found, so no separate `cover.jpg` is needed. Supported: MP3 (ID3v2
  `APIC`), M4A/AAC/ALAC (MP4 `covr`) and FLAC/Ogg Vorbis
  (`METADATA_BLOCK_PICTURE`). Folder images still work and take
  priority. *(spec 0001)*
- **Cover Flow stage**: a gradient background lit behind the covers in
  the theme's colors, with a glass edge at the reflection line and
  shadowed album/artist titles. *(spec 0021)*
- **Dynamic colors**: fades take one step per frame, so a fade no longer
  mixes colors within a frame. *(spec 0015; complementary accent for
  low-contrast art, spec 0003)*

### Speed and smoothness

- **Boost While Navigating** (Settings → General → System, on by
  default): button and wheel input runs the CPU at full speed until a
  second after the last input. The UI otherwise runs at 54 MHz on the 6G
  and 30 MHz on the 5G. List scrolling and entering menus, including
  their loading, get faster. *(spec 0020)* (*)

### Hardware

- **Hardware H.264 video** on the iPod Classic 6G: `.mp4`, `.m4v` and
  `.mov` files with Constrained Baseline H.264 up to 640x480, with
  AAC audio. Ported from upstream Rockbox. *(spec 0017)* (*)
- **Storage Mode on the iPod Video**: auto/HDD/SSD now works on the 5G,
  so flash upgrades (iFlash, SD) get the SSD behaviors.
  *(spec 0018)* (*)
- **Format-switch fix (6G/7G)**: the codec is muted while the sample rate
  changes. This targets playback stalling when switching between tracks
  of different rates (rockpod#13, #17). *(spec 0009)* (*)
- **Low-battery shutdown debounce**: a brief voltage dip no longer powers
  the iPod off. *(spec 0007)* (*)
- **Battery calibration**: an on-device plugin plus the PC tool below
  produce an accurate discharge curve. *(specs 0006, 0008)*

### Upstream fixes

The fork point and every cherry-picked fix are documented in
[`docs/upstream-sync.md`](docs/upstream-sync.md). Highlights:

- **Codecs:** FLAC robustness (corrupt and hi-res streams, oversized
  frames) and ALAC fixes.
- **Album art:** JPEG decoding fixes; FLAC, ID3, MP4 and APE metadata
  hardening.
- **Equalizer:** the low-band DC offset in the ARM EQ filter is fixed.
- **Playlist and database:** fixes in the playlist, tagcache and
  tagtree, including crashes while the database commits.
- **6G storage:** the drive is no longer powered off before its cache is
  flushed (*).
- **5G:** click-wheel recovery (*).
- **Playback:** gapless playback with fatigue reduction, and no
  crossfade after a pause.
- **Accessories:** Rockpod's iAP overhaul for docks, car kits and
  remotes.

---

## Aero7 theme

[`themes/Aero7`](themes/Aero7) is a Frutiger Aero / Windows 7 glass
theme for both iPods:

- **Background:** a blue wallpaper with light ribbons, an aqua glow and
  bubbles.
- **Glass:** a glass taskbar, and floating glass panels with soft
  shadows and hairline light rims.
- **Menus:** a transparent menu list with white text and an aqua
  selection pill.
- **Now Playing:** album art on a soft shadow, a slim progress bar with
  a glass knob, and a floating dock with a Media Player style
  play/pause orb.
- **Font:** Open Sans (OFL).

Copy `themes/Aero7/.rockbox/` to the iPod, then pick **Settings → Theme
Settings → Browse Theme Files → Aero7**. The theme is generated by
`tools/themes/make_aero7.py`; see `themes/Aero7/README.md`.

---

## PC tools

### Album art fetcher (`tools/coverart`)

Finds albums with no art at all, neither embedded nor a folder image. It
looks them up on iTunes, falling back to MusicBrainz / Cover Art Archive,
and embeds the match into every track.

```bash
pip install -r tools/coverart/requirements.txt

# Dry run (default): reports what would change, touches nothing
python3 tools/coverart/fetch_coverart.py /path/to/music

# Recommended for the iPod: 500px baseline JPEG, plus cover.jpg per folder
python3 tools/coverart/fetch_coverart.py /path/to/music --apply \
    --max-size 500 --write-cover-file
```

`--max-size` needs Pillow and re-encodes the art as baseline JPEG, which
Rockbox requires (it can't decode progressive JPEGs).
`--no-musicbrainz` disables the fallback. Requests are retried when the
service is rate-limited. Supports MP3, M4A/AAC, FLAC and Ogg Vorbis.

### Battery calibration (`tools/battcal`)

Turns a discharge log recorded by the on-device battery calibration
plugin into a discharge curve for your battery. See
`tools/battcal/README.md`.

### Device test media (`tools/devicetest`)

`tools/devicetest/make_test_media.sh DIR` generates the files used by
[`docs/on-device-tests.md`](docs/on-device-tests.md):
- format- and sample-rate-switch playlists;
- 24-bit hi-res FLAC;
- an H.264 Baseline test video.

---

## Installing

1. Install the Rockbox bootloader if the iPod still runs Apple's
   firmware (see the
   [Rockpod instructions](https://github.com/nuxcodes/rockpod)).
2. Download the zip for your model (`ipod6g` = Classic 6G/7G,
   `ipodvideo` = Video 5G/5.5G). Use the release, or the `rockbox-*`
   artifact of the latest green [CI run](../../actions).
3. With the iPod in disk mode, extract it to the root of the iPod and
   replace `.rockbox` entirely, plugins included. The plugin API changed,
   so old `.rock` files won't load.
4. Eject safely and reboot (Menu + Select).

---

## Building

```bash
./build-hw.sh          # iPod Classic 6G (default)
./build-hw.sh 5g       # iPod Video 5G
./build-sim.sh         # SDL simulator
make -C tests          # host unit tests (ASan + UBSan)
```

The ARM toolchain comes from `tools/rockboxdev.sh --target=a`. On Linux
it needs `texinfo libtool-bin flex bison`, and the simulator needs
`libsdl2-dev`. GitHub Actions builds the host tests, the simulator and
both firmwares on every push. Pushing a `better-v*` tag publishes a
release.

## Development workflow

- **`specs/`:** every feature or behavior change starts from a written
  spec (`specs/TEMPLATE.md`).
- **`tests/`:** host unit tests; decision logic is extracted into pure
  helpers so it can be tested without hardware.
- **`research/`:** investigation notes, such as UI smoothness and
  clean-room notes on Apple's iPod Video firmware.
- **`ROADMAP.md`, `milestone.md`:** what is done and what is next.
- **`.claude/skills/`:** project workflow skills (`spec-dev`,
  `unit-tests`, `verify`).

---

## Credits

- [Rockpod](https://github.com/nuxcodes/rockpod) by nuxcodes: the base of
  this fork (MFi digital audio, Cover Flow, dynamic colors, power
  management).
- Paul Sauro (Olsro): the WPS stop, playlist viewer, random album order
  and multiselection contributions (rockpod#2–#5).
- David Cormier: hardware H.264 playback (upstream Rockbox).
- [Rockbox](https://www.rockbox.org/) and its contributors.
- Open Sans: The Open Sans Project Authors (SIL OFL 1.1).

## License

[GNU General Public License v2.0](https://www.gnu.org/licenses/old-licenses/gpl-2.0.html)
