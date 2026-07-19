---
name: bootloaders-embedded
description: Embedded bootloader skill for firmware update and app handoff. Use when working on bootloader code, jumping to application firmware, relocating vectors, validating images, or firmware-update flows. Activates on queries about bootloader jump, vector relocation, application entry point, or firmware image validation.
---

# Embedded Bootloaders

## Purpose

Guide agents through embedded bootloader fundamentals: vector relocation, safe handoff from bootloader to application, flash/storage partitioning, and firmware-update safety patterns.

## When to Use

- Application must run at a non-zero offset / loaded into RAM
- Debugging "app works when flashed alone but not via bootloader"
- Implementing or modifying firmware update logic
- Validating firmware images before jumping to them

## Workflow

### 1. Valid application image check

Before jumping, verify the image:

```
Entry point lies inside the region the image was loaded to
Optional: checksum/CRC or magic word in image header
Never jump to an unvalidated buffer
```

### 2. Handoff sequence (generic)

```
Bootloader handoff
├── Disable interrupts
├── Stop bootloader-owned hardware (timers, DMA, USB)
├── Relocate/restore vector base for the app
├── Set stack pointer to app's initial SP
└── Jump to app entry — does not return
```

The application must not assume peripherals are in reset state if the
bootloader touched them: either the bootloader de-inits, or the app
re-inits everything it uses.

### 3. Bootloader responsibilities

```
Power-on
├── Init minimal clocks + storage
├── Check for update/recovery condition (key held, flag set)
├── If update requested → receive image, verify, then commit
└── Else if valid app → load and jump
    └── Else stay in recovery mode
```

### 4. Update safety

- Write to scratch area, verify checksum, then commit atomically
- Never erase the only valid image without a recovery path
- Keep a hardware-triggered recovery entry (button combo) that runs
  before any potentially-corrupt image

## Common Problems

| Symptom | Cause | Fix |
|---------|-------|-----|
| Crash after jump | SP/entry invalid | Validate image header before jump |
| IRQs hit bootloader handlers | Vectors not relocated | Restore vector base before jump |
| App OK standalone, fails via BL | Linked for wrong load address | Relink app for actual load address |
| Peripheral garbage after jump | BL left hardware running | De-init or re-init in app |
| Brick after update | Power loss mid-write | Scratch + atomic commit, recovery path |

## Better-Rockpod notes

- Rockbox bootloaders live in `bootloader/` (`ipod.c` for 5G,
  `ipod6g.c` for Classic). On iPod Video the Apple flash bootloader
  loads the Rockbox bootloader, which then loads `rockbox.ipod` from
  disk into RAM and jumps. On iPod Classic 6G the chain is
  emCORE/s5l8702 pwnage or the dualboot installer loading `rockbox.ipodx`.
- Firmware images are wrapped with `tools/scramble` (checksum + model
  ID); validation happens when the bootloader loads the file.
- Vector/entry handling is classic ARM (jump to load address), not
  Cortex-M VTOR.
