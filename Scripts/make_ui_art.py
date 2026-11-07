# Draws the cooking-UI frame art with PIL (run with system python).
#   python make_ui_art.py <out_dir>
# Panel = deep-sea menu card (navy gradient, light rays, bubbles, gold double frame, shell corners),
# chips / button = rounded pills, ticket = paper order slip with a torn edge.
import sys, os, math, random
from PIL import Image, ImageDraw, ImageFilter

OUT = sys.argv[1]
os.makedirs(OUT, exist_ok=True)
rng = random.Random(5)
GOLD = (232, 190, 102)
GOLD_D = (150, 110, 50)


def rounded_mask(w, h, r):
    m = Image.new("L", (w, h), 0)
    ImageDraw.Draw(m).rounded_rectangle((0, 0, w - 1, h - 1), r, fill=255)
    return m


def vgrad(w, h, top, bot):
    im = Image.new("RGBA", (w, h))
    d = ImageDraw.Draw(im)
    for y in range(h):
        t = y / max(h - 1, 1)
        d.line([(0, y), (w, y)], fill=tuple(int(top[i] + (bot[i] - top[i]) * t) for i in range(3)) + (255,))
    return im


def shell(d, cx, cy, s, col):
    # scallop: fan of ribs + base
    for k in range(7):
        a = math.radians(200 + k * 20)
        d.line([(cx, cy), (cx + math.cos(a) * s, cy + math.sin(a) * s)], fill=col, width=max(2, int(s / 9)))
    d.arc((cx - s, cy - s, cx + s, cy + s), 195, 345, fill=col, width=max(2, int(s / 8)))
    d.polygon([(cx - s * 0.25, cy), (cx + s * 0.25, cy), (cx + s * 0.15, cy + s * 0.25), (cx - s * 0.15, cy + s * 0.25)], fill=col)


def panel(w=2000, h=1120, r=46):
    im = vgrad(w, h, (16, 44, 72), (6, 16, 32))
    # light rays from the top
    rays = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    d = ImageDraw.Draw(rays)
    for k in range(7):
        x = rng.uniform(0.05, 0.95) * w
        wd = rng.uniform(60, 160)
        d.polygon([(x - wd * 0.3, 0), (x + wd * 0.3, 0), (x + wd * 1.6, h), (x - wd * 0.2, h)], fill=(120, 210, 255, 16))
    rays = rays.filter(ImageFilter.GaussianBlur(30))
    im.alpha_composite(rays)
    # soft vignette
    vig = Image.new("L", (w, h), 0)
    ImageDraw.Draw(vig).rounded_rectangle((60, 60, w - 60, h - 60), 120, fill=255)
    vig = vig.filter(ImageFilter.GaussianBlur(80))
    dark = Image.new("RGBA", (w, h), (2, 6, 14, 255))
    im = Image.composite(im, dark, vig)
    # bubbles
    d = ImageDraw.Draw(im)
    for _ in range(70):
        x, y = rng.uniform(0, w), rng.uniform(0, h)
        rr = rng.uniform(3, 14)
        d.ellipse((x - rr, y - rr, x + rr, y + rr), outline=(150, 220, 255, rng.randint(25, 60)), width=2)
    # wave lines near the bottom
    for k in range(3):
        pts = [(x, h - 70 - k * 22 + 8 * math.sin(x / 70 + k)) for x in range(0, w, 8)]
        d.line(pts, fill=(90, 170, 220, 30), width=3)
    # gold double frame
    d.rounded_rectangle((14, 14, w - 15, h - 15), r, outline=GOLD, width=6)
    d.rounded_rectangle((32, 32, w - 33, h - 33), r - 16, outline=GOLD_D, width=2)
    # header divider with a small diamond
    yv = 150
    d.line([(70, yv), (w / 2 - 40, yv)], fill=(232, 190, 102, 120), width=2)
    d.line([(w / 2 + 40, yv), (w - 70, yv)], fill=(232, 190, 102, 120), width=2)
    d.polygon([(w / 2, yv - 12), (w / 2 + 12, yv), (w / 2, yv + 12), (w / 2 - 12, yv)], fill=GOLD)
    # shell ornaments on the corners
    for (cx, cy) in ((70, 70), (w - 70, 70), (70, h - 70), (w - 70, h - 70)):
        d.ellipse((cx - 30, cy - 30, cx + 30, cy + 30), fill=(10, 26, 46, 255), outline=GOLD, width=4)
        shell(d, cx, cy + 8, 20, GOLD)
    im.putalpha(Image.composite(Image.new("L", (w, h), 246), Image.new("L", (w, h), 0), rounded_mask(w, h, r)))
    im.save(os.path.join(OUT, "UI_Panel.png"))


def pill(name, w, h, top, bot, border, glow=None):
    im = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    body = vgrad(w, h, top, bot)
    m = rounded_mask(w, h, h // 2)
    im.paste(body, (0, 0), m)
    d = ImageDraw.Draw(im)
    d.rounded_rectangle((2, 2, w - 3, h - 3), h // 2 - 2, outline=border, width=4)
    # top gloss
    gl = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    ImageDraw.Draw(gl).rounded_rectangle((10, 6, w - 10, h // 2), h // 4, fill=(255, 255, 255, 38))
    im.alpha_composite(gl)
    if glow:
        g = Image.new("RGBA", (w + 40, h + 40), (0, 0, 0, 0))
        ImageDraw.Draw(g).rounded_rectangle((20, 20, w + 19, h + 19), h // 2, outline=glow, width=10)
        g = g.filter(ImageFilter.GaussianBlur(8))
        g.alpha_composite(im, (20, 20))
        im = g
    im.save(os.path.join(OUT, name + ".png"))


def ticket(w=420, h=620):
    im = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    paper = vgrad(w, h, (250, 244, 228), (236, 226, 204))
    m = Image.new("L", (w, h), 0)
    d = ImageDraw.Draw(m)
    # torn bottom edge
    pts = [(0, 0), (w, 0)]
    x = w
    while x > 0:
        pts.append((x, h - 18 + rng.uniform(-8, 8)))
        x -= rng.uniform(10, 22)
    pts.append((0, h - 18))
    d.polygon(pts, fill=255)
    im.paste(paper, (0, 0), m)
    d = ImageDraw.Draw(im)
    # pin + header band
    d.rectangle((0, 0, w, 56), fill=(170, 60, 50, 255))
    d.ellipse((w / 2 - 14, 14, w / 2 + 14, 42), fill=(240, 200, 110, 255), outline=(120, 80, 30, 255), width=3)
    for y in range(120, h - 40, 46):
        d.line([(26, y), (w - 26, y)], fill=(180, 165, 140, 140), width=2)
    sh = Image.new("RGBA", (w + 30, h + 30), (0, 0, 0, 0))
    sm = Image.new("L", (w + 30, h + 30), 0)
    sm.paste(m, (18, 18))
    sh.putalpha(sm.filter(ImageFilter.GaussianBlur(9)).point(lambda v: v * 0.45))
    sh.alpha_composite(im, (6, 6))
    sh.save(os.path.join(OUT, "UI_Ticket.png"))


def card(name, w=240, h=200, sel=False):
    im = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    body = vgrad(w, h, (30, 70, 100) if sel else (22, 50, 76), (12, 30, 50))
    im.paste(body, (0, 0), rounded_mask(w, h, 26))
    d = ImageDraw.Draw(im)
    d.rounded_rectangle((2, 2, w - 3, h - 3), 24, outline=GOLD if sel else (120, 170, 200), width=4 if sel else 3)
    d.ellipse((w / 2 - 70, 18, w / 2 + 70, 140), fill=(255, 255, 255, 18))
    im.save(os.path.join(OUT, name + ".png"))


def slot(name, size=200):
    im = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    n = 28
    for k in range(n):
        if k % 2:
            continue
        a0, a1 = 360 * k / n, 360 * (k + 1) / n
        d.arc((8, 8, size - 8, size - 8), a0, a1, fill=(120, 140, 160, 200), width=6)
    d.ellipse((30, 30, size - 30, size - 30), fill=(90, 110, 130, 40))
    im.save(os.path.join(OUT, name + ".png"))


panel()
pill("UI_Chip_On", 260, 64, (120, 86, 30), (70, 48, 14), GOLD, glow=(255, 200, 90, 150))
pill("UI_Chip_Off", 260, 64, (24, 46, 66), (12, 26, 40), (90, 130, 160))
pill("UI_Chip_Done", 260, 64, (30, 90, 60), (14, 50, 32), (110, 220, 150))
pill("UI_Button", 420, 112, (70, 170, 90), (30, 100, 50), (200, 255, 200), glow=(140, 255, 160, 130))
ticket()
card("UI_Card")
card("UI_Card_Sel", sel=True)
slot("UI_Slot")
print("UI ART DONE")
