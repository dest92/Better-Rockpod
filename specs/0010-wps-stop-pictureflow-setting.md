# 0010: WPS stop only opens PictureFlow when "WPS select action" is Cover Flow

- **Status:** Draft
- **Branch/PR:** `master` (roadmap 3.1, port of rockpod#5 by Paul Sauro / Olsro)

## Problem

In the WPS, `ACTION_WPS_STOP` (long-press PLAY/PAUSE on iPods) always
opens PictureFlow instead of stopping playback
(`apps/gui/wps.c:1052-1066`). The original stop behavior is disabled
under `#if 0`. As a result:

- Users who never chose Cover Flow cannot stop playback from the WPS.
- Long-pressing PLAY/PAUSE to stop and then power off ends up in
  PictureFlow, which may start (re)building its album database instead
  of letting the player shut down (reported in rockpod#5).

The fork already has a `wps_select_action` setting
(`apps/settings_list.c:2165`: default / database / coverflow / files)
that controls what the WPS SELECT/browse action opens. When it is set
to `coverflow` (2), the user has explicitly opted in to PictureFlow.

## Requirements

- **R1:** When `wps_select_action == 2` (coverflow), `ACTION_WPS_STOP`
  keeps today's behavior: it opens PictureFlow and returns to the WPS
  afterward (or goes back through `GO_TO_WPS` if playback stopped
  inside PictureFlow).
- **R2:** For any other value, `ACTION_WPS_STOP` restores upstream
  Rockbox behavior: it creates the auto-bookmark (`bookmark = true`) and
  stops/exits the WPS (`exit = true`).
- **R3:** The dead `#if 0` block with the original stop behavior is
  removed, because R2 now covers it.

## Non-goals

- No new setting and no language strings; this reuses
  `wps_select_action`.
- No change to `ACTION_WPS_BROWSE` (the SELECT path) or to the
  `ACTION_WPS_STOP` case in the seek loop (`wps.c:647`).

## Design sketch

Apply the upstream rockpod#5 diff to the `ACTION_WPS_STOP` case in
`gui_wps_show()`: branch on `global_settings.wps_select_action == 2`,
keeping the existing PictureFlow block in the true branch and
`bookmark = true; exit = true;` in the else branch. Credit the original
author in the commit.

## Acceptance criteria

- **A1:** With the default setting, long-pressing PLAY/PAUSE in the WPS
  stops playback and leaves the WPS (back to the menu/browser).
- **A2:** With "WPS select action" set to Cover Flow, the same action
  opens PictureFlow, and exiting PictureFlow returns to the WPS.
- **A3:** Both hardware targets (`ipod6g`, `ipodvideo`) build cleanly.

## Test plan

| Criterion | How verified |
|-----------|--------------|
| A1, A2 | simulator: play a track, send the stop action with each setting value, check the resulting screen (screendump) |
| A3 | `./build-hw.sh`, `./build-hw.sh 5g` |

This is UI control flow inside `gui_wps_show()` with no extractable pure
logic, so it has no host unit test (the same as other WPS action
handling).
