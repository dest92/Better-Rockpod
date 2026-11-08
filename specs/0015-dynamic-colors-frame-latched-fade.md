# 0015: Dynamic colors: one fade step per frame

- **Status:** Implemented
- **Branch/PR:** `master` (roadmap 1.3, steps 2-3: state machine audit + tests)

## Problem

`dynamic_colors_resolve()` (`apps/gui/skin_engine/skin_albumart_color.c`)
recomputes the fade progress from `current_tick` on every call. One
frame calls it many times: skin viewports in `skin_render.c`, list
foreground/background and selector colors in `bitmap/list.c` and
`list-skinned.c`, and line colors in `line.c`. A 250 ms fade spans 25
ticks, so a frame that takes longer than a tick (common on the 5G,
where the CPU copies every pixel to the BCM) draws some elements with
step N and others with step N+1. That produces visible seams between
the background and the list or selector during fades, the "flicker/gap"
class of glitches the roadmap tracks. The fade math also has no tests.

## Requirements

- **R1:** Every `dynamic_colors_resolve()` call within one frame uses
  the same fade progress. A frame starts when
  `dynamic_colors_check_extraction()` runs, which already happens at
  the top of skin and list rendering.
- **R2:** Screens that resolve colors without calling the frame hook
  still see the fade advance. A latched tick older than 2 ticks is
  ignored in favour of `current_tick`.
- **R3:** Fade math is pure and unit-tested:
  - progress is clamped to 0..256, 0 at or before the start, and 256
    at or after the duration;
  - progress is monotonic in time;
  - channel lerp hits both endpoints exactly and stays between them.
- **R4:** No visible behavior change outside fades.

## Design sketch

- New `apps/gui/skin_engine/aa_fade.h` with no firmware includes:
  - `aa_fade_progress(elapsed, duration)`;
  - `aa_fade_lerp8(a, b, t)`;
  - `aa_fade_pick_tick(frame_tick, now, max_age)`.
- `skin_albumart_color.c`:
  - `check_extraction()` stores `cache.frame_tick = current_tick`.
  - `fade_progress()` uses `aa_fade_pick_tick()`.
  - `lerp_color()` uses `aa_fade_lerp8()`.

## Acceptance criteria

- **A1:** `tests/test_aa_fade.c` covers R3 and the latch selection:
  - fresh latch used;
  - stale latch replaced;
  - a frame spanning a tick boundary yields one progress value.
- **A2:** Simulator: a track change with dynamic colors still fades and
  settles on the album colors, with no regression in the WPS or menus.
- **A3:** Both hardware targets build.

## Verification results

- **A1:** `tests/test_aa_fade.c`, 3,559 checks, green under ASan/UBSan.
  It covers clamping, monotonicity, exact lerp endpoints, latch
  freshness and staleness, tick wrap-around, and one progress value
  for a frame that spans a tick boundary.
- **A2:** ipod6g simulator, two MP3s with red and blue embedded art.
  - The WPS and main menu took the red album's colors.
  - Skipping to the blue track settled on blue colors (album pixel
    (24,56,197), accent text recolored).
  - No screen shows stale colors.
  - Mid-fade frames could not be captured reliably: the screenshot
    cost is longer than the 250 ms fade.
- **A3:** `ipod6g` and `ipodvideo` build and zip.
