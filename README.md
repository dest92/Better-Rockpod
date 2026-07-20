<p align="center">
  <h1 align="center">Better-Rockpod</h1>
  <p align="center">
    A fork of <a href="https://github.com/nuxcodes/rockpod">Rockpod</a> — custom Rockbox firmware
    for iPod Classic (6G/7G) and iPod Video (5G/5.5G).
  </p>
</p>

---

## About this fork

Better-Rockpod builds on top of [Rockpod](https://github.com/nuxcodes/rockpod),
a [Rockbox](https://www.rockbox.org) fork that adds MFi digital audio output,
a rewritten Cover Flow, dynamic album art colors, and SSD-aware power
management for the iPod Classic and iPod Video.

For everything Rockpod itself provides — features, supported models,
installation, releases, and roadmap — see the
[original Rockpod repository](https://github.com/nuxcodes/rockpod).

This fork focuses on album art handling and on making the codebase easier to
develop against, with a test-driven, spec-first workflow.

---

## What this fork adds

### Embedded album art in Cover Flow

Stock PictureFlow (Cover Flow) only finds album art stored as a separate
image file (`cover.jpg` / `folder.jpg`) next to the music. Better-Rockpod
extends the cache builder to read cover art **embedded in the audio files
themselves**:

- **MP3** — ID3v2 `APIC` frames
- **M4A/AAC/ALAC** — MP4 `covr` atoms
- **FLAC / Ogg Vorbis** — `METADATA_BLOCK_PICTURE`

No separate `cover.jpg` is required anymore: albums whose tracks carry
embedded artwork show up in Cover Flow automatically. Folder image files
still work and take priority, so existing libraries are unaffected.

### Album Art Fetcher (`tools/coverart`)

A companion PC-side tool for the tracks that still have no art at all —
neither embedded nor a folder cover file. It scans a music library, looks up
missing albums on the iTunes Search API, and embeds the matched artwork into
every track of the album.

```bash
pip install -r tools/coverart/requirements.txt

# Dry run (default) — reports what would change, touches nothing
python3 tools/coverart/fetch_coverart.py /path/to/music

# Apply — downloads and embeds the matched artwork
python3 tools/coverart/fetch_coverart.py /path/to/music --apply
```

Point `/path/to/music` at the iPod's music folder while it's connected in
disk mode (e.g. `/mnt/g/Music` on WSL, `E:\Music` on Windows,
`/Volumes/iPod/Music` on macOS). The scan prints progress for large
libraries, then a per-album report (found / embedded / not found / error /
skipped) and a final summary. Albums that already have art — embedded or a
`cover`/`folder`/`album` image file — are left untouched. Supports MP3,
M4A/AAC, FLAC, and Ogg Vorbis.

### Spec-driven development harness

Upstream Rockbox has no unit test framework. This fork adds one, plus a
documented workflow for making changes safely:

- **`tests/`** — host-side unit test harness (`make -C tests`) built with
  AddressSanitizer and UBSan. Decision logic is extracted into pure helpers
  so it can be tested on the host without hardware.
- **`specs/`** — every feature or behavior change starts from a written spec
  (`specs/TEMPLATE.md`): problem statement, numbered requirements, design
  sketch, acceptance criteria, and test plan.
- **`.claude/skills/`** — project workflow skills (`spec-dev`,
  `unit-tests`, `verify`) plus low-level embedded reference skills, so
  AI-assisted development follows the same spec-first, test-first process.

---

## Building

Same as upstream Rockpod:

```bash
./build-hw.sh          # iPod Classic 6G (default)
./build-hw.sh 5g       # iPod Video 5G
./build-sim.sh         # SDL simulator
make -C tests          # host unit tests
```

See the [Rockpod README](https://github.com/nuxcodes/rockpod#building-from-source)
for details on toolchains and installation.

---

## Credits

- [Rockpod](https://github.com/nuxcodes/rockpod) by nuxcodes — the base of
  this fork (MFi digital audio, Cover Flow, dynamic colors, power management)
- [Rockbox](https://www.rockbox.org/) and its contributors

## License

[GNU General Public License v2.0](https://www.gnu.org/licenses/old-licenses/gpl-2.0.html)
