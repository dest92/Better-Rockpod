---
name: baremetal-startup
description: Bare-metal startup skill for reset-to-main bring-up. Use when writing startup code, vector tables, .data/.bss init, stack setup, or crt0. Activates on queries about reset vector, startup.s, crt0, bss init, or bare-metal entry point.
---

# Bare-Metal Startup

## Purpose

Guide agents through bare-metal startup from reset to `main()`: reset and exception vectors, `.data`/`.bss` initialization, stack and heap setup, and C runtime integration with linker scripts.

## When to Use

- Firmware hangs before reaching `main()`
- Writing or modifying `crt0.S` / `Reset_Handler`
- Debugging uninitialized globals or stack overflows at boot
- Integrating startup code with linker scripts

## Workflow

### 1. Boot sequence mental model

```
Power-on / reset
├── CPU starts at reset vector
├── Reset handler: init clocks (optional), copy .data, zero .bss
├── Set up stack pointer(s)
├── Call platform init
└── Call main() — must not return (loop or sleep)
```

### 2. crt0 responsibilities

| Task | Who does it |
|------|-------------|
| Copy `.data` LMA→VMA | Reset handler / crt0 |
| Zero `.bss` | Reset handler |
| Init stacks (per CPU mode on classic ARM) | crt0 |
| Heap (`_sbrk`) | Optional; libc or custom |
| Remap/relocate vectors | Platform init |

### 3. Linker script integration

```ld
_estack = ORIGIN(RAM) + LENGTH(RAM);
_sidata = LOADADDR(.data);
```

The startup code consumes symbols the linker script defines (`_sdata`,
`_edata`, `_sbss`, `_ebss`, load addresses). Any mismatch produces globals
with garbage values or a corrupted heap.

### 4. Classic ARM (ARMv4/v5) vector table

At address 0 (or remapped base), classic ARM has *instructions*, not
pointers: reset, undefined, SWI, prefetch abort, data abort, reserved,
IRQ, FIQ. Each entry is typically `ldr pc, [pc, #offset]` into a table of
handler addresses. Startup must also initialize the banked stack pointers
for IRQ/FIQ/SVC/ABT modes before enabling interrupts.

## Common Problems

| Symptom | Cause | Fix |
|---------|-------|-----|
| Crash before main | Stack pointer invalid | Point SP at valid RAM top per mode |
| Globals wrong at boot | `.data` not copied | Copy LMA→VMA in reset handler |
| BSS non-zero | `.bss` not zeroed | Add bss clear loop |
| IRQs jump to garbage | Vectors not remapped | Set up vector base / remap RAM |
| `main` returns into garbage | No loop after main | `while(1)` or sleep loop |

## Better-Rockpod notes

- Rockbox startup lives in `firmware/target/arm/crt0.S` (and
  bootloader variants); iPod Classic additionally has S5L8702-specific
  init under `firmware/target/arm/s5l8702/`. Both targets are classic
  ARM — the ARMv4/v5 vector-table section above applies, not Cortex-M
  vector-pointer tables.
- Memory maps and stack placement come from the `.lds` files referenced
  by each target's build (see `firmware/target/arm/*.lds`, `app.lds`,
  `boot.lds`).
- Rockbox already handles .data/.bss init; when adding sections (e.g.
  IRAM code with `ICODE_ATTR`), wire them into both the `.lds` and the
  crt0 copy loops.
