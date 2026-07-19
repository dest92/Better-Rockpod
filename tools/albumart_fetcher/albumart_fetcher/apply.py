"""Embed normalized art into tracks and optionally write cover.jpg.

All writers receive bytes that already passed imaging.normalize();
this module never re-encodes. fsync before reporting success: the
target is usually a FAT volume on USB.
"""

import base64
import logging
import os
from dataclasses import dataclass, field
from pathlib import Path

log = logging.getLogger(__name__)


@dataclass
class ApplyReport:
    done: int = 0
    errors: list = field(default_factory=list)


def make_vorbis_picture(jpeg, width, height):
    """base64 METADATA_BLOCK_PICTURE value (front cover, JPEG)."""
    from mutagen.flac import Picture
    pic = Picture()
    pic.type = 3
    pic.mime = "image/jpeg"
    pic.desc = "Cover"
    pic.width = width
    pic.height = height
    pic.depth = 24
    pic.data = jpeg
    return base64.b64encode(pic.write()).decode("ascii")


def _embed_mp3(path, jpeg):
    from mutagen.id3 import ID3, ID3NoHeaderError, APIC
    try:
        tag = ID3(str(path))
    except ID3NoHeaderError:
        tag = ID3()
    tag.delall("APIC")
    tag.add(APIC(encoding=3, mime="image/jpeg", type=3, desc="Cover",
                 data=jpeg))
    tag.save(str(path))


def _embed_flac(path, jpeg, width, height):
    from mutagen.flac import FLAC, Picture
    f = FLAC(str(path))
    f.clear_pictures()
    pic = Picture()
    pic.type = 3
    pic.mime = "image/jpeg"
    pic.desc = "Cover"
    pic.width = width
    pic.height = height
    pic.depth = 24
    pic.data = jpeg
    f.add_picture(pic)
    f.save()


def _embed_mp4(path, jpeg):
    from mutagen.mp4 import MP4, MP4Cover
    f = MP4(str(path))
    if f.tags is None:
        f.add_tags()
    f.tags["covr"] = [MP4Cover(jpeg, imageformat=MP4Cover.FORMAT_JPEG)]
    f.save()


def _embed_ogg(path, jpeg, width, height):
    import mutagen
    f = mutagen.File(str(path))
    if f is None:
        raise ValueError("unreadable ogg file")
    if f.tags is None:
        f.add_tags()
    f.tags["metadata_block_picture"] = [
        make_vorbis_picture(jpeg, width, height)]
    f.save()


def _fsync(path):
    fd = os.open(str(path), os.O_RDONLY)
    try:
        os.fsync(fd)
    finally:
        os.close(fd)


def embed_track(path, jpeg, width, height):
    """Embed jpeg as front cover into one track, by container type."""
    path = Path(path)
    ext = path.suffix.lower()
    if ext == ".mp3":
        _embed_mp3(path, jpeg)
    elif ext == ".flac":
        _embed_flac(path, jpeg, width, height)
    elif ext == ".m4a":
        _embed_mp4(path, jpeg)
    elif ext in (".ogg", ".opus"):
        _embed_ogg(path, jpeg, width, height)
    else:
        raise ValueError("unsupported container: %s" % ext)
    _fsync(path)


def write_cover_file(dirpath, jpeg):
    path = Path(dirpath) / "cover.jpg"
    with open(path, "wb") as f:
        f.write(jpeg)
        f.flush()
        os.fsync(f.fileno())
    return path


def apply_to_album(album, jpeg, width, height, cover_file=False,
                   dry_run=False):
    """Embed into every track of album; per-track failures collected."""
    report = ApplyReport()
    for track in album.tracks:
        try:
            if not dry_run:
                embed_track(track, jpeg, width, height)
            report.done += 1
        except Exception as e:
            log.warning("embed failed: %s (%s)", track, e)
            report.errors.append((str(track), str(e)))
    if cover_file and not dry_run:
        for d in album.dirs:
            try:
                write_cover_file(d, jpeg)
            except OSError as e:
                report.errors.append((str(d), str(e)))
    return report
