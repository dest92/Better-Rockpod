# 0021: Cover Flow "stage" background and shadowed titles

- **Status:** Implemented (on-device look and frame-rate check pending)
- **Branch/PR:** `master` (research/ui-smoothness.md, proposal 4)

## Problem

PictureFlow (Cover Flow) cleared each frame to the flat theme
background colour (`pf_clear_display()`), so it looked plain next to
themed menus (Aero7 in particular). The album and artist text had no
separation from the reflections under it.

## Requirements

- **R1:** Each frame starts from a "stage":
  - a vertical gradient from the theme background, lit by the theme
    selector colour, with a soft light band behind the covers;
  - exactly the plain background from the reflection line down, so
    reflections, which fade to the background colour, stay seamless.
- **R2:** A faint glass edge, one row, at the reflection line.
- **R3:** The album and artist lines get a 1 px drop shadow.
- **R4:** Cost per frame stays small: one precomputed colour per row
  (`pf_stage_rows[]`), drawn as horizontal lines, with no full-screen
  image.
- **R5:** The stage follows theme and dynamic-colour changes.
  Monochrome builds keep the old clear.

## Design

- `pf_stage.h`: pure `pf_stage_weight(y, glow_y, floor_y)`, the accent
  weight per row (0 on the floor).
- `pf_build_stage()` mixes `pf_bg_color` → `pf_lss_color` per row at
  init and in `pf_update_dynamic_colors()`.
- The floor row is `pf_half_height + pf_display_offs`, where the
  renderer switches from cover to reflection.

## Verification

- `tests/test_pf_stage.c`: 509 checks. The floor is exactly 0, the
  peak sits at the glow row, values stay bounded, and each side is
  monotonic.
- Simulator (ipod6g, Aero7, 5 albums): the light band behind the
  covers, a seamless reflection floor, the glass edge and shadowed
  titles. Navigation and the track list work. Before/after screenshots
  were taken with Aero7.
- `ipod6g` and `ipodvideo` build.
- **On device:** animation smoothness on the 5G (80 MHz while
  PictureFlow boosts); the per-row fill is ~240 hline calls per frame.
