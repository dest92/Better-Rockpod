# 0021: Cover Flow redesign: iPod-like layout, stage background, titles

- **Status:** Implemented (on-device look and frame-rate check pending)
- **Branch/PR:** `master` (research/ui-smoothness.md, proposal 4)

## Problem

PictureFlow (Cover Flow) cleared each frame to the flat theme
background colour (`pf_clear_display()`), so it looked plain next to
themed menus (Aero7 in particular). The album and artist text had no
separation from the reflections under it.

## Requirements

- **R0:** iPod-like layout by default:
  - full screen (no status bar or theme frame around it);
  - smaller covers (zoom 72) with more room (tuck 20/128, margin
    12/128 of the display width);
  - the year shown.
  Configs still holding the old untouched defaults are migrated once;
  customised configs are kept.

- **R1:** Each frame starts from a "stage":
  - a vertical gradient from the theme background, lit by the theme
    selector colour, with a soft light band behind the covers;
  - exactly the plain background from the reflection line down, so
    reflections, which fade to the background colour, stay seamless.
- **R2:** A faint glass edge, one row, at the reflection line. That row
  depends on the zoom: `pf_half_height + pf_display_offs * zoom / 100`,
  since covers scale about the centre row.
- **R2b:** The gradient is 2x2 ordered-dithered into RGB565, so it shows
  no banding.
- **R3:** The album and artist lines get a 1 px drop shadow, and the
  artist is dimmer than the album. Theme fonts must cover U+2013 for the
  "Album – Year" separator; Aero7's fonts now go up to U+2122.
- **R4:** Cost per frame stays small: four precomputed dither colours
  per row (`pf_stage_rows[][4]`), written straight into the frame
  buffer. That is one store per pixel, the same as the old clear.
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

## Verification (redesign)

- Simulator, starting from an old config (zoom 100, status bar on):
  - The layout migrated to full screen with smaller covers.
  - The gradient shows no banding, and the glass edge sits exactly at
    the covers' base.
  - "Neon Nights – 2008" shows the dash, and the artist is dimmed.
  - Mid-animation frames look right.
- `ipod6g` and `ipodvideo` build; the unit tests are green.
