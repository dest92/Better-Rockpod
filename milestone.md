# Milestone: Roadmap kickoff — colors, shuffle, CI

- **Status:** In progress
- **Branch:** `claude/rockpod-features-roadmap-qg0tc3`
- **Roadmap:** [ROADMAP.md](ROADMAP.md)

## Scope

The first batch of items picked off [ROADMAP.md](ROADMAP.md) after it was
written, worked in this order:

| # | Item | Spec | Priority |
|---|------|------|----------|
| 1 | Dynamic colors: complementary accent for low-contrast art | [0003](specs/0003-dynamic-colors-complementary-accent.md) | P1 (roadmap 1.3) |
| 2 | Disk-locality-aware shuffle for HDD (flagship) | [0004](specs/0004-disk-locality-shuffle.md) | P1 (roadmap 2.1) |
| 3 | Repeat Shuffle anti-repeat | [0005](specs/0005-repeat-shuffle-antirepeat.md) | P1 (roadmap 2.2) |
| 4 | CI: GitHub Actions (host tests, simulator, hardware×2) | — (`.github/workflows/ci.yml`) | P1 (roadmap 4.3) |

Each feature followed the repo's spec-driven TDD workflow (`CLAUDE.md`,
`spec-dev` skill): a written spec agreed with the user, a failing unit
test, the smallest implementation to pass it, then end-to-end
verification.

## Definition of done

- [x] Each spec has a written design and is agreed before implementation.
- [x] Host-testable logic has unit tests in `tests/`, green under
      ASan/UBSan (`make -C tests`).
- [x] Each feature is exercised on the simulator, not just unit-tested
      (per the `verify` skill) — screenshots and/or scripted playback
      runs recorded in [progress.md](progress.md).
- [ ] CI (`ci.yml`) green across all four jobs: host tests, simulator
      build, hardware build ipod6g, hardware build ipodvideo.
- **Out of scope for this milestone:** on-device testing on real iPod
  hardware. No physical device is available in this working
  environment; hardware builds are verified by compiling in CI only.
  On-device listening tests (disk seek behavior, dynamic-colors
  readability with real album art) are called out as follow-ups for
  the user in each spec.

## Follow-ups opened, not part of this milestone

- roadmap 1.1/1.2 (hi-res FLAC / track-switch audio bugs) — need
  hardware reproduction.
- roadmap 2.3 (album shuffle), 2.4 (5G power management), 2.6 (H.264
  PoC) — separate specs.
- roadmap 3.x (porting Olsro's rockpod PRs) — separate specs.
- roadmap 4.1/4.2 (fork-point identification, upstream cherry-picks).

See [progress.md](progress.md) for the detailed, chronological log of
what was done and verified for each item above.
