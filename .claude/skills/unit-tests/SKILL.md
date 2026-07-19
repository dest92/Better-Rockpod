---
name: unit-tests
description: Host-side unit test harness for Better-Rockpod (tests/ directory). Use when adding, running, or debugging unit tests, or deciding whether a piece of rockbox code is host-testable. Activates on queries about unit tests, make -C tests, rbtest.h, or TDD in this repo.
---

# Unit Tests (tests/)

Rockbox has no upstream unit test framework; this fork adds a minimal
host-side harness under `tests/`. Each `test_<name>.c` is an
independent host executable built with **ASan + UBSan** (shift check
off — rockbox fixed-point shifts negatives by design).

## Running

```bash
make -C tests          # build + run all tests, nonzero exit on failure
make -C tests build    # compile only
make -C tests clean
```

Output per test: `ok: N checks passed` or per-failure lines with
file:line. Sanitizer errors abort the test (fail).

## Adding a test

1. Create `tests/test_<name>.c`:

```c
#include "rbtest.h"
#include "header_under_test.h"

TEST(case_name)
{
    CHECK(cond);
    CHECK_EQ(a, b);
    CHECK_NEAR(a, b, tol);   /* for fixed-point approximations */
    CHECK_STR_EQ(s1, s2);
}

int main(void)
{
    RUN_TEST(case_name);
    return rbtest_report();
}
```

2. Register it in `tests/Makefile`:

```make
TESTS := fixedpoint <name>

test_<name>_SRCS := $(ROOT)/path/to/unit.c      # sources under test
test_<name>_CFLAGS := -I$(ROOT)/path/to         # their include dirs
```

3. `make -C tests` — in TDD, watch it fail before implementing.

## What is host-testable

Only code that compiles with the host toolchain and NO firmware
headers (`config.h`, `system.h`, `plugin.h` pull in the whole target
world). Known-good candidates:

- `lib/fixedpoint/` (already covered by `test_fixedpoint.c`)
- Pure decision/parsing logic **extracted** into standalone helpers

The extraction pattern: when a feature's logic lives inside a plugin
or firmware file, move the decidable core into a small pure function
in its own header/file with no rockbox includes (stdint/stdbool only),
call it from the plugin, and unit-test the helper. Keep rockbox code
style (see CLAUDE.md).

NOT host-testable (verify on the simulator instead — see `verify`
skill): anything touching `rb->` plugin API, LCD, threads, tagcache,
buflib, drivers.

## Notes

- The framework is `tests/rbtest.h` (~100 lines, no dependencies).
  Extend it there if a new CHECK flavor is genuinely needed.
- Pre-existing upstream warnings from compiled rockbox sources (e.g.
  the `abs()` warning in fixedpoint.c) are visible but non-fatal;
  don't "fix" upstream code just to silence a harness warning.
- Test binaries land in `tests/bin/` (gitignored).
