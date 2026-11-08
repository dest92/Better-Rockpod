# On-device test checklist

These are the roadmap items that can only be closed on real hardware.
1. Generate the test files with `tools/devicetest/make_test_media.sh
   <iPod>/Music/RockpodTests`.
2. Install the CI build (`rockbox.zip` artifact for `ipod6g` or
   `ipodvideo`).
3. Update the database, then run each check.

Report a result as pass/fail plus the debug log (System → Debug →
"Dump logs" if enabled).

| # | Target | Item | Steps | Pass when |
|---|---|---|---|---|
| 1 | 6G/7G | Spec 0009 / roadmap 1.2 | Play `formats/`, skip forward and back 30+ times quickly | No silent track with a frozen timer |
| 2 | 6G/7G (SSD) | Roadmap 1.1 | Play `hires/` in a loop for 10 min, mixed with `formats/` | Every track starts and plays to the end |
| 3 | 6G | Upstream `94313491a` | Copy files over USB, eject, idle until the disk sleeps, reboot | No filesystem errors, files intact |
| 4 | 6G | Spec 0017 / roadmap 2.6 | Open `video/test_h264.mp4` | Video and audio in sync; pause/seek/exit work |
| 5 | 6G/5G | Spec 0010 | Default settings: in the WPS, hold PLAY to stop, then hold PLAY to power off | Stops, then powers off without entering PictureFlow |
| 6 | 5G (flash) | Spec 0018 / roadmap 2.4 | Storage Mode = auto, backlight off for 10 s | Disk activity stops shortly after the backlight goes off |
| 7 | 5G (stock HDD) | Spec 0018 | Storage Mode = auto, play an album | Mode stays HDD (the disk-optimized shuffle stays available), no skips |
| 8 | both | Spec 0015 / roadmap 1.3 | Dynamic colors on, skip between albums with different art, open menus during the fade | No mismatched colors or seams during the fade |
| 9 | 6G + PHA-1A | Roadmap 1.5 | Connect, play; capture the iAP debug log | Audio plays (otherwise attach the log) |

Remaining hardware-blocked work: PP5022-specific SSD power strategies
(roadmap 2.4). They need a 5G with a flash adapter to measure power
draw and to confirm the drive survives the new sequence.
