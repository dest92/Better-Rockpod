# Agent Skills

Skills loaded on demand by Claude Code when working in this repo.

## Project skills (written for Better-Rockpod)

- `spec-dev` — spec-driven development workflow (spec first, then TDD, then verify)
- `unit-tests` — host-side unit test harness in `tests/`
- `verify` — how to verify changes end-to-end (simulator + hardware builds)

## Imported skills

The following skills are imported from
[mohitmishra786/low-level-dev-skills](https://github.com/mohitmishra786/low-level-dev-skills)
(https://www.lowleveldevskills.com), MIT licensed — see
`LICENSE.low-level-dev-skills`. Each one carries a trailing
"Better-Rockpod notes" section mapping it to this codebase; their
Cortex-M/STM32 examples do NOT apply verbatim to the iPod targets
(ARM7TDMI/ARM926EJ-S — PP5022 and S5L8702 SoCs).

- `mmio-and-bit-manipulation`, `interrupts-and-exceptions-baremetal`,
  `baremetal-startup`, `bootloaders-embedded`, `dma-baremetal`,
  `low-power-embedded` — bare-metal driver/firmware work
- `linker-scripts` — `.lds` files under `firmware/target/`
- `cross-gcc` — cross toolchains (`tools/rockboxdev.sh`)
- `gdb`, `debug-optimized-builds` — debugging (mainly the simulator)
- `sanitizers`, `valgrind` — memory checking on simulator builds
