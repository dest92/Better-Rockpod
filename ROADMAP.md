# Better-Rockpod Roadmap

Prioritized roadmap of upcoming work for this fork, compiled July 2026 from
three sources:

1. **Fork ideas** — features grounded in the current codebase.
2. **Upstream Rockpod** — open issues and pull requests on
   [nuxcodes/rockpod](https://github.com/nuxcodes/rockpod) with real
   user-reported bugs and ready-made contributions.
3. **Upstream Rockbox** — keeping the base current with
   [rockbox.org](https://www.rockbox.org).

Every item lists its source, the affected area, a rough effort estimate
(S = hours, M = days, L = weeks), and a priority:

- **P0** — bugs that break core playback; fix first.
- **P1** — high-value features and ports.
- **P2** — nice-to-have, research, or long-running efforts.

Per the fork workflow (see `CLAUDE.md`), each implemented item starts from a
spec in `specs/` and, where the logic is host-testable, unit tests in
`tests/`.

---

## 1. Bug fixes (from Rockpod issues)

### 1.1 Hi-res FLAC intermittent playback failure — P0, effort M

- **Source:** [rockpod#17](https://github.com/nuxcodes/rockpod/issues/17)
- **Area:** SSD power management, codec buffering
  (`firmware/target/arm/s5l8702/`, `apps/buffering.c`)

96/176.4/192 kHz FLAC files fail intermittently (silent skip or no play) on
an iPod Classic 7G with iFlash SSD, while the same files play fine on stock
Rockbox 4.0. The reporter suspects Rockpod's two-phase sleep strategy and
pre-wake on backlight racing the codec buffer on high-bitrate files. This
fork inherits that power management code, so the bug almost certainly exists
here too. Reproduce with hi-res FLAC on SSD, then audit the sleep/wake path
against rebuffer timing. The intermittent, load-dependent nature points at a
race — instrument first, then fix.

### 1.2 Audio not playing when switching tracks (6G) — P0, effort M

- **Source:** [rockpod#13](https://github.com/nuxcodes/rockpod/issues/13)
- **Area:** playback engine / codec handoff (`apps/playback.c`,
  `apps/codec_thread.c`)

Track transitions occasionally produce silence on the iPod Classic 6G.
Needs a reliable reproduction first; may share a root cause with 1.1
(wake timing vs. rebuffer).

### 1.3 Dynamic colors: fix remaining glitches and harden — P1, effort M

- **Source:** fork idea (user-observed glitches)
- **Area:** `apps/gui/skin_engine/skin_albumart_color.c` and its hooks in
  `skin_display.c`, `skin_render.c`, `statusbar-skinned.c`,
  `apps/gui/list.c` / `bitmap/list-skinned.c`

The dynamic album-art colors feature still glitches occasionally. The git
history shows this area has needed repeated point fixes (WPS→menu flicker
in v5.3-hf1, background gaps in v5.3-beta.1, quantization fixes in v5.4,
SBS bar issues), which suggests remaining edge cases rather than one bug.
Plan of attack:

1. Catalog the known glitch scenarios (screen transitions during the 500ms
   fade, track change while in a menu, albums with no/low-color art,
   theme switches with dynamic colors enabled) and reproduce each in the
   simulator.
2. Audit the color state machine for transition races — the recurring
   flicker/gap pattern points at draw-order and stale-color windows during
   screen changes.
3. Extract the color extraction/quantization and fade-interpolation logic
   into pure helpers with unit tests in `tests/` (golden-value tests for
   representative album art), so future changes can't silently regress.
4. Improvements while in there: better accent-color choice for low-contrast
   art, and a sane fallback palette when extraction yields unusable colors.

### 1.4 Build error from main — P1, effort S

- **Source:** [rockpod#22](https://github.com/nuxcodes/rockpod/issues/22)
- **Area:** build system

A user hit a local build failure on Rockpod main. Verify whether this fork
builds clean from a fresh clone with the documented toolchain
(`./build-hw.sh`, `./build-hw.sh 5g`, `./build-sim.sh`); fix or document as
needed. Cheap insurance for every other item on this list — and a natural
companion to the CI work in 4.3.

### 1.5 Sony PHA-1A: detected but no USB audio — P2, effort M

- **Source:** [rockpod#24](https://github.com/nuxcodes/rockpod/issues/24)
- **Area:** MFi/iAP digital audio (`firmware/target/arm/s5l8702/` USB stack,
  iAP handshake)

The PHA-1A DAC is detected but never receives audio on v5.4.1. Likely an
iAP negotiation edge case. Hard to fix without the hardware — gather logs
from the reporter first (the debug log infrastructure from the v5.x dock
audio work should help).

---

## 2. Features (fork ideas)

### 2.1 Disk-locality-aware shuffle for HDD — P1, effort M *(flagship)*

- **Source:** fork idea
- **Area:** `apps/playlist.c`, `firmware/export/fat.h`, settings

On spinning drives, fully random shuffle order forces long seeks and disk
spin-ups on every rebuffer, costing battery and latency. Idea: make shuffle
aware of physical disk layout — bucket tracks by disk region, shuffle the
region order and the track order within each region, so each rebuffer reads
from nearby sectors instead of seeking across the whole platter.

Feasibility is confirmed in the existing code:

- `fat_query_sectornum()` (`firmware/export/fat.h:160`) returns the physical
  sector for a file position, so a start-sector per track is obtainable.
- Rockpod's Storage Mode already auto-detects HDD vs. iFlash SSD; the
  feature gates on HDD only (no benefit on SSD — normal shuffle there).
- The bucketing/permutation logic is pure and host-testable in `tests/`.

Exposed as a setting (e.g. Shuffle: Normal / Disk-optimized). Trade-off to
document in the spec: fewer seeks and spin-ups vs. a slightly less
random-feeling order (tracks ripped together tend to sit together on disk).

### 2.2 Shuffle anti-repeat memory — P1, effort S

- **Source:** fork idea
- **Area:** `apps/playlist.c` (`randomise_playlist_unlocked`)

Repeat Shuffle reshuffles with a fresh seed when the playlist wraps
(`apps/playlist.c:2947`), so a track played minutes ago can immediately come
back. Keep a small ring buffer of recently played track indices and bias the
new permutation to push those toward the end. Pure logic — ideal first
TDD candidate alongside 2.1.

### 2.3 Album shuffle — P2, effort M

- **Source:** fork idea (long-requested upstream Rockbox feature)
- **Area:** `apps/playlist.c`, settings

Shuffle album order while preserving track order within each album. Pairs
naturally with 2.1: albums are the natural disk-locality unit on most
libraries.

### 2.4 Power management & storage mode on iPod Video 5G — P1, effort L

- **Source:** [rockpod#23](https://github.com/nuxcodes/rockpod/issues/23)
- **Area:** `firmware/target/arm/pp/` (PP5022)

Rockpod's Storage Mode and enhanced power management are iPod Classic
(S5L8702) only. Port the HDD/SSD detection and the applicable power
strategies to the PP5022-based iPod Video. Board-level work; needs 5G
hardware for verification.

### 2.5 Album art fetcher improvements — P2, effort S–M

- **Source:** fork idea (extends `tools/coverart`, spec 0002)
- **Area:** `tools/coverart/`

Incremental improvements to the PC-side tool: MusicBrainz/Cover Art Archive
as a fallback when iTunes Search misses, resize/re-encode oversized art to a
target resolution before embedding, and an option to also write `cover.jpg`
for folder-based themes.

### 2.6 H.264/JPEG hardware decoding (continue PoC) — P2, effort L

- **Source:** [rockpod#21](https://github.com/nuxcodes/rockpod/issues/21)
  + this repo's `h264_poc` plugin
- **Area:** `apps/plugins/`, S5L8702 video hardware

The Rockpod maintainer reports H.264/JPEG hardware decoding on the 6G is
nearly done, with the VPP (video processing pipeline) as the last missing
piece. This repo already carries the `h264_poc` plugin with the full power
sequence. Track upstream progress and contribute rather than duplicate;
heavy reverse-engineering work.

---

## 3. Ports from open Rockpod pull requests

All four are by Olsro and unreviewed upstream; they can be cherry-picked or
reimplemented here. Credit the original author in commit messages.

### 3.1 PictureFlow/WPS return behavior — P1, effort S

- **Source:** [rockpod#5](https://github.com/nuxcodes/rockpod/pull/5)
- **Area:** `apps/plugins/pictureflow/`, WPS action handling

Don't return to PictureFlow from the WPS unless the WPS-select option is
set accordingly. Small UX fix that complements this fork's existing WPS
select action setting (v5.2).

### 3.2 Playlist viewer display format — P1, effort S

- **Source:** [rockpod#4](https://github.com/nuxcodes/rockpod/pull/4)
- **Area:** `apps/playlist_viewer.c`

New option to show entries as disc + track number + title + album instead
of filename.

### 3.3 Tagtree: play all tracks of all albums — P2, effort S

- **Source:** [rockpod#3](https://github.com/nuxcodes/rockpod/pull/3)
- **Area:** `apps/tagtree.c`

A button that lists/queues every track across all albums of the current
tagtree view.

### 3.4 Multiselect operations in browsers — P2, effort M

- **Source:** [rockpod#2](https://github.com/nuxcodes/rockpod/pull/2)
- **Area:** file browser, tagtree, playlist viewer (7 files, has conflicts)

Mass move/delete/add-next/add-to-playlist. The largest of the four; the PR
already carries merge conflicts and the author flags untested voicing and
touchscreen paths, so budget rework time.

### 3.5 Art browser plugin / blurred-art skin (ideas only) — P2

- **Source:** [rockpod#18](https://github.com/nuxcodes/rockpod/pull/18)
  (closed, unmerged)

Not worth porting as-is, but the ideas (art-first browsing, blurred album
art backdrops) fit this fork's dynamic-colors direction and could inform a
future spec.

---

## 4. Upstream Rockbox sync

### 4.1 Identify the exact fork point — P1, effort S

- **Source:** fork maintenance
- **Area:** git history

The Rockpod history was squashed (this tree starts at v4.7-alpha.16,
March 2026), so the Rockbox base commit is unknown. Rockbox 4.0 shipped
April 2025; the base is most likely a post-4.0 dev build. Determine the
fork point by diffing this tree against upstream Rockbox commits (e.g.
bisecting on file checksums of untouched upstream files). Everything else
in this section depends on knowing this.

### 4.2 Cherry-pick post-fork upstream fixes — P2, effort ongoing

- **Source:** fork maintenance;
  [rockpod#19](https://github.com/nuxcodes/rockpod/issues/19)
- **Area:** codecs (`lib/rbcodec/`), metadata parsing, playback core

With the fork point known, review upstream Rockbox changes since then and
cherry-pick what applies to the two iPod targets: codec fixes (FLAC
especially, given 1.1), metadata/album-art parsing improvements, and
playback-engine fixes. A wholesale rebase is not realistic given the size of
Rockpod's divergence (USB audio stack, PictureFlow rewrite, power
management) — targeted cherry-picks are the sustainable strategy.

### 4.3 CI builds via GitHub Actions — P1, effort S–M

- **Source:** [rockpod#19](https://github.com/nuxcodes/rockpod/issues/19)
- **Area:** `.github/workflows/`

A workflow that builds both hardware targets (`ipod6g`, `ipodvideo`), the
simulator, and runs `make -C tests` on every push. The cross-toolchain
(`tools/rockboxdev.sh`) is the main cost — cache the built toolchain as an
artifact or prebuilt container image. Directly answers the community request
in rockpod#19 and backstops every other roadmap item.

---

## Suggested order of attack

1. **4.3 CI** + **1.4 build check** — make every later change verifiable.
2. **2.2 anti-repeat shuffle** — small, pure-logic, exercises the TDD
   harness end to end.
3. **1.3 dynamic colors hardening** — user-visible glitches, fully
   reproducible in the simulator, and yields reusable unit tests.
4. **2.1 disk-locality shuffle** — the flagship feature, building on 2.2's
   test scaffolding.
5. **1.1 hi-res FLAC race** — highest-impact bug; needs SSD hardware and
   instrumentation time.
6. **3.1 / 3.2 PR ports** — quick community-visible wins.
7. **4.1 fork point** → **4.2 cherry-picks** — background maintenance
   track.
