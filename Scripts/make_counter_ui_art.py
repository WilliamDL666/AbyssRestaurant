# PIL: frames / buttons / icons for the full-screen kitchen counter UI (brass + deep-sea style).
#   python make_counter_ui_art.py <ui_dir>
import sys, os, math, random
from PIL import Image, ImageDraw, ImageFilter, ImageChops

OUT = sys.argv[1]
rng = random.Random(9)
BRASS = (214, 164, 84)
BRASS_L = (250, 214, 140)
BRASS_D = (120, 80, 34)
NAVY = (14, 30, 52)
NAVY_L = (30, 62, 96)


def vgrad(w, h, top, bot):
    im = Image.new("RGBA", (w, h))
    d = ImageDraw.Draw(im)
    for y in range(h):
        t = y / max(h - 1, 1)
        d.line([(0, y), (w, y)], fill=tuple(int(top[i] + (bot[i] - top[i]) * t) for i in range(3)) + (255,))
    return im


def rmask(w, h, r, pad=0):
    m = Image.new("L", (w, h), 0)
    ImageDraw.Draw(m).rounded_rectangle((pad, pad, w - 1 - pad, h - 1 - pad), r, fill=255)
    return m


def brass_frame(im, r, width=8, pad=0):
    """beveled brass rim: dark outer, bright middle, dark inner line"""
    w, h = im.size
    d = ImageDraw.Draw(im)
    d.rounded_rectangle((pad, pad, w - 1 - pad, h - 1 - pad), r, outline=BRASS_D, width=width)
    d.rounded_rectangle((pad + 2, pad + 2, w - 3 - pad, h - 3 - pad), r - 2, outline=BRASS, width=width - 3)
    d.rounded_rectangle((pad + 3, pad + 3, w - 4 - pad, h - 4 - pad // 2), r - 3, outline=BRASS_L, width=2)
    d.rounded_rectangle((pad + width, pad + width, w - 1 - pad - width, h - 1 - pad - width), max(r - width, 2), outline=(60, 40, 18), width=2)


def shadowed(im, blur=10, off=(0, 6), alpha=0.55):
    w, h = im.size
    m = 2 * blur + 4
    out = Image.new("RGBA", (w + 2 * m, h + 2 * m), (0, 0, 0, 0))
    sh = Image.new("RGBA", out.size, (0, 0, 0, 0))
    a = im.split()[3].point(lambda v: int(v * alpha))
    sh.paste((0, 0, 0, 255), (m + off[0], m + off[1]), a)
    sh = sh.filter(ImageFilter.GaussianBlur(blur))
    out.alpha_composite(sh)
    out.alpha_composite(im, (m, m))
    return out, m


def save(im, name):
    im.save(os.path.join(OUT, name + ".png"))


def rivets(d, pts, r=5):
    for (x, y) in pts:
        d.ellipse((x - r, y - r, x + r, y + r), fill=BRASS_D)
        d.ellipse((x - r + 1, y - r + 1, x + r - 2, y + r - 2), fill=BRASS)
        d.ellipse((x - r + 2, y - r + 2, x, y), fill=BRASS_L)


# ---------------------------------------------------------------- order ticket (hangs from a chain)
def ticket(sel=False):
    W, H = 300, 380
    card = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    paper = vgrad(W, H, (246, 232, 204), (226, 204, 166))
    # paper grain + edge burn
    g = Image.effect_noise((W, H), 18).convert("L").point(lambda v: v - 128 if v > 128 else 0)
    paper = ImageChops.subtract(paper, Image.merge("RGBA", (g, g, g, Image.new("L", (W, H), 0))))
    burn = Image.new("L", (W, H), 0)
    ImageDraw.Draw(burn).rounded_rectangle((14, 14, W - 15, H - 15), 26, fill=255)
    burn = burn.filter(ImageFilter.GaussianBlur(14))
    darker = vgrad(W, H, (196, 160, 112), (170, 130, 86))
    paper = Image.composite(paper, darker, burn)
    card.paste(paper, (0, 0), rmask(W, H, 30))
    brass_frame(card, 30, 10)
    d = ImageDraw.Draw(card)
    rivets(d, [(24, 24), (W - 24, 24), (24, H - 24), (W - 24, H - 24)], 6)
    # divider above the ingredient row
    d.line([(30, 262), (W - 30, 262)], fill=(170, 130, 86, 200), width=2)
    if sel:
        glow = Image.new("RGBA", (W + 60, H + 60), (0, 0, 0, 0))
        ImageDraw.Draw(glow).rounded_rectangle((22, 22, W + 37, H + 37), 34, outline=(255, 220, 120, 230), width=12)
        glow = glow.filter(ImageFilter.GaussianBlur(10))
        glow.alpha_composite(card, (30, 30))
        card = glow.crop((30, 30, W + 30, H + 30))
    save(card, "UI_TicketCard_Sel" if sel else "UI_TicketCard")


def hanger():
    W, H = 80, 170
    im = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    # chain links
    for k in range(6):
        y = 6 + k * 22
        if k % 2 == 0:
            d.ellipse((W / 2 - 9, y, W / 2 + 9, y + 26), outline=(60, 56, 52), width=6)
            d.ellipse((W / 2 - 8, y + 1, W / 2 + 8, y + 24), outline=(150, 145, 138), width=3)
        else:
            d.rectangle((W / 2 - 3, y, W / 2 + 3, y + 26), fill=(70, 66, 60))
            d.rectangle((W / 2 - 1, y + 2, W / 2 + 1, y + 24), fill=(160, 155, 148))
    # brass clamp
    d.rounded_rectangle((10, H - 40, W - 10, H - 4), 8, fill=BRASS_D)
    d.rounded_rectangle((13, H - 38, W - 13, H - 8), 6, fill=BRASS)
    d.rounded_rectangle((16, H - 36, W - 16, H - 26), 4, fill=BRASS_L)
    rivets(d, [(22, H - 20), (W - 22, H - 20)], 5)
    save(im, "UI_Hanger")


# ---------------------------------------------------------------- generic dark panel with brass rim (9-slice friendly)
def dark_panel(name, W, H, r, glow=None):
    im = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    body = vgrad(W, H, (36, 58, 82), (12, 22, 38))
    im.paste(body, (0, 0), rmask(W, H, r))
    gl = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    ImageDraw.Draw(gl).rounded_rectangle((10, 8, W - 10, H * 0.45), r * 0.6, fill=(255, 255, 255, 26))
    im.alpha_composite(gl)
    brass_frame(im, r, 7)
    if glow:
        g = Image.new("RGBA", (W + 40, H + 40), (0, 0, 0, 0))
        ImageDraw.Draw(g).rounded_rectangle((16, 16, W + 23, H + 23), r + 4, outline=glow, width=10)
        g = g.filter(ImageFilter.GaussianBlur(8))
        g.alpha_composite(im, (20, 20))
        im = g.crop((20, 20, W + 20, H + 20))
    save(im, name)


# ---------------------------------------------------------------- ingredient bar (long tray on the counter front)
def ing_bar():
    W, H = 1300, 150
    im = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    body = vgrad(W, H, (40, 44, 52), (16, 18, 24))
    im.paste(body, (0, 0), rmask(W, H, 22))
    brass_frame(im, 22, 9)
    d = ImageDraw.Draw(im)
    rivets(d, [(22, 22), (W - 22, 22), (22, H - 22), (W - 22, H - 22)], 6)
    # arrows
    for sgn, x in ((-1, 40), (1, W - 40)):
        d.polygon([(x - 10 * sgn, H / 2 - 22), (x + 12 * sgn, H / 2), (x - 10 * sgn, H / 2 + 22)], fill=BRASS)
    save(im, "UI_IngBar")


def ing_slot(sel=False):
    W = H = 128
    im = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    body = vgrad(W, H, (52, 74, 98) if not sel else (80, 110, 140), (20, 32, 48))
    im.paste(body, (0, 0), rmask(W, H, 20))
    d = ImageDraw.Draw(im)
    d.rounded_rectangle((1, 1, W - 2, H - 2), 20, outline=(10, 14, 20), width=4)
    d.rounded_rectangle((4, 4, W - 5, H - 5), 17, outline=BRASS if sel else (120, 140, 160), width=3)
    d.ellipse((W * 0.18, H * 0.12, W * 0.82, H * 0.62), fill=(255, 255, 255, 22))
    save(im, "UI_IngSlot_Sel" if sel else "UI_IngSlot")


# ---------------------------------------------------------------- side menu button
def side_button():
    W, H = 300, 96
    im = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    body = vgrad(W, H, (58, 66, 78), (22, 26, 34))
    im.paste(body, (0, 0), rmask(W, H, 30))
    gl = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    ImageDraw.Draw(gl).rounded_rectangle((12, 8, W - 12, 40), 18, fill=(255, 255, 255, 30))
    im.alpha_composite(gl)
    brass_frame(im, 30, 8)
    d = ImageDraw.Draw(im)
    rivets(d, [(22, H / 2), (W - 22, H / 2)], 5)
    save(im, "UI_SideBtn")


def badge():
    W = H = 56
    im = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    d.ellipse((2, 2, W - 3, H - 3), fill=(150, 10, 10))
    d.ellipse((5, 5, W - 6, H - 6), fill=(230, 40, 30))
    d.ellipse((12, 8, W - 18, 24), fill=(255, 140, 120, 160))
    d.rounded_rectangle((W / 2 - 4, 13, W / 2 + 4, 34), 3, fill=(255, 255, 255))
    d.ellipse((W / 2 - 4, 38, W / 2 + 4, 46), fill=(255, 255, 255))
    save(im, "UI_Badge")


# ---------------------------------------------------------------- top bar pills + icons
def pill(name, W, H):
    im = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    body = vgrad(W, H, (34, 52, 76), (10, 18, 30))
    im.paste(body, (0, 0), rmask(W, H, H // 2))
    brass_frame(im, H // 2, 6)
    save(im, name)


def coin():
    S = 96
    im = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    d.ellipse((2, 2, S - 3, S - 3), fill=(150, 96, 20))
    d.ellipse((6, 6, S - 7, S - 7), fill=(240, 180, 40))
    d.ellipse((16, 16, S - 17, S - 17), fill=(212, 150, 30))
    d.ellipse((20, 18, S - 30, 44), fill=(255, 232, 140, 170))
    # anchor-ish S mark
    d.arc((34, 26, 62, 50), 90, 360, fill=(255, 226, 120), width=6)
    d.arc((34, 46, 62, 70), 270, 180, fill=(255, 226, 120), width=6)
    d.line([(48, 20), (48, 76)], fill=(255, 226, 120), width=4)
    save(im, "UI_Coin")


def pearl():
    S = 96
    im = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    # shell
    for k in range(9):
        a = math.radians(200 + k * 17.5)
        d.line([(S / 2, S * 0.78), (S / 2 + math.cos(a) * 44, S * 0.78 + math.sin(a) * 44)], fill=(200, 130, 190), width=9)
    d.pieslice((4, S * 0.78 - 44, S - 4, S * 0.78 + 44), 195, 345, fill=(235, 170, 220))
    for k in range(9):
        a = math.radians(200 + k * 17.5)
        d.line([(S / 2, S * 0.78), (S / 2 + math.cos(a) * 44, S * 0.78 + math.sin(a) * 44)], fill=(190, 110, 180), width=3)
    d.ellipse((S / 2 - 18, S * 0.5 - 18, S / 2 + 18, S * 0.5 + 18), fill=(250, 246, 255))
    d.ellipse((S / 2 - 12, S * 0.5 - 14, S / 2 - 2, S * 0.5 - 4), fill=(255, 255, 255))
    save(im, "UI_Pearl")


def sun():
    S = 96
    im = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    for k in range(12):
        a = 2 * math.pi * k / 12
        d.line([(S / 2 + math.cos(a) * 26, S / 2 + math.sin(a) * 26), (S / 2 + math.cos(a) * 42, S / 2 + math.sin(a) * 42)], fill=(255, 190, 40), width=6)
    d.ellipse((S / 2 - 22, S / 2 - 22, S / 2 + 22, S / 2 + 22), fill=(255, 210, 60))
    d.ellipse((S / 2 - 14, S / 2 - 16, S / 2 + 2, S / 2 - 2), fill=(255, 240, 170))
    save(im, "UI_Sun")


def gear():
    S = 110
    im = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    d.ellipse((2, 2, S - 3, S - 3), fill=BRASS_D)
    d.ellipse((6, 6, S - 7, S - 7), fill=(30, 46, 66))
    c = S / 2
    pts = []
    for k in range(16):
        a = 2 * math.pi * k / 16
        r = 34 if k % 2 == 0 else 26
        pts.append((c + math.cos(a) * r, c + math.sin(a) * r))
    d.polygon(pts, fill=(205, 210, 218))
    d.ellipse((c - 12, c - 12, c + 12, c + 12), fill=(30, 46, 66))
    save(im, "UI_Gear")


def plus():
    S = 56
    im = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    d.ellipse((1, 1, S - 2, S - 2), fill=(20, 90, 170))
    d.ellipse((4, 4, S - 5, S - 5), fill=(50, 150, 240))
    d.rounded_rectangle((S / 2 - 4, 14, S / 2 + 4, S - 14), 2, fill=(255, 255, 255))
    d.rounded_rectangle((14, S / 2 - 4, S - 14, S / 2 + 4), 2, fill=(255, 255, 255))
    save(im, "UI_Plus")


# ---------------------------------------------------------------- avatar ring (chef render clipped into it)
def avatar():
    S = 240
    src = os.path.join(OUT, "UI_Avatar.png")
    im = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    bg = vgrad(S, S, (60, 110, 160), (14, 34, 60))
    m = Image.new("L", (S, S), 0)
    ImageDraw.Draw(m).ellipse((14, 14, S - 15, S - 15), fill=255)
    im.paste(bg, (0, 0), m)
    if os.path.exists(src):
        a = Image.open(src).convert("RGBA")
        a = a.crop((40, 0, a.width - 40, a.height - 40)).resize((S - 20, S - 20))
        lay = Image.new("RGBA", (S, S), (0, 0, 0, 0))
        lay.alpha_composite(a, (10, 18))
        lay.putalpha(ImageChops.multiply(lay.split()[3], m))
        im.alpha_composite(lay)
    d = ImageDraw.Draw(im)
    d.ellipse((2, 2, S - 3, S - 3), outline=BRASS_D, width=12)
    d.ellipse((5, 5, S - 6, S - 6), outline=BRASS, width=7)
    d.ellipse((7, 7, S - 8, S - 8), outline=BRASS_L, width=2)
    rivets(d, [(S / 2, 9), (9, S / 2), (S - 9, S / 2), (S / 2, S - 9)], 6)
    save(im, "UI_AvatarRing")


def sub_button():
    S = 300
    im = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    glow = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    ImageDraw.Draw(glow).ellipse((6, 6, S - 7, S - 7), fill=(60, 200, 255, 140))
    glow = glow.filter(ImageFilter.GaussianBlur(10))
    im.alpha_composite(glow)
    bg = vgrad(S, S, (40, 150, 210), (8, 40, 90))
    m = Image.new("L", (S, S), 0)
    ImageDraw.Draw(m).ellipse((22, 22, S - 23, S - 23), fill=255)
    im.paste(bg, (0, 0), m)
    d = ImageDraw.Draw(im)
    for _ in range(14):
        x, y, r = rng.uniform(40, S - 40), rng.uniform(40, S - 90), rng.uniform(3, 9)
        d.ellipse((x - r, y - r, x + r, y + r), outline=(200, 240, 255, 150), width=2)
    src = os.path.join(OUT, "UI_SubIcon.png")
    if os.path.exists(src):
        sb = Image.open(src).convert("RGBA").resize((230, 192))
        lay = Image.new("RGBA", (S, S), (0, 0, 0, 0))
        lay.alpha_composite(sb, (35, 34))
        lay.putalpha(ImageChops.multiply(lay.split()[3], m))
        im.alpha_composite(lay)
    d.ellipse((18, 18, S - 19, S - 19), outline=(150, 230, 255), width=8)
    d.ellipse((24, 24, S - 25, S - 25), outline=(20, 80, 130), width=3)
    # label band
    d.rounded_rectangle((26, S - 96, S - 26, S - 40), 26, fill=(8, 30, 60, 235), outline=(150, 230, 255), width=3)
    save(im, "UI_SubBtn")


def open_sign():
    W, H = 420, 300
    im = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    cx, cy = W / 2, 118
    # ship wheel behind
    for k in range(8):
        a = 2 * math.pi * k / 8
        x2, y2 = cx + math.cos(a) * 112, cy + math.sin(a) * 112
        d.line([(cx, cy), (x2, y2)], fill=(110, 66, 30), width=16)
        d.ellipse((x2 - 13, y2 - 13, x2 + 13, y2 + 13), fill=(140, 86, 40), outline=(80, 46, 20), width=3)
    d.ellipse((cx - 92, cy - 92, cx + 92, cy + 92), outline=(130, 80, 36), width=16)
    d.ellipse((cx - 92, cy - 92, cx + 92, cy + 92), outline=(80, 46, 20), width=3)
    d.ellipse((cx - 22, cy - 22, cx + 22, cy + 22), fill=BRASS, outline=BRASS_D, width=4)
    # wooden board
    bw, bh = 380, 130
    bx, by = (W - bw) / 2, 150
    board = vgrad(bw, bh, (170, 108, 56), (110, 64, 28))
    grain = Image.new("RGBA", (bw, bh), (0, 0, 0, 0))
    gd = ImageDraw.Draw(grain)
    for y in range(6, bh, 9):
        gd.line([(0, y + rng.uniform(-2, 2)), (bw, y + rng.uniform(-2, 2))], fill=(80, 44, 18, 60), width=2)
    board.alpha_composite(grain)
    im.paste(board, (int(bx), by), rmask(bw, bh, 28))
    frame = Image.new("RGBA", (bw, bh), (0, 0, 0, 0))
    brass_frame(frame, 28, 9)
    im.alpha_composite(frame, (int(bx), by))
    rivets(d, [(bx + 24, by + 24), (bx + bw - 24, by + 24), (bx + 24, by + bh - 24), (bx + bw - 24, by + bh - 24)], 6)
    save(im, "UI_OpenSign")


def bubble_round(name, S, col):
    im = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    d.ellipse((2, 2, S - 3, S - 3), fill=BRASS_D)
    d.ellipse((5, 5, S - 6, S - 6), fill=col)
    d.ellipse((S * 0.22, S * 0.14, S * 0.6, S * 0.38), fill=(255, 255, 255, 70))
    save(im, name)


def pan_ring():
    S = 256
    im = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    d.ellipse((8, 8, S - 9, S - 9), outline=(255, 255, 255, 255), width=16)
    save(im, "UI_Ring")


def burnt_steak():
    # dark charred variant for the burnt state
    src = os.path.join(OUT, "UI_Steak_Cooked.png")
    if os.path.exists(src):
        a = Image.open(src).convert("RGBA")
        r, g, b, al = a.split()
        dark = Image.merge("RGBA", (r.point(lambda v: v // 4), g.point(lambda v: v // 5), b.point(lambda v: v // 5), al))
        dark.save(os.path.join(OUT, "UI_Steak_Burnt.png"))


for sel in (False, True):
    ticket(sel); ing_slot(sel)
hanger()
ing_bar()
side_button(); badge()
pill("UI_PillWide", 420, 84); pill("UI_PillMid", 320, 84)
coin(); pearl(); sun(); gear(); plus()
avatar(); sub_button(); open_sign()
dark_panel("UI_XpBar", 420, 40, 20)
pan_ring()
burnt_steak()
print("COUNTER UI ART DONE")
