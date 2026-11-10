# 0020: Boost the CPU while the user navigates

- **Status:** Implemented (on-device check pending)
- **Branch/PR:** `master` (research/ui-smoothness.md, proposal 1)

## Problem

The UI runs at the lowest CPU clock: 54 MHz on the 6G (of 216) and
30 MHz on the 5G (of 80). Nothing calls `cpu_boost()` while the user
scrolls lists, opens menus or redraws skins. Lists lag behind the
wheel, and entering a menu (loading a directory or database view,
drawing a themed screen) is slower than it needs to be.

## Requirements

- **R1:** Any button or wheel event received by the action system
  (`apps/action.c`) boosts the CPU if it isn't already boosted by this
  feature, and sets the deadline to 1 s after that event.
- **R2:** The boost covers the work triggered by the event, such as
  entering a menu and loading its contents, since that runs before the
  UI thread polls buttons again.
- **R3:** The boost is dropped once the deadline passes and the UI is
  waiting for input:
  - A blocking wait is split at the deadline; the CPU is unboosted and
    the wait continues, so callers see unchanged timeout semantics.
  - A finite timeout keeps its total length.
- **R4:** New setting "Boost While Navigating" (Settings → General
  Settings → System), default on. When off, nothing changes.
- **R5:** Only the UI thread calls `cpu_boost()` (never the tick ISR).
  The feature holds at most one boost reference.

## Design

- Pure helper `apps/ui_boost.h`:
  - `ui_boost_wait(timeout, boosted, left)`: given the caller's timeout
    (`TIMEOUT_BLOCK` = -1, `TIMEOUT_NOBLOCK` = 0, or ticks) and the
    boost state, it returns how long to wait before re-checking the
    boost.
  - `ui_boost_get_button()`: the whole wait/boost/release loop over
    injected button-queue, clock and boost functions, so it is
    host-tested.
- `action_poll_button()` loops:
  1. wait for the bounded time;
  2. on expiry, unboost;
  3. continue with the remaining caller timeout.
  A real button event kicks the boost.

## Acceptance criteria

- **A1:** Unit tests for `ui_boost_wait`: blocking, no-block, finite
  shorter/longer than the boost, and expired boost.
- **A2:** Simulator: navigation, playback and the WPS behave as before,
  with no hang waiting for input and the boost counter back at 0 after
  idling.
- **A3:** Both targets build.
- **On device:** perceived smoothness of scrolling and of entering
  menus, and battery drain over a browsing session with the setting on
  and off.

## Verification results

- **A1:** `tests/test_ui_boost.c`, 34 checks, green under ASan/UBSan.
  A fake clock and button queue drive the real loop. The tests check:
  - input boosts once;
  - a blocking wait drops the boost after 1 s idle and keeps waiting;
  - a finite timeout keeps its full length;
  - no-block returns at once;
  - system events don't boost;
  - turning the setting off releases the boost;
  - the boost count always balances.
- **A2:** The simulator has no adjustable CPU clock (`config/sim.h`
  undefines `HAVE_ADJUSTABLE_CPU_FREQ`), so this path compiles out
  there; sim behavior is unchanged.
- **A3:** `ipod6g` and `ipodvideo` build and zip. On the 6G `cpu_boost`
  is a logging macro, hence the `ui_boost_cpu()` wrapper.
- **On device:** check Settings → General Settings → System → Boost
  While Navigating. With it on, list scrolling and menu entry should
  feel quicker, and Debug → CPU frequency should show `boost_counter`
  returning to 0 about 1 s after the last input.
