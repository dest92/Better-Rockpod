---
name: gdb
description: GDB debugger skill for C/C++ programs. Use when starting a GDB session, setting breakpoints, stepping through code, inspecting variables, debugging crashes, remote debugging with gdbserver, or loading core dumps. Activates on queries about GDB commands, segfaults, hangs, watchpoints, conditional breakpoints, or multi-threaded debugging.
---

# GDB

## Purpose

Walk agents through GDB sessions from first launch to advanced workflows: crash diagnosis, remote debugging, and multi-thread inspection.

## Triggers

- "My program segfaults / crashes — how do I debug it?"
- "How do I set a breakpoint on condition X?"
- "How do I inspect memory / variables in GDB?"
- "GDB shows `??` frames / no source"

## Workflow

### 1. Prerequisite: compile with debug info

Always compile with `-g` (GCC/Clang). Use `-Og` or `-O0` for most debuggable code.

```bash
gcc -g -Og -o prog main.c
```

### 2. Start GDB

```bash
gdb ./prog                          # load binary
gdb ./prog core                     # load with core dump
gdb -p 12345                        # attach to running process
gdb --args ./prog arg1 arg2         # pass arguments
gdb -batch -ex 'run' -ex 'bt' ./prog  # non-interactive (CI/agent use)
```

### 3. Essential commands

| Command | Shortcut | Effect |
|---------|----------|--------|
| `run [args]` | `r` | Start the program |
| `continue` | `c` | Resume after break |
| `next` | `n` | Step over (source line) |
| `step` | `s` | Step into |
| `nexti` / `stepi` | `ni` / `si` | Instruction-level stepping |
| `finish` | | Run to end of current function |
| `quit` | `q` | Exit GDB |

### 4. Breakpoints and watchpoints

```gdb
break main                          # break at function
break file.c:42                     # break at line
break foo if x > 10                 # conditional break
tbreak foo                          # temporary breakpoint (fires once)

watch x                             # watchpoint: break when x changes
watch *(int*)0x601060               # watch memory address
rwatch x / awatch x                 # break on read / read-or-write

info breakpoints
delete 3 / disable 3 / enable 3
```

### 5. Inspect state

```gdb
print x                             # print variable
print/x x                           # print in hex
print *ptr                          # dereference pointer
print arr[0]@10                     # print 10 elements of array
display x                           # auto-print x on every stop

info locals / info args / info registers
x/10wx 0x7fff0000                   # examine 10 words at address
x/s addr                            # examine as string
x/i $pc                             # examine current instruction

backtrace / bt full                 # call stack
frame 2 / up / down                 # navigate frames
```

### 6. Multi-thread debugging

```gdb
info threads
thread 3
thread apply all bt
set scheduler-locking on            # pause other threads while stepping
```

### 7. Remote debugging with gdbserver

```bash
# On target
gdbserver :1234 ./prog
# On host
gdb ./prog -ex "target remote HOST:1234"
```

### 8. Common problems

| Symptom | Cause | Fix |
|---------|-------|-----|
| `No symbol table` | Binary not compiled with `-g` | Recompile with `-g` |
| `??` frames in backtrace | Missing debug info or stack corruption | Add symbols; check for stack smash |
| `Cannot access memory` | Null deref / freed memory | Use ASan; check pointers |
| GDB hangs on `run` | Binary waiting for input | `run < /dev/null` |
| Breakpoint in wrong place | Optimizer moved code | Compile with `-Og`; or step with `ni` |

### 9. GDB init file (~/.gdbinit)

```gdb
set history save on
set print pretty on
set pagination off
set confirm off
```

## Better-Rockpod notes

- GDB applies to the **simulator** build (`build-sim/rockboxui`, a host
  SDL binary): `gdb --args ./rockboxui`. Non-interactive agent use:
  `gdb -batch -ex run -ex bt --args ./rockboxui`.
- The simulator's virtual filesystem is `simdisk/`; crashes in plugin
  code appear under their real source paths (e.g.
  `apps/plugins/pictureflow/pictureflow.c`).
- Rockbox sim threads are real host threads — `thread apply all bt`
  works.
- There is no practical GDB path to the iPod hardware targets (no JTAG
  in the standard setup); hardware diagnosis relies on the sim, logf,
  and panic screens.
