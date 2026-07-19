"""CLI entry point: scan, report, and serve the review UI."""

import argparse
import logging
import sys
import webbrowser

from .server import AppState, serve


def main(argv=None):
    ap = argparse.ArgumentParser(
        prog="albumart_fetcher",
        description="Find albums without art on a mounted iPod (or any "
                    "music folder), fetch covers online and apply them "
                    "after review in a local web UI.")
    ap.add_argument("root", help="music root, e.g. /Volumes/IPOD/Music")
    ap.add_argument("--port", type=int, default=8000,
                    help="web UI port (default 8000)")
    ap.add_argument("--max-side", type=int, default=600,
                    help="max cover dimension in px (default 600)")
    ap.add_argument("--cover-file", action="store_true",
                    help="also write cover.jpg into album directories")
    ap.add_argument("--dry-run", action="store_true",
                    help="never write to the music files")
    ap.add_argument("--scan-only", action="store_true",
                    help="print the report and exit (no server)")
    ap.add_argument("--no-browser", action="store_true",
                    help="do not open the browser automatically")
    ap.add_argument("-v", "--verbose", action="store_true")
    args = ap.parse_args(argv)

    logging.basicConfig(
        level=logging.DEBUG if args.verbose else logging.INFO,
        format="%(levelname)s %(name)s: %(message)s")

    print("Escaneando %s ..." % args.root)
    state = AppState(args.root, max_side=args.max_side,
                     cover_file=args.cover_file, dry_run=args.dry_run)
    total = len(state.rescan())
    missing = len(state.albums)
    print("Álbumes: %d — sin carátula: %d" % (total, missing))

    if args.scan_only:
        for a in state.albums:
            print("  %s — %s (%d pistas)" % (a.artist, a.album,
                                             len(a.tracks)))
        return 0
    if missing == 0:
        print("Nada que hacer.")
        return 0

    httpd = serve(state, port=args.port)
    url = "http://127.0.0.1:%d/" % args.port
    print("UI de revisión: %s  (Ctrl-C para terminar)" % url)
    if not args.no_browser:
        webbrowser.open(url)
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\nListo.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
