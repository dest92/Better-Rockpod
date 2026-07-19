"""Local web UI: review candidates and apply art (spec 0002 R4).

stdlib http.server only; the page is self-contained and every image
is proxied through /img so the browser never talks to an external
origin. Binds to 127.0.0.1 — this is a single-user local tool.
"""

import json
import logging
import threading
import urllib.parse
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

from . import apply as apply_mod
from . import imaging, scanner, sources

log = logging.getLogger(__name__)

_ALLOWED_IMG_HOSTS = ("mzstatic.com", "coverartarchive.org", "archive.org")


class AppState:
    def __init__(self, root, max_side=600, cover_file=False, dry_run=False):
        self.root = Path(root)
        self.max_side = max_side
        self.cover_file = cover_file
        self.dry_run = dry_run
        self.lock = threading.Lock()
        self.albums = []
        self.status = {}      # album id -> "missing"|"applied"|"skipped"
        self.candidates = {}  # album id -> [Candidate]
        self.rescan()

    def rescan(self):
        albums = scanner.scan(self.root)
        with self.lock:
            self.albums = [a for a in albums if not a.has_art]
            for i, _ in enumerate(self.albums):
                self.status.setdefault(i, "missing")
        return albums

    def album(self, idx):
        with self.lock:
            return self.albums[idx]

    def get_candidates(self, idx):
        with self.lock:
            cached = self.candidates.get(idx)
        if cached is not None:
            return cached
        a = self.album(idx)
        cands = sources.search_all(a.artist, a.album, size=self.max_side)
        with self.lock:
            self.candidates[idx] = cands
        return cands

    def apply_image(self, idx, img_bytes):
        jpeg, w, h = imaging.normalize(img_bytes, self.max_side)
        a = self.album(idx)
        report = apply_mod.apply_to_album(
            a, jpeg, w, h, cover_file=self.cover_file, dry_run=self.dry_run)
        if report.done and not report.errors:
            with self.lock:
                self.status[idx] = "applied"
        return report


def _album_json(state, idx, a):
    return {"id": idx, "artist": a.artist, "album": a.album,
            "tracks": len(a.tracks),
            "dirs": [str(d) for d in sorted(a.dirs)],
            "status": state.status.get(idx, "missing")}


def _cand_json(c):
    return {"url": c.url, "thumb": c.thumb_url, "source": c.source,
            "artist": c.artist, "album": c.album, "size": c.size}


class Handler(BaseHTTPRequestHandler):
    state = None  # set by serve()

    def log_message(self, fmt, *args):
        log.debug("http: " + fmt, *args)

    def _send(self, code, body, ctype="application/json"):
        if isinstance(body, (dict, list)):
            body = json.dumps(body).encode()
        self.send_response(code)
        self.send_header("Content-Type", ctype)
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def _err(self, code, msg):
        self._send(code, {"error": msg})

    def _read_body(self):
        length = int(self.headers.get("Content-Length", 0))
        return self.rfile.read(length)

    def do_GET(self):
        url = urllib.parse.urlparse(self.path)
        try:
            if url.path == "/":
                page = (Path(__file__).parent / "static" /
                        "index.html").read_bytes()
                self._send(200, page, "text/html; charset=utf-8")
            elif url.path == "/api/albums":
                with self.state.lock:
                    body = [_album_json(self.state, i, a)
                            for i, a in enumerate(self.state.albums)]
                self._send(200, body)
            elif url.path.startswith("/api/candidates/"):
                idx = int(url.path.rsplit("/", 1)[1])
                cands = self.state.get_candidates(idx)
                self._send(200, [_cand_json(c) for c in cands])
            elif url.path == "/img":
                self._proxy_image(url)
            else:
                self._err(404, "not found")
        except IndexError:
            self._err(404, "no such album")
        except Exception as e:
            log.exception("GET %s failed", self.path)
            self._err(500, str(e))

    def _proxy_image(self, url):
        qs = urllib.parse.parse_qs(url.query)
        target = qs.get("u", [""])[0]
        host = urllib.parse.urlparse(target).hostname or ""
        if not any(host == d or host.endswith("." + d)
                   for d in _ALLOWED_IMG_HOSTS):
            return self._err(403, "host not allowed")
        try:
            data = sources.fetch_image(target)
        except Exception:
            return self._err(502, "fetch failed")
        self._send(200, data, "image/jpeg")

    def do_POST(self):
        url = urllib.parse.urlparse(self.path)
        try:
            if url.path.startswith("/api/apply/"):
                idx = int(url.path.rsplit("/", 1)[1])
                req = json.loads(self._read_body() or b"{}")
                if req.get("skip"):
                    with self.state.lock:
                        self.state.status[idx] = "skipped"
                    return self._send(200, {"status": "skipped"})
                img_url = req.get("url")
                if not img_url:
                    return self._err(400, "missing url")
                img = sources.fetch_image(img_url)
                self._finish_apply(idx, img)
            elif url.path.startswith("/api/upload/"):
                idx = int(url.path.rsplit("/", 1)[1])
                img = self._read_body()
                if not img:
                    return self._err(400, "empty upload")
                self._finish_apply(idx, img)
            else:
                self._err(404, "not found")
        except IndexError:
            self._err(404, "no such album")
        except ValueError as e:
            self._err(400, str(e))
        except Exception as e:
            log.exception("POST %s failed", self.path)
            self._err(500, str(e))

    def _finish_apply(self, idx, img_bytes):
        report = self.state.apply_image(idx, img_bytes)
        self._send(200, {"status": self.state.status.get(idx),
                         "done": report.done,
                         "errors": report.errors})


def serve(state, port=8000):
    Handler.state = state
    httpd = ThreadingHTTPServer(("127.0.0.1", port), Handler)
    return httpd
