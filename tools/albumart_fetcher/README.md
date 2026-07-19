# Album Art Fetcher (spec 0002)

Host-side tool: finds albums on the USB-mounted iPod that have no
album art, searches iTunes and MusicBrainz/Cover Art Archive for
covers, and lets you review/override each match in a local web UI
before anything is written. Applied art is embedded into every
track's tags (and optionally written as `cover.jpg`) as baseline
JPEG ≤ 600 px — the only form the Rockbox JPEG decoder accepts.

## Setup

```sh
cd tools/albumart_fetcher
python3 -m pip install -r requirements.txt
```

## Usage

Mount the iPod over USB (Rockbox: just plug it in), then:

```sh
cd tools/albumart_fetcher
python3 -m albumart_fetcher /Volumes/IPOD/Music
```

A browser opens at `http://127.0.0.1:8000` listing every album
without art with fetched candidates. Per album you can accept a
candidate, paste an image URL, upload a local file, or skip.

Options:

- `--cover-file` — also write `cover.jpg` in the album folder
- `--max-side N` — max cover dimension in px (default 600)
- `--scan-only` — just print the report, no server
- `--dry-run` — go through the motions without writing
- `--port N`, `--no-browser`, `-v`

Supported track formats: mp3, flac, m4a/alac, ogg, opus.

## Tests

```sh
cd tools/albumart_fetcher
python3 -m unittest discover -s tests
```
