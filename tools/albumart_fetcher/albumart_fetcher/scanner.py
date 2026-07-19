"""Scan a music tree and classify albums by album-art presence.

Decision logic (group_key, has_art) is kept as pure functions taking
plain values so it is unit-testable without touching the filesystem.
The art-file rules mirror the firmware search in
apps/recorder/albumart.c (cover.* and <albumname>.* in the album
directory and its parent).
"""

import logging
from dataclasses import dataclass, field
from pathlib import Path

import mutagen

log = logging.getLogger(__name__)

AUDIO_EXTS = {".mp3", ".flac", ".m4a", ".ogg", ".opus"}
ART_EXTS = ("jpeg", "jpg", "bmp")


def group_key(tags):
    """(albumartist|artist, album) folded for grouping; None if no album."""
    album = (tags.get("album") or "").strip()
    if not album:
        return None
    artist = (tags.get("albumartist") or tags.get("artist") or "").strip()
    return (artist.casefold(), album.casefold())


def art_file_names(album):
    """Filenames the firmware would accept as this album's art."""
    names = ["cover.%s" % ext for ext in ART_EXTS]
    if album:
        names += ["%s.%s" % (album, ext) for ext in ART_EXTS]
    return names


def has_art(embedded_any, album, dir_listing, parent_listing):
    """Album-has-art decision per spec 0002 R2 (case-insensitive: FAT)."""
    if embedded_any:
        return True
    targets = {n.casefold() for n in art_file_names(album)}
    for listing in (dir_listing, parent_listing):
        for name in listing:
            if name.casefold() in targets:
                return True
    return False


@dataclass
class Album:
    key: tuple
    artist: str
    album: str
    tracks: list = field(default_factory=list)
    dirs: set = field(default_factory=set)
    embedded_any: bool = False
    has_art: bool = False
    skipped: list = field(default_factory=list)


def _read_tags(path):
    """Return (tags dict, embedded_art bool) or None if unreadable."""
    try:
        f = mutagen.File(str(path))
    except Exception as e:
        log.warning("unreadable: %s (%s)", path, e)
        return None
    if f is None:
        return None
    tags = {}
    easy = mutagen.File(str(path), easy=True)
    if easy and easy.tags:
        for k in ("album", "artist", "albumartist"):
            v = easy.tags.get(k)
            if v:
                tags[k] = v[0]
    embedded = _has_embedded(f)
    return tags, embedded


def _has_embedded(f):
    if getattr(f, "pictures", None):
        return True
    if f.tags is None:
        return False
    for k in f.tags.keys():
        lk = k.lower() if isinstance(k, str) else k
        if isinstance(lk, str) and (lk.startswith("apic") or lk == "covr"
                                    or lk == "metadata_block_picture"):
            return True
    return False


def scan(root):
    """Walk root, group tracks into Albums and decide has_art for each."""
    root = Path(root)
    albums = {}
    listings = {}

    for path in sorted(root.rglob("*")):
        if not path.is_file() or path.suffix.lower() not in AUDIO_EXTS:
            continue
        read = _read_tags(path)
        if read is None:
            continue
        tags, embedded = read
        key = group_key(tags)
        if key is None:
            continue
        a = albums.get(key)
        if a is None:
            a = albums[key] = Album(
                key=key,
                artist=tags.get("albumartist") or tags.get("artist") or "",
                album=tags.get("album", ""))
        a.tracks.append(path)
        a.dirs.add(path.parent)
        a.embedded_any = a.embedded_any or embedded

    def listing(d):
        if d not in listings:
            try:
                listings[d] = [p.name for p in d.iterdir() if p.is_file()]
            except OSError:
                listings[d] = []
        return listings[d]

    for a in albums.values():
        a.has_art = any(
            has_art(a.embedded_any, a.album, listing(d), listing(d.parent))
            for d in a.dirs)

    return sorted(albums.values(), key=lambda a: a.key)
