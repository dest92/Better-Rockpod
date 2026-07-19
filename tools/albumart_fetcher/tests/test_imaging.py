"""R6/A3: normalization always yields firmware-decodable baseline JPEG."""

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from albumart_fetcher.imaging import (normalize, is_rockbox_safe_jpeg,
                                      jpeg_dimensions)
from helpers import make_test_image


class TestSafetyChecker(unittest.TestCase):
    """The checker mirrors jpeg_load.c: baseline SOF0, sampling <= 2x2."""

    def test_baseline_jpeg_is_safe(self):
        self.assertTrue(is_rockbox_safe_jpeg(make_test_image()))

    def test_progressive_jpeg_is_rejected(self):
        data = make_test_image(progressive=True)
        self.assertFalse(is_rockbox_safe_jpeg(data))

    def test_png_is_rejected(self):
        self.assertFalse(is_rockbox_safe_jpeg(make_test_image(fmt="PNG")))

    def test_garbage_is_rejected(self):
        self.assertFalse(is_rockbox_safe_jpeg(b"not an image at all"))
        self.assertFalse(is_rockbox_safe_jpeg(b""))


class TestNormalize(unittest.TestCase):
    def test_progressive_input_becomes_safe(self):
        out, w, h = normalize(make_test_image(1000, 1000, progressive=True),
                              max_side=600)
        self.assertTrue(is_rockbox_safe_jpeg(out))
        self.assertLessEqual(max(w, h), 600)

    def test_png_input_becomes_safe(self):
        out, _, _ = normalize(make_test_image(500, 500, fmt="PNG"),
                              max_side=600)
        self.assertTrue(is_rockbox_safe_jpeg(out))

    def test_rgba_png_input_becomes_safe(self):
        from io import BytesIO
        from PIL import Image
        buf = BytesIO()
        Image.new("RGBA", (300, 300), (0, 0, 0, 0)).save(buf, "PNG")
        out, _, _ = normalize(buf.getvalue(), max_side=600)
        self.assertTrue(is_rockbox_safe_jpeg(out))

    def test_downscale_keeps_aspect(self):
        out, w, h = normalize(make_test_image(1200, 600), max_side=600)
        self.assertEqual((w, h), (600, 300))

    def test_small_image_not_upscaled(self):
        out, w, h = normalize(make_test_image(300, 300), max_side=600)
        self.assertEqual((w, h), (300, 300))

    def test_reported_size_matches_jpeg_header(self):
        out, w, h = normalize(make_test_image(640, 480), max_side=600)
        self.assertEqual(jpeg_dimensions(out), (w, h))

    def test_garbage_raises_valueerror(self):
        with self.assertRaises(ValueError):
            normalize(b"junk", max_side=600)


if __name__ == "__main__":
    unittest.main()
