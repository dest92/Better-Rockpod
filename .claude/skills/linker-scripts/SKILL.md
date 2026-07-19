---
name: linker-scripts
description: Linker script skill for embedded bare-metal targets. Use when writing or modifying GNU ld linker scripts, placing code and data in specific memory regions, understanding VMA vs LMA, configuring startup .bss/.data initialization, using MEMORY and SECTIONS commands, or debugging linker errors about regions. Activates on queries about linker scripts, MEMORY command, SECTIONS command, .bss init, VMA vs LMA, weak symbols, or placing functions in specific memory regions.
---

# Linker Scripts

## Purpose

Guide agents through writing and modifying GNU ld linker scripts for embedded targets: MEMORY and SECTIONS commands, VMA vs LMA for code relocation, startup `.bss`/`.data` initialization, placing sections in specific regions, and using PROVIDE/KEEP/ALIGN directives.

## Triggers

- "How do I place a function in a specific RAM/IRAM region?"
- "What's the difference between VMA and LMA in a linker script?"
- "How does .bss and .data initialization work at startup?"
- "Linker error: region overflowed"
- "How do I use weak symbols in a linker script?"

## Workflow

### 1. Linker script anatomy

```ld
ENTRY(start)                    /* entry point symbol */

MEMORY
{
    DRAM (rwx) : ORIGIN = 0x08000000, LENGTH = 32M
    IRAM (rwx) : ORIGIN = 0x40000000, LENGTH = 96K
}

SECTIONS
{
    .text :
    {
        KEEP(*(.vectors))       /* vector code must be first */
        *(.text*)
        *(.rodata*)
        . = ALIGN(4);
        _etext = .;
    } > DRAM

    .data : AT(_etext)          /* VMA = RAM, LMA = image */
    {
        _sdata = .;
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > DRAM

    .bss :
    {
        _sbss = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > DRAM
}
```

### 2. VMA vs LMA

- **VMA** (Virtual Memory Address): where the section runs at runtime
- **LMA** (Load Memory Address): where the section is stored in the image

For `.data`: stored in the image (LMA), copied to its VMA at startup.

```ld
.data : AT(ADDR(.text) + SIZEOF(.text))
{
    _sdata = .;
    *(.data)
    _edata = .;
} > RAM
_sidata = LOADADDR(.data);    /* LMA of .data for startup code */
```

### 3. Startup .bss / .data initialization

The startup code must copy `.data` from its load address and zero `.bss`
before `main()`:

```c
extern uint32_t _sdata, _edata, _sidata;
extern uint32_t _sbss, _ebss;

void reset_handler(void) {
    uint32_t *src = &_sidata;
    uint32_t *dst = &_sdata;
    while (dst < &_edata) *dst++ = *src++;
    dst = &_sbss;
    while (dst < &_ebss) *dst++ = 0;
    main();
    for (;;);
}
```

### 4. Placing code in specific regions

```ld
.icode : AT(LOADADDR(.data) + SIZEOF(.data))
{
    _iramstart = .;
    *(.icode)      /* functions marked with a section attribute */
    *(.idata)
    _iramend = .;
} > IRAM
```

```c
__attribute__((section(".icode")))
void hot_inner_loop(void) { /* runs from fast IRAM */ }
```

### 5. KEEP, ALIGN, PROVIDE

```ld
KEEP(*(.vectors))            /* linker gc won't remove it */
. = ALIGN(8);                /* advance location counter */
PROVIDE(_stack_size = 0x400);  /* default, overridable */
```

### 6. Weak symbols

```ld
PROVIDE(irq_handler = default_handler);
```

```c
__attribute__((weak)) void default_handler(void) { for (;;); }
/* Override by defining a non-weak symbol with the same name */
```

### 7. Common linker errors

| Error | Cause | Fix |
|-------|-------|-----|
| `region overflowed` | Binary too large for region | `-Os`, `--gc-sections`, move data out of IRAM |
| `undefined reference to '_x'` | Missing linker script symbol | Define it in the script |
| `.data` at wrong address | LMA not set | Add `AT(...)` / `LOADADDR` |
| `cannot find linker script` | Wrong path | `-L dir -T name.lds` |

```bash
# Analyze section sizes and addresses
arm-elf-eabi-size -A rockbox.elf
arm-elf-eabi-objdump -h rockbox.elf
arm-elf-eabi-readelf -S rockbox.elf
```

## Better-Rockpod notes

- Rockbox linker scripts are *preprocessed* (`.lds` files run through
  cpp with the target config): main binary `apps/app.lds` (hardware) —
  the build generates `rockbox.lds` in the build dir — plus
  `firmware/target/arm/*.lds` and `boot.lds` for bootloaders. Edit the
  source `.lds`, then `make` regenerates.
- IRAM placement uses Rockbox macros `ICODE_ATTR`, `IDATA_ATTR`,
  `ICONST_ATTR` (see `firmware/export/config.h`) — use those instead of
  raw section attributes.
- iPod Video has 96K IRAM (PP5022); iPod Classic has 256K+ IRAM
  (S5L8702). IRAM overflow shows up as "region IRAM overflowed" when
  too much is marked `ICODE_ATTR`.
- Plugins and codecs have their own link maps (`plugin.lds`,
  `codecs.lds` generated from `apps/plugins/plugin.lds`).
