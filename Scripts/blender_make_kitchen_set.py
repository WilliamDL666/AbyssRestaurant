# Blender (headless): the whole fitted kitchen for the undersea restaurant - shell (floor, walls, trim) plus fixtures and props,
# designed around the user's five Tripo appliances (stove, prep table, cook counter island, shelf, seafood fridge), which the
# game places itself. Coordinates below are "kitchen" metres: x across (-4.6..4.6), y depth from the back wall (0) to the
# open front (6.3), z up. The set is mirrored in Y before export because Unreal's FBX import flips Y.
#   blender -b -P blender_make_kitchen_set.py -- <out_dir>
# Writes SM_KitchenSet.fbx, KitchenSet.blend, preview_kitchen_*.png
import sys, os, math, random
import bpy, bmesh
from mathutils import Vector, Matrix

OUT = sys.argv[sys.argv.index("--") + 1]
os.makedirs(OUT, exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
sc = bpy.context.scene
rng = random.Random(5)
HW, DEPTH, WALL = 4.6, 6.3, 2.7

MATS = {}
def mat(name, col, rough=0.5, metal=0.0, emit=0.0):
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    b = m.node_tree.nodes["Principled BSDF"]
    b.inputs["Base Color"].default_value = (*col, 1)
    b.inputs["Roughness"].default_value = rough
    b.inputs["Metallic"].default_value = metal
    if emit:
        b.inputs["Emission Color"].default_value = (*col, 1)
        b.inputs["Emission Strength"].default_value = emit
    MATS[name] = m
    return m

mat("Navy", (0.02, 0.05, 0.16), 0.4); mat("NavyLight", (0.05, 0.12, 0.3), 0.35); mat("Brass", (0.85, 0.6, 0.25), 0.25, 1.0)
mat("Teal", (0.03, 0.2, 0.28), 0.4); mat("Pearl", (0.7, 0.74, 0.76), 0.3); mat("Wood", (0.33, 0.19, 0.1), 0.55)
mat("WoodLight", (0.6, 0.42, 0.25), 0.6); mat("Copper", (0.85, 0.42, 0.22), 0.25, 1.0); mat("Iron", (0.04, 0.04, 0.045), 0.4, 1.0)
mat("SeaGlow", (0.15, 0.65, 0.95), 0.2, 0.0, 2.5); mat("Warm", (1.0, 0.78, 0.45), 0.3, 0.0, 6.0); mat("Cream", (0.85, 0.78, 0.6), 0.6)
mat("Green", (0.15, 0.5, 0.18), 0.6); mat("Red", (0.75, 0.12, 0.08), 0.5); mat("Yellow", (0.95, 0.8, 0.15), 0.5)
mat("Pane", (0.55, 0.8, 0.9), 0.05); mat("Glass", (0.6, 0.8, 0.85), 0.08); mat("Rope", (0.7, 0.6, 0.4), 0.9); mat("Slate", (0.08, 0.09, 0.1), 0.7)

PARTS = []
def add(o, m):
    o.data.materials.append(MATS[m])
    sc.collection.objects.link(o)
    PARTS.append(o)
    return o

def box(size, loc, m, bevel=0.012, rot=(0, 0, 0)):
    bm = bmesh.new()
    bmesh.ops.create_cube(bm, size=1.0)
    bmesh.ops.scale(bm, vec=size, verts=bm.verts)
    if bevel > 0:
        bmesh.ops.bevel(bm, geom=bm.edges[:], offset=min(bevel, min(size) * 0.3), segments=2, affect="EDGES")
    me = bpy.data.meshes.new("b"); bm.to_mesh(me); bm.free()
    o = bpy.data.objects.new("b", me); o.location = loc; o.rotation_euler = rot
    return add(o, m)

def cyl(r, h, loc, m, rot=(0, 0, 0), segs=28, r2=None):
    bm = bmesh.new()
    bmesh.ops.create_cone(bm, cap_ends=True, segments=segs, radius1=r, radius2=r if r2 is None else r2, depth=h)
    me = bpy.data.meshes.new("c"); bm.to_mesh(me); bm.free()
    o = bpy.data.objects.new("c", me); o.location = loc; o.rotation_euler = rot
    return add(o, m)

def ball(r, loc, m, scale=(1, 1, 1)):
    bm = bmesh.new()
    bmesh.ops.create_uvsphere(bm, u_segments=20, v_segments=12, radius=r)
    bmesh.ops.scale(bm, vec=scale, verts=bm.verts)
    me = bpy.data.meshes.new("s"); bm.to_mesh(me); bm.free()
    o = bpy.data.objects.new("s", me); o.location = loc
    return add(o, m)

def lathe(profile, loc, m, segs=28):
    bm = bmesh.new()
    rings = [[bm.verts.new((r * math.cos(2 * math.pi * i / segs), r * math.sin(2 * math.pi * i / segs), z)) for i in range(segs)] for r, z in profile]
    for k in range(len(rings) - 1):
        for i in range(segs):
            bm.faces.new([rings[k][i], rings[k][(i + 1) % segs], rings[k + 1][(i + 1) % segs], rings[k + 1][i]])
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    me = bpy.data.meshes.new("l"); bm.to_mesh(me); bm.free()
    o = bpy.data.objects.new("l", me); o.location = loc
    return add(o, m)

RX = math.radians(90)
# stand-ins that the user's Tripo props have replaced (see Scripts/import_kitchen_props.py) are built and then removed
SKIP = {"hood", "portholes", "pendants", "board", "crates", "planters", "icecrate", "buoy"}
def drop(tag, start):
    if tag not in SKIP: return
    for o in PARTS[start:]:
        if o is not None: bpy.data.objects.remove(o)
    del PARTS[start:]

# ---------------------------------------------------------------- floor: checker tiles, brass border, runner
box((2 * HW, DEPTH, 0.04), (0, DEPTH / 2, 0.0), "Pearl", 0)
TS = 0.76
nx, ny = int(2 * HW / TS), int(DEPTH / TS)
for ix in range(nx):
    for iy in range(ny):
        if (ix + iy) % 2 == 0:
            box((TS - 0.02, TS - 0.02, 0.012), (-nx * TS / 2 + (ix + 0.5) * TS, (DEPTH - ny * TS) / 2 + (iy + 0.5) * TS, 0.026), "Navy", 0)
box((2 * HW + 0.1, 0.08, 0.07), (0, DEPTH, 0.03), "Brass")
box((6.4, 1.15, 0.012), (0, 2.9, 0.034), "Brass", 0)
box((6.26, 1.01, 0.012), (0, 2.9, 0.04), "Teal", 0)
for sx in (-1, 1):
    for k in range(7):
        box((0.03, 0.9, 0.012), (sx * (3.0 - k * 0.0) + sx * 0.0 - sx * k * 0.0, 2.9, 0.046), "Brass", 0) if k == 0 else None

# ---------------------------------------------------------------- back wall: panelled dado, tile band, scalloped crown
box((2 * HW + 0.6, 0.3, WALL), (0, -0.15, WALL / 2), "Navy", 0.02)
box((2 * HW + 0.5, 0.06, 0.12), (0, 0.03, 0.06), "Brass")
for x in [i * 1.15 - 4.02 for i in range(8)]:
    box((1.0, 0.03, 0.62), (x, 0.015, 0.5), "NavyLight", 0.02)
    box((1.06, 0.02, 0.03), (x, 0.03, 0.83), "Brass", 0.005); box((1.06, 0.02, 0.03), (x, 0.03, 0.17), "Brass", 0.005)
box((2 * HW + 0.5, 0.05, 0.05), (0, 0.03, 0.92), "Brass")
box((2 * HW, 0.04, 1.1), (0, 0.02, 1.5), "Teal", 0)
for z in (1.13, 1.5, 1.87):
    box((2 * HW, 0.012, 0.02), (0, 0.045, z), "Pearl", 0)
x = -HW + 0.37
while x < HW:
    box((0.02, 0.012, 1.1), (x, 0.045, 1.5), "Pearl", 0); x += 0.37
x = -HW + 0.3
while x < HW - 0.1:
    cyl(0.26, 0.03, (x, 0.03, 2.08), "Brass", (RX, 0, 0)); cyl(0.22, 0.04, (x, 0.04, 2.08), "Pearl", (RX, 0, 0)); x += 0.52
box((2 * HW + 0.5, 0.1, 0.1), (0, 0.02, 2.36), "Brass")
box((2 * HW + 0.7, 0.42, 0.12), (0, -0.12, WALL + 0.04), "Brass", 0.03)

_m = len(PARTS)
# two portholes with a glowing sea view, a ship's clock between them
for px in (-1.5, 1.4):
    cyl(0.36, 0.06, (px, 0.05, 1.62), "Brass", (RX, 0, 0), 36)
    cyl(0.29, 0.07, (px, 0.06, 1.62), "SeaGlow", (RX, 0, 0), 36)
    box((0.58, 0.02, 0.03), (px, 0.1, 1.62), "Brass", 0.004); box((0.03, 0.02, 0.58), (px, 0.1, 1.62), "Brass", 0.004)
    for b in range(8):
        a = b * math.pi / 4
        ball(0.025, (px + math.cos(a) * 0.325, 0.09, 1.62 + math.sin(a) * 0.325), "Brass")

drop("portholes", _m)
# ---------------------------------------------------------------- side walls + corner columns with lanterns
# The kitchen stands in the middle of the dining room: a solid back wall, glass partitions on the other three sides
# (navy dado with a brass cap, brass posts, glass panes, a brass top rail) and a gap for the swing doors in front.
DOOR = 0.95   # half width of the doorway
def partition(p0, p1, dado):
    """glazed partition from p0 to p1 (x, y) on a dado of the given height"""
    (x0, y0), (x1, y1) = p0, p1
    L = math.hypot(x1 - x0, y1 - y0); ang = math.atan2(y1 - y0, x1 - x0)
    cx, cy = (x0 + x1) / 2, (y0 + y1) / 2
    box((L, 0.26, dado), (cx, cy, dado / 2), "Navy", 0.02, (0, 0, ang))
    box((L + 0.06, 0.34, 0.08), (cx, cy, dado + 0.03), "Brass", 0.02, (0, 0, ang))
    box((L, 0.05, WALL - dado - 0.1), (cx, cy, (WALL + dado) / 2), "Pane", 0, (0, 0, ang))
    box((L + 0.06, 0.16, 0.1), (cx, cy, WALL), "Brass", 0.02, (0, 0, ang))
    n = max(1, int(round(L / 1.55)))
    for k in range(n + 1):
        t = k / n
        box((0.09, 0.12, WALL - dado), (x0 + (x1 - x0) * t, y0 + (y1 - y0) * t, (WALL + dado) / 2), "Brass", 0.015, (0, 0, ang))
partition((HW + 0.13, 0.0), (HW + 0.13, DEPTH), 0.95)
partition((-HW - 0.13, 0.0), (-HW - 0.13, DEPTH), 1.15)
partition((-HW - 0.13, DEPTH), (-DOOR - 0.12, DEPTH), 0.95)
partition((DOOR + 0.12, DEPTH), (HW + 0.13, DEPTH), 0.95)
# door frame: two stout brass jambs, a header with a shell crest and a lantern
for sx in (-1, 1):
    box((0.16, 0.2, WALL + 0.25), (sx * (DOOR + 0.08), DEPTH, (WALL + 0.25) / 2), "Brass", 0.03)
box((2 * DOOR + 0.5, 0.24, 0.22), (0, DEPTH, WALL + 0.2), "NavyLight", 0.03)
box((2 * DOOR + 0.6, 0.28, 0.06), (0, DEPTH, WALL + 0.33), "Brass", 0.02)
for k in range(7):
    a = math.pi * (k + 0.5) / 7
    box((0.07, 0.06, 0.42), (math.cos(a) * 0.2, DEPTH, WALL + 0.36 + math.sin(a) * 0.2), "Brass", 0.02, (0, a - math.pi / 2, 0))
ball(0.11, (0, DEPTH, WALL + 0.34), "Warm")
for cx in (-HW - 0.13, HW + 0.13):   # front corner posts with pearl finials
    box((0.2, 0.2, WALL + 0.12), (cx, DEPTH, (WALL + 0.12) / 2), "Brass", 0.03)
    ball(0.13, (cx, DEPTH, WALL + 0.22), "Pearl")
for cx in (-HW - 0.15, HW + 0.15):
    lathe([(0.24, 0), (0.24, 0.12), (0.17, 0.2), (0.17, WALL - 0.25), (0.25, WALL - 0.12), (0.27, WALL + 0.02)], (cx, -0.02, 0), "NavyLight")
    cyl(0.28, 0.06, (cx, -0.02, WALL + 0.05), "Brass")
    lathe([(0.0, 0), (0.12, 0.0), (0.14, 0.06), (0.14, 0.3), (0.06, 0.4), (0.0, 0.42)], (cx, -0.02, WALL + 0.08), "Warm", 16)
    for a in range(4):
        box((0.015, 0.015, 0.32), (cx + math.cos(a * math.pi / 2 + 0.78) * 0.14, -0.02 + math.sin(a * math.pi / 2 + 0.78) * 0.14, WALL + 0.24), "Brass", 0)
    cyl(0.16, 0.03, (cx, -0.02, WALL + 0.5), "Brass", r2=0.02)

_m = len(PARTS)
# ---------------------------------------------------------------- hood over the stove (stove: x -3.35, y 0.9, 1.7 wide, 2.1 tall)
SXc = -3.35
lathe_pts = None
bm = bmesh.new()   # tapered canopy
bmesh.ops.create_cube(bm, size=1.0)
for v in bm.verts:
    top = v.co.z > 0
    v.co.x *= (1.2 if top else 1.95); v.co.y *= (0.6 if top else 1.15); v.co.z *= 0.55
me = bpy.data.meshes.new("hood"); bm.to_mesh(me); bm.free()
o = bpy.data.objects.new("hood", me); o.location = (SXc, 0.6, 2.72); add(o, "NavyLight")
box((1.99, 1.19, 0.07), (SXc, 0.6, 2.45), "Brass"); box((1.24, 0.64, 0.05), (SXc, 0.6, 3.0), "Brass")
box((0.7, 0.45, 0.9), (SXc, 0.25, 3.45), "NavyLight", 0.02); box((0.78, 0.52, 0.06), (SXc, 0.25, 3.9), "Brass")
for dx in (-0.55, 0, 0.55):
    cyl(0.09, 0.03, (SXc + dx, 0.7, 2.41), "Warm")
# pan rail in front of the hood with copper pans, a ladle and a strainer
cyl(0.02, 2.0, (SXc, 1.22, 2.36), "Brass", (0, RX, 0), 12)
for i, r in enumerate((0.2, 0.17, 0.14, 0.11)):
    px = SXc - 0.72 + i * 0.44
    cyl(r, 0.05, (px, 1.22, 2.36 - 0.14 - r), "Copper" if i % 2 == 0 else "Iron", (RX, 0, 0))
    box((0.03, 0.02, 0.16), (px, 1.22, 2.3), "Brass", 0.004)
box((0.02, 0.02, 0.4), (SXc + 0.92, 1.22, 2.15), "Brass", 0.004); ball(0.06, (SXc + 0.92, 1.22, 1.93), "Copper", (1, 1, 0.6))

drop("hood", _m)
# ---------------------------------------------------------------- sink unit on the back wall between stove and prep table
SKx = -0.55
box((1.7, 0.7, 0.86), (SKx, 0.38, 0.45), "NavyLight", 0.02)
box((1.76, 0.76, 0.06), (SKx, 0.38, 0.9), "Pearl", 0.015)
box((0.9, 0.46, 0.05), (SKx - 0.2, 0.4, 0.915), "Iron", 0.01)
for dx in (-0.42, 0.42):
    box((0.72, 0.02, 0.6), (SKx + dx, 0.74, 0.45), "Navy", 0.02); ball(0.03, (SKx + dx * 0.25, 0.76, 0.55), "Brass")
    cyl(0.07, 0.02, (SKx + dx, 0.755, 0.3), "Brass", (RX, 0, 0), 20)
cyl(0.03, 0.34, (SKx - 0.2, 0.12, 1.1), "Brass", segs=12); cyl(0.026, 0.26, (SKx - 0.2, 0.25, 1.27), "Brass", (RX, 0, 0), 12)
cyl(0.026, 0.07, (SKx - 0.2, 0.37, 1.24), "Brass", segs=12)
for dx in (-0.36, -0.04):
    ball(0.035, (SKx + dx, 0.12, 0.98), "Brass")
for i in range(4):   # stacked plates + a bowl of lemons on the drainer
    cyl(0.16, 0.02, (SKx + 0.52, 0.4, 0.945 + i * 0.022), "Pearl")
lathe([(0.0, 0.0), (0.1, 0.0), (0.17, 0.09), (0.16, 0.1), (0.09, 0.02), (0.0, 0.02)], (SKx + 0.52, 0.16, 0.93), "Copper", 20)
for i in range(4):
    ball(0.045, (SKx + 0.52 + math.cos(i * 1.6) * 0.06, 0.16 + math.sin(i * 1.6) * 0.06, 1.0 + (i % 2) * 0.03), "Yellow", (1, 0.8, 0.8))

# ---------------------------------------------------------------- wall shelves above the sink: jars, bottles, copper moulds
for z in (1.18, 1.62):
    pass
for sh, z in enumerate((2.52,)):
    pass
for z, items in ((1.22, "jars"),):
    pass
SHY = 0.16
for z in (1.02,):
    pass
def shelf(xc, z, w):
    box((w, 0.26, 0.035), (xc, SHY, z), "Wood", 0.008)
    for dx in (-w / 2 + 0.08, w / 2 - 0.08):
        box((0.03, 0.2, 0.16), (xc + dx, SHY - 0.02, z - 0.09), "Brass", 0.006)
shelf(SKx + 0.1, 2.2, 1.9)
for i in range(6):
    jx = SKx - 0.68 + i * 0.3
    h = rng.uniform(0.16, 0.26)
    cyl(0.075, h, (jx, SHY, 2.22 + h / 2), "Glass", segs=16); cyl(0.08, 0.03, (jx, SHY, 2.24 + h), "Brass", segs=16)
    cyl(0.068, h * 0.6, (jx, SHY, 2.22 + h * 0.3), ("Red", "Yellow", "Green", "Cream", "Copper", "Rope")[i], segs=16)
# strings of garlic / chillies / herbs hanging under the crown
for i, (m, n) in enumerate((("Cream", 6), ("Red", 8), ("Green", 5), ("Cream", 6))):
    hx = 2.6 + i * 0.42
    box((0.012, 0.012, 0.5), (hx, 0.1, 2.05), "Rope", 0)
    for k in range(n):
        ball(0.045 if m != "Red" else 0.03, (hx + (k % 2 - 0.5) * 0.06, 0.12, 2.26 - k * 0.06), m, (1, 1, 1.4 if m == "Red" else 1))

# ---------------------------------------------------------------- pot rack: an oval brass frame on chains over the runner, pots hanging from it
RKy, RKz = 1.7, 3.85   # high and towards the back wall: clear of the third-person camera behind the cook
for a in range(24):
    t = a * math.pi / 12
    box((0.2, 0.03, 0.03), (math.cos(t) * 1.5, RKy + math.sin(t) * 0.55, RKz), "Brass", 0.005, (0, 0, t + math.pi / 2))
for sx in (-1.2, 1.2):
    box((0.012, 0.012, 0.9), (sx, RKy, RKz + 0.45), "Iron", 0)
for i in range(7):
    t = i * 2 * math.pi / 7
    px, py = math.cos(t) * 1.5, RKy + math.sin(t) * 0.55
    box((0.012, 0.012, 0.18), (px, py, RKz - 0.09), "Brass", 0)
    r = (0.16, 0.12, 0.19, 0.13, 0.15, 0.11, 0.17)[i]
    lathe([(0.0, 0.0), (r * 0.8, 0.0), (r, 0.05), (r, r * 1.1), (r * 0.93, r * 1.1), (r * 0.93, 0.06), (0.0, 0.04)], (px, py, RKz - 0.2 - r * 1.1), "Copper" if i % 3 else "Iron", 18)
_m = len(PARTS)
# three pendant lamps over the island (island: y 4.7)
for dx in (-1.25, 0, 1.25):
    box((0.012, 0.012, 0.9), (dx, 4.7, 4.05), "Brass", 0)
    lathe([(0.03, 0.34), (0.06, 0.3), (0.2, 0.06), (0.22, 0.0), (0.2, 0.0), (0.05, 0.27)], (dx, 4.7, 3.3), "Brass", 20)
    ball(0.075, (dx, 4.7, 3.36), "Warm")

drop("pendants", _m)
# ---------------------------------------------------------------- right wall: menu chalkboard, lifebuoy; produce crates and a barrel below
_m = len(PARTS)
box((0.05, 1.5, 0.95), (HW - 0.03, 1.2, 1.75), "Wood", 0.02); box((0.03, 1.36, 0.81), (HW - 0.06, 1.2, 1.75), "Slate", 0.005)
for k in range(5):
    box((0.012, rng.uniform(0.5, 1.0), 0.025), (HW - 0.08, 1.2 - 0.1, 2.02 - k * 0.14), "Cream", 0)
drop("board", _m)
_m = len(PARTS)
lathe([(0.24, -0.06), (0.3, -0.03), (0.32, 0.0), (0.3, 0.03), (0.24, 0.06), (0.2, 0.03), (0.18, 0.0), (0.2, -0.03), (0.24, -0.06)], (HW - 0.06, 5.0, 1.8), "Cream", 28).rotation_euler = (0, RX, 0)
for a in range(4):
    t = a * math.pi / 2
    box((0.08, 0.14, 0.14), (HW - 0.06, 5.0 + math.cos(t) * 0.26, 1.8 + math.sin(t) * 0.26), "Red", 0.02)
drop("buoy", _m)
_m = len(PARTS)
def crate(x, y, z, fill, n=7):
    box((0.62, 0.46, 0.04), (x, y, z + 0.02), "WoodLight", 0.006)
    for sy in (-0.21, 0.21):
        for k in range(2):
            box((0.62, 0.03, 0.1), (x, y + sy, z + 0.1 + k * 0.13), "WoodLight", 0.006)
    for sxx in (-0.29, 0.29):
        for k in range(2):
            box((0.03, 0.4, 0.1), (x + sxx, y, z + 0.1 + k * 0.13), "WoodLight", 0.006)
    for k in range(n):
        ball(0.075, (x + rng.uniform(-0.2, 0.2), y + rng.uniform(-0.12, 0.12), z + 0.24 + rng.uniform(0, 0.06)), fill, (1, 1, 0.85))
crate(4.15, 5.3, 0.02, "Yellow"); crate(4.15, 5.3, 0.33, "Green"); crate(4.12, 5.9, 0.02, "Red")
lathe([(0.0, 0.0), (0.26, 0.0), (0.32, 0.25), (0.33, 0.45), (0.32, 0.65), (0.26, 0.9), (0.0, 0.9)], (3.4, 5.75, 0.02), "Wood", 24)
for z in (0.14, 0.45, 0.78):
    lathe([(0.3 if z != 0.45 else 0.335, 0.0), (0.31 if z != 0.45 else 0.345, 0.02), (0.3 if z != 0.45 else 0.335, 0.04)], (3.4, 5.75, z), "Brass", 24)
drop("crates", _m)
for i in range(3):   # sacks of rice against the right wall
    ball(0.3, (4.15, 4.35 - i * 0.02, 0.26 + i * 0.36), "Rope", (1.0, 1.25, 0.62))

_m = len(PARTS)
# ---------------------------------------------------------------- left low wall: herb planters; an ice crate with the day's catch by the fridge
for i in range(4):
    py = 0.5 + i * 0.85
    box((0.3, 0.6, 0.16), (-HW - 0.15, py, 1.3), "Copper", 0.02)
    for k in range(7):
        ball(0.07, (-HW - 0.15 + rng.uniform(-0.08, 0.08), py + rng.uniform(-0.22, 0.22), 1.42 + rng.uniform(0, 0.08)), "Green", (1, 1, 1.5))
box((0.9, 0.6, 0.32), (-4.0, 4.3, 0.18), "WoodLight", 0.02); box((0.82, 0.52, 0.06), (-4.0, 4.3, 0.34), "Pearl", 0.02)
for k in range(3):
    ball(0.16, (-4.2 + k * 0.2, 4.3 + (k % 2) * 0.1 - 0.05, 0.42), ("NavyLight", "Copper", "Pearl")[k], (1.6, 0.5, 0.35))
drop("planters", _m)
# a step stool and a mop bucket tucked at the back left corner
box((0.42, 0.34, 0.04), (-4.25, 3.4, 0.42), "Wood", 0.01)
for sx in (-0.17, 0.17):
    for sy in (-0.13, 0.13):
        box((0.04, 0.04, 0.4), (-4.25 + sx, 3.4 + sy, 0.2), "Wood", 0.006)

# open brass canopy over the glazed room: posts carried up from the corners, a ring beam and cross beams.
# The pendants and the pot rack hang from it, so nothing floats above the glass any more.
CZ = 4.45
for cx in (-HW - 0.13, HW + 0.13):
    for cy in (0.0, DEPTH):
        box((0.1, 0.1, CZ - WALL), (cx, cy, (CZ + WALL) / 2), "Brass", 0.02)
for cy in (0.0, DEPTH):
    box((2 * HW + 0.4, 0.12, 0.12), (0, cy, CZ), "Brass", 0.02)
for cx in (-HW - 0.13, HW + 0.13):
    box((0.12, DEPTH + 0.1, 0.12), (cx, DEPTH / 2, CZ), "Brass", 0.02)
for cy in (1.7, 4.7):
    box((2 * HW + 0.3, 0.09, 0.09), (0, cy, CZ), "Brass", 0.02)
for cx in (-2.3, 0.0, 2.3):
    box((0.07, DEPTH, 0.07), (cx, DEPTH / 2, CZ + 0.02), "Brass", 0.02)
for cx in (-HW - 0.13, HW + 0.13):
    for cy in (0.0, DEPTH):
        ball(0.11, (cx, cy, CZ + 0.14), "Pearl")
for dx in (-1.25, 0, 1.25):   # rods down to the three pendant lamps over the island
    box((0.02, 0.02, CZ - 4.5 + 0.9), (dx, 4.7, CZ - 0.45), "Brass", 0)

# outside of the back wall (it faces the display tank and the booths): panelling, a big shell emblem, two lanterns
for x in [i * 1.5 - 3.75 for i in range(6)]:
    box((1.3, 0.03, 0.7), (x, -0.315, 0.55), "NavyLight", 0.02)
    box((1.36, 0.02, 0.03), (x, -0.33, 0.92), "Brass", 0.005); box((1.36, 0.02, 0.03), (x, -0.33, 0.18), "Brass", 0.005)
box((2 * HW + 0.5, 0.05, 0.06), (0, -0.32, 1.02), "Brass")
box((2 * HW + 0.5, 0.06, 0.12), (0, -0.33, 0.06), "Brass")
box((2 * HW + 0.5, 0.05, 0.08), (0, -0.32, 2.36), "Brass")
for k in range(9):   # scallop shell: nine ribs fanning from a pearl
    a = math.pi * (k + 0.5) / 9
    box((0.1, 0.05, 0.62), (2.2 + math.cos(a) * 0.3, -0.33, 1.45 + math.sin(a) * 0.3), "Brass" if k % 2 == 0 else "Pearl", 0.02, (0, a - math.pi / 2, 0))
ball(0.1, (2.2, -0.36, 1.42), "Pearl")
for lx in (-4.1, 0.6, 4.1):
    box((0.1, 0.12, 0.06), (lx, -0.36, 1.95), "Brass", 0.01)
    lathe([(0.0, 0), (0.09, 0.0), (0.11, 0.05), (0.11, 0.24), (0.05, 0.32), (0.0, 0.33)], (lx, -0.44, 1.6), "Warm", 14)
    cyl(0.13, 0.03, (lx, -0.44, 1.95), "Brass", segs=16)

PARTS = [p for p in PARTS if p is not None]
print("KITCHEN parts", len(PARTS))

# ---------------------------------------------------------------- preview (with stand-in blocks for the five Tripo appliances)
try:
    sc.render.engine = "BLENDER_EEVEE"
except TypeError:
    sc.render.engine = "BLENDER_EEVEE_NEXT"
w = bpy.data.worlds.new("W"); sc.world = w; w.use_nodes = True
w.node_tree.nodes["Background"].inputs["Color"].default_value = (0.05, 0.12, 0.25, 1)
w.node_tree.nodes["Background"].inputs["Strength"].default_value = 0.9
ghost = bpy.data.materials.new("Ghost"); ghost.use_nodes = True
ghost.node_tree.nodes["Principled BSDF"].inputs["Base Color"].default_value = (0.55, 0.5, 0.42, 1)
STAND = []
for name, size, loc in (("stove", (1.7, 0.9, 2.1), (-3.35, 0.9, 1.05)), ("prep", (1.5, 0.8, 1.9), (2.45, 0.9, 0.95)), ("island", (2.4, 0.9, 1.0), (0, 4.7, 0.5)),
                        ("shelf", (0.6, 1.4, 2.0), (4.0, 3.4, 1.0)), ("fridge", (0.95, 1.4, 1.25), (-4.02, 2.6, 0.62))):
    bm = bmesh.new(); bmesh.ops.create_cube(bm, size=1.0); bmesh.ops.scale(bm, vec=size, verts=bm.verts)
    me = bpy.data.meshes.new(name); bm.to_mesh(me); bm.free()
    o = bpy.data.objects.new("STANDIN_" + name, me); o.location = loc; o.data.materials.append(ghost); sc.collection.objects.link(o); STAND.append(o)
for loc, e, col in (((0, 4.0, 5.0), 900, (1, 0.85, 0.65)), ((-3, 2, 3.5), 300, (1, 0.8, 0.6)), ((3, 2, 3.5), 300, (0.6, 0.8, 1))):
    l = bpy.data.objects.new("L", bpy.data.lights.new("L", "POINT")); l.data.energy = e; l.data.color = col; l.location = loc; sc.collection.objects.link(l)
cam = bpy.data.objects.new("Cam", bpy.data.cameras.new("Cam")); sc.collection.objects.link(cam); sc.camera = cam
cam.data.lens = 24
def shot(name, loc, target, res=(1600, 900)):
    cam.location = loc
    cam.rotation_euler = (Vector(target) - Vector(loc)).to_track_quat("-Z", "Y").to_euler()
    sc.render.resolution_x, sc.render.resolution_y = res
    sc.render.filepath = os.path.join(OUT, name)
    bpy.ops.render.render(write_still=True)
shot("preview_kitchen_front.png", (0.5, 12.5, 4.6), (0, 2.2, 1.3))
shot("preview_kitchen_left.png", (-7.5, 9.5, 4.2), (0.5, 2.0, 1.2))
shot("preview_kitchen_top.png", (0, 3.1, 13.0), (0, 3.15, 0), (1400, 1000))
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT, "KitchenSet.blend"))

# ---------------------------------------------------------------- export: one mesh, slots named after the materials, mirrored in Y for Unreal
for o in STAND:
    bpy.data.objects.remove(o)
bpy.ops.object.select_all(action="DESELECT")
for p in PARTS:
    p.select_set(True)
bpy.context.view_layer.objects.active = PARTS[0]
bpy.ops.object.join()
o = bpy.context.view_layer.objects.active
o.name = "SM_KitchenSet"
# (no mirroring: with this exporter setup Unreal keeps +Y as the depth axis)
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
bpy.ops.export_scene.fbx(filepath=os.path.join(OUT, "SM_KitchenSet.fbx"), use_selection=True, apply_unit_scale=True, mesh_smooth_type="FACE", path_mode="STRIP")
print("KITCHEN exported tris", sum(len(p.vertices) - 2 for p in o.data.polygons), "slots", [m.name for m in o.data.materials], [round(d, 2) for d in o.dimensions])
