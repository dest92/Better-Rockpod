# 0019: Blurred album-art backdrop and art-first browsing (idea)

- **Status:** Draft (idea only, roadmap 3.5)
- **Source:** rockpod#18 (closed, unmerged) by Olsro

## Idea

rockpod#18 proposed an art browser plugin and a skin with a blurred
album-art background. The PR isn't worth porting as-is, but two ideas
fit this fork's dynamic-colors direction:

1. **Blurred backdrop in the WPS.** Downscale the current track's album
   art to about 40x30, box-blur it twice, and upscale it into a
   320x240 background. This is cheap enough to run once per track
   change on both SoCs.
2. **Art-first browsing.** Album lists in the database browser show a
   small cover per row, reusing PictureFlow's cached, scaled art.

## Design notes for a future spec

- **Where it lives:** a new skin tag (for example `%Ab`, "album art
  backdrop") rendered by the skin engine before the other viewports,
  like dynamic colors hooks in through `skin_albumart_color.c`. The
  blur runs once on `PLAYBACK_EVENT_CUR_TRACK_READY`.
- **Memory:** a 320x240 RGB565 backdrop is 150 KiB. Allocate it from
  buflib only while a skin uses the tag. Free it on theme change.
- **Math:** separable box blur in fixed point. Make it a pure,
  host-testable helper (`aa_blur.h`), with golden tests like those in
  `aa_color_math.h`.
- **Contrast:** reuse `aa_fix_contrast()` (spec 0003) to darken or
  lighten the backdrop so that text using the dynamic accent stays
  readable.
- **5G cost:** a full-screen background makes every WPS refresh a full
  150 KiB BCM transfer. Measure redraw time on a 5G first. The DMA LCD
  path in `research/apple-ipod5g-firmware.md` would make this cheap.

## Acceptance (when picked up)

- Unit tests for the blur helper (golden values, edge clamping).
- Simulator screenshots with several art types: high contrast, mono,
  missing art (fall back to the theme background).
- Measured WPS redraw time on 5G and 6G hardware.
