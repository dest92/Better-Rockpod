---
name: spec-dev
description: Spec-driven development workflow for Better-Rockpod. Use when starting ANY new feature, behavior change, or non-trivial fix in this repo — before writing implementation code. Activates on requests to implement, add, change, or port functionality. Not needed for typo fixes, comment/doc edits, or build-script tweaks.
---

# Spec-Driven Development (Better-Rockpod)

Every feature starts with a spec, is driven by tests where the code
allows it, and ends verified on the simulator (and hardware builds).
No implementation code before the spec exists and the user has agreed
to it.

## Workflow

### 1. Write the spec first

Create `specs/NNNN-short-name.md` (next free number, see existing
files) from `specs/TEMPLATE.md`. A spec must fit on ~1-2 screens and
answer:

- **Problem** — what's wrong or missing today, from the user's point
  of view, with the code paths involved (`file:line` references).
- **Requirements** — numbered, testable statements ("R1: PictureFlow
  shows embedded JPEG art when no cover file exists").
- **Non-goals** — what is deliberately out of scope.
- **Design sketch** — the intended mechanism, reusing existing
  functions/utilities (name them with paths).
- **Acceptance criteria** — observable checks that decide "done".
- **Test plan** — which criteria get host unit tests (`tests/`),
  which get simulator verification, which need hardware.

Present the spec to the user for agreement before implementing.
Committing the spec together with the implementation is fine; writing
implementation before the spec is not.

### 2. Red — write the test before the code

For every requirement that is host-testable, add a failing unit test
in `tests/` FIRST and run `make -C tests` to watch it fail (see the
`unit-tests` skill for how to add one, and for what counts as
host-testable).

Rockbox reality check: most firmware/UI code cannot run on the host.
Make logic testable by extracting decision logic into small pure
functions (a `static inline` helper in a header, or a standalone .c
with no firmware includes) so the unit test exercises exactly the
logic the feature depends on. What cannot be unit-tested moves to the
simulator column of the test plan — it still needs a written
verification step, not hope.

### 3. Green — implement

Implement the smallest change that makes the tests pass, following
the codebase rules in CLAUDE.md (C99/gnu99, 4-space indent, `/* */`
comments, existing file style). Re-run `make -C tests` until green.

### 4. Verify end-to-end

Follow the `verify` skill: build the simulator, exercise the feature
(and the surrounding flows) there; then build both hardware targets
to prove they still compile. Record what was actually verified in the
final report — tests passing is not the same as the feature working.

### 5. Close the loop

- Check every acceptance criterion in the spec; mark unmet ones
  explicitly rather than silently dropping them.
- Update the spec's Status line (`Draft` → `Implemented`).
- Commit spec + tests + implementation on the working branch.

## Anti-rationalization table

| Temptation | Reality |
|------------|---------|
| "It's a small change, no spec needed" | If it changes behavior, spec it. Small spec is fine — skipping isn't. |
| "This can't be unit tested" | Extract the decision logic into a pure function; test that. Only I/O and rendering are exempt. |
| "Tests pass, ship it" | Unit tests don't exercise the plugin/firmware integration — run the simulator step. |
| "I'll write the test after" | Then the test proves nothing about the requirement. Red first. |
| "The sim build is slow" | Incremental rebuilds (`cd build-sim && make`) are fast after the first build. |

## Related skills

- `unit-tests` — the `tests/` harness mechanics
- `verify` — simulator + hardware verification steps
