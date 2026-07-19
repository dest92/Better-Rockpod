"""Fixture builders: minimal-but-valid audio files for mutagen."""

import struct


def minimal_mp3_bytes():
    """A few MPEG-1 Layer III 128kbps 44.1kHz frames of silence."""
    frame = bytes([0xFF, 0xFB, 0x90, 0x00]) + b"\x00" * (417 - 4)
    return frame * 8


def minimal_flac_bytes():
    """fLaC magic + a single (last) STREAMINFO block, no audio frames.

    Enough for mutagen.flac.FLAC to open and rewrite metadata.
    """
    min_block = max_block = 4096
    min_frame = max_frame = 0
    sample_rate = 44100
    channels = 2
    bps = 16
    total_samples = 0

    packed = (sample_rate << 44) | ((channels - 1) << 41) \
        | ((bps - 1) << 36) | total_samples
    streaminfo = struct.pack(">HH", min_block, max_block)
    streaminfo += min_frame.to_bytes(3, "big") + max_frame.to_bytes(3, "big")
    streaminfo += packed.to_bytes(8, "big")
    streaminfo += b"\x00" * 16  # md5 of no audio
    assert len(streaminfo) == 34
    # 0x80 = last-metadata-block flag, block type 0 (STREAMINFO)
    header = bytes([0x80]) + len(streaminfo).to_bytes(3, "big")
    return b"fLaC" + header + streaminfo


def write_mp3(path, artist=None, album=None):
    path.write_bytes(minimal_mp3_bytes())
    if artist or album:
        from mutagen.id3 import ID3, TPE1, TALB
        tag = ID3()
        if artist:
            tag.add(TPE1(encoding=3, text=[artist]))
        if album:
            tag.add(TALB(encoding=3, text=[album]))
        tag.save(str(path))
    return path


def write_flac(path, artist=None, album=None):
    path.write_bytes(minimal_flac_bytes())
    from mutagen.flac import FLAC
    f = FLAC(str(path))
    if artist:
        f["artist"] = [artist]
    if album:
        f["album"] = [album]
    f.save()
    return path


def make_test_image(width=800, height=800, fmt="JPEG", progressive=False):
    from io import BytesIO
    from PIL import Image
    img = Image.new("RGB", (width, height), (200, 30, 30))
    buf = BytesIO()
    if fmt == "JPEG":
        img.save(buf, "JPEG", progressive=progressive, quality=85)
    else:
        img.save(buf, fmt)
    return buf.getvalue()
