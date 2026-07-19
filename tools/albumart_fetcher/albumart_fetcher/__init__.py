"""Host-side album art fetcher for Better-Rockpod (spec 0002).

Scans a music tree (typically the USB-mounted iPod), finds albums
without art, fetches candidates from iTunes / Cover Art Archive and
applies user-approved covers as Rockbox-safe baseline JPEG.
"""

__version__ = "1.0"
