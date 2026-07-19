#!/usr/bin/env python3
"""Find and embed missing album art for a music library.

See specs/0002-coverart-fetch-tool.md. Scans a directory (e.g. an
iPod's music folder mounted as a USB disk) for albums that have
neither an embedded cover picture nor a folder-level cover file, looks
up a match on the iTunes Search API, and (with --apply) embeds it into
every track of the album so PictureFlow and the WPS pick it up.

Usage:
    python3 fetch_coverart.py /path/to/music            # dry run (default)
    python3 fetch_coverart.py /path/to/music --apply     # actually fetch + embed

Requires: mutagen, requests (pip install mutagen requests)
"""
import argparse
import io
import os
import re
import sys
import time
import urllib.parse
from collections import namedtuple

import requests
from mutagen import File as mutagen_file
from mutagen.easyid3 import EasyID3
from mutagen.flac import FLAC, Picture
from mutagen.id3 import APIC, ID3, ID3NoHeaderError
from mutagen.mp4 import MP4, MP4Cover
from mutagen.oggvorbis import OggVorbis

TrackInfo = namedtuple("TrackInfo", ["path", "artist", "album"])
Album = namedtuple("Album", ["dir_path", "artist", "album", "tracks"])

AUDIO_EXTENSIONS = (".mp3", ".m4a", ".flac", ".ogg")
COVER_FILE_NAMES = ("cover", "folder", "album")
COVER_FILE_EXTS = (".jpg", ".jpeg", ".png")

ITUNES_SEARCH_URL = "https://itunes.apple.com/search"
ITUNES_RATE_LIMIT_SECONDS = 1.0


# ---------------------------------------------------------------------------
# Pure decision logic (unit-tested in test_fetch_coverart.py, no I/O/network)
# ---------------------------------------------------------------------------

def build_itunes_search_url(artist, album):
    """Build the iTunes Search API URL for an artist+album lookup."""
    term = urllib.parse.quote_plus(f"{artist} {album}")
    return f"{ITUNES_SEARCH_URL}?term={term}&media=music&entity=album&limit=5"


def upsize_itunes_artwork_url(url):
    """Upsize iTunes' default 100x100 (or similar) artwork URL to 600x600."""
    return re.sub(r"\d+x\d+bb\.", "600x600bb.", url)


def group_tracks_into_albums(track_infos):
    """Group TrackInfo entries into Albums, keyed by (directory, artist, album).

    Keying on the directory (not just artist+album) keeps same-named
    albums by different artists in different folders from merging.
    """
    groups = {}
    order = []
    for t in track_infos:
        dir_path = os.path.dirname(t.path)
        key = (dir_path, t.artist, t.album)
        if key not in groups:
            groups[key] = []
            order.append(key)
        groups[key].append(t.path)

    return [
        Album(dir_path=dir_path, artist=artist, album=album, tracks=tracks)
        for (dir_path, artist, album), tracks in ((k, groups[k]) for k in order)
    ]


def album_needs_art(embedded_flags, has_folder_cover):
    """True if none of the album's tracks have embedded art and there's
    no folder-level cover file either."""
    if has_folder_cover:
        return False
    return not any(embedded_flags)


def folder_has_cover_file(dir_path):
    """Case-insensitive check for a cover/folder/album image file."""
    try:
        entries = os.listdir(dir_path)
    except OSError:
        return False
    for name in entries:
        stem, ext = os.path.splitext(name)
        if stem.lower() in COVER_FILE_NAMES and ext.lower() in COVER_FILE_EXTS:
            return True
    return False


def pick_itunes_artwork(itunes_json):
    """Pick an artwork URL from an iTunes Search API JSON response, or
    None if there's no usable match. Naive top-result matching (R3) -
    no fuzzy scoring in this version."""
    results = itunes_json.get("results") or []
    if not results:
        return None
    url = results[0].get("artworkUrl100")
    if not url:
        return None
    return upsize_itunes_artwork_url(url)


# ---------------------------------------------------------------------------
# I/O and network (not unit-tested; verified manually per the spec)
# ---------------------------------------------------------------------------

def read_track_tags(path):
    """Read (artist, album) from a track, tolerating missing tags."""
    try:
        audio = mutagen_file(path, easy=True)
    except Exception:
        return TrackInfo(path=path, artist=None, album=None)
    if audio is None:
        return TrackInfo(path=path, artist=None, album=None)
    artist = (audio.get("artist") or audio.get("albumartist") or [None])[0]
    album = (audio.get("album") or [None])[0]
    return TrackInfo(path=path, artist=artist, album=album)


def has_embedded_art(path):
    """Whether a track already carries an embedded picture tag."""
    ext = os.path.splitext(path)[1].lower()
    try:
        if ext == ".mp3":
            tags = ID3(path)
            return bool(tags.getall("APIC"))
        if ext == ".m4a":
            tags = MP4(path)
            return bool(tags.tags and tags.tags.get("covr"))
        if ext == ".flac":
            return bool(FLAC(path).pictures)
        if ext == ".ogg":
            audio = OggVorbis(path)
            return bool(audio.get("metadata_block_picture"))
    except Exception:
        return False
    return False


def scan_library(root):
    """Walk `root` for audio files and group them into albums."""
    track_infos = []
    for dirpath, _dirnames, filenames in os.walk(root):
        for name in filenames:
            if os.path.splitext(name)[1].lower() in AUDIO_EXTENSIONS:
                track_infos.append(read_track_tags(os.path.join(dirpath, name)))
    return group_tracks_into_albums(track_infos)


def fetch_itunes_json(artist, album):
    url = build_itunes_search_url(artist, album)
    resp = requests.get(url, timeout=10)
    resp.raise_for_status()
    return resp.json()


def download_image(url):
    resp = requests.get(url, timeout=15)
    resp.raise_for_status()
    return resp.content


def embed_artwork(track_path, image_bytes, mime="image/jpeg"):
    """Embed `image_bytes` as the cover picture of one track."""
    ext = os.path.splitext(track_path)[1].lower()
    if ext == ".mp3":
        try:
            tags = ID3(track_path)
        except ID3NoHeaderError:
            tags = ID3()
        tags.delall("APIC")
        tags.add(APIC(encoding=3, mime=mime, type=3, desc="Cover",
                      data=image_bytes))
        tags.save(track_path)
    elif ext == ".m4a":
        tags = MP4(track_path)
        cover_format = (MP4Cover.FORMAT_PNG if mime == "image/png"
                        else MP4Cover.FORMAT_JPEG)
        tags["covr"] = [MP4Cover(image_bytes, imageformat=cover_format)]
        tags.save()
    elif ext == ".flac":
        audio = FLAC(track_path)
        audio.clear_pictures()
        pic = Picture()
        pic.data = image_bytes
        pic.type = 3
        pic.mime = mime
        audio.add_picture(pic)
        audio.save()
    elif ext == ".ogg":
        audio = OggVorbis(track_path)
        pic = Picture()
        pic.data = image_bytes
        pic.type = 3
        pic.mime = mime
        import base64
        audio["metadata_block_picture"] = [
            base64.b64encode(pic.write()).decode("ascii")
        ]
        audio.save()
    else:
        raise ValueError(f"Unsupported extension for embedding: {ext}")


def process_album(album, apply_changes):
    """Report (and optionally fix) one album. Returns a status string:
    'skipped' | 'found' | 'embedded' | 'not-found' | 'error'."""
    embedded_flags = [has_embedded_art(t) for t in album.tracks]
    has_folder = folder_has_cover_file(album.dir_path)

    if not album_needs_art(embedded_flags, has_folder):
        return "skipped"

    if not album.artist or not album.album:
        print(f"  ! missing artist/album tag, cannot search: {album.dir_path}")
        return "not-found"

    try:
        data = fetch_itunes_json(album.artist, album.album)
    except requests.RequestException as e:
        print(f"  ! iTunes lookup failed for {album.artist} - {album.album}: {e}")
        return "error"
    finally:
        time.sleep(ITUNES_RATE_LIMIT_SECONDS)

    artwork_url = pick_itunes_artwork(data)
    if not artwork_url:
        print(f"  x not found: {album.artist} - {album.album}")
        return "not-found"

    match_name = data["results"][0].get("collectionName", "?")
    match_artist = data["results"][0].get("artistName", "?")
    print(f"  > found: {album.artist} - {album.album}"
          f"  ->  matched \"{match_artist} - {match_name}\"")

    if not apply_changes:
        return "found"

    try:
        image_bytes = download_image(artwork_url)
        for track in album.tracks:
            embed_artwork(track, image_bytes)
        print(f"    embedded into {len(album.tracks)} track(s)")
        return "embedded"
    except Exception as e:
        print(f"  ! failed to embed artwork for {album.dir_path}: {e}")
        return "error"


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("library", help="path to the music library "
                        "(e.g. the mounted iPod's /Music folder)")
    parser.add_argument("--apply", action="store_true",
                        help="download and embed artwork (default: dry-run report only)")
    args = parser.parse_args(argv)

    albums = scan_library(args.library)
    print(f"Scanned {len(albums)} album(s) under {args.library}")
    print(f"Mode: {'APPLY (writing tags)' if args.apply else 'dry-run (no changes)'}")
    print()

    counts = {}
    for album in albums:
        status = process_album(album, args.apply)
        counts[status] = counts.get(status, 0) + 1

    print()
    print("Summary:")
    for status in ("embedded", "found", "not-found", "error", "skipped"):
        if status in counts:
            print(f"  {status}: {counts[status]}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
