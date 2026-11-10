#!/usr/bin/env python3
"""Generate the Aero7 theme (Frutiger Aero / Windows 7 glass) for 320x240.

Writes themes/Aero7/.rockbox/... (backdrops, skin bitmaps, .sbs/.wps/.cfg)
and a preview sheet. Glass is pre-rendered: the wallpaper is blurred and
tinted under every panel, since Rockbox can't blur at runtime.

Usage: python3 tools/themes/make_aero7.py [--fonts DIR] [--preview PNG]
Needs Pillow. DIR holds the converted Open Sans .fnt files (see
themes/Aero7/README.md).
"""
import argparse
import math
import os
import random
import shutil
import struct

from PIL import Image, ImageChops, ImageDraw, ImageFilter, ImageFont

W, H = 320, 240
SS = 4                      # supersampling for antialiased shapes
NAME = "Aero7"
ROOT = os.path.join(os.path.dirname(__file__), "..", "..", "themes", NAME,
                    ".rockbox")

# Layout (shared by the generator and the skins)
BAR_H = 22                                  # top glass bar
WIN = (8, 28, 312, 234)                     # menu window (glass frame)
CONTENT = (13, 33, 307, 229)                # list area inside the window
LIST_X, LIST_Y, LIST_W = 17, 36, 286
ITEM_H = 26
SEL_H = 24
SEL_TOP, SEL_BOTTOM = (96, 182, 246, 255), (26, 104, 206, 255)
TEXT, SEL_TEXT = "ffffff", "ffffff"
CARD = (8, 28, 312, 172)                    # WPS glass card
ART = (18, 38, 130, 150)                    # 112x112 album art
PB = (22, 182, 276, 6)                      # visible progress track
KNOB = 10                                   # slider knob = bar height
PB_H = KNOB                                 # drawn bar height (track centred)
DOCK = (96, 207, 224, 237)                  # floating glass dock pill
DOCK_Y = DOCK[1]


def lerp(a, b, t):
    return tuple(int(round(a[i] + (b[i] - a[i]) * t)) for i in range(len(a)))


def big(w, h, color=(0, 0, 0, 0)):
    return Image.new("RGBA", (w * SS, h * SS), color)


def down(img, w, h):
    return img.resize((w, h), Image.LANCZOS)


def vgradient(w, h, top, bottom):
    g = Image.new("RGBA", (w, h))
    d = ImageDraw.Draw(g)
    for y in range(h):
        d.line([(0, y), (w, y)], fill=lerp(top, bottom, y / max(1, h - 1)))
    return g


def rounded_mask(w, h, r):
    m = Image.new("L", (w * SS, h * SS), 0)
    ImageDraw.Draw(m).rounded_rectangle([0, 0, w * SS - 1, h * SS - 1],
                                        r * SS, fill=255)
    return down(m, w, h)


def with_mask(img, mask):
    out = img.copy()
    out.putalpha(ImageChops.multiply(img.getchannel("A"), mask))
    return out


# ---------------------------------------------------------------- wallpaper
def wallpaper(phase=0.0):
    """Windows 7 inspired: deep blue sky, light ribbons, aero glow, bubbles.
    phase (0..1) drifts the ribbons and glows, for animated backdrops; the
    animation loops seamlessly from phase 1 back to 0."""
    tau = 2 * math.pi * phase
    # the sky slowly shifts hue: blue -> teal -> soft violet -> blue
    top = lerp((6, 40, 112, 255), (4, 70, 110, 255), (1 - math.cos(tau)) / 2)
    bottom = lerp((20, 116, 196, 255), (60, 96, 205, 255), (1 - math.cos(tau * 2)) / 4)
    bg = vgradient(W * SS, H * SS, top, bottom)
    # radial glow, upper left (the Win7 "light source")
    glow = big(W, H)
    gd = ImageDraw.Draw(glow)
    for i in range(40, 0, -1):
        r = i * 7 * SS
        a = int(5 + (40 - i) * 0.9)
        gx = 90 + 110 * math.sin(tau)
        gy = 50 + 35 * math.cos(tau)
        gd.ellipse([gx * SS - r, gy * SS - r, gx * SS + r, gy * SS + r],
                   fill=(120, 200, 255, a))
    bg.alpha_composite(glow.filter(ImageFilter.GaussianBlur(14 * SS)))
    # flowing light ribbons
    rib = big(W, H)
    rd = ImageDraw.Draw(rib)
    for k, (amp, off, col, wid) in enumerate([
            (30, 150, (170, 230, 255, 80), 16),
            (26, 168, (110, 205, 255, 60), 26),
            (36, 138, (255, 255, 255, 55), 5),
            (20, 184, (90, 245, 205, 55), 18)]):
        pts = []
        for x in range(-20, W + 21, 4):
            y = (off + amp * math.sin((x + k * 40) / 62.0 + tau * (1 + k % 2))
                 - x * 0.18)
            pts.append((x * SS, y * SS))
        rd.line(pts, fill=col, width=wid * SS, joint="curve")
    bg.alpha_composite(rib.filter(ImageFilter.GaussianBlur(8 * SS)))
    # Frutiger Aero green-teal glow at the bottom right
    aur = big(W, H)
    ax = 120 - 110 * math.sin(tau)
    ImageDraw.Draw(aur).ellipse([ax * SS, 175 * SS, (ax + 270) * SS, 330 * SS],
                                fill=(70, 230, 160, 110))
    bg.alpha_composite(aur.filter(ImageFilter.GaussianBlur(28 * SS)))
    # bubbles
    random.seed(7)
    bub = big(W, H)
    bd = ImageDraw.Draw(bub)
    for _ in range(7):
        r = random.randint(4, 12) * SS
        x = random.randint(10, W - 10) * SS
        y = random.randint(30, 160) * SS
        bd.ellipse([x - r, y - r, x + r, y + r], fill=(255, 255, 255, 12),
                   outline=(220, 245, 255, 70), width=SS)
        bd.ellipse([x - r * 0.6, y - r * 0.7, x - r * 0.1, y - r * 0.3],
                   fill=(255, 255, 255, 100))
    bg.alpha_composite(bub.filter(ImageFilter.GaussianBlur(SS * 0.6)))
    return down(bg, W, H)


# ------------------------------------------------------------------- glass
def glass(base, box, radius, tint, highlight=True, border=True, blur=6,
          shadow=False):
    """Bake a glass panel onto base: optional soft drop shadow, blur what's
    under it, tint it, add a gentle top sheen and hairline light rims."""
    x0, y0, x1, y1 = box
    w, h = x1 - x0, y1 - y0
    if shadow:
        sh = Image.new("RGBA", base.size)
        ImageDraw.Draw(sh).rounded_rectangle([x0, y0 + 3, x1 - 1, y1 + 2],
                                             radius, fill=(0, 12, 40, 110))
        base.alpha_composite(sh.filter(ImageFilter.GaussianBlur(5)))
    mask = rounded_mask(w, h, radius)
    under = base.crop(box).filter(ImageFilter.GaussianBlur(blur))
    panel = Image.alpha_composite(under, Image.new("RGBA", (w, h), tint))
    if highlight:
        sheen = vgradient(w, max(1, h * 2 // 5), (255, 255, 255, 60),
                          (255, 255, 255, 0))
        layer = Image.new("RGBA", (w, h))
        layer.paste(sheen, (0, 0))
        panel = Image.alpha_composite(panel, layer)
    base.paste(panel, (x0, y0), mask)
    if border:
        edge = big(w, h)
        ed = ImageDraw.Draw(edge)
        ed.rounded_rectangle([0, 0, w * SS - 1, h * SS - 1], radius * SS,
                             outline=(0, 20, 50, 70), width=SS)
        ed.rounded_rectangle([SS, SS, w * SS - 1 - SS, h * SS - 1 - SS],
                             max(0, radius - 1) * SS,
                             outline=(255, 255, 255, 95), width=SS)
        layer = Image.new("RGBA", base.size)
        layer.paste(down(edge, w, h), (x0, y0))
        base.alpha_composite(layer)


def top_bar(base):
    glass(base, (0, 0, W, BAR_H), 0, (8, 26, 58, 55), border=False, blur=10)
    line = Image.new("RGBA", base.size)
    ImageDraw.Draw(line).line([(0, BAR_H - 1), (W, BAR_H - 1)],
                              fill=(255, 255, 255, 45))
    base.alpha_composite(line)


def menu_backdrop(phase=0.0):
    base = wallpaper(phase)
    top_bar(base)
    # the list floats on dark tinted glass: the (animated) wallpaper shows
    # through, blurred just enough to keep white text readable
    glass(base, WIN, 12, (6, 28, 66, 96), shadow=True, blur=7)
    return base


def wps_backdrop(phase=0.0):
    base = wallpaper(phase)
    top_bar(base)
    glass(base, CARD, 12, (6, 30, 70, 90), shadow=True)
    # album art floats on a soft shadow with a hairline light rim
    x0, y0, x1, y1 = ART
    sh = Image.new("RGBA", base.size)
    ImageDraw.Draw(sh).rounded_rectangle([x0, y0 + 3, x1, y1 + 4], 6,
                                         fill=(0, 8, 28, 170))
    base.alpha_composite(sh.filter(ImageFilter.GaussianBlur(4)))
    rim = big(x1 - x0 + 2, y1 - y0 + 2)
    ImageDraw.Draw(rim).rounded_rectangle(
        [0, 0, (x1 - x0 + 2) * SS - 1, (y1 - y0 + 2) * SS - 1], 3 * SS,
        outline=(255, 255, 255, 150), width=SS)
    layer = Image.new("RGBA", base.size)
    layer.paste(down(rim, x1 - x0 + 2, y1 - y0 + 2), (x0 - 1, y0 - 1))
    base.alpha_composite(layer)
    # slim progress track
    x, y, w, h = PB
    track = Image.new("RGBA", (w, h), (0, 14, 40, 120))
    base.alpha_composite(with_mask(track, rounded_mask(w, h, h // 2)), (x, y))
    # floating glass dock pill
    glass(base, DOCK, (DOCK[3] - DOCK[1]) // 2, (8, 30, 66, 105), shadow=True)
    return base


# ----------------------------------------------------------------- sprites
def selection_bar():
    """Windows 7 Explorer selection: pale blue gradient, blue border,
    white inner highlight."""
    w, h = LIST_W, SEL_H
    g = vgradient(w, h, SEL_TOP, SEL_BOTTOM)
    g = with_mask(g, rounded_mask(w, h, 7))
    edge = big(w, h)
    ed = ImageDraw.Draw(edge)
    ed.rounded_rectangle([0, 0, w * SS - 1, h * SS - 1], 7 * SS,
                         outline=(150, 210, 255, 230), width=SS)
    ed.rounded_rectangle([SS, SS, w * SS - 1 - SS, h * SS - 1 - SS], 6 * SS,
                         outline=(255, 255, 255, 110), width=SS)
    return Image.alpha_composite(g, down(edge, w, h))


def no_art():
    """Placeholder when a track has no album art: glass tile with a note."""
    w, h = ART[2] - ART[0], ART[3] - ART[1]
    t = vgradient(w, h, (80, 160, 232, 255), (24, 84, 168, 255))
    corner = Image.new("RGBA", (w, h), (12, 46, 98, 255))
    corner.paste(t, (0, 0), rounded_mask(w, h, 3))
    t = corner
    d = ImageDraw.Draw(t)
    cx, cy = w // 2, h // 2
    d.ellipse([cx - 22, cy + 8, cx - 2, cy + 24], fill=(235, 248, 255, 255))
    d.rectangle([cx - 5, cy - 26, cx - 1, cy + 16], fill=(235, 248, 255, 255))
    d.polygon([(cx - 1, cy - 26), (cx + 22, cy - 16), (cx + 22, cy - 6),
               (cx - 1, cy - 16)], fill=(235, 248, 255, 255))
    return t


def progress_fill():
    """Fill image as tall as the knob, with the 6px pill centred in it."""
    x, y, w, h = PB
    g = vgradient(w, h, (130, 225, 255, 255), (20, 140, 235, 255))
    sheen = vgradient(w, max(1, h // 2), (255, 255, 255, 120), (255, 255, 255, 20))
    layer = Image.new("RGBA", (w, h))
    layer.paste(sheen, (0, 0))
    g = with_mask(Image.alpha_composite(g, layer), rounded_mask(w, h, h // 2))
    out = Image.new("RGBA", (w, PB_H))
    out.paste(g, (0, (PB_H - h) // 2))
    return out


def orb(size, symbol):
    """Windows Media Player 12 style glass orb with a play-state glyph."""
    o = big(size, size)
    d = ImageDraw.Draw(o)
    s = size * SS
    d.ellipse([0, 0, s - 1, s - 1], fill=(20, 80, 160, 255),
              outline=(190, 228, 255, 230), width=SS)
    body = vgradient(s - 4 * SS, s - 4 * SS, (70, 165, 240, 255),
                     (12, 66, 150, 255))
    o.paste(body, (2 * SS, 2 * SS), circle_mask(s - 4 * SS))
    hl = big(size, size)
    ImageDraw.Draw(hl).ellipse([s * 0.18, s * 0.06, s * 0.82, s * 0.48],
                               fill=(255, 255, 255, 110))
    o.alpha_composite(hl.filter(ImageFilter.GaussianBlur(SS)))
    c, u = s / 2, s / 9
    white = (255, 255, 255, 255)
    if symbol == "play":
        d.polygon([(c - u * 1.1, c - u * 1.6), (c - u * 1.1, c + u * 1.6),
                   (c + u * 1.7, c)], fill=white)
    elif symbol == "pause":
        d.rectangle([c - u * 1.4, c - u * 1.5, c - u * 0.45, c + u * 1.5], fill=white)
        d.rectangle([c + u * 0.45, c - u * 1.5, c + u * 1.4, c + u * 1.5], fill=white)
    elif symbol == "stop":
        d.rectangle([c - u * 1.3, c - u * 1.3, c + u * 1.3, c + u * 1.3], fill=white)
    elif symbol in ("ff", "rew"):
        k = 1 if symbol == "ff" else -1
        for dx in (-u * 1.2, u * 0.4):
            d.polygon([(c + k * dx - k * u * 0.4, c - u * 1.3),
                       (c + k * dx - k * u * 0.4, c + u * 1.3),
                       (c + k * dx + k * u * 1.2, c)], fill=white)
    return down(o, size, size)


def circle_mask(s):
    m = Image.new("L", (s, s), 0)
    ImageDraw.Draw(m).ellipse([0, 0, s - 1, s - 1], fill=255)
    return m


def glyph(size, kind, on):
    """Small dock glyphs: shuffle and repeat, dim when off."""
    g = big(size, size)
    d = ImageDraw.Draw(g)
    s = size * SS
    col = (255, 255, 255, 240) if on else (190, 215, 240, 110)
    w = int(1.6 * SS)
    if kind == "shuffle":
        d.line([(s * .1, s * .3), (s * .4, s * .3), (s * .65, s * .7), (s * .85, s * .7)], fill=col, width=w)
        d.line([(s * .1, s * .7), (s * .4, s * .7), (s * .65, s * .3), (s * .85, s * .3)], fill=col, width=w)
        for yy in (.3, .7):
            d.polygon([(s * .82, s * (yy - .12)), (s * .98, s * yy), (s * .82, s * (yy + .12))], fill=col)
    else:
        d.rounded_rectangle([s * .12, s * .3, s * .88, s * .72], s * .18, outline=col, width=w)
        d.polygon([(s * .5, s * .18), (s * .66, s * .3), (s * .5, s * .42)], fill=col)
    return down(g, size, size)


def battery_strip():
    """Frames: charging, plugged, then 8 charge levels (low -> high)."""
    fw, fh = 26, 14
    frames = []
    for i in range(10):
        f = big(fw, fh)
        d = ImageDraw.Draw(f)
        d.rounded_rectangle([0, 2 * SS, (fw - 4) * SS, (fh - 2) * SS], 3 * SS,
                            outline=(255, 255, 255, 200), width=SS)
        d.rounded_rectangle([(fw - 3) * SS, 5 * SS, (fw - 1) * SS, 9 * SS],
                            SS, fill=(255, 255, 255, 200))
        if i < 2:
            level, col = 1.0, (120, 230, 255, 255)
        else:
            level = (i - 1) / 8.0
            col = (240, 80, 60, 255) if level <= .25 else (120, 230, 110, 255)
        iw = int((fw - 8) * SS * level) or 1
        inner = vgradient(iw, (fh - 8) * SS,
                          lerp(col, (255, 255, 255, 255), .3), col)
        f.paste(inner, (2 * SS, 4 * SS),
                rounded_mask_raw(iw, (fh - 8) * SS, 2 * SS))
        if i == 0:
            d.polygon([(11 * SS, 2 * SS), (7 * SS, 8 * SS), (11 * SS, 8 * SS),
                       (9 * SS, 13 * SS), (15 * SS, 6 * SS), (11 * SS, 6 * SS)],
                      fill=(255, 255, 255, 255), outline=(0, 60, 120, 255))
        frames.append(down(f, fw, fh))
    strip = Image.new("RGBA", (fw, fh * len(frames)))
    for i, f in enumerate(frames):
        strip.paste(f, (0, i * fh))
    return strip


def rounded_mask_raw(w, h, r):
    m = Image.new("L", (w, h), 0)
    ImageDraw.Draw(m).rounded_rectangle([0, 0, w - 1, h - 1], r, fill=255)
    return m


def knob():
    """Small glass knob for the progress bar slider."""
    s = KNOB
    k = big(s, s)
    d = ImageDraw.Draw(k)
    d.ellipse([0, 0, s * SS - 1, s * SS - 1], fill=(240, 250, 255, 255),
              outline=(120, 180, 235, 255), width=SS)
    d.ellipse([s * SS * .22, s * SS * .1, s * SS * .78, s * SS * .45],
              fill=(255, 255, 255, 200))
    return down(k, s, s)


def strip(images):
    w, h = images[0].size
    s = Image.new("RGBA", (w, h * len(images)))
    for i, im in enumerate(images):
        s.paste(im, (0, i * h))
    return s


# ---------------------------------------------------------------- BMP out
def save_bmp(img, path, alpha):
    """Uncompressed BMP: 32-bit BGRA (alpha) or 24-bit BGR, bottom-up."""
    img = img.convert("RGBA")
    w, h = img.size
    bpp = 32 if alpha else 24
    row = (w * bpp // 8 + 3) & ~3
    px = img.load()
    data = bytearray()
    for y in range(h - 1, -1, -1):
        line = bytearray()
        for x in range(w):
            r, g, b, a = px[x, y]
            line += bytes((b, g, r, a)) if alpha else bytes((b, g, r))
        line += b"\0" * (row - len(line))
        data += line
    header = struct.pack("<2sIHHI", b"BM", 54 + len(data), 0, 0, 54)
    info = struct.pack("<IiiHHIIiiII", 40, w, h, 1, bpp, 0, len(data),
                       2835, 2835, 0, 0)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "wb") as f:
        f.write(header + info + data)


# ------------------------------------------------------------------ skins
CFG = """\
# Aero7 - Frutiger Aero / Windows 7 glass theme for Better-Rockpod (320x240)
# Generated by tools/themes/make_aero7.py
wps: /.rockbox/wps/{n}.wps
sbs: /.rockbox/wps/{n}.sbs
backdrop: /.rockbox/backdrops/{n}.bmp
font: /.rockbox/fonts/15-OpenSans-Regular.fnt
foreground color: ffffff
background color: 0c2c56
line selector start color: 60b6f6
line selector end color: 1a68ce
line selector text color: ffffff
selector type: bar (gradient)
show icons: off
scrollbar: off
statusbar: custom
ui viewport: -
# The theme has its own palette; album-art recoloring would fight the
# glass and the animated wallpaper.
dynamic colors: off
"""

SBS = """\
# Aero7 status bar and menus (generated by tools/themes/make_aero7.py)
%Fl(2,16-OpenSans-SemiBold.fnt)
%Fl(3,12-OpenSans-SemiBold.fnt)
%xl(Battery,battery.bmp,0,0,10)
%xl(Sel,selection.bmp,0,0)
%V(0,0,-,-,-)
#
# Menu list inside the glass window, Windows 7 Explorer selection
%Vi(-,{lx},{ly},{lw},{lh},1)
%Lb(Item,{lw},{ih})
%Vl(Item,0,1,{lw},{sh},-)%?Lc<%xd(Sel)>
%Vl(Item,8,{ty},{tw},{th},1)%Vf({tc})%Vg({g0},{g1},{stc})%?Lc<%Vs(gradient)>%LT
#
# Page title with a soft shadow, Aero style
%V(60,3,200,17,2)%Vf(062244)%ac%Lt
%V(60,2,200,17,2)%Vf(ffffff)%ac%Lt
#
# Clock
%V(6,4,52,14,3)%Vf(ffffff)%?cf<%cH:%cM|%cl:%cM>
#
# Hold
%V(254,4,30,14,3)%Vf(d8ecff)%?mh<HOLD>
#
# Battery
%V(288,4,26,14,-)
%?bp<%?bc<%xd(Battery,1)|%xd(Battery,2)>|%?bl<%xd(Battery,3)|%xd(Battery,3)|%xd(Battery,4)|%xd(Battery,5)|%xd(Battery,6)|%xd(Battery,7)|%xd(Battery,8)|%xd(Battery,9)|%xd(Battery,10)>>
"""

WPS = """\
# Aero7 Now Playing (generated by tools/themes/make_aero7.py)
%wd
%X(wps_backdrop.bmp)
%Fl(2,16-OpenSans-SemiBold.fnt)
%Fl(3,12-OpenSans-SemiBold.fnt)
%Fl(4,20-OpenSans-Bold.fnt)
%xl(Battery,battery.bmp,0,0,10)
%xl(NoArt,no_art.bmp,0,0)
%xl(Orb,orb.bmp,0,0,5)
%xl(Shuffle,shuffle.bmp,0,0,2)
%xl(Repeat,repeat.bmp,0,0,2)
%xl(Knob,knob.bmp,0,0)
#
# Top glass bar
%V(6,4,52,14,3)%Vf(ffffff)%?cf<%cH:%cM|%cl:%cM>
%V(60,3,200,17,2)%Vf(062244)%ac%Sx(Now Playing)
%V(60,2,200,17,2)%Vf(ffffff)%ac%Sx(Now Playing)
%V(288,4,26,14,-)
%?bp<%?bc<%xd(Battery,1)|%xd(Battery,2)>|%?bl<%xd(Battery,3)|%xd(Battery,3)|%xd(Battery,4)|%xd(Battery,5)|%xd(Battery,6)|%xd(Battery,7)|%xd(Battery,8)|%xd(Battery,9)|%xd(Battery,10)>>
#
# Album art with glass bezel, or the placeholder tile
%V({ax},{ay},{aw},{ah},-)
%Cl(0,0,{aw},{ah},c,c)
%?C<%Cd|%xd(NoArt)>
#
# Track info
%V(142,42,162,26,4)%Vf(ffffff)%s%?it<%it|%fn>
%V(142,68,162,20,2)%Vf(d6ebff)%s%?ia<%ia|%?iA<%iA|>>
%V(142,89,162,20,1)%Vf(a3c9f0)%s%?id<%id|>
%V(142,126,162,16,3)%Vf(8fbbe8)%pp %Sx(of) %pe
%V(142,143,162,16,3)%Vf(8fbbe8)%?Fn<Next: %?It<%It|%Fn>|>
#
# Progress
%V({kx},{ky},{kw},{kh},-)
%pb(0,0,{kw},{kh},progress.bmp,slider,Knob)
%V(20,190,280,14,3)%Vf(cfe6ff)%al%pc%ar-%pr
#
# Dock: shuffle, play state orb, repeat
%V({ox},{oy},28,28,-)%?mp<%xd(Orb,1)|%xd(Orb,2)|%xd(Orb,1)|%xd(Orb,4)|%xd(Orb,5)>
%V({sx},{gy},16,16,-)%?ps<%xd(Shuffle,2)|%xd(Shuffle,1)>
%V({rx},{gy},16,16,-)%?mm<%xd(Repeat,1)|%xd(Repeat,2)|%xd(Repeat,2)|%xd(Repeat,2)|%xd(Repeat,2)>
#
# Hold
%V(254,4,30,14,3)%Vf(d8ecff)%?mh<HOLD>
"""


def write_skins(out):
    lx, ly, lw = LIST_X, LIST_Y, LIST_W
    lh = (CONTENT[3] - 4) - ly
    # the selected text line repeats the selection gradient over its rows,
    # so the line's own background blends into the selection image
    ty, th = 4, 19
    top, bot = SEL_TOP, SEL_BOTTOM
    c0 = lerp(top, bot, (ty - 1) / (SEL_H - 1))
    c1 = lerp(top, bot, (ty - 1 + th - 1) / (SEL_H - 1))
    hexc = lambda c: "%02x%02x%02x" % c[:3]
    fmt = dict(n=NAME, lx=lx, ly=ly, lw=lw, lh=lh, ih=ITEM_H, sh=SEL_H,
               ty=ty, th=th, tw=lw - 16, g0=hexc(c0), g1=hexc(c1),
               tc=TEXT, stc=SEL_TEXT, ax=ART[0], ay=ART[1], aw=ART[2] - ART[0],
               ah=ART[3] - ART[1], px=PB[0], py=PB[1], pw=PB[2], ph=PB[3],
               kx=PB[0] - KNOB // 2, ky=PB[1] - (PB_H - PB[3]) // 2,
               kw=PB[2] + KNOB, kh=PB_H,
               ox=W // 2 - 14, oy=DOCK[1] + 1,
               sx=DOCK[0] + 14, rx=DOCK[2] - 30, gy=DOCK[1] + 7)
    files = {
        os.path.join(out, "themes", NAME + ".cfg"): CFG,
        os.path.join(out, "wps", NAME + ".sbs"): SBS,
        os.path.join(out, "wps", NAME + ".wps"): WPS,
    }
    for path, tmpl in files.items():
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, "w") as f:
            f.write(tmpl.format(**fmt))


def build(out, fonts_dir):
    imgdir = os.path.join(out, "wps", NAME)
    menu = menu_backdrop()
    wps = wps_backdrop()
    save_bmp(menu, os.path.join(out, "backdrops", NAME + ".bmp"), False)
    save_bmp(wps, os.path.join(imgdir, "wps_backdrop.bmp"), False)
    sprites = {
        "selection": selection_bar(),
        "no_art": no_art(),
        "progress": progress_fill(),
        "battery": battery_strip(),
        "orb": strip([orb(28, s) for s in ("play", "pause", "stop", "ff", "rew")]),
        "shuffle": strip([glyph(16, "shuffle", False), glyph(16, "shuffle", True)]),
        "repeat": strip([glyph(16, "repeat", False), glyph(16, "repeat", True)]),
        "knob": knob(),
    }
    for name, img in sprites.items():
        save_bmp(img, os.path.join(imgdir, name + ".bmp"), True)
    write_skins(out)
    if fonts_dir:
        fdst = os.path.join(out, "fonts")
        os.makedirs(fdst, exist_ok=True)
        for fn in os.listdir(fonts_dir):
            if fn.endswith(".fnt") and "OpenSans" in fn:
                shutil.copy(os.path.join(fonts_dir, fn), fdst)
    return menu, wps, sprites


def preview(menu, wps, sprites, path):
    """Static mockup of both screens using the generated assets."""
    font = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
    bold = "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"
    m = menu.copy()
    d = ImageDraw.Draw(m)
    items = ["Music", "Playlists", "Cover Flow", "Shuffle Songs", "Videos",
             "Settings", "System"]
    m.alpha_composite(sprites["selection"], (LIST_X, LIST_Y + ITEM_H + 1))
    for i, t in enumerate(items):
        d.text((LIST_X + 8, LIST_Y + i * ITEM_H + 5), t,
               font=ImageFont.truetype(font, 14), fill=(27, 61, 99))
    d.text((160, 4), "Main Menu", font=ImageFont.truetype(bold, 13),
           fill=(255, 255, 255), anchor="mt")
    d.text((6, 5), "11:42", font=ImageFont.truetype(bold, 10), fill=(255, 255, 255))
    m.alpha_composite(sprites["battery"].crop((0, 14 * 8, 26, 14 * 9)), (288, 4))
    p = wps.copy()
    d = ImageDraw.Draw(p)
    art = Image.new("RGBA", (112, 112))
    ad = ImageDraw.Draw(art)
    for y in range(112):
        ad.line([(0, y), (112, y)], fill=lerp((255, 140, 60, 255), (130, 40, 170, 255), y / 111))
    ad.ellipse([28, 28, 84, 84], fill=(255, 215, 95, 255))
    ad.ellipse([46, 46, 66, 66], fill=(40, 20, 60, 255))
    p.alpha_composite(art, ART[:2])
    d.text((142, 40), "Sunset Drive", font=ImageFont.truetype(bold, 17), fill=(255, 255, 255))
    d.text((142, 68), "Neon Coast", font=ImageFont.truetype(bold, 13), fill=(214, 235, 255))
    d.text((142, 90), "Midnight Radio", font=ImageFont.truetype(font, 13), fill=(169, 205, 242))
    d.text((142, 128), "3 of 12", font=ImageFont.truetype(bold, 10), fill=(169, 205, 242))
    d.text((142, 146), "Next: Afterglow", font=ImageFont.truetype(bold, 10), fill=(169, 205, 242))
    fill = sprites["progress"].crop((0, 0, int(PB[2] * .42), PB[3]))
    p.alpha_composite(fill, PB[:2])
    d.text((20, 196), "1:32", font=ImageFont.truetype(bold, 10), fill=(214, 235, 255))
    d.text((300, 196), "-2:11", font=ImageFont.truetype(bold, 10), fill=(214, 235, 255), anchor="ra")
    p.alpha_composite(sprites["orb"].crop((0, 32, 32, 64)), (144, DOCK_Y - 2))
    p.alpha_composite(sprites["shuffle"].crop((0, 18, 18, 36)), (110, DOCK_Y + 7))
    p.alpha_composite(sprites["repeat"].crop((0, 0, 18, 18)), (192, DOCK_Y + 7))
    d.text((160, 4), "Now Playing", font=ImageFont.truetype(bold, 13), fill=(255, 255, 255), anchor="mt")
    d.text((6, 5), "11:42", font=ImageFont.truetype(bold, 10), fill=(255, 255, 255))
    p.alpha_composite(sprites["battery"].crop((0, 14 * 8, 26, 14 * 9)), (288, 4))
    sheet = Image.new("RGB", (W * 4 + 30, H * 2 + 20), (24, 24, 28))
    sheet.paste(m.convert("RGB").resize((W * 2, H * 2), Image.NEAREST), (10, 10))
    sheet.paste(p.convert("RGB").resize((W * 2, H * 2), Image.NEAREST), (W * 2 + 20, 10))
    sheet.save(path)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--fonts", help="directory with Open Sans .fnt files")
    ap.add_argument("--preview", help="write a preview PNG here")
    ap.add_argument("--out", default=ROOT)
    a = ap.parse_args()
    menu, wps, sprites = build(a.out, a.fonts)
    if a.preview:
        preview(menu, wps, sprites, a.preview)


if __name__ == "__main__":
    main()
