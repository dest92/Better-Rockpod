# 0018: Storage Mode (HDD/SSD) on iPod Video 5G

- **Status:** Implemented (partial roadmap 2.4; on-device check pending)
- **Branch/PR:** `master` (roadmap 2.4, rockpod#23)

## Problem

Settings → General → Disk → Storage Mode (auto / HDD / SSD) appears on
the iPod Video, but the generic ATA driver used there
(`firmware/drivers/ata.c`) stubs it out: `ata_set_storage_mode()` does
nothing and `ata_get_ssd_mode()` always returns false. iFlash/CF/SD
upgrades on a 5G therefore never get the SSD behaviors that are shared
with the 6G:
- the minimal rebuffer watermark (`apps/playback.c`);
- sleeping the drive when the backlight turns off (`firmware/backlight.c`);
- disabling the disk-locality shuffle on flash (`apps/playlist.c`).

## Requirements

- **R1:** On drivers using `firmware/drivers/ata.c`, Storage Mode works:
  - auto resolves with `ata_disk_isssd()` from the IDENTIFY data;
  - HDD and SSD force the mode.
  - Auto detection runs at init, before settings load, and again
    whenever the setting is applied.
- **R2:** The board-level power sequence (spin-up/down, ATA power-off)
  is unchanged. The 6G's SSD clock gating and deep-LDO sleep use
  S5L8702 registers and are not ported.

## Non-goals and remaining work

The rest of roadmap 2.4 needs a 5G to verify: PP5022-specific SSD power
strategies (gating IDE power instead of STANDBY on flash, pre-wake on
backlight). Changing the power sequence blind risks filesystem
corruption (compare upstream `94313491a`).

## Verification

- `ipodvideo` and `ipod6g` build. The simulator has no ATA driver.
- **On device (5G, still to check):**
  1. With an iFlash/CF/SD adapter and Storage Mode = auto, check that
     the System → Debug disk info screen and the behavior match SSD:
     the drive sleeps shortly after the backlight goes off.
  2. With the stock HDD, auto must stay HDD.
  3. Playback with a full buffer must not skip in either mode.
