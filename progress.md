# Progress log

Chronological log for the [milestone.md](milestone.md) work. One entry per
spec/task; newest at the bottom. Commit hashes refer to
`claude/rockpod-features-roadmap-qg0tc3`.

---

## ROADMAP.md written (`0910744`, `acf6c7b`, `fab9a94`)

Compiled from three sources: fork-grounded feature ideas, triage of open
[nuxcodes/rockpod](https://github.com/nuxcodes/rockpod) issues/PRs, and an
upstream Rockbox sync strategy. Refined twice more with the user: added
the dynamic-colors hardening item (1.3) and detailed the complementary-
accent design within it.

## Spec 0003 — Dynamic colors complementary accent (`b5376e0`, `120f8c5`, `0e5cd47`)

**Problem:** the accent (text) color picker in
`apps/gui/skin_engine/skin_albumart_color.c` fell back to pure black/white
for monochrome/low-saturation album art, and its readability-correction
step scaled RGB channels proportionally, clipping at 255 and shifting hue
instead of reaching the target luminance.

**Implementation:** new pure header `apps/gui/skin_engine/aa_color_math.h`
— integer RGB↔HSV conversion, complementary-hue derivation with the value
solved by binary search so contrast is met by construction, and an
HSV-space contrast fix that preserves hue. Wired into the two failing
branches of `skin_albumart_color.c` with a 15-line diff.

**Verified:**
- 32 unit checks (`tests/test_aa_color_math.c`), ASan/UBSan clean.
- Built SDL2 in this container from Ubuntu's `libsdl2-dev` headers +
  a manylinux `pysdl2-dll` wheel's shared lib (no libsdl.org/GitHub
  access through the environment's proxy) to get the `ipod6g` simulator
  compiling and running headless here.
- Simulator, scripted via a temporary (never-committed) autopilot patch
  to `sim_tasks.c` driving SDL key events + Rockbox's built-in
  `screen_dump()`: monochrome test art produced the derived neutral
  accent (measured pixel (139,141,139), luminance delta ≈100 vs. the old
  pure white); colorful art kept its extracted accent unchanged (R5).
  Screenshots sent to the user.
- Hardware compile (A6): deferred, no ARM toolchain in this session at
  the time — later covered by CI (see below).

## Spec 0004 — Disk-locality-aware shuffle for HDD (`f914cf8`)

**Problem:** shuffle is a plain Fisher-Yates with no notion of physical
disk layout, so HDD iPods seek across the whole platter on every
rebuffer.

**Implementation:** locality key = FAT first-cluster, read from the
dircache fileref already kept per playlist track (new accessor
`dircache_get_fileref_firstcluster()`); pure ordering algorithm in
`apps/shuffle_locality.h` (sort by key, shuffle 32-track regions and
region order); new `disk_shuffle` setting; new `H:` playlist-control
command so resume replays the identical order; transparent fallback to
plain shuffle off HDD, in the simulator, or when dircache data is
missing.

**Verified:**
- 36 unit checks (permutation validity, determinism, locality ratio,
  unknown-key handling).
- Simulator: with the setting on, shuffled playback, clean shutdown, and
  resume landed on the same track at the same position (fallback path,
  since the simulator has no dircache/ATA).

## Spec 0005 — Repeat Shuffle anti-repeat (`4ec8795`)

**Problem:** Repeat Shuffle re-shuffles with a fresh seed on wrap, so a
track can repeat right after the previous pass ended.

**Implementation:** `shuffle_repeat_overlap()` helper in
`shuffle_locality.h`; the wrap handler in `apps/playlist.c` retries up to
8 seeds, keeping the one with the least (ideally zero) overlap between
the new pass's first K tracks and the last K played, then commits only
that seed — so resume reproduces the exact result. Works with either
plain or disk-locality shuffle underneath.

**Verified:**
- 7 more unit checks (overlap counting, retry convergence).
- Simulator: repeat=shuffle playback wrapped cleanly with exactly one
  shuffle command recorded per wrap.

## CI workflow added (`bf9dfa1`, `0c8d5de`)

`.github/workflows/ci.yml`: host unit tests, `ipod6g` simulator build,
and hardware builds for `ipod6g`/`ipodvideo` (ARM cross-toolchain built
by `tools/rockboxdev.sh`, cached between runs). Answers
[rockpod#19](https://github.com/nuxcodes/rockpod/issues/19).

**First CI run (`bf9dfa1`):** host tests ✅, simulator build in progress
when checked; both hardware jobs failed immediately —
`rockboxdev.sh` needs `libtool` (and `automake`/`autoconf`), not in the
initial apt package list. Fixed in `0c8d5de`.

**Second run (`0c8d5de`):** host tests ✅, simulator ✅. Toolchain build
now succeeded (~24 min) and is cached, but both hardware jobs failed at
the actual firmware build: `as: unrecognized option '--64'` compiling
Rockbox's *host-side* helper tools (`rdf2binary`, `bmp2rb`, `codepages`,
`convbdf` — these run on the CI runner, not the iPod). Root cause: the
workflow put both `$HOME/rbdev/bin` (prefixed `arm-elf-eabi-gcc` etc.)
and `$HOME/rbdev/arm-elf-eabi/bin` (GCC's *unprefixed* per-target bin
dir) on `PATH`; the unprefixed dir shadows the system `as` with the ARM
cross-assembler, breaking host tool compilation, which needs the
runner's native `as`. Fix: drop the unprefixed directory from `PATH` —
`tools/configure`/the Makefile only need the prefixed names.

**Third run (`a5a8a58`):** all four jobs green — host tests, simulator
build, and both hardware builds (ipod6g, ipodvideo). The ARM toolchain
took ~21 min to build (the cache did not carry over from the failed
runs, since a job must succeed to save its cache); the firmware builds
themselves took ~4-5 min each and produced uploadable `rockbox.zip`
artifacts. This green hardware build is also the first compile check of
the disk-locality-shuffle code path (`HAVE_DISK_SHUFFLE`, which only
compiles on hardware targets), and closes the A6/A7/A4 hardware-compile
items that specs 0003/0004/0005 had deferred to CI.

**Milestone complete.** All four items (specs 0003/0004/0005 + CI) are
implemented, unit-tested, simulator-verified, and hardware-compiled in
CI. On-device listening tests remain as user follow-ups (out of scope —
no physical iPod in this environment).

---

## Battery work (specs 0006 / 0007 / 0008)

Follow-on from two battery-optimization analyses, each claim verified
against the code before acting.

- **Spec 0006 — battery gauge calibration, PC tool** (`104f67e`).
  `tools/battcal/` turns a `battery_bench.txt` discharge log into a
  calibrated `battery_levels.cfg`. Fixes the wrong percentage (stuck at
  0% / dual-boot desync) on high-capacity LiPo cells. 9 Python unit
  tests; end-to-end run on a synthetic non-linear log.

- **Spec 0007 — anti-premature-shutdown** (`4fff330`).
  `firmware/powermgmt.c` powered off the moment the fast-filtered voltage
  dipped below shutoff, with no debounce — and that branch runs precisely
  during disk activity, so an SD-wake voltage sag could shut the device
  off with real charge left. Now requires the below-shutoff condition to
  persist for `LOWBATT_SHUTDOWN_SAMPLES` (4, ≈2 s) consecutive samples.
  Pure debounce in `firmware/export/lowbatt_debounce.h`, 22 unit checks;
  compiles in the sim (real `powermgmt.o`).

- **Spec 0008 — battery calibration, on-device** (`ea42716`).
  `apps/plugins/battcal.c` does the same calibration on the iPod without
  a PC; the curve math is the pure, host-tested
  `apps/plugins/lib/battcurve.h` (40 unit checks). Verified in the
  simulator: launched the plugin against a synthetic log, it wrote a
  valid `battery_levels.cfg` — byte-identical to the PC tool's output.

Correction to the source analyses: Rockbox does **not** read raw ADC
voltage — it already applies a 128-sample EWMA (~64 s), so "add a filter"
was already done; the real premature-shutdown bug was the undebounced
shutdown decision (0007). The remaining autonomy lever (more aggressive
storage sleep) is coupled to the hi-res FLAC race (roadmap 1.1) and needs
on-device validation, so it is deliberately not shipped blind.

**CI:** runs 5/6/7 all green — host tests, simulator, and both hardware
builds (ipod6g, ipodvideo) — so the powermgmt change and the new plugin
compile clean on real targets. On-device confirmation (a real discharge
log; no premature shutoff at low battery on a flash-modded iPod) remains
a user follow-up.
