# Milestone: Roadmap kickoff — colors, shuffle, CI

- **Status:** Complete ✅
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
- [x] CI (`ci.yml`) green across all four jobs: host tests, simulator
      build, hardware build ipod6g, hardware build ipodvideo. Confirmed
      green on run #3 (`a5a8a58`) — this also serves as the hardware
      compile check (A6/A7/A4) that specs 0003/0004/0005 had left to CI.
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

---

# Milestone 2: Roadmap completion (2026-11-08)

- **Status:** Complete for everything that can be done without hardware ✅
- **Branch:** `master`

| Roadmap | Item | Result |
|---|---|---|
| — | Sync with rockpod v5.5+ (30 commits) | merged `127678abff` |
| 1.3 | Dynamic colors: per-frame fade latch | spec 0015 |
| 1.4 | Fresh build on Linux | `nproc` fix, verified |
| 1.5 | PHA-1A USB audio | investigated; upstream causes ruled out; needs reporter logs |
| 2.3 | Shuffle Albums | spec 0013 |
| 2.4 | 5G Storage Mode | spec 0018 (PP5022 power strategies need a 5G) |
| 2.5 | Cover-art fetcher: MusicBrainz/CAA, resize, `cover.jpg` | spec 0014 |
| 2.6 | Hardware H.264 player (6G) | spec 0017, upstream port |
| 3.1 | WPS stop → PictureFlow only with Cover Flow | spec 0010 |
| 3.2 | Playlist viewer disc/track/title/album | spec 0011 |
| 3.3 | `[By album, random order]` | spec 0012 |
| 3.4 | Multiselection | spec 0016 |
| 3.5 | Blurred backdrop / art-first browsing | spec 0019 (idea) |
| 4.1 | Rockbox fork point | upstream `dd21a1d1d` |
| 4.2 | Upstream fixes, batch 1 | 39 commits, `docs/upstream-sync.md` |

Every implemented item has host unit tests where the logic allows,
simulator verification, and clean `ipod6g` and `ipodvideo` builds, with
CI green on GitHub.

## On-device checks still owed (no hardware in this environment)

- 6G/7G: format-switch hang (spec 0009), hi-res FLAC (1.1), drive
  power-down after flush (upstream `94313491a`), H.264 playback (spec
  0017), long-press PLAY then power-off (spec 0010).
- 5G: Storage Mode auto-detection with flash and with HDD (spec 0018),
  click-wheel recovery (upstream `437737913`).
- Both: dynamic-colors fades look seamless (spec 0015).
