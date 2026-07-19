---
name: verify
description: End-to-end verification for Better-Rockpod changes — simulator run plus hardware compile checks. Use before declaring any feature or fix done, when asked to verify/test a change works, or as the final step of the spec-dev workflow. Activates on requests to verify, test end-to-end, or confirm a change works.
---

# Verify (Better-Rockpod)

"Done" means: unit tests green, feature exercised on the simulator,
and both hardware targets still compile. Tests passing alone is not
verification.

## 1. Unit tests

```bash
make -C tests
```

## 2. Simulator — exercise the actual feature

```bash
./build-sim.sh                     # first time: configure + build + install
cd build-sim && make -j$(nproc)    # incremental after that
cd build-sim && ./rockboxui        # run; simdisk/ is the virtual disk
```

- The sim is an ipod6g build (320x240). Put test media/files under
  `build-sim/simdisk/` before launching.
- Database-dependent features (PictureFlow, database browser) need the
  database initialized inside the sim first (Main menu → Database →
  Initialize Now), then a restart of the sim.
- `--help` on rockboxui lists options (e.g. `--zoom`). Keyboard maps to
  iPod controls (arrow keys = wheel/menu, see sim docs).
- For memory bugs, use a sanitizer sim build in a separate dir:
  `mkdir build-sim-asan && cd build-sim-asan && ../tools/configure
  --target=ipod6g --type=s --with-address-sanitizer --with-ubsan && make`.
- Headless/CI environment note: rockboxui needs SDL video; if no
  display is available use `SDL_VIDEODRIVER=dummy` (feature logic and
  logf still run; take screenshots only when a display exists).

State plainly WHAT was exercised (which screens, which files, which
settings) — not just "the sim runs".

## 3. Hardware builds — both targets must compile

```bash
./build-hw.sh          # iPod Classic 6G
./build-hw.sh 5g       # iPod Video 5G
```

Shared code compiles differently per target (different SoC defines,
`#ifdef` paths) — a change that builds for 6G can still break 5G.
Both must succeed. Cross toolchain missing? See the `cross-gcc` skill;
ask the user to install rather than skipping the check.

## 4. What cannot be verified here

Real-hardware behavior (audio output, disk spinup, battery, USB,
MFi) can only be tested by the user on device. Say so explicitly in
the report and list exactly what needs on-device confirmation.

## Report checklist

- [ ] `make -C tests` result
- [ ] Simulator: what was exercised and what was observed
- [ ] `build-hw.sh` (6G) and `build-hw.sh 5g` both build
- [ ] Remaining on-device items called out
