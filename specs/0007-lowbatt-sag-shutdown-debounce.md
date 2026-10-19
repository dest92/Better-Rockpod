# 0007: Storage-sag-resistant low-battery shutdown

- **Status:** Implemented. A1–A3 verified by unit tests (22 checks in
  `tests/test_lowbatt_debounce.c`, ASan/UBSan). A4 verified: the
  simulator compiles `firmware/powermgmt.c` with the change (its
  `powermgmt.o` is built) and boots. A5 hardware compile via CI. The
  actual no-premature-shutoff behavior needs on-device confirmation on a
  flash-modded iPod at low battery (user follow-up).
- **Branch/PR:** claude/rockpod-features-roadmap-qg0tc3

## Problem

On a flash-modded iPod with a modern LiPo cell, the device can power off
prematurely at low battery — with real charge still left — because a
transient voltage sag from the SD/iFlash controller waking to refill the
audio buffer is treated as a genuine depletion.

Mechanism, traced in `firmware/powermgmt.c`:
- Normal operation already filters voltage heavily: a 128-sample EWMA
  (`BATT_AVE_SAMPLES`, `powermgmt.c:297`) at `HZ/2` steps ≈ 64 s time
  constant, so ordinary sags never move the gauge. (The premise that
  Rockbox uses raw ADC readings is incorrect — the filter exists.)
- **But** once `percent_now < 8` the power thread switches to a *fast*
  filter (`average_step(true)`, α≈0.5, `powermgmt.c:295`) and calls
  `query_force_shutdown()` every step with **no debounce**
  (`powermgmt.c:756`). `query_force_shutdown()` shuts down as soon as
  `voltage_now < battery_level_shutoff` (`powermgmt.c:538`).
- So near empty, a single deep SD-wake sag drags the fast-filtered
  voltage below the shutoff threshold and powers off immediately, even
  though the cell recovers as soon as storage sleeps.

## Requirements

- **R1:** A low-battery shutdown fires only when the below-shutoff
  condition **persists** — at least `LOWBATT_SHUTDOWN_SAMPLES`
  consecutive power-thread samples below `battery_level_shutoff` — so a
  transient storage sag (one or two samples) cannot trigger it.
- **R2:** The safety property is preserved: a genuinely depleted cell
  (sustained below shutoff) still powers off, after at most
  `LOWBATT_SHUTDOWN_SAMPLES × POWER_THREAD_STEP_TICKS` (target ≈ 2 s with
  4 samples at HZ/2) — negligible vs. real depletion, safe against
  over-discharge.
- **R3:** Any sample at or above shutoff, or the charger being inserted,
  resets the counter (the condition must be *consecutive*).
- **R4:** The `input_millivolts()`-based force-shutdown paths (targets
  that shut down on charger removal, `powermgmt.c:544`) are unchanged —
  this only debounces the discharge-time low-battery path.
- **R5:** The debounce decision is a pure function, host-tested.

## Non-goals

- No change to the normal-operation gauge filter, the discharge curve,
  or `battcal` (spec 0006 already covers gauge accuracy).
- No change to storage sleep/power timing (that is the autonomy lever,
  coupled to the hi-res FLAC race in roadmap 1.1, and needs on-device
  validation — out of scope here).
- Not a replacement for calibrating `shutoff:`/`disksafe:` via battcal;
  the two are complementary.

## Design sketch

- New pure helper (host-testable header, e.g. `apps/lowbatt_debounce.h`,
  same pattern as `aa_color_math.h` / `shuffle_locality.h`):
  `lowbatt_shutdown_debounce(bool below_shutoff, int *consec)` → returns
  true when `*consec` (incremented on true, reset on false) reaches
  `LOWBATT_SHUTDOWN_SAMPLES`. Keeps the counter in the caller.
- `firmware/powermgmt.c`: replace the bare
  `if (!shutdown_timeout && query_force_shutdown())` at `:756` with the
  debounced decision, holding a `static int` counter. `query_force_shutdown()`
  itself is unchanged; only the low-battery discharge branch gets the
  consecutive-sample gate. Define `LOWBATT_SHUTDOWN_SAMPLES` (default 4)
  in `powermgmt.h`, overridable per target.

## Acceptance criteria

- **A1:** A single or double below-shutoff sample surrounded by
  above-shutoff samples never returns "shut down" (sag rejected).
- **A2:** `LOWBATT_SHUTDOWN_SAMPLES` consecutive below-shutoff samples
  returns "shut down" exactly on the Nth (not before, not never).
- **A3:** An above-shutoff sample resets the counter (11 below with a
  reset in the middle does not trigger until N consecutive again).
- **A4:** Simulator builds and runs (the low-battery path is not
  reachable in the sim without battery simulation, so this is a compile
  + no-regression check).
- **A5:** Both hardware targets compile (via CI).

## Test plan

| Criterion | How verified |
|-----------|--------------|
| A1–A3 | unit test `tests/test_lowbatt_debounce.c` (`make -C tests`) |
| A4 | simulator build + boot (no crash) |
| A5 | CI hardware builds |
| On-device | user: confirm no premature shutoff at low battery during playback on a flash-modded iPod |
