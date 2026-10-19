# 0006: Battery discharge-curve calibration tool

- **Status:** Implemented. A1–A5 verified: `tools/battcal/` with 9 unit
  tests (`python3 -m unittest`) all passing, plus an end-to-end run on a
  synthetic non-linear discharge log producing a valid ascending curve.
  End-to-end on a real device log remains a user follow-up (needs a full
  discharge `battery_bench.txt` from an iPod).
- **Branch/PR:** claude/rockpod-features-roadmap-qg0tc3

## Problem

On iPods with modern high-capacity LiPo replacements, the battery
percentage is wrong: it sits near 0% for hours or desyncs from Apple's
firmware after a dual boot. Rockbox derives the percentage from a
compiled-in voltage→percent table (`percent_to_volt_discharge[11]`,
`firmware/powermgmt.c:334` `voltage_to_percent()`), calibrated for the
small original Apple cells. A replacement cell has a different discharge
curve, so the mapping is off.

Rockbox already supports overriding the table at runtime:
`firmware/powermgmt.c:902` loads `ROCKBOX_DIR"/battery_levels.cfg"`
(`powermgmt.h:87`) and replaces the discharge table. The `battery_bench`
plugin logs a full discharge to `battery_bench.txt`. What's missing is
the step in between: turning a real discharge log into the 11 calibrated
voltages. Today a user must eyeball the log and interpolate by hand.

**Scope note:** this fixes the *gauge accuracy*, not battery life. It
adds no autonomy. It is the highest-value, hardware-free battery
improvement available, and it produces the measured data needed to later
tune the sleep/power path safely.

## Requirements

- **R1:** A PC-side tool reads a `battery_bench.txt` log and emits a
  ready-to-use `battery_levels.cfg` `discharge: {v0, v1, …, v10}` line,
  where `v_i` is the measured cell voltage at `i*10`% of usable charge,
  ascending (`v0` = empty, `v10` = full), matching the table semantics
  in `voltage_to_percent()`.
- **R2:** The percentage axis is derived from measured data, never from
  the log's `Level:` column (that column is Rockbox's own estimate using
  the *default*, wrong table — using it would be circular). Use charge
  consumed (integrating the `Current[mA]` column over `Seconds`) when
  present; otherwise fall back to elapsed time, and say which was used.
- **R3:** The tool validates the result: the 11 values must be strictly
  ascending (a non-monotonic curve means a noisy/interrupted bench run);
  warn and report the offending points instead of emitting a bad table.
- **R4:** It also surfaces suggested `#shutoff` and `#disksafe` voltages
  (the measured minimum, and a small margin above it) as commented lines,
  matching the `battery_levels.cfg` format the plugin already documents.
- **R5:** Parsing + curve computation is pure and unit-tested on the host
  with synthetic logs (same precedent as `tools/coverart/`, which ships
  `test_fetch_coverart.py`). No device needed to build or verify.

## Non-goals

- No firmware changes: the loader and `battery_bench` plugin already
  exist and are untouched.
- No change to actual power consumption / autonomy (separate, hardware-
  gated work coupled to roadmap 1.1/#17).
- No on-device UI to generate the file — this is a PC tool, consistent
  with `tools/coverart`. (A future plugin could wrap it.)
- No raising of `BATTERY_CAPACITY_MAX` (that only affects the runtime
  estimate, not the percentage; out of scope here).

## Design sketch

New `tools/battcal/` mirroring `tools/coverart/`:

- `battcal.py`:
  - `parse_bench_log(text)` → list of samples `(seconds, voltage_mV,
    current_mA|None)`, skipping `#` comment lines and the header, tolerant
    of the optional `Current[mA]` column (comma-separated, per
    `battery_bench.c:694`).
  - `discharge_curve(samples)` → 11 ascending mV values. Builds a
    charge-or-time axis (R2), then for each of the 11 points finds the
    voltage by linear interpolation between the nearest samples. The
    bench runs full→empty, so the axis is reversed to produce
    empty→full ascending output.
  - `format_cfg(curve, shutoff, disksafe)` → the `discharge: {…}` block
    plus commented `#shutoff:` / `#disksafe:` lines.
  - CLI: `python3 battcal.py battery_bench.txt [-o battery_levels.cfg]`,
    `--time` / `--charge` to force the axis, prints a summary
    (axis used, min/max voltage, monotonicity check).
- `test_battcal.py`: synthetic logs (a clean linear ramp with and
  without a current column; a non-monotonic/noisy one; a too-short one)
  asserting the curve, the axis selection, and the R3 validation.
- `requirements.txt` (stdlib only — likely empty/none) and a short
  README section.

## Acceptance criteria

- **A1:** For a synthetic linear-discharge log (known voltage vs. time),
  `discharge_curve()` returns the 11 expected ascending values within
  ±1 mV.
- **A2:** With a `Current[mA]` column whose draw varies, the charge-based
  axis is used and yields a curve different from (and more correct than)
  the naive time-based one; without the column, the time axis is used
  and reported.
- **A3:** A non-monotonic input triggers the R3 warning and the tool does
  not emit a `discharge:` line (exit non-zero).
- **A4:** `format_cfg()` output parses back cleanly against the format
  `battery_bench.c` documents (a round-trip parse of the emitted block
  recovers the 11 values).
- **A5:** `python3 -m unittest test_battcal` passes.

## Test plan

| Criterion | How verified |
|-----------|--------------|
| A1–A4 | unit tests `tools/battcal/test_battcal.py` |
| A5 | `cd tools/battcal && python3 -m unittest` |
| End-to-end | run `battcal.py` on a real `battery_bench.txt` — user, on device (needs a full discharge log from their iPod) |
