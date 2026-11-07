# Cuts Model/UI_sheet2.webp (the user's second UI sheet, transparent) into named pieces -> SourceArt/Food/ui/H_*.png
import os
from PIL import Image
S = Image.open("Model/UI_sheet2.webp").convert("RGBA")
OUT = "SourceArt/Food/ui"
B = {
 "DepthGauge": (35, 8, 171, 144), "BatteryBar": (192, 46, 403, 111), "Sonar": (423, 4, 564, 146), "Compass": (584, 30, 871, 111),
 "Cargo": (893, 31, 1107, 124), "Oxygen": (32, 149, 175, 288), "ToolHarpoon": (219, 151, 353, 285), "ToolNet": (396, 152, 531, 285),
 "SelRing": (574, 148, 712, 288), "ProgRing": (744, 148, 883, 288), "Reticle": (924, 153, 1061, 280), "Objective": (32, 285, 269, 420),
 "Hints": (319, 298, 427, 428), "FishTag": (463, 335, 675, 394), "Key": (718, 324, 807, 411), "Check": (858, 325, 939, 406),
 "CheckOn": (989, 325, 1070, 406), "OrderList": (19, 425, 158, 555), "OrderRow": (168, 468, 317, 517), "Bubble": (327, 456, 433, 536),
 "Happy": (445, 461, 508, 523), "Angry": (512, 448, 617, 525), "Toast": (623, 464, 830, 519), "Fridge": (843, 467, 1025, 516),
 "Marker": (1043, 447, 1101, 532), "Summary": (19, 558, 260, 679), "Star": (270, 579, 352, 660), "StarOff": (358, 579, 440, 660),
 "UpgCard": (448, 544, 557, 690), "UpgBattery": (572, 574, 671, 670), "UpgLight": (679, 574, 777, 670), "UpgOxygen": (785, 574, 884, 670),
 "UpgDome": (892, 573, 994, 670), "Coins": (1010, 571, 1109, 667), "Logo": (9, 690, 255, 842), "MenuBtn": (266, 725, 537, 805),
 "Frame": (557, 697, 824, 828), "Danger": (843, 697, 1110, 827),
}
P = 5
for n, (x0, y0, x1, y1) in B.items():
    im = S.crop((max(0, x0 - P), max(0, y0 - P), min(S.width, x1 + P), min(S.height, y1 + P)))
    im = im.resize((im.width * 3, im.height * 3), Image.LANCZOS)
    im.save(os.path.join(OUT, "H_%s.png" % n))
print("cut", len(B))
