"""Online album-art sources: iTunes Search API and MusicBrainz + CAA.

Both are keyless. MusicBrainz requires a descriptive User-Agent and
max 1 request/second (https://musicbrainz.org/doc/MusicBrainz_API).
JSON parsing and ranking are pure functions over plain dicts so they
unit-test without network.
"""

import difflib
import logging
import threading
import time
from dataclasses import dataclass

import requests

log = logging.getLogger(__name__)

USER_AGENT = ("Better-Rockpod-AlbumArtFetcher/1.0 "
              "(https://github.com/dest92/Better-Rockpod)")
TIMEOUT = 15

_mb_lock = threading.Lock()
_mb_last = [0.0]


@dataclass
class Candidate:
    url: str
    thumb_url: str
    source: str
    artist: str = ""
    album: str = ""
    size: int = 0


def itunes_parse(data, size=600):
    """Candidates from an iTunes Search API response dict."""
    cands = []
    for r in data.get("results", []):
        art = r.get("artworkUrl100")
        if not art:
            continue
        cands.append(Candidate(
            url=art.replace("100x100bb", "%dx%dbb" % (size, size)),
            thumb_url=art,
            source="itunes",
            artist=r.get("artistName", ""),
            album=r.get("collectionName", ""),
            size=size))
    return cands


def caa_candidates_from_mb(data):
    """Candidates from a MusicBrainz release-search response dict.

    Cover Art Archive serves /release/<mbid>/front-<size> without
    needing to know beforehand whether art exists (404 if not)."""
    cands = []
    for r in data.get("releases", []):
        mbid = r.get("id")
        if not mbid:
            continue
        credits = r.get("artist-credit") or []
        artist = credits[0].get("name", "") if credits else ""
        base = "https://coverartarchive.org/release/%s" % mbid
        cands.append(Candidate(
            url=base + "/front-500",
            thumb_url=base + "/front-250",
            source="musicbrainz",
            artist=artist,
            album=r.get("title", ""),
            size=500))
    return cands


def _similarity(a, b):
    return difflib.SequenceMatcher(None, a.casefold(), b.casefold()).ratio()


def rank_candidates(cands, artist, album):
    """Best match first: label similarity to the album being searched."""
    def score(c):
        s = _similarity(c.album, album)
        if artist and c.artist:
            s += _similarity(c.artist, artist)
        return s
    return sorted(cands, key=score, reverse=True)


def _get(url, **kw):
    headers = kw.pop("headers", {})
    headers.setdefault("User-Agent", USER_AGENT)
    return requests.get(url, headers=headers, timeout=TIMEOUT, **kw)


def search_itunes(artist, album, size=600):
    try:
        resp = _get("https://itunes.apple.com/search",
                    params={"term": ("%s %s" % (artist, album)).strip(),
                            "entity": "album", "limit": 6})
        resp.raise_for_status()
        return itunes_parse(resp.json(), size=size)
    except Exception as e:
        log.warning("itunes search failed for %s - %s: %s",
                    artist, album, e)
        return []


def _mb_throttle():
    with _mb_lock:
        wait = _mb_last[0] + 1.0 - time.monotonic()
        if wait > 0:
            time.sleep(wait)
        _mb_last[0] = time.monotonic()


def search_caa(artist, album):
    try:
        _mb_throttle()
        query = 'release:"%s"' % album
        if artist:
            query += ' AND artist:"%s"' % artist
        resp = _get("https://musicbrainz.org/ws/2/release",
                    params={"query": query, "fmt": "json", "limit": 4})
        resp.raise_for_status()
        return caa_candidates_from_mb(resp.json())
    except Exception as e:
        log.warning("musicbrainz search failed for %s - %s: %s",
                    artist, album, e)
        return []


def search_all(artist, album, size=600):
    cands = search_itunes(artist, album, size=size)
    cands += search_caa(artist, album)
    return rank_candidates(cands, artist, album)


def fetch_image(url):
    """Download image bytes from a candidate or user-supplied URL."""
    resp = _get(url)
    resp.raise_for_status()
    return resp.content
