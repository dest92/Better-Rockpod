# 0009: iPod Classic 6G/7G playback hang on sample-rate change

- **Status:** Implemented. A1 confirmed in CI: both `ipod6g` (the target
  this fix touches) and `ipodvideo` compile clean. A2 confirmed by
  inspection (mute calls only bracket the existing frequency-change
  branch; the `pcm-s5l8702.c`/`cs42l55.c` files are not part of the
  simulator build, so no local runtime check was possible here, same as
  the spec 0004 dircache precedent). A3/A4 are on-device criteria and
  remain open — this is the only thing left to close out rockpod#13/#17.
- **Branch/PR:** claude/rockpod-features-roadmap-qg0tc3

## Problem

- **rockpod#13**: iPod Classic 6.5G (HDD). Switching between lossless
  (FLAC/ALAC) and lossy (Opus/MP3) tracks makes playback "get stuck" —
  no sound, the track timer freezes. Bidirectional, format-independent,
  persists across a settings reset. Not reproducible on a 5G device with
  the same files, and not reproducible on stock Rockbox.
- **rockpod#17**: iPod Classic 7G + SSD, hi-res FLAC (96/176.4/192kHz)
  intermittently fails to play. Reporter suspects SSD power-management
  timing; not reproducible on stock Rockbox 4.0.

Both are P0 items on `ROADMAP.md` (1.1, 1.2). This spec documents a
concrete root cause traced for #13 in this fork's own 6G/7G audio driver
code — not upstream Rockbox territory.

## Root cause

1. When a new track's native sample rate differs from the current output
   rate, Rockbox switches the hardware output frequency to match (avoids
   resampling) — gated on `HAVE_PLAY_FREQ`, enabled on both `ipod6g` and
   `ipodvideo` (both declare `SAMPR_CAP_44 | SAMPR_CAP_48`). The generic
   mechanism is not 6G-specific.
2. `apps/playback.c:4266` (`audio_auto_change_frequency`) calls
   `mixer_set_frequency()` (`firmware/pcm_mixer.c:453`), which — when the
   rate actually changes — calls `mixer_reset()` → `pcm_play_stop()`,
   properly halting DMA and gating the SoC's I2S clock (`I2SCLKCON=0`)
   before anything is reprogrammed. This part is safe and identical on
   both targets.
3. The next `pcm_play_data()` call (new track's first buffer) triggers
   `pcm_apply_settings()` → `pcm_dma_apply_settings()`
   (`firmware/target/arm/s5l8702/pcm-s5l8702.c:178`), which reprograms
   `I2SCLKDIV` and calls `audiohw_set_frequency(fsel)`
   (`firmware/drivers/audio/cs42l55.c:261`) — writing the codec's
   **CLKCTL2** register, which selects the CS42L55's internal PLL/filter
   "speed mode" (`CLKCTL2_SPEED_*`) and MCLK/LRCK ratio.
4. `audiohw_set_frequency()` carries its own doc comment: *"Note: Disable
   output before calling this function"* (`cs42l55.c:260`) — the CS42L55
   datasheet's documented precondition. `pcm_dma_apply_settings()` never
   honors it: the codec's master-mute (`audiohw_mute()`, `cs42l55.c:54`,
   `static`, only ever called from `postinit`/`idle_powerdown`/
   `idle_powerup`) is not engaged around a plain frequency change.
5. CS42L55 is configured as the I2S **clock master**
   (`CLKCTL1_MASTER`, `cs42l55.c:90`) — it generates the SCLK/LRCK that
   the S5L8702's own I2S peripheral depends on as slave. Reprogramming the
   codec's internal PLL/speed-mode register live (unmuted) risks a
   transient clock-generation glitch; because the SoC's I2S block is
   slave to those clocks, a bad glitch can leave its FIFO/DMA-request
   state machine wedged waiting for a clean edge that never arrives —
   explaining why both audio *and* the playback-position timer freeze
   together (`dma_play_callback` in `pcm-s5l8702.c` simply never fires
   again), not just an audible click.
6. Intermittency plausibly depends on exactly where in the codec's
   internal PLL cycle the register write lands, and whether the old/new
   rates share a compatible internal "speed" family or cross a PLL-mode
   boundary.
7. iPod Video (5G) uses a different driver (`firmware/target/arm/pp/
   pcm-pp.c` + `wm8758.c`) with the same "no mute around live frequency
   write" pattern, but WM8758 evidently tolerates it (no datasheet
   warning noted in that driver, and 5G doesn't reproduce this). The bug
   is chip-specific to CS42L55, in code only reachable on 6G/7G —
   matching the reports (generation-specific, not on stock Rockbox, since
   this is this fork's own S5L8702 audio path).

## Requirements

- **R1:** `pcm_dma_apply_settings()` on s5l8702 must mute the CS42L55
  before writing `CLKCTL2` (via `audiohw_set_frequency()`) and unmute it
  after, honoring the codec driver's own documented precondition.
- **R2:** No change to the generic `audiohw.h` interface, `pcm.c`, or
  `pcm_mixer.c` — those are correct and shared by 80+ other targets.
- **R3:** No change to `pcm-pp.c`/`wm8758.c` (5G) — not reported broken.

## Non-goals

- rockpod#17's SSD power-management angle (roadmap 1.1) is a separate,
  unverified hypothesis. Hi-res files also cross sample-rate boundaries,
  so this fix may reduce or resolve #17 too, but that needs on-device
  confirmation — not claimed as fixed by this spec alone.
- No attempt to model or unit-test codec register sequencing (direct
  hardware I/O, no extractable pure logic).

## Design sketch

- `firmware/drivers/audio/cs42l55.c`: drop `static` from
  `audiohw_mute(bool mute)`.
- `firmware/export/cs42l55.h`: add `void audiohw_mute(bool mute);` (every
  current user of `pcm-s5l8702.c` — ipod6g, nano3g, nano4g — defines
  `HAVE_CS42L55` unconditionally, so no `#ifdef` needed).
- `firmware/target/arm/s5l8702/pcm-s5l8702.c`, `pcm_dma_apply_settings()`:
  wrap `audiohw_set_frequency(fsel)` with `audiohw_mute(true)` /
  `audiohw_mute(false)`. This runs while DMA is already stopped (step 2
  above), so it only affects the brief already-occurring silence during a
  rate change — no new audible gap, just removes the live-reprogram
  hazard.

## Acceptance criteria

- **A1:** Both hardware targets (`ipod6g`, `ipodvideo`) compile.
- **A2:** No behavior change for same-rate track transitions (mute/unmute
  only brackets the `pcm_sampr != pcm_curr_sampr` branch, unchanged).
- **A3 (on-device, user):** A playlist mixing FLAC/ALAC and Opus/MP3 on a
  6G/7G plays through repeated skips without the stuck-silence/frozen-
  timer symptom from rockpod#13.
- **A4 (on-device, user, best-effort):** Hi-res FLAC (96/176.4/192kHz)
  interspersed with normal-rate tracks on a 7G+SSD plays more reliably
  than before (rockpod#17) — not guaranteed, since a power-management
  factor may also be at play.

## Test plan

| Criterion | How verified |
|-----------|--------------|
| A1 | CI hardware builds |
| A2 | code inspection (mute calls only added around the existing frequency-change branch, no other logic touched) |
| A3, A4 | user, on-device — this bug class cannot be reproduced or verified without real 6G/7G hardware and a DAC/speaker to observe playback |
