"""R3/A2: candidate parsing and ranking from source JSON (no network)."""

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from albumart_fetcher.sources import (itunes_parse, caa_candidates_from_mb,
                                      rank_candidates)


ITUNES_FIXTURE = {
    "resultCount": 2,
    "results": [
        {"collectionName": "Lateralus", "artistName": "TOOL",
         "artworkUrl100":
             "https://is1.mzstatic.com/image/thumb/x/100x100bb.jpg"},
        {"collectionName": "Lateralus (Karaoke Tribute)",
         "artistName": "Ameritz",
         "artworkUrl100":
             "https://is1.mzstatic.com/image/thumb/y/100x100bb.jpg"},
    ],
}

MB_FIXTURE = {
    "releases": [
        {"id": "aaaa-1111", "title": "Lateralus", "score": 100,
         "artist-credit": [{"name": "Tool"}]},
        {"id": "bbbb-2222", "title": "Lateralus", "score": 95,
         "artist-credit": [{"name": "Tool"}]},
    ],
}


class TestItunesParse(unittest.TestCase):
    def test_upscales_artwork_url(self):
        cands = itunes_parse(ITUNES_FIXTURE, size=600)
        self.assertTrue(cands[0].url.endswith("600x600bb.jpg"))
        self.assertEqual(cands[0].source, "itunes")

    def test_keeps_thumb_url(self):
        cands = itunes_parse(ITUNES_FIXTURE, size=600)
        self.assertIn("100x100bb", cands[0].thumb_url)

    def test_carries_artist_album_labels(self):
        cands = itunes_parse(ITUNES_FIXTURE, size=600)
        self.assertEqual(cands[0].album, "Lateralus")
        self.assertEqual(cands[0].artist, "TOOL")

    def test_empty_results(self):
        self.assertEqual(itunes_parse({"results": []}, size=600), [])


class TestCaaParse(unittest.TestCase):
    def test_builds_front_urls_per_release(self):
        cands = caa_candidates_from_mb(MB_FIXTURE)
        self.assertEqual(len(cands), 2)
        self.assertEqual(
            cands[0].url,
            "https://coverartarchive.org/release/aaaa-1111/front-500")
        self.assertIn("front-250", cands[0].thumb_url)
        self.assertEqual(cands[0].source, "musicbrainz")

    def test_empty(self):
        self.assertEqual(caa_candidates_from_mb({"releases": []}), [])


class TestRanking(unittest.TestCase):
    def test_exact_match_ranks_first(self):
        cands = itunes_parse(ITUNES_FIXTURE, size=600)
        ranked = rank_candidates(cands, artist="Tool", album="Lateralus")
        self.assertEqual(ranked[0].album, "Lateralus")
        self.assertEqual(ranked[0].artist, "TOOL")
        self.assertIn("Karaoke", ranked[1].album)


if __name__ == "__main__":
    unittest.main()
