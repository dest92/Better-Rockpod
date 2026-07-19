"""Image normalization guaranteeing firmware-decodable output.

The firmware JPEG decoder (apps/recorder/jpeg_load.c) only accepts
baseline DCT (SOF0) with component sampling factors <= 2x2; it
rejects progressive JPEG (returns -4) and never decodes embedded
PNG/BMP. normalize() re-encodes anything Pillow can open into that
safe subset; is_rockbox_safe_jpeg() re-checks the exact same rules
on the produced bytes as an independent oracle.
"""

from io import BytesIO

from PIL import Image

_SOF_BASELINE = 0xC0
_UNSUPPORTED_SOF = {0xC1, 0xC2, 0xC3, 0xC5, 0xC6, 0xC7,
                    0xC8, 0xC9, 0xCA, 0xCB, 0xCD, 0xCE, 0xCF}
_STANDALONE = set(range(0xD0, 0xD8)) | {0x01}  # RSTn, TEM


def _iter_segments(data):
    """Yield (marker, payload_offset, payload_len) for JPEG segments."""
    if len(data) < 4 or data[0] != 0xFF or data[1] != 0xD8:
        return
    i = 2
    n = len(data)
    while i + 3 < n:
        if data[i] != 0xFF:
            return
        marker = data[i + 1]
        if marker in _STANDALONE:
            i += 2
            continue
        if marker == 0xD9:  # EOI
            return
        seglen = (data[i + 2] << 8) | data[i + 3]
        if seglen < 2 or i + 2 + seglen > n:
            return
        yield marker, i + 4, seglen - 2
        if marker == 0xDA:  # SOS: entropy data follows, stop parsing
            return
        i += 2 + seglen


def _parse_sof0(data, off, length):
    """Return (width, height, max_sampling) from an SOF0 payload."""
    if length < 6:
        return None
    height = (data[off + 1] << 8) | data[off + 2]
    width = (data[off + 3] << 8) | data[off + 4]
    ncomp = data[off + 5]
    if length < 6 + 3 * ncomp:
        return None
    max_samp = 0
    for c in range(ncomp):
        samp = data[off + 6 + 3 * c + 1]
        max_samp = max(max_samp, samp >> 4, samp & 0x0F)
    return width, height, max_samp


def is_rockbox_safe_jpeg(data):
    """True iff data is a JPEG jpeg_load.c can decode."""
    for marker, off, length in _iter_segments(data):
        if marker in _UNSUPPORTED_SOF:
            return False
        if marker == _SOF_BASELINE:
            parsed = _parse_sof0(data, off, length)
            return parsed is not None and parsed[2] <= 2
    return False


def jpeg_dimensions(data):
    """(width, height) from the SOF0 header, or None."""
    for marker, off, length in _iter_segments(data):
        if marker == _SOF_BASELINE:
            parsed = _parse_sof0(data, off, length)
            if parsed:
                return parsed[0], parsed[1]
    return None


def normalize(data, max_side=600):
    """Re-encode arbitrary image bytes as Rockbox-safe baseline JPEG.

    Returns (jpeg_bytes, width, height). Raises ValueError on
    undecodable input.
    """
    try:
        img = Image.open(BytesIO(data))
        img.load()
    except Exception as e:
        raise ValueError("cannot decode image: %s" % e) from e
    if img.mode != "RGB":
        img = img.convert("RGB")
    if max(img.size) > max_side:
        img.thumbnail((max_side, max_side), Image.LANCZOS)
    out = BytesIO()
    img.save(out, "JPEG", quality=90, progressive=False,
             subsampling=2, optimize=True)
    jpeg = out.getvalue()
    if not is_rockbox_safe_jpeg(jpeg):
        raise ValueError("normalized image failed safety check")
    return jpeg, img.size[0], img.size[1]
