# Upstream Rockbox sync

## Fork point (roadmap 4.1)

This tree branched from upstream Rockbox at **`dd21a1d1d`**
("echoplayer: reduce LCD RGB & SPI output speed", 2026-02-10).

Method: the fork's first commit (`fec62d9bd7`, "log 8") was compared,
blob by blob, against every upstream `master` commit from 2024-10 to
2026-03 (1,537 commits).
- `dd21a1d1d` matches 10,680 of its 10,724 paths, more than any other
  commit.
- Its own change to `lcd-echoplayer.c` is present in the fork, while
  the changes from its first child `aea9e1b63` (`plugins.make`) and
  later children (`jz4760b.h`) are not.
- The remaining 44 differing paths are Rockpod's own changes.

Upstream remote: `git remote add rockbox https://github.com/Rockbox/rockbox`.

## Batch 1 (roadmap 4.2), 2026-11-08

Cherry-picked with `-x`, so each commit names its upstream origin.
Selection: codec, metadata, JPEG (album art), playlist and database
robustness, and iPod-specific storage and click-wheel fixes. iAP fixes
were skipped, because the rockpod v5.5+ merge carries its own iAP
overhaul.

Applied (39, including one hand-resolved):

- `958e84c04` codec: flac: fix playback of certain files
- `a81fd7b77` flac: handle zero-width escaped rice partitions
- `04ecb3c64` flac: don't divide by zero when STREAMINFO has no sample count
- `330236911` flac: decode residuals that use all 32 bits
- `6a69ead1d` flac: reject a second or short STREAMINFO block
- `10857759e` metadata: flac: check the PICTURE MIME type length
- `5d3b00815` metadata: flac: skip a bad PICTURE instead of failing
- `a97bbf6cf` metadata: id3: skip a UTF-16 APIC description correctly
- `ed0802e96` metadata: ape: fix the cover art size and the seek after it
- `a71a22919` metadata: ape: find an APE tag before an ID3v1 tag
- `2bb498b96` jpeg: fix decoding of grayscale images with 2x2 sampling
- `b897766a7` jpeg: use the Huffman tables the scan header selects
- `0e50ed3c4` jpeg: use the quantization table each component selects
- `e32bfaadb` jpeg: reject layouts the decoders cannot handle
- `943b73851` playback: prevent crossfade of new track after pause (FS#13958)
- `619f2f109` afr: fix gapless playback with fatigue reduction enabled
- `9b1f1e9e6` playlist: give the lock back when an insert cannot start
- `8a1aee5c7` playlist: allocate before locking in playlist_create()
- `91e1b2444` playlist: drop a torn last line from the control file on resume
- `3716177b5` playlist: read track info under the playlist lock
- `11c4f6865` tagcache: unlock the command queue when the master won't open
- `ab8859915` tagcache: commit a waiting temp file instead of skipping an update
- `b486769b2` tagcache: close the master when a search can't open its tag file
- `204c49b79` tagcache: don't wait for a commit in search and serial increase
- `0958cb732` alac: reject oversized frames and stop on decode errors
- `15d73677b` alac: keep the uncompressed low bytes of 24-bit samples
- `35f08f7eb` alac: clamp a zero run to the samples left in the frame
- `5b67f8a57` alac: reject a rice limit outside 1 to 25
- `1c0d1f5cf` alac: count leading zeros over 32 bits in the Rice decoder
- `437737913` ipod: limit clickwheel opto-I2C recovery to PortalPlayer
- `a0338697f` FAT:  Handle FAT32 filesystems with a zeroed FsInfo block
- `07d875ff7` thread: avoid arithmetic on a null pointer
- `28b422502` flac: read escaped residuals wider than 25 bits
- `66f3075f9` metadata: wave: bound RIFF INFO items to the buffer
- `2b8db3476` metadata: ape: check the header offset and divisors
- `1e4a74dfd` metadata: aiff: walk all chunks, bound the text chunks
- `c199d9a36` playback: fix single mode briefly plays audio from the next track before pausing
- `3373ed674` playback: fix single mode handling with auto frequency switch enabled
- `94313491a` ipod6g: don't cut drive power before the cache flush completes

`94313491a` (ipod6g: don't cut drive power before the cache flush
completes) conflicted with the fork's SSD sleep path. Resolution: keep
the fork's SSD branch, adopt upstream's checked flush (stay powered and
retry when it fails), and use the 3 s standby timeout on HDDs.

Deferred: these need upstream refactors the fork doesn't have yet.

- `e15451815` tagcache infinite scan/commit loop: already fixed in this
  tree in an equivalent way.
- `ecdeb02dd` 44.1 kHz floor when guessing the playback frequency:
  depends on the `pcm_sink` capability API (`dfa33c246`).
- `eb4cad520` biquad low bits: depends on the rewritten
  `dsp_arm.S` filter loop.
- `0ed3734e5` FLAC frames larger than one buffer request: conflicts
  with the fork's FLAC buffer changes.
- `790d28274` tagtree/pictureflow refused search: depends on the newer
  `tagtree` search helper signature.

Verification: `make -C tests` is green. The simulator, `ipod6g` and
`ipodvideo` build. Simulator playback of FLAC (embedded art), ALAC (m4a)
and MP3 (ID3 APIC art) works, with art decoded and dynamic colors
applied.
