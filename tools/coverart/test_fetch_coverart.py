"""Unit tests for fetch_coverart.py's pure/host-testable logic.

Per specs/0002-coverart-fetch-tool.md: network calls and real tag I/O
are not covered here (no mocking gymnastics) - only the decision logic
that doesn't need them. Run with:

    python3 -m unittest tools/coverart/test_fetch_coverart.py -v
"""
import os
import tempfile
import unittest

from fetch_coverart import (
    Album,
    TrackInfo,
    album_needs_art,
    build_itunes_search_url,
    folder_has_cover_file,
    group_tracks_into_albums,
    pick_itunes_artwork,
    scan_library,
    upsize_itunes_artwork_url,
)


class BuildItunesSearchUrlTest(unittest.TestCase):
    def test_includes_artist_and_album_in_term(self):
        url = build_itunes_search_url("Daft Punk", "Discovery")
        self.assertIn("itunes.apple.com/search", url)
        self.assertIn("term=Daft+Punk+Discovery", url)

    def test_scopes_to_music_albums(self):
        url = build_itunes_search_url("Artist", "Album")
        self.assertIn("media=music", url)
        self.assertIn("entity=album", url)

    def test_url_encodes_special_characters(self):
        url = build_itunes_search_url("AC/DC", "Highway to Hell")
        self.assertNotIn("/", url.split("term=")[1].split("&")[0])
        self.assertIn("term=", url)


class UpsizeItunesArtworkUrlTest(unittest.TestCase):
    def test_upsizes_standard_100x100_url(self):
        url = "https://is1-ssl.mzstatic.com/image/thumb/Music/abc/100x100bb.jpg"
        self.assertEqual(
            upsize_itunes_artwork_url(url),
            "https://is1-ssl.mzstatic.com/image/thumb/Music/abc/600x600bb.jpg",
        )

    def test_leaves_unrecognized_url_unchanged(self):
        url = "https://example.com/cover.jpg"
        self.assertEqual(upsize_itunes_artwork_url(url), url)

    def test_handles_other_source_sizes(self):
        url = "https://example.com/foo/128x128bb.png"
        self.assertEqual(upsize_itunes_artwork_url(url),
                        "https://example.com/foo/600x600bb.png")


class GroupTracksIntoAlbumsTest(unittest.TestCase):
    def test_groups_by_dir_artist_album(self):
        tracks = [
            TrackInfo(path="/music/A/01.mp3", artist="Artist A", album="Album A"),
            TrackInfo(path="/music/A/02.mp3", artist="Artist A", album="Album A"),
            TrackInfo(path="/music/B/01.mp3", artist="Artist B", album="Album B"),
        ]
        albums = group_tracks_into_albums(tracks)
        self.assertEqual(len(albums), 2)
        by_dir = {a.dir_path: a for a in albums}
        self.assertEqual(len(by_dir["/music/A"].tracks), 2)
        self.assertEqual(len(by_dir["/music/B"].tracks), 1)
        self.assertEqual(by_dir["/music/A"].artist, "Artist A")
        self.assertEqual(by_dir["/music/A"].album, "Album A")

    def test_same_album_name_different_directories_stay_separate(self):
        # a compilation-style "Greatest Hits" in two different folders
        # (different artists) must not get merged into one album.
        tracks = [
            TrackInfo(path="/music/X/01.mp3", artist="X", album="Greatest Hits"),
            TrackInfo(path="/music/Y/01.mp3", artist="Y", album="Greatest Hits"),
        ]
        albums = group_tracks_into_albums(tracks)
        self.assertEqual(len(albums), 2)

    def test_empty_input_returns_empty_list(self):
        self.assertEqual(group_tracks_into_albums([]), [])


class AlbumNeedsArtTest(unittest.TestCase):
    def test_needs_art_when_nothing_present(self):
        self.assertTrue(album_needs_art(embedded_flags=[False, False],
                                        has_folder_cover=False))

    def test_does_not_need_art_when_any_track_embedded(self):
        self.assertFalse(album_needs_art(embedded_flags=[False, True, False],
                                         has_folder_cover=False))

    def test_does_not_need_art_when_folder_cover_present(self):
        self.assertFalse(album_needs_art(embedded_flags=[False, False],
                                         has_folder_cover=True))

    def test_does_not_need_art_when_both_present(self):
        self.assertFalse(album_needs_art(embedded_flags=[True],
                                         has_folder_cover=True))

    def test_empty_track_list_with_no_folder_cover_needs_art(self):
        self.assertTrue(album_needs_art(embedded_flags=[],
                                        has_folder_cover=False))


class FolderHasCoverFileTest(unittest.TestCase):
    def test_finds_cover_jpg(self):
        with tempfile.TemporaryDirectory() as d:
            open(os.path.join(d, "cover.jpg"), "w").close()
            self.assertTrue(folder_has_cover_file(d))

    def test_finds_folder_png_case_insensitive(self):
        with tempfile.TemporaryDirectory() as d:
            open(os.path.join(d, "FOLDER.PNG"), "w").close()
            self.assertTrue(folder_has_cover_file(d))

    def test_finds_album_jpeg(self):
        with tempfile.TemporaryDirectory() as d:
            open(os.path.join(d, "album.jpeg"), "w").close()
            self.assertTrue(folder_has_cover_file(d))

    def test_unrelated_files_do_not_count(self):
        with tempfile.TemporaryDirectory() as d:
            open(os.path.join(d, "readme.txt"), "w").close()
            open(os.path.join(d, "discography.jpg"), "w").close()
            self.assertFalse(folder_has_cover_file(d))

    def test_empty_directory_has_no_cover(self):
        with tempfile.TemporaryDirectory() as d:
            self.assertFalse(folder_has_cover_file(d))


class PickItunesArtworkTest(unittest.TestCase):
    def test_no_results_returns_none(self):
        self.assertIsNone(pick_itunes_artwork({"resultCount": 0, "results": []}))

    def test_returns_upsized_artwork_url_of_first_result(self):
        response = {
            "resultCount": 2,
            "results": [
                {"artworkUrl100": "https://x.example/a/100x100bb.jpg",
                 "collectionName": "Discovery", "artistName": "Daft Punk"},
                {"artworkUrl100": "https://x.example/b/100x100bb.jpg",
                 "collectionName": "Discovery (Deluxe)", "artistName": "Daft Punk"},
            ],
        }
        self.assertEqual(pick_itunes_artwork(response),
                        "https://x.example/a/600x600bb.jpg")

    def test_missing_artwork_field_returns_none(self):
        response = {"resultCount": 1, "results": [{"collectionName": "No Art"}]}
        self.assertIsNone(pick_itunes_artwork(response))


class ScanLibraryProgressTest(unittest.TestCase):
    def test_on_progress_called_once_per_track_found(self):
        with tempfile.TemporaryDirectory() as d:
            open(os.path.join(d, "01.mp3"), "wb").close()
            open(os.path.join(d, "02.mp3"), "wb").close()
            open(os.path.join(d, "notes.txt"), "w").close()

            counts = []
            scan_library(d, on_progress=counts.append)

            self.assertEqual(counts, [1, 2])

    def test_scan_without_progress_callback_still_works(self):
        with tempfile.TemporaryDirectory() as d:
            open(os.path.join(d, "01.mp3"), "wb").close()
            albums = scan_library(d)
            self.assertEqual(len(albums), 1)


if __name__ == "__main__":
    unittest.main()
