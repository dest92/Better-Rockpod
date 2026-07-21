"""Unit tests for battcal.py — battery discharge-curve calibration
(specs/0006-battery-calibration-tool.md, A1-A5)."""

import unittest

import battcal


def make_log(rows, header_current=False):
    """Build a synthetic battery_bench.txt from (seconds, level, mV[, mA])."""
    lines = [
        "# This plugin will log your battery performance.",
        "# Battery type: 3000 mAh      Buffer Entries: 10",
        "# Time:,  Seconds:,  Level:,  Time Left:,  Voltage[mV]:"
        + (", Current[mA]:" if header_current else ""),
    ]
    for r in rows:
        secs, level, mv = r[0], r[1], r[2]
        hh, mm, ss = secs // 3600, (secs % 3600) // 60, secs % 60
        line = "%02d:%02d:%02d,  %05d,     %03d%%,     00:00,         %04d" % (
            hh, mm, ss, secs, level, mv)
        if header_current:
            line += ",         %04d" % r[3]
        lines.append(line)
    return "\n".join(lines) + "\n"


class TestParse(unittest.TestCase):
    def test_parse_basic(self):
        log = make_log([(0, 100, 4120), (60, 90, 4068), (120, 80, 4016)])
        samples = battcal.parse_bench_log(log)
        self.assertEqual(len(samples), 3)
        self.assertEqual(samples[0], (0, 4120, None))
        self.assertEqual(samples[1], (60, 4068, None))

    def test_parse_with_current(self):
        log = make_log([(0, 100, 4120, 120), (60, 90, 4068, 110)],
                       header_current=True)
        samples = battcal.parse_bench_log(log)
        self.assertEqual(samples[0], (0, 4120, 120))
        self.assertEqual(samples[1], (60, 4068, 110))

    def test_parse_skips_comments_and_blanks(self):
        log = make_log([(0, 100, 4120)])
        log += "\n# --Battery bench ended--\n"
        samples = battcal.parse_bench_log(log)
        self.assertEqual(len(samples), 1)


class TestCurve(unittest.TestCase):
    def test_linear_time_axis(self):
        """A1: linear voltage-vs-time discharge recovers an even ramp."""
        v_full, v_empty = 4120, 3600
        rows = []
        for i in range(101):  # 101 samples, full -> empty over time
            mv = round(v_full - (v_full - v_empty) * i / 100)
            rows.append((i * 60, 100 - i, mv))
        curve, axis, _ = battcal.discharge_curve(
            battcal.parse_bench_log(make_log(rows)))
        self.assertEqual(axis, "time")
        expected = [v_empty + (v_full - v_empty) * i // 10 for i in range(11)]
        for got, exp in zip(curve, expected):
            self.assertLessEqual(abs(got - exp), 1, "%d vs %d" % (got, exp))

    def test_ascending_output(self):
        v_full, v_empty = 4200, 3600
        rows = [(i * 60, 100 - i, round(v_full - (v_full - v_empty) * i / 100))
                for i in range(101)]
        curve, _, _ = battcal.discharge_curve(
            battcal.parse_bench_log(make_log(rows)))
        self.assertEqual(len(curve), 11)
        self.assertTrue(all(curve[i] < curve[i + 1] for i in range(10)))
        self.assertEqual(curve[0], min(curve))   # empty first
        self.assertEqual(curve[10], max(curve))  # full last

    def test_charge_axis_differs_from_time(self):
        """A2: front-loaded current -> charge axis, different curve."""
        v_full, v_empty = 4120, 3600
        rows = []
        for i in range(101):
            mv = round(v_full - (v_full - v_empty) * i / 100)
            # heavy draw early, light late -> charge consumed front-loaded
            ma = 300 if i < 50 else 60
            rows.append((i * 60, 100 - i, mv, ma))
        samples = battcal.parse_bench_log(make_log(rows, header_current=True))
        curve_c, axis_c, _ = battcal.discharge_curve(samples)
        curve_t, _, _ = battcal.discharge_curve(samples, axis="time")
        self.assertEqual(axis_c, "charge")
        self.assertNotEqual(curve_c, curve_t)

    def test_non_monotonic_rejected(self):
        """A3: a noisy/non-monotone discharge is rejected."""
        rows = [(0, 100, 4120), (60, 90, 3600), (120, 80, 4000),
                (180, 70, 3700), (240, 60, 4100), (300, 0, 3600)]
        with self.assertRaises(battcal.CalibrationError):
            battcal.discharge_curve(battcal.parse_bench_log(make_log(rows)))

    def test_too_short_rejected(self):
        with self.assertRaises(battcal.CalibrationError):
            battcal.discharge_curve(
                battcal.parse_bench_log(make_log([(0, 100, 4120)])))


class TestFormat(unittest.TestCase):
    def test_roundtrip(self):
        """A4: emitted discharge line re-parses to the same values."""
        curve = [3600, 3652, 3704, 3756, 3808, 3860,
                 3912, 3964, 4016, 4068, 4120]
        text = battcal.format_cfg(curve, shutoff=3400, disksafe=3450)
        self.assertIn("discharge:", text)
        recovered = battcal.parse_discharge_line(text)
        self.assertEqual(recovered, curve)


if __name__ == "__main__":
    unittest.main()
