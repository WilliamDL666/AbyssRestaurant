# Cuts the user's UI sheet (Model/UI_sheet.png) into individual pieces, erases baked-in labels/numbers
# (the game draws localized text on top), and crops the kitchen illustrations.
#   python make_sheet_ui.py <sheet.png> <model_dir> <ui_out_dir>
import sys, os
from PIL import Image, ImageFilter, ImageDraw

SHEET, MODEL, OUT = sys.argv[1], sys.argv[2], sys.argv[3]
os.makedirs(OUT, exist_ok=True)
S = Image.open(SHEET).convert("RGBA")
UP = 3  # the sheet is small; pieces are upscaled for the 1080p UI


def crop(box, up=UP):
    im = S.crop(box)
    return im.resize((im.width * up, im.height * up), Image.LANCZOS)


def erase(im, box, sample_dx=-4):
    """paint over a text area row by row with the color just left of it (horizontal gradients stay intact)"""
    im = im.copy()
    px = im.load()
    x0, y0, x1, y1 = [v * UP for v in box]
    sx = max(0, x0 + sample_dx * UP)
    for y in range(y0, y1):
        c = px[sx, y]
        for x in range(x0, x1):
            px[x, y] = (c[0], c[1], c[2], px[x, y][3])
    # soften the patch edges
    patch = im.crop((x0 - 2, y0 - 2, x1 + 2, y1 + 2)).filter(ImageFilter.GaussianBlur(1.5))
    im.paste(patch, (x0 - 2, y0 - 2))
    return im


def save(im, name):
    im.save(os.path.join(OUT, name + ".png"))


def rel(box, sub):
    """sub-box given in sheet coords -> coords relative to box"""
    return (sub[0] - box[0], sub[1] - box[1], sub[2] - box[0], sub[3] - box[1])


# ---- top bar
B = (17, 4, 110, 107);  save(crop(B), "S_AvatarRing")
B = (100, 22, 316, 93); save(erase(crop(B), rel(B, (124, 28, 210, 54)), -6), "S_LvPill")
B = (335, 34, 520, 82); save(erase(crop(B), rel(B, (398, 44, 478, 72))), "S_CoinPill")
B = (536, 34, 697, 82); save(erase(crop(B), rel(B, (592, 44, 652, 72))), "S_GemPill")
B = (712, 28, 882, 82); save(erase(crop(B), rel(B, (775, 44, 830, 72))), "S_ShellPill")
save(crop((897, 26, 958, 89)), "S_Mail")
save(crop((972, 26, 1032, 89)), "S_BellIcon")
save(crop((1046, 26, 1106, 89)), "S_Gear")

# ---- left menu buttons (labels erased)
for i, (name, y0) in enumerate((("Menu", 122), ("Shop", 181), ("Decor", 240), ("Tasks", 297), ("Codex", 355), ("Upgrade", 414))):
    B = (16, y0, 184, y0 + 54)
    save(erase(crop(B), rel(B, (98, y0 + 12, 152, y0 + 42)), -6), "S_Side_" + name)
save(crop((153, 293, 182, 321)), "S_Badge")

# ---- ingredient tiles
for i, (name, x0) in enumerate((("Shrimp", 227), ("Salmon", 312), ("Mussel", 396), ("Octopus", 479), ("Clam", 561), ("Tomato", 642), ("Lemon", 722), ("Parsley", 800))):
    save(crop((x0, 116, x0 + 72, 184)), "S_Tile_" + name)

# ---- dish cards (labels erased)
for i, (name, x0) in enumerate((("Pasta", 224), ("Grill", 331), ("Soup", 437), ("Sashimi", 544), ("Platter", 650), ("Dessert", 757))):
    B = (x0, 200, x0 + 102, 306)
    save(erase(crop(B), rel(B, (x0 + 18, 272, x0 + 84, 298)), -8), "S_Card_" + name)

# ---- signs (labels erased)
B = (884, 102, 1112, 252); save(erase(crop(B), rel(B, (917, 175, 1080, 222)), -6), "S_SignOpen")
B = (893, 276, 1102, 366); save(erase(crop(B), rel(B, (920, 296, 1072, 334)), -6), "S_SignClosed")

# ---- big round buttons
B = (205, 318, 362, 472); save(erase(crop(B), rel(B, (240, 418, 335, 440)), -6), "S_SubBtn")
for name, x0 in (("Serve", 379), ("Cook", 467), ("Fish", 555), ("Codex", 644), ("Decor", 734), ("Up", 823)):
    B = (x0, 347, x0 + 80, 432)
    save(erase(crop(B), rel(B, (x0 + 18, 404, x0 + 62, 424)), -4), "S_Round_" + name)

# ---- panels / frames
save(crop((18, 482, 392, 588)), "S_JellyPanel")
save(crop((402, 438, 526, 592)), "S_Clipboard")
save(crop((540, 465, 808, 587)), "S_Inventory")
save(crop((540 + 51, 477, 540 + 79, 523)), "S_InvCell")      # one empty cell
save(crop((825, 458, 960, 594)), "S_RingTimer")
save(crop((964, 444, 1113, 596)), "S_PlayWheel")

# ---- small controls
for name, box in (("ArrowL", (24, 601, 55, 652)), ("ArrowR", (76, 601, 108, 652)), ("Refresh", (129, 602, 177, 651)), ("Trash", (199, 601, 243, 650)),
                  ("Lock", (265, 602, 304, 650)), ("Plus", (327, 603, 368, 646)), ("Minus", (384, 603, 424, 646)),
                  ("StarOn", (959, 603, 1003, 646)), ("StarOff", (1058, 604, 1100, 645)),
                  ("Info", (545, 669, 597, 723)), ("Help", (613, 668, 667, 723)), ("StarBtn", (681, 668, 734, 723)),
                  ("Alert", (835, 667, 888, 720)), ("New", (909, 668, 981, 712)), ("Crown", (995, 656, 1104, 724))):
    save(crop(box), "S_" + name)
B = (455, 608, 689, 645); save(crop(B), "S_BarBlue")
B = (702, 609, 939, 645); save(crop(B), "S_BarGreen")
for name, box, txt in (("Ok", (16, 667, 189, 730), (60, 680, 150, 716)), ("Cancel", (202, 667, 354, 728), (238, 680, 320, 716)), ("Back", (370, 667, 517, 725), (395, 680, 495, 714))):
    save(erase(crop(box), rel(box, txt), -6), "S_Btn" + name)

# ---- kitchen illustrations (transparent PNGs from Model/), cropped to content
for src, name in (("灶台.png", "K_Stove"), ("切菜台子.png", "K_Prep"), ("海鲜柜台.png", "K_Display"), ("主角.png", "K_Diver")):
    p = os.path.join(MODEL, src)
    if os.path.exists(p):
        im = Image.open(p).convert("RGBA")
        bb = im.split()[3].point(lambda v: 255 if v > 40 else 0).getbbox()
        im.crop(bb).save(os.path.join(OUT, name + ".png"))
print("SHEET UI DONE")
