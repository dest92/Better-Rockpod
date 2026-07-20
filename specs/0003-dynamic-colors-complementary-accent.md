# 0003: Dynamic colors — mathematically derived complementary accent

- **Status:** Implemented (A1–A4 verified by unit tests; A5 simulator
  and A6 hardware builds pending — no SDL2/ARM toolchain in the
  implementation environment)
- **Branch/PR:** claude/rockpod-features-roadmap-qg0tc3

## Problem

Dynamic colors sometimes produce an unreadable or ugly accent (text)
color against the dominant (background) color. Two code paths in
`apps/gui/skin_engine/skin_albumart_color.c` are responsible:

1. **Hard black/white fallback** (`skin_albumart_color.c:315`): when no
   histogram bucket differs from the dominant by `MIN_CONTRAST` (plain
   luminance delta, `:38`) — typical for monochromatic or low-saturation
   album art — the accent becomes pure white or black, which looks harsh
   and ignores the art's character.
2. **Clipping luminance correction** (`skin_albumart_color.c:329`): when
   the chosen accent has insufficient contrast, channels are scaled
   proportionally toward a target luminance. Scaling a saturated color
   up clips channels at 255, which both shifts the hue and undershoots
   the luminance target — the accent ends up neither the right color
   nor readable.

## Requirements

- **R1:** When no histogram bucket passes the contrast filter, the
  accent is derived mathematically from the dominant color: its hue
  rotated 180° (complementary), saturation retained (capped), and
  value chosen so the luminance delta vs. the dominant is ≥
  `MIN_CONTRAST` by construction.
- **R2:** If the dominant color is near-achromatic (saturation below a
  threshold), the derived accent is a neutral tone at the target
  luminance (no fabricated hue), replacing today's pure black/white.
- **R3:** The luminance-correction step reaches its target luminance
  without hue shift: value (and, only if value saturates, saturation)
  is adjusted in HSV space instead of proportional RGB scaling.
- **R4:** All new math is integer-only (no floats; firmware is
  `-nostdlib -ffreestanding`) and lives in pure, host-testable helpers
  with no firmware includes.
- **R5:** Album art whose extracted accent already passes the contrast
  filter renders exactly as before (no visual churn on working art).

## Non-goals

- No change to dominant-color extraction, histogram, sampling, fades,
  or any draw path — accent derivation only.
- No perceptual (WCAG-ratio) contrast metric yet; keep the existing
  luminance-delta metric so R5 is trivially guaranteed. Upgrading the
  metric is a separate future spec.
- No new user setting; this fixes behavior under the existing
  Dynamic Colors setting.

## Design sketch

New header `apps/gui/skin_engine/aa_color_math.h` (pure, standalone —
same pattern as `apps/plugins/pictureflow/aa_source.h` from spec 0001):

- `aa_rgb2hsv(r, g, b, *h, *s, *v)` / `aa_hsv2rgb(h, s, v, *r, *g, *b)`
  — integer HSV, h in 0..359, s/v in 0..255.
- `aa_luminance(r, g, b)` — the existing weighted sum from
  `compute_luminance()` (`skin_albumart_color.c:74`), moved here and
  reused by the .c file.
- `aa_derive_complement(dom_r, dom_g, dom_b, min_contrast, *r, *g, *b)`
  — R1/R2: complement hue, pick target luminance on the far side of
  the dominant (dark dominant → light accent and vice versa), then
  solve for V by binary search over `aa_luminance(aa_hsv2rgb(...))`
  (≤ 8 iterations), desaturating only if V alone cannot reach the
  target (R3's mechanism, reused).
- `aa_fix_contrast(acc_*, dom_*, min_contrast, ...)` — R3: same
  V-then-S adjustment applied to a histogram-extracted accent that
  missed the contrast bar.

`skin_albumart_color.c` changes are minimal: the `:315` fallback calls
`aa_derive_complement()`, the `:329` correction block calls
`aa_fix_contrast()`, `compute_luminance()` delegates to
`aa_luminance()`. The accent-selection loop and everything else stay
untouched (R5).

## Acceptance criteria

- **A1:** HSV round-trip: for a sweep of RGB values,
  `aa_hsv2rgb(aa_rgb2hsv(c))` returns each channel within ±2.
- **A2:** For an exhaustive grid of dominant colors (e.g. every 8th
  value per channel: 32³ ≈ 33k cases), `aa_derive_complement()` yields
  luminance delta ≥ `MIN_CONTRAST` — zero failures.
- **A3:** For saturated dominants, the derived accent's hue is within
  ±20° of (dominant hue + 180°); for near-achromatic dominants the
  accent's saturation is 0 (R2).
- **A4:** `aa_fix_contrast()` reaches the contrast target on the same
  grid without changing hue by more than ±2° while saturation permits.
- **A5:** Simulator: with Dynamic Colors on, a monochrome/low-sat test
  cover produces a readable, non-pure-black/white accent; colorful
  covers render as before (R5).
- **A6:** Both hardware targets (`ipod6g`, `ipodvideo`) still compile.

## Test plan

| Criterion | How verified |
|-----------|--------------|
| A1–A4 | unit tests `tests/test_aa_color_math.c` (`make -C tests`) |
| A5 | simulator: play tracks with monochrome and colorful embedded art, inspect WPS + menus |
| A6 | hardware builds — no ARM toolchain in this session: user or CI runs `./build-hw.sh` + `./build-hw.sh 5g` |
