---
name: cross-gcc
description: Cross-compilation with GCC skill for embedded and multi-architecture targets. Use when setting up cross-gcc toolchains, building for ARM/MIPS from an x86-64 host, troubleshooting wrong-architecture errors, or missing-toolchain build failures. Activates on queries about cross-compilation triplets, embedded toolchains, arm-elf-eabi, or "cannot execute binary file".
---

# Cross-GCC

## Purpose

Guide agents through setting up and using cross-compilation GCC toolchains: triplets, bare-metal flags, and common failure modes.

## Triggers

- "How do I compile for ARM on my x86 machine?"
- "I'm getting 'wrong ELF class' or 'cannot execute binary file'"
- "configure says the cross compiler is missing"
- "undefined reference to __aeabi_*"

## Workflow

### 1. Understand the triplet

A GNU triplet has the form `<arch>-<vendor>-<os>-<abi>`:

| Triplet | Target |
|---------|--------|
| `arm-elf-eabi` | Bare-metal ARM (Rockbox native targets) |
| `arm-none-eabi` | Bare-metal ARM (generic newlib) |
| `arm-linux-gnueabihf` | 32-bit ARM Linux hard-float |
| `mipsel-elf` | Little-endian MIPS bare-metal |

### 2. Bare-metal compilation shape

```bash
arm-elf-eabi-gcc -mcpu=arm926ej-s \
    -ffreestanding -nostdlib -T linker.lds -o firmware.elf crt0.o main.o -lgcc
```

Key flags: `-ffreestanding -nostdlib` (no host libc), `-T` linker
script, `-lgcc` for compiler runtime helpers (`__aeabi_*` division etc.).

### 3. Inspect cross binaries

```bash
arm-elf-eabi-objdump -d -S prog.elf   # disassemble with source
arm-elf-eabi-size -A prog.elf         # per-section sizes
arm-elf-eabi-readelf -h prog.elf      # confirm machine/ABI
file prog.elf                          # quick arch check
```

### 4. Common errors

| Error | Cause | Fix |
|-------|-------|-----|
| `cannot execute binary file` | Running target binary on host | It's a target binary — use sim build for host testing |
| `wrong ELF class` | Wrong-architecture object linked | Same toolchain for all objects; `make clean` |
| `undefined reference to '__aeabi_*'` | Missing ARM ABI runtime | Link with `-lgcc` |
| `unrecognized opcode` | Wrong `-mcpu`/`-march` | Match CPU flags to target core |
| `command not found: arm-elf-eabi-gcc` | Toolchain not installed / not in PATH | Build with `tools/rockboxdev.sh`; add to PATH |

### 5. Environment variables

```bash
export PATH=$PATH:/usr/local/arm-elf-eabi/bin   # rockboxdev.sh default prefix
```

## Better-Rockpod notes

- Rockbox toolchains are built by `tools/rockboxdev.sh`; the ARM one is
  `arm-elf-eabi-` (option `y` builds arm-eabi with GCC 4.9.4 +
  binutils 2.26.1 as pinned by the script). `tools/configure` probes for
  it and prints which prefix it expects.
- CPU flags per target: iPod Video (PP5022) is ARM7TDMI
  (`-mcpu=arm7tdmi`); iPod Classic (S5L8702) is ARM926EJ-S
  (`-mcpu=arm926ej-s`). configure sets these — don't hand-roll CFLAGS.
- Host-testable code paths belong in the simulator build
  (`./build-sim.sh`) or the `tests/` harness; never try to run
  `rockbox.bin` on the host.
- `brew`/`sudo` are unavailable in this environment — if a toolchain is
  missing, ask the user to install it rather than attempting apt/brew.
