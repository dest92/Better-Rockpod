# Apple iPod Video 5G firmware: behavioral notes (clean-room)

Notes from static analysis of Apple's stock iPod Video 5G/5.5G firmware.
They describe **behavior and hardware usage only**. No Apple code is
copied into this tree; implementations here are written from these
notes and from Rockbox's own register definitions.

## Source and tooling

- Firmware: `iPod_25.1.3.ipsw`, downloaded from Apple's update server
  (URL in iTunes' version plist, `https://itunes.com/version`).
  `Firmware-25.6.3` holds an Apple firmware partition with the image
  directory at `0x4200`: `osos` (devOffset `0x4800`, len `0x736000`),
  `rsrc`, `aupd`.
- `osos` has a 0x200-byte header. The code (`osos[0x200:]`) is plain
  ARM (not encrypted) and loads at `0x10000000`.
- Analysis: [REA](https://github.com/morluto/rea) 6.8.0 with Ghidra
  12.1.4 (raw-binary, `arm`, entry `0x10000000`). It recovered 14,857
  functions and 11,232 strings, and `decompile-all` exported 14,702
  functions.
- **iPod Classic 6G/7G** (`iPod_24.1.1.2`, `iPod_38.2.0.5`): `osos` is
  an `8702` IMG1 image with entropy 8.0, encrypted with the S5L8702
  hardware AES key. It cannot be analyzed without decrypting it on a
  device (freemyipod / emCORE).

Addresses below refer to firmware 1.3 (`osos` sha256 of the code
region: `d6e95307…4290`).

## Finding 1: LCD updates go through the BCM2722 by DMA

| Apple function | Role (inferred) |
|---|---|
| `0x10287f98` | BCM bus init: writes a fixed sequence to `0x30030000` / `0x30070000`, then polls status bit 0 |
| `0x102881e8` | BCM block write: lock, write the 32-bit BCM address to `0x30010000` as two halfwords, copy an unaligned head by PIO, hand the 256-byte-aligned bulk to `0x102875b4`, then PIO the tail |
| `0x1028806c` | BCM block read (PIO, polls `0x30030000` bit 4) |
| `0x102875b4` | Bulk transfer: cleans the D-cache range (`0x1009afec`), then submits **64 KiB DMA transfers** (`0x10159ffc`) and blocks on a completion callback (vtable `+0x18`) between chunks |
| `0x10159ffc` | DMA controller object: queues a transfer on channel 0 or 1 |
| `0x101da75c` | DMA controller constructor: base `0x6000a000`, channels at `base + 0x1000 + n*0x20` (`0x6000b000`, `0x6000b020`), two channels |

**Contrast with Rockbox** (`firmware/target/arm/ipod/video/lcd-video.c`
`lcd_update_rect()` → `lcd-as-video.S` `lcd_write_data`): the CPU
copies every pixel to `BCM_DATA32` with `ldm`/`stm`. A full frame is
320x240x2 = 150 KiB of uncached bus writes, during which the CPU can do
nothing else. Apple's firmware lets the DMA engine move the data while
the thread sleeps, so codec and UI threads keep running. This is a
plausible contributor to the stock firmware's smoother scrolling and to
fewer audio underruns during heavy redraws (Cover Flow, scrolling
lists).

Rockbox already uses PP5022 DMA channel 0 for audio
(`firmware/target/arm/pp/pcm-pp.c:128`). Channel 1 is unused.

## Open questions (next analysis steps)

- Exact DMA command flags Apple writes for the BCM transfer (vtable
  methods of the object built in `0x101da75c`): request ID, width,
  burst, and whether it waits on a request line or free-runs.
- How Apple triggers the BCM "LCD update" command after a DMA write,
  and whether it double-buffers in BCM SRAM.
- Click wheel driver (base `0x7000c000`, functions around
  `0x10283390`–`0x10283558`): sampling rate and acceleration curve.
- CPU clock management (`0x60006020` CLOCK_SOURCE users
  `0x101b79c8`, `0x101b8b64`, `0x101b95d0`): when and how far Apple
  boosts the CPU.
