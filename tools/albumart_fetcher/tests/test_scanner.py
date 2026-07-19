"""R1/R2: album grouping and has-art decision logic (pure helpers)."""

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from albumart_fetcher.scanner import group_key, has_art, scan
from helpers import write_mp3, write_flac, make_test_image

import tempfile


class TestGroupKey(unittest.TestCase):
    def test_albumartist_preferred_over_artist(self):
        key = group_key({"albumartist": "Various", "artist": "X",
                         "album": "Hits"})
        self.assertEqual(key, ("various", "hits"))

    def test_artist_fallback(self):
        key = group_key({"artist": "Tool", "album": "Lateralus"})
        self.assertEqual(key, ("tool", "lateralus"))

    def test_case_and_whitespace_folded(self):
        a = group_key({"artist": " Tool ", "album": "Lateralus"})
        b = group_key({"artist": "tool", "album": " LATERALUS "})
        self.assertEqual(a, b)

    def test_no_album_tag_is_ungroupable(self):
        self.assertIsNone(group_key({"artist": "Tool"}))
        self.assertIsNone(group_key({"artist": "Tool", "album": "  "}))


class TestHasArt(unittest.TestCase):
    def test_embedded_art_wins(self):
        self.assertTrue(has_art(True, "Lateralus", [], []))

    def test_cover_jpg_in_dir(self):
        self.assertTrue(has_art(False, "Lateralus", ["cover.jpg"], []))

    def test_cover_case_insensitive(self):
        self.assertTrue(has_art(False, "Lateralus", ["Cover.JPG"], []))

    def test_albumname_file_in_dir(self):
        self.assertTrue(has_art(False, "Lateralus", ["Lateralus.jpg"], []))

    def test_cover_in_parent_dir(self):
        self.assertTrue(has_art(False, "Lateralus", [], ["cover.jpeg"]))

    def test_cover_bmp_counts(self):
        self.assertTrue(has_art(False, "Lateralus", ["cover.bmp"], []))

    def test_unrelated_files_do_not_count(self):
        self.assertFalse(has_art(False, "Lateralus",
                                 ["folder.txt", "art.png", "cover.png"],
                                 ["back.jpg"]))

    def test_no_art_anywhere(self):
        self.assertFalse(has_art(False, "Lateralus", [], []))


class TestScanTree(unittest.TestCase):
    """A1: fixture tree classification, and no file is modified."""

    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        root = Path(self.tmp.name)

        d1 = root / "Artist1" / "NoArt"
        d1.mkdir(parents=True)
        write_mp3(d1 / "01.mp3", "Artist1", "NoArt")
        write_flac(d1 / "02.flac", "Artist1", "NoArt")

        d2 = root / "Artist2" / "CoverFile"
        d2.mkdir(parents=True)
        write_mp3(d2 / "01.mp3", "Artist2", "CoverFile")
        (d2 / "cover.jpg").write_bytes(make_test_image(100, 100))

        d3 = root / "Artist3" / "Embedded"
        d3.mkdir(parents=True)
        p = write_mp3(d3 / "01.mp3", "Artist3", "Embedded")
        from mutagen.id3 import ID3, APIC
        tag = ID3(str(p))
        tag.add(APIC(encoding=3, mime="image/jpeg", type=3, desc="Cover",
                     data=make_test_image(100, 100)))
        tag.save()

        (root / "Artist1" / "NoArt" / "notes.txt").write_text("x")
        self.root = root

    def tearDown(self):
        self.tmp.cleanup()

    def test_classification(self):
        albums = {a.key: a for a in scan(self.root)}
        self.assertEqual(len(albums), 3)
        self.assertFalse(albums[("artist1", "noart")].has_art)
        self.assertTrue(albums[("artist2", "coverfile")].has_art)
        self.assertTrue(albums[("artist3", "embedded")].has_art)

    def test_track_lists(self):
        albums = {a.key: a for a in scan(self.root)}
        self.assertEqual(len(albums[("artist1", "noart")].tracks), 2)

    def test_scan_modifies_nothing(self):
        before = {p: (p.stat().st_mtime_ns, p.stat().st_size)
                  for p in self.root.rglob("*") if p.is_file()}
        scan(self.root)
        after = {p: (p.stat().st_mtime_ns, p.stat().st_size)
                 for p in self.root.rglob("*") if p.is_file()}
        self.assertEqual(before, after)


if __name__ == "__main__":
    unittest.main()
