---
name: debug-optimized-builds
description: Debugging optimized builds skill for diagnosing issues in release code. Use when debugging optimized binaries, using -Og for debuggable optimization, reading inlined frames, or understanding "value optimized out" messages. Activates on queries about debugging optimized code, -Og, inlined functions in GDB, value optimized out, or GDB with -O2/-Os.
---

# Debugging Optimized Builds

## Purpose

Guide agents through debugging code compiled with optimization: choosing the right debug-friendly optimization level, reading inlined frames, diagnosing "value optimized out", and applying GDB techniques specific to optimized code.

## Triggers

- "GDB says 'value optimized out' — what does that mean?"
- "How do I debug a release build?"
- "Breakpoints in optimized code land on wrong lines"

## Workflow

### 1. Choose the right build configuration

```
Goal?
├── Full debuggability, no optimization
│   → -O0 -g                        (slowest, all vars visible)
├── Debuggable, some optimization (recommended for most dev work)
│   → -Og -g                        (keeps debug experience good)
├── Release build with debug info
│   → -O2 -g
└── Full release (no debug symbols)
    → -O2 -DNDEBUG
```

**`-Og`**: GCC's "debug-friendly optimization" — variables stay visible
to GDB, line numbers stay accurate. Best balance for development.

### 2. "Value optimized out" — causes and workarounds

```text
(gdb) print my_variable
$1 = <optimized out>
```

The compiler decided the value doesn't need to be stored at this point:
kept in a register, constant-folded, or dead after this point.

Workarounds:

```c
/* 1. volatile (prevents optimization away — use sparingly) */
volatile int counter = 0;

/* 2. keep the symbol */
int counter __attribute__((used)) = 0;
```

- Compile the problematic file at lower optimization for the session
- Look at registers directly: `info registers`, `p/x $r0`

### 3. Reading inlined frames in GDB

```text
(gdb) bt
#0  process (data=..., len=<optimized out>) at network.c:45
#1  0x... in dispatch (pkt=...) at handler.c:102
#2  (inlined by) event_loop () at main.c:78
```

`(inlined by)` frames are virtual — they show the inlined call chain.
`break function_name` hits all inline expansions.

### 4. Line number discrepancies

Optimizers reorder instructions, so the "current line" may jump around:

```gdb
disassemble /s function_name    # interleaved source and asm
si / ni                         # instruction stepping is reliable
set disassemble-next-line on
```

## Better-Rockpod notes

- Rockbox builds with `-Os` everywhere (see CLAUDE.md CFLAGS) — expect
  heavy inlining and `<optimized out>` in both sim and hardware builds.
- For a debugging session on the simulator, the quick lever is editing
  `GCCOPTS` for the affected file or temporarily rebuilding the sim
  with `-O0 -g` — but remember timing-sensitive bugs may vanish at -O0.
- Hardware builds must stay `-Os`: binary size matters (IRAM/DRAM
  budgets), so debug locally on the sim instead of shipping -O0
  firmware.
