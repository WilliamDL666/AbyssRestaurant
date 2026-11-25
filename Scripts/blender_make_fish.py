# Blender (headless): the three catchable fish, the grouper den, the stew pot / stew dish, seasoning + bait icons.
#   blender -b -P blender_make_fish.py -- <food_out_dir> <fish_out_dir>
# Reuses the helpers of blender_make_food.py (materials, lathe, leaf, render ...).
# Writes <fish_out>/SM_Fish_*.fbx, SM_FishTail_*.fbx, SM_GrouperDen.fbx, preview_fish.png
#        <food_out>/models/SM_Dish_Stew.fbx, <food_out>/ui/UI_Pot.png UI_Dish_4.png UI_Ing_8.png UI_Sea_1..7.png UI_Bait.png
import sys, os, math, random

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = open(os.path.join(HERE, "blender_make_food.py"), encoding="utf-8").read()
exec(SRC[:SRC.index("dishes = build_dishes()")], globals())
exec(SRC[SRC.index("# ------------------------------------------------------------------ renders"):SRC.index("# dish pictures")], globals())

FISH = sys.argv[sys.argv.index("--") + 2]
os.makedirs(FISH, exist_ok=True)
rng = random.Random(11)

for k, v in {
    "Back": pmat("Back", (0.2, 0.35, 0.6), 0.35),
    "Belly": pmat("Belly", (0.85, 0.86, 0.88), 0.3),
    "Stripe": pmat("Stripe", (0.95, 0.8, 0.2), 0.35),
    "Fin": pmat("Fin", (0.3, 0.4, 0.55), 0.45),
    "Eye": pmat("Eye", (0.01, 0.01, 0.012), 0.08),
    "Rock": pmat("Rock", (0.12, 0.14, 0.18), 0.85, noise_dark=0.4),
    "Coral": pmat("Coral", (0.75, 0.2, 0.5), 0.6),
    "Glow": pmat("Glow", (0.2, 0.9, 0.8), 0.4, emit=2.0),
    "PotBlue": pmat("MF_PotBlue", (0.04, 0.1, 0.32), 0.22),
    "Gold": pmat("MF_Gold", (0.95, 0.68, 0.22), 0.28, metal=1.0),
    "Broth": pmat("MF_Broth", (0.85, 0.6, 0.28), 0.12, emit=0.15),
    "Glass": pmat("MF_Glass", (0.75, 0.85, 0.9), 0.08),
    "Dark": pmat("MF_Dark", (0.05, 0.03, 0.03), 0.3),
    "Red": pmat("MF_Red", (0.8, 0.1, 0.06), 0.3),
    "Salt": pmat("MF_Salt", (0.96, 0.96, 0.98), 0.8, noise_dark=0.08),
    "Saffron": pmat("MF_Saffron", (0.9, 0.25, 0.05), 0.4, emit=0.6),
    "Yuzu": pmat("MF_Yuzu", (1.0, 0.82, 0.12), 0.45, noise_dark=0.1),
    "Shiso": pmat("MF_Shiso", (0.2, 0.55, 0.2), 0.5),
    "Urchin": pmat("MF_Urchin", (0.12, 0.05, 0.18), 0.5),
    "Roe": pmat("MF_Roe", (1.0, 0.55, 0.1), 0.3, sss=0.3),
    "Chum": pmat("MF_Chum", (1.0, 0.45, 0.2), 0.4, emit=0.8),
    "Cork": pmat("MF_Cork", (0.6, 0.42, 0.25), 0.8),
}.items():
    M[k] = v


def keep_join(name, parts, smooth=True):
    """join without moving the origin"""
    bpy.ops.object.select_all(action="DESELECT")
    for p in parts:
        bpy.context.view_layer.objects.active = p
        p.select_set(True)
        for m in list(p.modifiers):
            bpy.ops.object.modifier_apply(modifier=m.name)
    bpy.context.view_layer.objects.active = parts[0]
    bpy.ops.object.join()
    o = bpy.context.view_layer.objects.active
    o.name = name
    if smooth:
        bpy.ops.object.shade_smooth()
    return o


def export_to(o, folder, name):
    bpy.ops.object.select_all(action="DESELECT")
    o.select_set(True)
    bpy.context.view_layer.objects.active = o
    bpy.ops.export_scene.fbx(filepath=os.path.join(folder, name + ".fbx"), use_selection=True, apply_unit_scale=True,
                             axis_forward="X", axis_up="Z", mesh_smooth_type="FACE", path_mode="STRIP")
    print("EXPORT", name, tuple(round(d, 3) for d in o.dimensions), [m.name for m in o.data.materials])


# ------------------------------------------------------------------ fish
def body(name, L, H, Wd, a, b, sag=0.0, band=0.28, ped=0.022, segs=30, ring=20, spots=False):
    """Lofted fish body, head on +X. Slots: 0 Back, 1 Belly, 2 Stripe (lateral band)."""
    bm = bmesh.new()
    rings = []
    for i in range(segs + 1):
        t = 0.015 + (1 - 0.015) * i / segs
        s = math.sin(math.pi * t ** a) ** b
        h = max(H * s, ped if t > 0.6 else 0.0)
        w = max(Wd * s, ped * 0.6 if t > 0.6 else 0.0)
        x = L * (0.5 - t)
        zc = -sag * math.sin(math.pi * t)
        r = []
        for k in range(ring):
            th = 2 * math.pi * k / ring
            sz = math.sin(th)
            # flatter belly, slightly sharper back
            r.append(bm.verts.new((x, w * math.cos(th), zc + h * sz * (1.0 if sz > 0 else 0.9))))
        rings.append(r)
    nose = bm.verts.new((L * 0.5, 0, 0))
    for k in range(ring):
        th = 2 * math.pi * (k + 0.5) / ring
        f = bm.faces.new([nose, rings[0][(k + 1) % ring], rings[0][k]])
        f.material_index = 0 if math.sin(th) > band else 1
    for i in range(segs):
        for k in range(ring):
            th = 2 * math.pi * (k + 0.5) / ring
            sz = math.sin(th)
            f = bm.faces.new([rings[i][k], rings[i][(k + 1) % ring], rings[i + 1][(k + 1) % ring], rings[i + 1][k]])
            if spots:
                f.material_index = 0 if sz > -0.25 else 1
            else:
                f.material_index = 0 if sz > band else (1 if sz < -0.1 else 2)
    bm.faces.new(list(reversed(rings[-1])))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    o = obj(name, bm, "Back")
    o.data.materials.append(M["Belly"]); o.data.materials.append(M["Stripe"])
    mod(o, "SUBSURF", levels=1, render_levels=1)
    return o


def flat(name, pts, mat="Fin", thick=0.008, loc=(0, 0, 0), rot=(0, 0, 0)):
    """thin fin from an outline in the XZ plane"""
    bm = bmesh.new()
    # a forked tail comes as two convex lobes: [[...], [...]]
    for poly in (pts if isinstance(pts[0], list) else [pts]):
        bm.faces.new([bm.verts.new((x, 0, z)) for x, z in poly])
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=1e-5)
    o = obj(name, bm, mat)
    mod(o, "SOLIDIFY", thickness=thick, offset=0)
    o.rotation_euler = rot
    o.location = loc
    bpy.context.view_layer.update()
    return o


def eyes(name, x, y, z, r):
    parts = []
    for sgn in (1, -1):
        parts.append(ellipsoid("%s_w%d" % (name, sgn), (r * 1.35, r * 0.5, r * 1.35), "Belly", (x, sgn * y, z), sub=2))
        parts.append(ellipsoid("%s_p%d" % (name, sgn), (r, r * 0.6, r), "Eye", (x + r * 0.15, sgn * (y + r * 0.3), z), sub=2))
    return parts


def pectorals(name, pts, x, y, z, tilt=55):
    parts = []
    for sgn in (1, -1):
        parts.append(flat("%s%d" % (name, sgn), pts, loc=(x, sgn * y, z), rot=(math.radians(sgn * tilt), 0, math.radians(-sgn * 18))))
    return parts


def make_fish(key, L, H, Wd, a, b, sag, dorsal, anal, tail, pect, extra=None, spots=False, eye=(0.34, 0.03, 0.02)):
    parts = [body("B" + key, L, H, Wd, a, b, sag=sag, spots=spots)]
    for i, pts in enumerate(dorsal):
        parts.append(flat("D%s%d" % (key, i), pts))
    for i, pts in enumerate(anal):
        parts.append(flat("A%s%d" % (key, i), pts))
    parts += pectorals("P" + key, pect, L * 0.2, Wd * 0.82, -H * 0.2)
    # eye sits on the side of the head; the body is narrow there
    t = 0.5 - eye[0] / L
    s = math.sin(math.pi * t ** a) ** b
    parts += eyes("E" + key, eye[0], Wd * s * 0.86, eye[1], eye[2])
    if extra:
        parts += extra()
    o = keep_join("SM_Fish_" + key, parts)
    tl = keep_join("SM_FishTail_" + key, [flat("T" + key, tail, thick=0.01)])
    return o, tl


def tuna_extra():
    parts = []
    for i in range(5):
        x = -0.24 - i * 0.055
        for sgn in (1, -1):
            parts.append(flat("Fl%d_%d" % (i, sgn), [(x, sgn * 0.035), (x - 0.03, sgn * 0.075), (x - 0.04, sgn * 0.03)], thick=0.006))
    # keel on the tail stalk
    for sgn in (1, -1):
        parts.append(ellipsoid("Keel%d" % sgn, (0.05, 0.012, 0.008), "Back", (-0.5, sgn * 0.02, 0), sub=2))
    return parts


def grouper_extra():
    parts = []
    # thick lips
    parts.append(ellipsoid("LipU", (0.045, 0.085, 0.022), "Belly", (0.5, 0, 0.012), sub=2))
    parts.append(ellipsoid("LipL", (0.05, 0.09, 0.024), "Belly", (0.495, 0, -0.03), sub=2))
    # glowing spots along the flanks (Stripe slot is emissive in game)
    L, H, Wd, a, b = 1.1, 0.22, 0.14, 0.6, 0.7
    for i in range(22):
        t = rng.uniform(0.22, 0.82)
        th = rng.uniform(-0.35, 0.95)
        s = math.sin(math.pi * t ** a) ** b
        x = L * (0.5 - t)
        for sgn in (1, -1):
            y = sgn * Wd * s * math.cos(th) * 0.99
            z = -0.03 * math.sin(math.pi * t) + H * s * math.sin(th)
            r = rng.uniform(0.012, 0.024)
            parts.append(ellipsoid("Sp%d_%d" % (i, sgn), (r, r * 0.45, r), "Stripe", (x, y, z), sub=1))
    return parts


fish = {}
# Yellowtail: slim, forked tail, long low dorsal, yellow lateral band
fish["Yellowtail"] = make_fish("Yellowtail", 1.1, 0.14, 0.078, 0.75, 0.8, 0.0,
    dorsal=[[(0.2, 0.06), (0.1, 0.22), (0.0, 0.17), (-0.34, 0.085), (-0.4, 0.02), (0.2, 0.02)]],
    anal=[[(-0.08, -0.05), (-0.16, -0.18), (-0.38, -0.06), (-0.38, -0.015)]],
    tail=[[(0.05, 0.028), (-0.19, 0.2), (-0.27, 0.215), (-0.12, 0.0), (0.05, 0.0)],
          [(0.05, 0.0), (-0.12, 0.0), (-0.27, -0.215), (-0.19, -0.2), (0.05, -0.028)]],
    pect=[(0, 0), (-0.17, 0.035), (-0.14, -0.06)])
# Tuna: thick torpedo, sickle fins, finlets, lunate tail
fish["Tuna"] = make_fish("Tuna", 1.1, 0.175, 0.125, 0.72, 0.72, 0.0,
    dorsal=[[(0.2, 0.1), (0.13, 0.3), (0.05, 0.16), (-0.02, 0.1), (0.1, 0.03)],
            [(-0.1, 0.1), (-0.2, 0.33), (-0.25, 0.31), (-0.2, 0.08), (-0.12, 0.03)]],
    anal=[[(-0.12, -0.1), (-0.22, -0.32), (-0.27, -0.3), (-0.22, -0.08), (-0.14, -0.03)]],
    tail=[[(0.05, 0.024), (-0.02, 0.06), (-0.13, 0.31), (-0.2, 0.33), (-0.08, 0.0), (0.05, 0.0)],
          [(0.05, 0.0), (-0.08, 0.0), (-0.2, -0.33), (-0.13, -0.31), (-0.02, -0.06), (0.05, -0.024)]],
    pect=[(0, 0), (-0.26, 0.02), (-0.2, -0.05)], extra=tuna_extra, eye=(0.37, 0.02, 0.022))
# Grouper: deep, heavy head, spiny dorsal, round tail, glowing spots
spines = [(0.22, 0.12)]
for i in range(7):
    x = 0.2 - i * 0.055
    spines += [(x - 0.01, 0.31 - i * 0.012), (x - 0.045, 0.22 - i * 0.008)]
spines += [(-0.25, 0.25), (-0.36, 0.2), (-0.4, 0.06), (-0.3, 0.03), (0.22, 0.04)]
fish["Grouper"] = make_fish("Grouper", 1.1, 0.22, 0.14, 0.6, 0.7, 0.03,
    dorsal=[spines],
    anal=[[(-0.14, -0.12), (-0.2, -0.27), (-0.34, -0.22), (-0.4, -0.06), (-0.3, -0.03)]],
    tail=[(0.06, 0.07), (-0.12, 0.17), (-0.22, 0.14), (-0.27, 0.0), (-0.22, -0.14), (-0.12, -0.17), (0.06, -0.07)],
    pect=[(0, 0), (-0.1, 0.1), (-0.2, 0.06), (-0.22, -0.04), (-0.12, -0.1)], extra=grouper_extra, spots=True, eye=(0.36, 0.09, 0.028))

for key, (o, tl) in fish.items():
    export_to(o, FISH, "SM_Fish_" + key)
    export_to(tl, FISH, "SM_FishTail_" + key)


# ------------------------------------------------------------------ grouper den: a rocky horseshoe with a dark mouth
def rock(name, r, loc, squash=(1, 1, 0.75), seed=0):
    bm = bmesh.new()
    bmesh.ops.create_icosphere(bm, subdivisions=3, radius=1.0)
    rr = random.Random(seed)
    ph = [rr.uniform(0, 6.28) for _ in range(6)]
    for v in bm.verts:
        n = v.co.normalized()
        d = 1 + 0.22 * math.sin(n.x * 3.1 + ph[0]) * math.sin(n.y * 2.7 + ph[1]) + 0.14 * math.sin(n.z * 5.3 + ph[2]) + 0.08 * math.sin(n.x * 9 + n.y * 7 + ph[3])
        v.co = n * d
    bmesh.ops.scale(bm, vec=(r * squash[0], r * squash[1], r * squash[2]), verts=bm.verts)
    bmesh.ops.translate(bm, vec=loc, verts=bm.verts)
    return obj(name, bm, "Rock")


den = []
N = 9
for i in range(N):
    # ring of boulders, open towards +X
    ang = math.radians(55 + (250 * i / (N - 1)))
    R = 1.75 + rng.uniform(-0.15, 0.2)
    r = rng.uniform(0.75, 1.05)
    den.append(rock("R%d" % i, r, (math.cos(ang) * R, math.sin(ang) * R, r * 0.55), (1, 1, rng.uniform(0.8, 1.15)), seed=i))
for i in range(4):
    # the roof
    ang = rng.uniform(0, 6.28)
    den.append(rock("Top%d" % i, rng.uniform(0.95, 1.25), (math.cos(ang) * 0.55 - 0.25, math.sin(ang) * 0.6, 2.1 + rng.uniform(-0.1, 0.15)), (1.25, 1.25, 0.5), seed=20 + i))
for sgn in (1, -1):
    den.append(rock("Jamb%d" % sgn, 0.7, (1.35, sgn * 1.45, 0.45), (0.9, 0.9, 1.3), seed=30 + sgn))
for i in range(9):
    # coral fingers + glow buds around the mouth
    ang = math.radians(rng.choice([rng.uniform(25, 75), rng.uniform(-75, -25)]))
    R = rng.uniform(1.9, 2.5)
    h = rng.uniform(0.35, 0.8)
    c = lathe("C%d" % i, [(0.0, 0.0), (0.07, 0.0), (0.05, h * 0.6), (0.075, h), (0.0, h + 0.05)], "Coral", segs=10)
    c.location = (math.cos(ang) * R, math.sin(ang) * R, 0.1)
    c.rotation_euler = (rng.uniform(-0.25, 0.25), rng.uniform(-0.25, 0.25), 0)
    den.append(c)
    den.append(ellipsoid("G%d" % i, (0.07, 0.07, 0.07), "Glow", (math.cos(ang) * R, math.sin(ang) * R, 0.1 + h + 0.08), sub=2))
den_o = keep_join("SM_GrouperDen", den)
export_to(den_o, FISH, "SM_GrouperDen")

# preview of the fish + den (not imported)
prev = []
for i, key in enumerate(fish):
    o, tl = fish[key]
    o.location = (0, i * 0.7, 3.2 + i * 0.0); tl.location = (-0.55, i * 0.7, 3.2)
    prev += [o, tl]
bpy.context.view_layer.update()
UI_BACKUP = UI
UI = FISH
render(prev, "preview_fish", 1400, 900, view="persp", pad=1.1)
render([den_o], "preview_den", 900, 700, view="persp", pad=1.1)
UI = UI_BACKUP

# ------------------------------------------------------------------ stew: pot (counter picture) + plated bowl
def chunk(name, loc, yaw):
    o = rbox(name, (0.03, 0.022, 0.014), "Yellowtail", bevel=0.35, segs=3, loc=loc)
    o.rotation_euler = (rng.uniform(-0.2, 0.2), rng.uniform(-0.2, 0.2), yaw)
    return o


def stew_top(prefix, R, z, n_fish=4, n_kelp=6, n_green=4):
    parts = []
    bm = bmesh.new()
    bmesh.ops.create_circle(bm, cap_ends=True, segments=48, radius=R)
    bmesh.ops.translate(bm, vec=(0, 0, z), verts=bm.verts)
    parts.append(obj(prefix + "Broth", bm, "Broth"))
    for i in range(n_fish):
        ang = 2 * math.pi * i / n_fish + 0.4
        parts.append(chunk("%sF%d" % (prefix, i), (math.cos(ang) * R * 0.42, math.sin(ang) * R * 0.42, z + 0.004), ang))
    parts += kelp_pile(prefix + "K", (0, 0), n=n_kelp, radius=R * 0.9, z=z - 0.004)
    parts += greens(prefix + "G", (R * 0.15, -R * 0.2, z + 0.004), n=n_green, s=0.9)
    return parts


# plated bowl (model + ticket picture + small icon)
p = [bowl("Bowl4", 0.095)]
p += stew_top("S4", 0.08, 0.043)
p += sesame("Ses4", (0, 0), 0.04, n=26, z=0.05)
stew = join("SM_Dish_Stew", p)
export(stew, "SM_Dish_Stew")
render([stew], "UI_Dish_4", 384, 384)
render([stew], "UI_Ing_8", 256, 256, pad=1.2)

# the pot that stands on the second burner of the counter picture
R = 0.115
prof = [(0.0, 0.0), (R * 0.9, 0.0), (R, 0.012), (R * 1.02, 0.1), (R * 1.07, 0.108), (R * 1.07, 0.118), (R * 0.97, 0.118), (R * 0.95, 0.1), (R * 0.93, 0.02), (0.0, 0.016)]
pot = [lathe("PotBody", prof, "PotBlue", segs=64)]
rim = lathe("PotRim", [(R * 0.955, 0.116), (R * 1.075, 0.116), (R * 1.08, 0.122), (R * 0.95, 0.122)], "Gold", segs=64)
pot.append(rim)
band = lathe("PotBand", [(R * 1.012, 0.03), (R * 1.03, 0.032), (R * 1.03, 0.04), (R * 1.016, 0.042)], "Gold", segs=64)
pot.append(band)
for sgn in (1, -1):
    bm = bmesh.new()
    bmesh.ops.create_cube(bm, size=1.0)
    bmesh.ops.scale(bm, vec=(0.03, 0.06, 0.014), verts=bm.verts)
    hd = obj("Handle%d" % sgn, bm, "Gold")
    mod(hd, "BEVEL", width=0.006, segments=3)
    hd.location = (sgn * (R * 1.07 + 0.012), 0, 0.095)
    pot.append(hd)
bm = bmesh.new()
bmesh.ops.create_circle(bm, cap_ends=True, segments=48, radius=R * 0.95)
bmesh.ops.translate(bm, vec=(0, 0, 0.092), verts=bm.verts)
pot.append(obj("PotBroth", bm, "Broth"))
pot_o = join("UIPot", pot)
render([pot_o], "UI_Pot", 512, 512, pad=1.12)

# ------------------------------------------------------------------ seasoning / garnish / bait icons
def icon_named(name, parts, pad=1.25):
    o = join(name, parts)
    render([o], name, 256, 256, pad=pad)


# 1 sea salt: little wooden bowl with a white mound
icon_named("UI_Sea_1", [lathe("SaltBowl", [(0.0, 0.0), (0.03, 0.0), (0.05, 0.02), (0.055, 0.035), (0.05, 0.035), (0.045, 0.022), (0.0, 0.01)], "Wood", segs=40),
                        ellipsoid("SaltHeap", (0.04, 0.04, 0.028), "Salt", (0, 0, 0.028), sub=3)])
# 2 soy sauce: dark bottle, red cap
icon_named("UI_Sea_2", [lathe("SoyGlass", [(0.0, 0.0), (0.028, 0.0), (0.032, 0.006), (0.032, 0.06), (0.012, 0.085), (0.012, 0.1), (0.0, 0.1)], "Dark", segs=40),
                        lathe("SoyCap", [(0.0, 0.1), (0.015, 0.1), (0.015, 0.115), (0.007, 0.122), (0.0, 0.122)], "Red", segs=24),
                        lathe("SoyLabel", [(0.0325, 0.022), (0.033, 0.022), (0.033, 0.05), (0.0325, 0.05)], "Salt", segs=40)])
# 3 chili oil: glass jar of red oil with a cork
chili = [lathe("ChiliJar", [(0.0, 0.0), (0.035, 0.0), (0.04, 0.008), (0.04, 0.06), (0.03, 0.07), (0.03, 0.078), (0.0, 0.078)], "Red", segs=40),
         lathe("ChiliCork", [(0.0, 0.078), (0.026, 0.078), (0.028, 0.095), (0.0, 0.095)], "Cork", segs=24)]
for i in range(3):
    c = lathe("Pep%d" % i, [(0.0, 0.0), (0.008, 0.004), (0.007, 0.03), (0.0, 0.05)], "Red", segs=12)
    c.rotation_euler = (math.radians(90), 0, math.radians(40 + i * 50))
    c.location = (0.055 + i * 0.004, -0.03 + i * 0.02, 0.008)
    chili.append(c)
icon_named("UI_Sea_3", chili)
# 4 abyss saffron: glowing threads in a small gold-capped vial
saf = [lathe("SafVial", [(0.0, 0.0), (0.022, 0.0), (0.026, 0.006), (0.026, 0.07), (0.016, 0.08), (0.0, 0.08)], "Glass", segs=40),
       lathe("SafCap", [(0.0, 0.08), (0.018, 0.08), (0.02, 0.092), (0.0, 0.096)], "Gold", segs=24)]
for i in range(16):
    t = lathe("Th%d" % i, [(0.0, 0.0), (0.0022, 0.002), (0.0015, 0.03), (0.0, 0.034)], "Saffron", segs=6)
    t.rotation_euler = (rng.uniform(-0.6, 0.6), rng.uniform(-0.6, 0.6), 0)
    t.location = (rng.uniform(-0.014, 0.014) + 0.0, rng.uniform(-0.03, -0.027), 0.008 + rng.uniform(0, 0.03))
    saf.append(t)
for i in range(7):
    t = lathe("ThOut%d" % i, [(0.0, 0.0), (0.0025, 0.002), (0.0015, 0.034), (0.0, 0.038)], "Saffron", segs=6)
    t.rotation_euler = (math.radians(90), 0, rng.uniform(0, 6.28))
    t.location = (0.045 + rng.uniform(-0.012, 0.012), rng.uniform(-0.02, 0.02), 0.003)
    saf.append(t)
icon_named("UI_Sea_4", saf)
# 5 fresh yuzu: whole fruit with a leaf + a cut half
yz = [ellipsoid("Yuzu", (0.04, 0.04, 0.035), "Yuzu", (0, 0, 0.035), sub=3),
      leaf("YuzuLeaf", (0.0, 0.0, 0.068), 0.6, s=1.1, mat=M["Shiso"]),
      lemon_wedge("YuzuCut", (0.06, -0.03, 0.0), 0.8, s=1.2)]
icon_named("UI_Sea_5", yz)
# 6 shiso: three broad green leaves
sh = [leaf("Sh%d" % i, (i * 0.004, 0, i * 0.003), -0.5 + i * 0.55, s=2.2, mat=M["Shiso"]) for i in range(3)]
icon_named("UI_Sea_6", sh)
# 7 sea urchin: spiny shell opened on its roe
ur = [ellipsoid("UrchinShell", (0.04, 0.04, 0.03), "Urchin", (0, 0, 0.03), sub=3)]
for i in range(70):
    th = rng.uniform(0, 6.28); ph = rng.uniform(0.15, 1.55)
    n = (math.cos(th) * math.cos(ph), math.sin(th) * math.cos(ph), math.sin(ph) * 0.3 + 0.05)
    sp = lathe("Spine%d" % i, [(0.0, 0.0), (0.0025, 0.0), (0.0, 0.035)], "Urchin", segs=6)
    sp.location = (n[0] * 0.038, n[1] * 0.038, 0.03 + n[2] * 0.03)
    sp.rotation_euler = (0, math.radians(90) - ph * 0.9, th)
    ur.append(sp)
for i in range(5):
    a = 2 * math.pi * i / 5
    ur.append(ellipsoid("Roe%d" % i, (0.014, 0.008, 0.006), "Roe", (math.cos(a) * 0.014, math.sin(a) * 0.014, 0.06), sub=2))
icon_named("UI_Sea_7", ur)
# bait: a glowing ball of chum on a hook
bt = [ellipsoid("Chum", (0.035, 0.03, 0.028), "Chum", (0, 0, 0.03), sub=3)]
for i in range(8):
    bt.append(ellipsoid("Bit%d" % i, (0.008, 0.008, 0.006), "Roe", (rng.uniform(-0.06, 0.06), rng.uniform(-0.04, 0.04), rng.uniform(0.0, 0.02)), sub=1))
hook = lathe("HookRing", [(0.012, 0.064), (0.016, 0.064), (0.016, 0.068), (0.012, 0.068)], "Steel", segs=20)
bt.append(hook)
bt.append(rbox("HookLine", (0.003, 0.003, 0.05), "Steel", bevel=0.3, segs=1, loc=(0, 0, 0.09)))
icon_named("UI_Bait", bt)
# ------------------------------------------------------------------ fish portraits (side view) for the aquarium screen
def tint(name, col):
    b = M[name].node_tree.nodes.get("Principled BSDF")
    b.inputs["Base Color"].default_value = (*col, 1)

LOOK = {
    "Yellowtail": ((0.35, 0.55, 0.75), (0.92, 0.93, 0.95), (1.0, 0.82, 0.15), (0.95, 0.78, 0.2)),
    "Tuna": ((0.08, 0.14, 0.3), (0.8, 0.83, 0.88), (0.45, 0.55, 0.7), (0.3, 0.35, 0.5)),
    "Grouper": ((0.45, 0.18, 0.75), (0.75, 0.6, 0.9), (0.2, 1.0, 0.85), (0.2, 0.75, 0.75)),
}
for i, key in enumerate(["Yellowtail", "Tuna", "Grouper"]):
    o, tl = fish[key]
    back, belly, stripe, fin = LOOK[key]
    tint("Back", back); tint("Belly", belly); tint("Stripe", stripe); tint("Fin", fin)
    for ob, x in ((o, 0.0), (tl, -0.55)):
        ob.location = (x, 0, 0)
        ob.rotation_euler = (math.radians(-90), 0, 0)
    bpy.context.view_layer.update()
    render([o, tl], "UI_Fish_%d" % i, 512, 256, view="top", pad=1.06)

# ------------------------------------------------------------------ display tank for the dining room (temporary until the Tripo one)
AQ = os.path.join(os.path.dirname(FISH), "Aquarium")
os.makedirs(AQ, exist_ok=True)
for k, v in {"Base": pmat("Base", (0.03, 0.07, 0.22), 0.3), "TGold": pmat("Gold", (0.9, 0.62, 0.2), 0.3, metal=1.0),
             "TGlass": pmat("Glass", (0.7, 0.85, 0.9), 0.05), "Sand": pmat("Sand", (0.75, 0.68, 0.5), 0.9)}.items():
    M[k] = v
tank = []
R = 1.45
tank.append(lathe("TBase", [(0.0, 0.0), (R + 0.32, 0.0), (R + 0.34, 0.06), (R + 0.26, 0.14), (R + 0.2, 0.4), (R + 0.26, 0.5), (R + 0.22, 0.62), (0.0, 0.62)], "Base", segs=64))
tank.append(lathe("TFoot", [(R + 0.3, 0.0), (R + 0.38, 0.0), (R + 0.38, 0.07), (R + 0.3, 0.09)], "TGold", segs=64))
tank.append(lathe("TRing1", [(R + 0.2, 0.5), (R + 0.3, 0.5), (R + 0.3, 0.6), (R + 0.2, 0.64)], "TGold", segs=64))
tank.append(lathe("TSand", [(0.0, 0.62), (R - 0.03, 0.62), (R - 0.03, 0.7), (R * 0.6, 0.76), (0.0, 0.72)], "Sand", segs=48))
tank.append(lathe("TGlassWall", [(R, 0.62), (R, 2.95)], "TGlass", segs=64))
tank.append(lathe("TRing2", [(R - 0.02, 2.9), (R + 0.26, 2.9), (R + 0.3, 2.98), (R + 0.24, 3.06), (R - 0.02, 3.06)], "TGold", segs=64))
tank.append(lathe("TLid", [(0.0, 3.04), (R + 0.2, 3.04), (R + 0.16, 3.2), (R * 0.7, 3.42), (R * 0.3, 3.52), (0.0, 3.54)], "Base", segs=64))
tank.append(lathe("TKnob", [(0.0, 3.52), (0.16, 3.52), (0.2, 3.62), (0.12, 3.74), (0.0, 3.78)], "TGold", segs=32))
for i in range(12):
    a = 2 * math.pi * i / 12
    # shells on the base, pearls of light under the lid
    sh = ellipsoid("TShell%d" % i, (0.05, 0.16, 0.14), "TGold", (math.cos(a) * (R + 0.22), math.sin(a) * (R + 0.22), 0.3), sub=2)
    sh.rotation_euler = (0, 0, a)
    tank.append(sh)
    tank.append(ellipsoid("TPearl%d" % i, (0.07, 0.07, 0.07), "Glow", (math.cos(a + 0.26) * (R + 0.2), math.sin(a + 0.26) * (R + 0.2), 2.86), sub=2))
for i in range(9):
    a = rng.uniform(0, 6.28); rr = rng.uniform(0.5, 1.15); h = rng.uniform(0.3, 0.75)
    c = lathe("TCoral%d" % i, [(0.0, 0.0), (0.08, 0.0), (0.05, h * 0.6), (0.085, h), (0.0, h + 0.06)], "Coral", segs=10)
    c.location = (math.cos(a) * rr, math.sin(a) * rr, 0.68)
    c.rotation_euler = (rng.uniform(-0.2, 0.2), rng.uniform(-0.2, 0.2), 0)
    tank.append(c)
tank_o = keep_join("SM_AquariumTank", tank)
export_to(tank_o, AQ, "SM_AquariumTank")
UI_BACKUP = UI
UI = AQ
render([tank_o], "preview_tank", 700, 900, view="persp", pad=1.1)
UI = UI_BACKUP
print("FISH DONE")
