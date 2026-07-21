# battcal — battery discharge-curve calibration

Turns a `battery_bench.txt` log into a calibrated `battery_levels.cfg`
so the battery percentage reads correctly on iPods with modern
high-capacity LiPo cells (fixes the "stuck at 0%" and dual-boot desync
problems). See `specs/0006-battery-calibration-tool.md`.

**This fixes the gauge, not battery life** — it makes the percentage
accurate; it does not change how long the battery lasts.

## How it works

Rockbox reads the battery percentage from a compiled-in voltage→percent
table calibrated for the small original Apple cells. A replacement cell
discharges along a different curve, so the reading is wrong. Rockbox can
load a per-device override from `.rockbox/battery_levels.cfg`; this tool
generates that file from a real discharge log.

## Usage

1. On the iPod, run the **Battery Bench** plugin, play an album on
   repeat, and let the battery run all the way down. It writes
   `battery_bench.txt` to the root.
2. Copy `battery_bench.txt` to your computer and run:

   ```bash
   python3 battcal.py battery_bench.txt -o battery_levels.cfg
   ```

3. Copy `battery_levels.cfg` into the iPod's `.rockbox/` directory and
   reboot. Rockbox now uses your cell's real discharge curve.

Options:
- `--axis time|charge` — force the percentage axis. Default is auto:
  charge (integrated `Current[mA]`) when the log has a current column,
  otherwise elapsed time. Charge is more accurate because current draw
  varies during playback.

Only the Python standard library is required.

## Tests

```bash
cd tools/battcal && python3 -m unittest
```
