---
name: dma-baremetal
description: Bare-metal DMA skill for memory-peripheral transfers. Use when configuring DMA channels, circular/double buffering, or DMA IRQ completion. Activates on queries about DMA, circular buffer, memory-to-peripheral transfers, or DMA descriptor configuration.
---

# DMA (Bare-Metal)

## Purpose

Configure DMA controllers for memory-to-peripheral and peripheral-to-memory transfers: channel setup, burst sizes, circular/double buffering, completion interrupts, and cache coherency.

## When to Use

- Offloading audio/LCD/storage bulk transfers from the CPU
- Audio/streaming double buffers
- Debugging DMA not triggering or corrupt data

## Workflow

### 1. Generic DMA channel setup

```
DMA transfer checklist
├── Enable DMA controller clock
├── Program source address (+ increment mode)
├── Program destination address (+ increment mode)
├── Program transfer count and burst/width
├── Select peripheral request line
├── Enable completion/half interrupts as needed
└── Enable the channel AND the peripheral's DMA request
```

Both sides must be armed: the channel and the peripheral's DMA-enable
bit. Forgetting the peripheral side is the most common "DMA never
starts" cause.

### 2. Circular / double buffer

Audio-style streaming uses either hardware circular mode (auto-reload
count) or ping-pong descriptors. Process one half in the IRQ while the
other half streams.

### 3. Cache coherency

On cores with data caches (ARM926 and up), DMA does not see the CPU
cache:

- Before DMA reads memory the CPU wrote: clean (write back) the range
- After DMA writes memory the CPU will read: invalidate the range
- Or place DMA buffers in uncached/IRAM regions

## Common Problems

| Symptom | Cause | Fix |
|---------|-------|-----|
| DMA never starts | Peripheral request not enabled | Enable request on both sides |
| Corrupt data | Cache coherency | Clean/invalidate or uncached buffer |
| Transfer count stuck | Wrong request line mapping | Check SoC request matrix |
| IRQ flood | Flags not cleared in ISR | Clear status per datasheet |

## Better-Rockpod notes

- iPod Video (PP5022): audio out via IIS FIFO with the PP DMA engine
  (`firmware/target/arm/pp/`); LCD uses the BCM/LCD bridge path.
- iPod Classic (S5L8702): audio and LCD DMA under
  `firmware/target/arm/s5l8702/` (`dma-s5l8702.c`, `pcm-s5l8702.c`) —
  descriptor-based; follow the existing descriptor setup helpers.
- ARM926EJ-S (Classic) has caches: the coherency rules above are real
  there (`commit_dcache()` / `discard_dcache_range()` in Rockbox,
  `firmware/export/system.h` and target cache code). ARM7TDMI (5G) has
  no data cache on the main path, but IRAM placement still matters for
  performance.
