# Aero7

Frutiger Aero / Windows 7 glass theme for Better-Rockpod, 320x240
(iPod Classic 6G/7G and iPod Video 5G/5.5G).

- **Wallpaper:** deep blue with light ribbons, an aqua glow and bubbles.
- **Glass:** a taskbar-style top bar, plus floating panels with soft
  shadows and hairline light rims. The menu list sits on transparent
  dark glass with white text, as do the Now Playing card and the dock.
  Each panel blurs and tints the wallpaper under it.
- **Menus:** an aqua selection pill with a light rim. Page titles have
  an Aero text shadow.
- **Now Playing:** album art floating on a soft shadow (or a glass
  placeholder tile when a track has none), a slim aqua progress bar
  with a glass knob, and a floating glass dock with a Windows Media
  Player style play/pause orb plus shuffle and repeat indicators.
- **Font:** Open Sans (SIL OFL 1.1, see `fonts/OpenSans-OFL.txt`) as an
  open stand-in for Segoe UI.

The theme turns off **dynamic colors**, because album-art recoloring
would fight the glass palette.

## Install

Copy `themes/Aero7/.rockbox/` onto the iPod's `.rockbox/` folder, then
pick **Settings → Theme Settings → Browse Theme Files → Aero7**.

## Rebuild

Glass can't be blurred at runtime on the iPod, so every panel is
pre-rendered by a script. To change colors or layout, edit
`tools/themes/make_aero7.py` and run:

```bash
# Open Sans .fnt files, converted with tools/convttf from the Open Sans
# TTFs (sizes 15 regular, 12/16 semibold, 20 bold):
python3 tools/themes/make_aero7.py --fonts DIR --preview /tmp/aero7.png
```

Check the skins with `checkwps` (configure type `C`) before shipping.
