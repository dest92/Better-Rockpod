# UI smoothness: where the time goes and what to change

Investigation of what limits UI fluidity on the two targets, from the
code (2026-11-10). Smoothness itself can only be measured on hardware;
each proposal below lists what needs on-device confirmation.

## Findings

### 1. The UI runs at the lowest CPU clock (both targets)

| Target | Normal clock | Max clock | Source |
|---|---|---|---|
| iPod Classic 6G/7G (S5L8702) | **54 MHz** | 216 MHz | `firmware/target/arm/s5l8702/system-target.h:31-33` |
| iPod Video 5G (PP5022) | **30 MHz** | 80 MHz | `firmware/target/arm/pp/system-target.h:34-35` |

Rockbox only boosts for specific work: buffering, decoding, playlist
and database building, skin loading (`skin_engine.c:321`), and
PictureFlow while it runs (`pictureflow.c:4991`). Nothing boosts while
the user navigates. Button and wheel handling (`firmware/drivers/button.c`,
`apps/action.c`) never call `cpu_boost()`. So every list scroll, menu
redraw, skin repaint, alpha-blended bitmap (Aero7's selection) and
text render runs at 54 MHz on the 6G, or 30 MHz on the 5G, a quarter
or less of what the hardware can do. This is very likely the single
largest factor in how "heavy" the UI feels compared with the stock
firmware.

### 2. LCD transfer

- **6G:** already DMA (`firmware/target/arm/s5l8702/lcd-s5l8702.c:142`).
  `lcd_update_rect()` copies the rectangle into a double buffer and
  queues it; the waiter yields. Not a bottleneck beyond the copy, which
  also runs at the slow clock (see 1).
- **5G:** the CPU writes every pixel to the BCM2722 (`lcd-as-video.S`),
  at 30 MHz. Apple's firmware uses PP5022 DMA instead
  (`research/apple-ipod5g-firmware.md`). This is the second big factor
  on the 5G.

### 3. Lists and wheel

- With `HAVE_WHEEL_ACCELERATION` (both iPods), the list's frame-drop
  path (`apps/gui/list.c:736`, `FRAMEDROP_TRIGGER`) is compiled out, so
  every wheel step redraws the list even when events queue up faster
  than the slow CPU can draw. A boost (1) addresses the cause. Frame
  dropping could be re-enabled as a second line of defence.

### 4. PictureFlow / Cover Flow

- It already boosts for its whole run, and upstream's elapsed-time
  animation fixes are merged (rockpod v5.5+). Its background is the
  theme's flat background colour (`pictureflow.c:235-270`), which is why
  it looks plain next to Aero7.

## Proposals, by value per risk

1. **Interaction boost (both targets, spec to write: 0020).** Boost the
   CPU on any button or wheel event, and drop the boost about 1 s after
   the last input.
   - Implement it in the UI thread (`get_action`), never in the tick
     ISR. Bound blocking waits while boosted so the unboost always runs.
   - Add a setting ("Boost while navigating", default on).
   - Battery cost: only the seconds of active navigation; playback is
     unaffected.
   - On device: perceived smoothness, and battery over a fixed
     browsing session.
2. **5G LCD via DMA (spec to write).** From the Apple firmware notes.
   Needs a 5G to validate.
3. **Re-enable list frame dropping** when the button queue backs up,
   even with wheel acceleration. Small change; validate on device.
4. **Cover Flow polish (spec to write: 0021).**
   - Draw a gradient or theme backdrop behind the slides instead of the
     flat colour.
   - Render titles in the theme font.
   - Optionally show a blurred-art background (overlaps with spec 0019).
   - Cost: one extra full-frame fill per animation frame. Cheap on the
     6G at 216 MHz; measure on the 5G at 80 MHz.
