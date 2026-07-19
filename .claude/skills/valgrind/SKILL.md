---
name: valgrind
description: Valgrind profiler skill for memory error detection and cache profiling. Use when running Memcheck to find heap corruption, use-after-free, memory leaks, or uninitialised reads; or Cachegrind/Callgrind for cache simulation and function-level profiling. Activates on queries about valgrind, memcheck, heap leaks, use-after-free without sanitizers, cachegrind, callgrind, or massif memory profiling.
---

# Valgrind

## Purpose

Guide agents through Valgrind tools: Memcheck for memory errors, Cachegrind for cache simulation, Callgrind for call graphs, and Massif for heap profiling.

## Triggers

- "My program has a memory leak / use-after-free"
- "I can't use ASan — can I use Valgrind instead?"
- "How do I profile heap allocation patterns?"

## Workflow

### 1. Memcheck — memory error detection

Compile with `-g -O1` for best results; avoid `-O2`+ which can produce false positives.

```bash
valgrind --tool=memcheck \
         --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         --error-exitcode=1 \
         ./prog [args]
```

| Flag | Effect |
|------|--------|
| `--leak-check=full` | Full leak details |
| `--track-origins=yes` | Show where uninit values came from (slow) |
| `--error-exitcode=N` | Exit N if errors found (CI integration) |
| `--suppressions=file` | Suppress known false positives |
| `--gen-suppressions=yes` | Print suppression directives for errors |

### 2. Understanding Memcheck output

```text
==12345== Invalid read of size 4
==12345==    at 0x4007A2: foo (main.c:15)
==12345==  Address 0x5204040 is 0 bytes after a block of size 40 alloc'd
```

- **Invalid read/write**: out-of-bounds access
- **Use of uninitialised value**: read before write; use `--track-origins=yes`
- **Invalid free / double free**: mismatched malloc/free
- **Definitely lost**: clear leak; **still reachable**: never freed but not lost

### 3. Leak kinds

| Kind | Meaning |
|------|---------|
| Definitely lost | No pointer to block |
| Indirectly lost | Lost via another lost block |
| Possibly lost | Pointer into middle of block |
| Still reachable | Pointer exists at exit |

### 4. Cachegrind / Callgrind / Massif

```bash
valgrind --tool=cachegrind ./prog && cg_annotate cachegrind.out.*
valgrind --tool=callgrind ./prog && callgrind_annotate callgrind.out.*
valgrind --tool=massif ./prog && ms_print massif.out.*
```

Callgrind gives exact call counts without root; Massif shows heap usage
over time.

### 5. Performance considerations

Memcheck runs ~10-50x slower than native. Prefer ASan for day-to-day
development; reserve Valgrind for cases ASan can't cover (uninit-read
origin tracking, no-rebuild binaries).

## Better-Rockpod notes

- Applies to the **simulator** binary: `valgrind ./rockboxui` from
  `build-sim/`. The sim allocates its "audio buffer" once at startup —
  Rockbox's internal buflib allocations inside it are invisible to
  Valgrind (they're one big block), so buflib overruns are better
  caught by ASan sim builds or the `tests/` harness.
- Expect "still reachable" noise from SDL/X11; filter with
  suppressions, judge only definite losses in Rockbox code.
- Callgrind on the sim is useful for relative profiling of codecs/UI
  code paths, but remember host x86 timings don't map to ARM7/ARM9
  performance — for real performance use `test_codec`/`test_fps`
  plugins on hardware.
