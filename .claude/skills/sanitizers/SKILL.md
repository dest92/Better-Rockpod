---
name: sanitizers
description: Compiler sanitizer skill for runtime bug detection in C/C++. Use when enabling and interpreting AddressSanitizer (ASan), UndefinedBehaviorSanitizer (UBSan), ThreadSanitizer (TSan), or LeakSanitizer (LSan) with GCC or Clang. Activates on queries about sanitizer flags, sanitizer reports, ASAN_OPTIONS, memory errors, data races, undefined behaviour, or choosing which sanitizer to use for a given bug class.
---

# Sanitizers

## Purpose

Guide agents through choosing, enabling, and interpreting compiler runtime sanitizers for finding memory errors, undefined behaviour, data races, and memory leaks.

## Triggers

- "My program has a memory error — which sanitizer do I use?"
- "How do I enable ASan?"
- "ASan says heap-buffer-overflow — what does that mean?"
- "How do I suppress false positives in sanitizers?"

## Workflow

### 1. Decision tree: which sanitizer?

```
Bug class?
├── Memory OOB, use-after-free, double-free → AddressSanitizer (ASan)
├── Stack OOB, global OOB → ASan (all three covered)
├── Undefined behaviour (int overflow, null deref, bad shift) → UBSan
├── Data races (multi-thread) → ThreadSanitizer (TSan)
├── Memory leaks only → LeakSanitizer (LSan, standalone or via ASan)
└── Multiple classes → ASan + UBSan (common combo); cannot combine with TSan
```

### 2. AddressSanitizer (ASan)

```bash
gcc -fsanitize=address -fno-omit-frame-pointer -g -O1 -o prog main.c
```

Runtime options (via `ASAN_OPTIONS`):

```bash
ASAN_OPTIONS=detect_leaks=1:abort_on_error=1:log_path=/tmp/asan.log ./prog
```

| `ASAN_OPTIONS` key | Effect |
|--------------------|--------|
| `detect_leaks=0/1` | Enable LeakSanitizer (default 1 on Linux) |
| `abort_on_error=1` | `abort()` instead of `_exit()` (for core dumps) |
| `log_path=path` | Write report to file |
| `fast_unwind_on_malloc=0` | More accurate stacks (slower) |
| `quarantine_size_mb=256` | Delay reuse of freed memory |

**Interpreting ASan output:**

```text
==12345==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x602000000050
READ of size 4 at 0x602000000050 thread T0
    #0 0x401234 in foo /home/user/src/main.c:15
0x602000000050 is located 0 bytes after a 40-byte region
```

The top frame in `READ/WRITE` is the access site; the `allocated at`
stack shows the allocation. "0 bytes after a 40-byte region" = classic
off-by-one past the end.

### 3. UndefinedBehaviorSanitizer (UBSan)

```bash
gcc -fsanitize=undefined -g -O1 -o prog main.c
# Abort on first error (important for CI):
gcc -fsanitize=undefined -fno-sanitize-recover=all -g -O1 -o prog main.c
```

Common checks: `signed-integer-overflow`, `null`, `bounds`, `alignment`,
`shift-exponent`, `float-cast-overflow`.

```text
src/main.c:15:12: runtime error: signed integer overflow: 2147483647 + 1
```

### 4. ThreadSanitizer (TSan)

```bash
clang -fsanitize=thread -g -O1 -o prog main.c
# Incompatible with ASan
```

### 5. ASan + UBSan combined

```bash
gcc -fsanitize=address,undefined -fno-sanitize-recover=all \
    -fno-omit-frame-pointer -g -O1 -o prog main.c
```

### 6. Suppressions

```bash
cat > asan.supp << 'EOF'
leak:SDL_
EOF
LSAN_OPTIONS=suppressions=asan.supp ./prog

cat > ubsan.supp << 'EOF'
signed-integer-overflow:third_party/fast_math.c
EOF
UBSAN_OPTIONS=suppressions=ubsan.supp:print_stacktrace=1 ./prog
```

## Better-Rockpod notes

- The Rockbox **simulator** supports sanitizers natively via configure
  flags: `../tools/configure --target=ipod6g --type=s
  --with-address-sanitizer --with-ubsan` (see CLAUDE.md). Rebuild in a
  separate build dir to keep a clean sim around.
- The `tests/` host harness builds with ASan+UBSan by default (see the
  `unit-tests` skill) — sanitizer findings there fail the test run.
- Expect some noise from SDL internals in the sim; suppress with
  `LSAN_OPTIONS=suppressions=...` rather than turning leak detection
  off.
- Sanitizers are host-only: they never apply to `arm-elf-eabi` firmware
  builds. Anything you want sanitized must run on the sim or in
  `tests/`.
