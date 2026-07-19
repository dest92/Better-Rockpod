"""R5/R8/A2/A5: embedding art and rescan idempotence."""

import base64
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from albumart_fetcher.apply import (embed_track, apply_to_album,
                                    make_vorbis_picture, write_cover_file)
from albumart_fetcher.scanner import scan
from albumart_fetcher.imaging import normalize
from helpers import write_mp3, write_flac, make_test_image


class TestEmbed(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.dir = Path(self.tmp.name)
        self.jpeg, self.w, self.h = normalize(make_test_image(700, 700), 600)

    def tearDown(self):
        self.tmp.cleanup()

    def test_mp3_gets_apic(self):
        p = write_mp3(self.dir / "t.mp3", "A", "B")
        embed_track(p, self.jpeg, self.w, self.h)
        from mutagen.id3 import ID3
        apics = ID3(str(p)).getall("APIC")
        self.assertEqual(len(apics), 1)
        self.assertEqual(apics[0].data, self.jpeg)
        self.assertEqual(apics[0].mime, "image/jpeg")

    def test_mp3_replaces_existing_apic(self):
        p = write_mp3(self.dir / "t.mp3", "A", "B")
        embed_track(p, self.jpeg, self.w, self.h)
        other, w, h = normalize(make_test_image(300, 300), 600)
        embed_track(p, other, w, h)
        from mutagen.id3 import ID3
        apics = ID3(str(p)).getall("APIC")
        self.assertEqual(len(apics), 1)
        self.assertEqual(apics[0].data, other)

    def test_flac_gets_picture(self):
        p = write_flac(self.dir / "t.flac", "A", "B")
        embed_track(p, self.jpeg, self.w, self.h)
        from mutagen.flac import FLAC
        pics = FLAC(str(p)).pictures
        self.assertEqual(len(pics), 1)
        self.assertEqual(pics[0].data, self.jpeg)
        self.assertEqual((pics[0].width, pics[0].height), (self.w, self.h))
        self.assertEqual(pics[0].type, 3)

    def test_unsupported_extension_raises(self):
        p = self.dir / "t.wav"
        p.write_bytes(b"RIFF")
        with self.assertRaises(ValueError):
            embed_track(p, self.jpeg, self.w, self.h)

    def test_vorbis_picture_block_roundtrips(self):
        b64 = make_vorbis_picture(self.jpeg, self.w, self.h)
        from mutagen.flac import Picture
        pic = Picture(base64.b64decode(b64))
        self.assertEqual(pic.data, self.jpeg)
        self.assertEqual(pic.mime, "image/jpeg")
        self.assertEqual((pic.width, pic.height), (self.w, self.h))

    def test_write_cover_file(self):
        write_cover_file(self.dir, self.jpeg)
        self.assertEqual((self.dir / "cover.jpg").read_bytes(), self.jpeg)


class TestApplyToAlbum(unittest.TestCase):
    """A2 (host part): apply to all tracks, then rescan sees art."""

    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        root = Path(self.tmp.name)
        d = root / "Artist" / "Album"
        d.mkdir(parents=True)
        write_mp3(d / "01.mp3", "Artist", "Album")
        write_flac(d / "02.flac", "Artist", "Album")
        self.root = root
        self.dir = d
        self.jpeg, self.w, self.h = normalize(make_test_image(700, 700), 600)

    def tearDown(self):
        self.tmp.cleanup()

    def _album(self):
        albums = [a for a in scan(self.root)]
        self.assertEqual(len(albums), 1)
        return albums[0]

    def test_apply_then_rescan_complete(self):
        album = self._album()
        self.assertFalse(album.has_art)
        report = apply_to_album(album, self.jpeg, self.w, self.h)
        self.assertEqual(report.errors, [])
        self.assertEqual(report.done, 2)
        self.assertTrue(self._album().has_art)

    def test_cover_file_option(self):
        album = self._album()
        apply_to_album(album, self.jpeg, self.w, self.h, cover_file=True)
        self.assertTrue((self.dir / "cover.jpg").exists())

    def test_one_bad_track_does_not_abort(self):
        album = self._album()
        (self.dir / "02.flac").write_bytes(b"")  # corrupt after scan
        report = apply_to_album(album, self.jpeg, self.w, self.h)
        self.assertEqual(report.done, 1)
        self.assertEqual(len(report.errors), 1)


if __name__ == "__main__":
    unittest.main()
