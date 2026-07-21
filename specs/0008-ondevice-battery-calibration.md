# 0008: On-device battery-curve calibration plugin

- **Status:** Implemented. A1–A4 verified by unit tests (40 checks in
  `tests/test_battcurve.c`, ASan/UBSan). A5 verified on the ipod6g
  simulator: launched the plugin (Plugins → Applications → battcal)
  against a synthetic `battery_bench.txt`; it wrote a valid
  `battery_levels.cfg` and showed "Calibrated (time axis, 3600-4180 mV)".
  The written curve is byte-identical to what `tools/battcal` (spec 0006)
  produces for the same log — cross-validating both implementations. A6
  hardware compile via CI. End-to-end on a real device log remains a user
  follow-up.
- **Branch/PR:** claude/rockpod-features-roadmap-qg0tc3

## Problem

`tools/battcal` (spec 0006) generates a calibrated `battery_levels.cfg`
from a `battery_bench.txt` log, but it needs a PC. Users without one
can't calibrate. The `battery_bench` plugin already produces the log on
the device; what's missing is doing the curve computation on the device
too, so the whole flow (bench → calibrated cfg) happens on the iPod.

## Requirements

- **R1:** A plugin reads `/battery_bench.txt`, computes the 11-point
  ascending discharge curve, and writes `/.rockbox/battery_levels.cfg`
  in the exact format the loader expects (`firmware/powermgmt.c:902`).
- **R2:** Same calibration semantics as `tools/battcal` (spec 0006):
  percentage axis from integrated `Current[mA]` when the log has it,
  else elapsed time; never the log's own `Level:` column; non-monotonic
  results are rejected with a message rather than written.
- **R3:** The curve computation and log-line parsing are a pure C module
  (`apps/plugins/lib/battcurve.h`) with **no plugin API dependency**, so
  the same logic is host-tested in `tests/` and reused by the plugin for
  I/O only.
- **R4:** The plugin reports the outcome on screen (axis used, measured
  voltage range, or the reason it failed) and never overwrites an
  existing `battery_levels.cfg` without the user confirming.
- **R5:** Bounded memory: a fixed sample cap (sized for a long
  discharge, e.g. 4096 one-per-minute samples ≈ 68 h); longer logs keep
  the most recent samples. No dynamic allocation beyond the plugin
  buffer.

## Non-goals

- No change to `battery_bench` itself or to `tools/battcal`.
- No automatic calibration loop / unattended discharge — the user still
  runs the bench; this only turns its log into the cfg.
- No firmware/powermgmt changes (the loader already exists).

## Design sketch

- `apps/plugins/lib/battcurve.h` (pure, static inline, C only):
  - `battcurve_parse_line(line, *secs, *mv, *ma)` → bool; skips
    comment/blank/short lines; `*ma = -1` when no current column
    (tolerant of the trailing charger/USB flag characters).
  - `battcurve_compute(secs[], mv[], ma[], n, force_time, out[11],
    *used_charge)` → error enum (`OK`, `TOO_SHORT`, `NO_SPAN`,
    `NONMONOTONIC`). Same math as `battcal.py discharge_curve`.
  - Mirrors `tools/battcal/battcal.py`; a comment in each cross-
    references the other so they stay in sync.
- `apps/plugins/battcal.c`: opens `/battery_bench.txt`, reads lines
  (`rb->read_line`), fills capped arrays via the parser, calls
  `battcurve_compute`, and on success writes the `discharge:` block plus
  commented `#shutoff`/`#disksafe` to `/.rockbox/battery_levels.cfg`
  (confirming overwrite if present). Registered in
  `apps/plugins/SOURCES` and `apps/plugins/CATEGORIES` (category `apps`,
  next to `battery_bench`).

## Acceptance criteria

- **A1:** `battcurve_compute` on a synthetic linear-discharge sample set
  returns the expected ascending 11 values (±1 mV), matching the
  `battcal.py` behavior for the same input.
- **A2:** A current column with varying draw selects the charge axis and
  yields a different curve than the time axis; absent it, time is used.
- **A3:** A non-monotonic / too-short sample set returns the error enum
  and the plugin writes nothing.
- **A4:** `battcurve_parse_line` parses rows with and without the current
  column and rejects `#`/blank/short lines.
- **A5:** Simulator: run the plugin against a synthetic
  `simdisk/battery_bench.txt`; it writes a valid
  `simdisk/.rockbox/battery_levels.cfg` that re-parses to the same 11
  values.
- **A6:** Both hardware targets compile (CI).

## Test plan

| Criterion | How verified |
|-----------|--------------|
| A1–A4 | unit tests `tests/test_battcurve.c` (`make -C tests`) |
| A5 | simulator: autopilot run of the plugin on a synthetic log, inspect the written cfg |
| A6 | CI hardware builds |
| End-to-end | user: run on a real device `battery_bench.txt` |
