---
name: low-power-embedded
description: Low-power embedded skill for sleep modes and energy optimization. Use when configuring CPU sleep/idle, peripheral clock gating, wake-up sources, or reducing firmware power draw. Activates on queries about sleep mode, clock gating, wake-up sources, battery runtime, or reducing embedded power consumption.
---

# Low-Power Embedded

## Purpose

Guide agents through MCU/SoC low-power techniques: sleep vs deep sleep, peripheral and bus clock gating, wake-up source configuration, and practical current measurement.

## When to Use

- Battery-powered firmware missing its power budget
- Wake-up latency vs consumption tradeoffs
- Debugging "device won't wake" or "current still high in sleep"
- Auditing idle loops and tickless sleep

## Workflow

### 1. Power mode hierarchy (generic)

| Mode | CPU | Peripherals | RAM | Wake source | Relative current |
|------|-----|-------------|-----|-------------|------------------|
| Run | on | on | on | — | highest |
| Idle/sleep | off | on | on | any IRQ | medium |
| Deep sleep | off | most off | on | selected sources | low |
| Off/hibernate | off | off | lost | RTC/buttons | lowest |

### 2. Enter sleep

Ensure pending interrupts are cleared/handled before sleeping or the
wake may be immediate; conversely, make sure at least one wake source
is armed or the device appears dead.

### 3. Clock gating checklist

```
Before sleep
├── Gate clocks of unused peripherals
├── Stop DMA channels and continuous conversions
├── Put external devices (codec, storage, LCD) in low-power states
└── Drop CPU/bus frequency if staying in run mode
```

### 4. Measurement tips

- Measure current in series with the battery; average over a real
  workload (e.g. one playback cycle), not an instant
- Compare before/after with the same clock configuration documented
- Watch for peripherals whose "off" still leaks (floating pins, pull-ups)

## Common Problems

| Symptom | Cause | Fix |
|---------|-------|-----|
| Still high current in sleep | A peripheral or debug link stays active | Gate clocks; audit enables |
| Immediate wake | Pending IRQ before sleep | Clear/handle flags first |
| Lost state after wake | Entered a deeper mode than intended | Use a RAM-retaining mode |
| Device won't wake | No wake source armed | Arm wake source before sleeping |

## Better-Rockpod notes

- Rockbox's idle path: `cpu_idle()` / sleep cores in
  `firmware/target/arm/pp/system-pp502x.c` (5G) and
  `firmware/target/arm/s5l8702/system-s5l8702.c` (Classic); frequency
  scaling via `set_cpu_frequency()` (`HAVE_ADJUSTABLE_CPU_FREQ`).
- Storage power is a big lever here: this fork has SSD power management
  for iPod Classic (`storage_mode` setting seen in pictureflow) and
  disk spindown logic in `firmware/storage.c` / ATA drivers.
- Backlight, codec (CS42L55/WM8758) power-down, and LCD sleep are the
  other main consumers — see `firmware/drivers/` for each.
- Battery benchmarking: `battery_bench` plugin measures runtime on
  hardware.
