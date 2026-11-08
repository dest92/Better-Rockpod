# 0017: Hardware H.264 video player on the iPod Classic 6G

- **Status:** Implemented (on-device check pending)
- **Branch/PR:** `master` (roadmap 2.6, upstream Rockbox `e0136bc7f` by David Cormier)

## Problem

The roadmap tracked finishing the S5L8702 H.264 hardware-decoding proof
of concept (`h264_poc`). Upstream Rockbox has since merged a complete
implementation, tested on an iPod Classic 6G:
- the VPU-B driver (`vpu-6g.c`, `vpu-h264-6g.c`, `HAVE_HW_H264`);
- a capability-gated decoder interface in the plugin API;
- the `h264_player` viewer plugin (MP4 demux, AAC audio, A/V sync).

Contributing that work here beats re-deriving it from the PoC.

## Requirements

- **R1:** The 6G build ships `viewers/h264_player.rock` and registers
  it for `.mp4`, `.m4v` and `.mov`. Supported input: non-fragmented
  MP4/M4V, Constrained Baseline H.264 up to level 3.0 and 640x480 at
  30 fps, optional AAC-LC audio.
- **R2:** iPod Video and the simulator are unaffected (no
  `HAVE_HW_H264`).
- **R3:** Normal `.m4a` audio (AAC, ALAC) keeps playing after the
  libm4a video-first changes.

## Port

- Cherry-picked:
  - `064c16536` (libm4a: sparse chunk maps, video-first MP4);
  - `e0136bc7f` (the driver and plugin);
  - `d27af08ff` (cleanup).
- Conflicts resolved by keeping both sides. `hw_h264` is appended
  after this fork's API entries.
- Adaptations for this tree (`cd14097155`):
  - `panicf` in the plugin and codec APIs (upstream `9ac6edf75`);
  - the pre-`mixer_play_cbs` mixer call;
  - `1u` for `UINT32_C(1)`.
- Versions: `PLUGIN_API_VERSION` 284, `CODEC_API_VERSION` 51.
- The fork's `h264_poc` plugin stays for now as a hardware diagnostic.

## Verification

- `ipod6g` builds `h264_player.rock`, and `viewers.config` maps
  mp4/m4v/mov to it.
- `ipodvideo` and the simulator build.
- Simulator: AAC and ALAC `.m4a` play and advance (R3).
- **On device (6G), still to check:** an H.264 Baseline MP4 (for
  example `ffmpeg -i in.mp4 -c:v libx264 -profile:v baseline -level 3.0
  -vf scale=320:-2 -c:a aac out.mp4`) plays with sound and responds to
  the controls. The simulator has no VPU.
