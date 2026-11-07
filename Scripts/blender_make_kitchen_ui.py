# Blender (headless): the Papa's-style kitchen counter picture + anchors, chef avatar, submarine icon.
#   blender -b -P blender_make_kitchen_ui.py -- <ui_out_dir> <chef.fbx> <sub.fbx>
# Writes UI_Counter.png (1920x700, transparent above the counter), counter_anchors.json (pixel coords in that image),
# UI_Avatar.png (chef head), UI_SubIcon.png
import sys, os, math, json, random
import bpy, bmesh
from mathutils import Vector, Matrix
from bpy_extras.object_utils import world_to_camera_view

argv = sys.argv[sys.argv.index("--") + 1:]
OUT, CHEF_FBX, SUB_FBX = argv[0], argv[1], argv[2]
os.makedirs(OUT, exist_ok=True)
rng = random.Random(4)


def reset():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    sc = bpy.context.scene
    try:
        sc.render.engine = "BLENDER_EEVEE"
    except TypeError:
        sc.render.engine = "BLENDER_EEVEE_NEXT"
    sc.render.film_transparent = True
    sc.view_settings.view_transform = "Standard"
    w = bpy.data.worlds.new("W"); sc.world = w
    w.use_nodes = True
    w.node_tree.nodes["Background"].inputs["Color"].default_value = (0.35, 0.45, 0.6, 1)
    w.node_tree.nodes["Background"].inputs["Strength"].default_value = 0.8
    return sc


MATS = {}
def mat(name, color, rough=0.5, metal=0.0, emit=0.0, noise=0.0, wave=False, brushed=False):
    key = name
    if key in MATS:
        return MATS[key]
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    nt = m.node_tree
    b = nt.nodes.get("Principled BSDF")
    b.inputs["Base Color"].default_value = (*color, 1)
    b.inputs["Roughness"].default_value = rough
    b.inputs["Metallic"].default_value = metal
    if emit > 0:
        b.inputs["Emission Color"].default_value = (*color, 1)
        b.inputs["Emission Strength"].default_value = emit
    if noise > 0 or wave or brushed:
        tc = nt.nodes.new("ShaderNodeTexCoord")
        ramp = nt.nodes.new("ShaderNodeValToRGB")
        if wave:
            t = nt.nodes.new("ShaderNodeTexWave"); t.bands_direction = "X"
            t.inputs["Scale"].default_value = 5.0; t.inputs["Distortion"].default_value = 7.0
        elif brushed:
            t = nt.nodes.new("ShaderNodeTexNoise")
            t.inputs["Scale"].default_value = 60.0
            mp = nt.nodes.new("ShaderNodeMapping")
            mp.inputs["Scale"].default_value = (1.0, 40.0, 1.0)
            nt.links.new(tc.outputs["Object"], mp.inputs["Vector"])
            nt.links.new(mp.outputs["Vector"], t.inputs["Vector"])
        else:
            t = nt.nodes.new("ShaderNodeTexNoise"); t.inputs["Scale"].default_value = 25.0
        if not brushed:
            nt.links.new(tc.outputs["Object"], t.inputs["Vector"])
        dark = noise if noise > 0 else (0.35 if wave else 0.15)
        ramp.color_ramp.elements[0].color = (*(c * (1 - dark) for c in color), 1)
        ramp.color_ramp.elements[1].color = (*color, 1)
        nt.links.new(t.outputs["Fac"], ramp.inputs["Fac"])
        nt.links.new(ramp.outputs["Color"], b.inputs["Base Color"])
    MATS[key] = m
    return m


def link(o):
    bpy.context.scene.collection.objects.link(o)
    return o


def box(name, size, loc, m, bevel=0.01):
    bm = bmesh.new()
    bmesh.ops.create_cube(bm, size=1.0)
    bmesh.ops.scale(bm, vec=size, verts=bm.verts)
    me = bpy.data.meshes.new(name); bm.to_mesh(me); bm.free()
    o = link(bpy.data.objects.new(name, me))
    o.location = loc
    o.data.materials.append(m)
    if bevel:
        bv = o.modifiers.new("b", "BEVEL"); bv.width = bevel; bv.segments = 3
    return o


def cyl(name, r, h, loc, m, verts=48, r2=None):
    bm = bmesh.new()
    bmesh.ops.create_cone(bm, cap_ends=True, segments=verts, radius1=r, radius2=r if r2 is None else r2, depth=h)
    me = bpy.data.meshes.new(name); bm.to_mesh(me); bm.free()
    o = link(bpy.data.objects.new(name, me))
    o.location = loc
    o.data.materials.append(m)
    return o


def torus(name, R, r, loc, m):
    bpy.ops.mesh.primitive_torus_add(major_radius=R, minor_radius=r, location=loc, major_segments=48, minor_segments=10)
    o = bpy.context.active_object
    o.data.materials.append(m)
    return o


def lathe(name, prof, loc, m, segs=48):
    bm = bmesh.new()
    rings = [[bm.verts.new((r * math.cos(2 * math.pi * i / segs), r * math.sin(2 * math.pi * i / segs), z)) for i in range(segs)] for r, z in prof]
    for k in range(len(rings) - 1):
        for i in range(segs):
            bm.faces.new([rings[k][i], rings[k][(i + 1) % segs], rings[k + 1][(i + 1) % segs], rings[k + 1][i]])
    me = bpy.data.meshes.new(name); bm.to_mesh(me); bm.free()
    o = link(bpy.data.objects.new(name, me))
    o.location = loc
    o.data.materials.append(m)
    o.modifiers.new("s", "SUBSURF").levels = 1
    return o


def shade_smooth_all():
    for o in bpy.context.scene.objects:
        if o.type == "MESH":
            for p in o.data.polygons:
                p.use_smooth = True


# ================================================================= 1) the counter
sc = reset()
STEEL = mat("Steel", (0.55, 0.57, 0.6), 0.22, 1.0, brushed=True)
STEEL_D = mat("SteelDark", (0.2, 0.22, 0.25), 0.35, 1.0, noise=0.2)
BRASS = mat("Brass", (0.85, 0.62, 0.28), 0.25, 1.0)
WOOD = mat("Wood", (0.42, 0.25, 0.12), 0.55, wave=True)
BLACK = mat("Black", (0.03, 0.03, 0.035), 0.3, 0.3)
ICE = mat("Ice", (0.55, 0.68, 0.78), 0.2, 0.0, noise=0.35)
FLAME = mat("Flame", (0.2, 0.5, 1.0), 0.3, 0.0, emit=6.0)
CERAMIC = mat("Ceramic", (0.95, 0.95, 0.93), 0.15)
RIM = mat("Rim", (0.15, 0.32, 0.62), 0.2)
GLASS = mat("Glass", (0.1, 0.32, 0.36), 0.08, 0.0)
ENAMEL = mat("Enamel", (0.015, 0.05, 0.16), 0.35, 0.0)
CREAM = mat("Cream", (0.92, 0.86, 0.72), 0.4)
SLATE = mat("Slate", (0.08, 0.09, 0.1), 0.6, noise=0.3)
TOWEL = mat("Towel", (0.12, 0.14, 0.28), 0.9)

TOP = 0.9
box("CounterTop", (4.6, 1.1, 0.06), (0, 0.45, TOP - 0.03), STEEL, 0.015)
box("TopEdge", (4.62, 0.04, 0.03), (0, -0.1, TOP + 0.002), BRASS, 0.008)
box("CounterFront", (4.6, 0.06, 0.9), (0, -0.08, 0.45), ENAMEL, 0.01)
box("FrontTrimTop", (4.62, 0.07, 0.035), (0, -0.09, TOP - 0.03), BRASS, 0.008)
box("FrontTrimBot", (4.62, 0.07, 0.05), (0, -0.09, 0.04), BRASS, 0.008)
for k in range(9):
    cyl("Shell%d" % k, 0.022, 0.01, (-2.0 + k * 0.5, -0.115, 0.62), BRASS, 12).rotation_euler = (math.radians(90), 0, 0)
box("Rail", (4.6, 0.04, 0.04), (0, -0.13, TOP - 0.08), BRASS, 0.01)
for x in ():                                   # rivets on the front
    for z in (0.15, 0.7):
        cyl("Rivet", 0.018, 0.02, (x, -0.115, z), BRASS, 16).rotation_euler = (math.radians(90), 0, 0)

# stove (left)
box("Stove", (1.3, 0.8, 0.08), (-1.42, 0.4, TOP + 0.03), STEEL_D, 0.01)
box("StoveTrim", (1.32, 0.03, 0.09), (-1.42, 0.0, TOP + 0.03), BRASS, 0.008)
PANS = []
for i, x in enumerate((-1.72, -1.12)):
    torus("Burner%d" % i, 0.17, 0.012, (x, 0.42, TOP + 0.08), BLACK)
    torus("Flame%d" % i, 0.13, 0.008, (x, 0.42, TOP + 0.085), FLAME)
    # frying pan
    pan = lathe("Pan%d" % i, [(0.0, 0.0), (0.21, 0.0), (0.24, 0.035), (0.255, 0.05), (0.245, 0.052), (0.225, 0.02), (0.0, 0.012)], (x, 0.42, TOP + 0.1), BLACK)
    h = box("PanHandle%d" % i, (0.04, 0.24, 0.025), (x, 0.42 - 0.36, TOP + 0.135), BLACK, 0.008)
    PANS.append(Vector((x, 0.42, TOP + 0.12)))
for i, x in enumerate((-1.85, -1.6, -1.25, -1.0)):                        # knobs on the stove front
    k = cyl("Knob%d" % i, 0.035, 0.03, (x, 0.0, TOP + 0.03), STEEL, 24)
    k.rotation_euler = (math.radians(90), 0, 0)

# cutting board (center) + knife resting on it is drawn by the game
box("Board", (1.1, 0.5, 0.045), (0.0, 0.32, TOP + 0.02), WOOD, 0.02)
BOARD = [Vector((-0.55, 0.07, TOP + 0.045)), Vector((0.55, 0.57, TOP + 0.045))]

# ingredient trays with ice (back row)
BINS = []
for i in range(5):
    x = -0.55 + i * 0.36
    box("Tray%d" % i, (0.33, 0.24, 0.07), (x, 0.82, TOP + 0.035), STEEL, 0.008)
    box("Ice%d" % i, (0.29, 0.2, 0.02), (x, 0.82, TOP + 0.07), ICE, 0.01)
    BINS.append(Vector((x, 0.82, TOP + 0.08)))

# plating mat (right) + plate stack + bell + bottles + towel
cyl("PlateMat", 0.27, 0.01, (1.05, 0.33, TOP + 0.005), SLATE, 48)
PLATE = Vector((1.05, 0.33, TOP + 0.01))
for i in range(6):
    lathe("Stack%d" % i, [(0.0, 0.0), (0.12, 0.0), (0.16, 0.012), (0.17, 0.016), (0.0, 0.01)], (1.75, 0.72, TOP + i * 0.018), CERAMIC if i % 2 == 0 else RIM)
cyl("BellBase", 0.11, 0.025, (1.55, 0.3, TOP + 0.012), STEEL_D, 32)
lathe("Bell", [(0.0, 0.11), (0.03, 0.105), (0.07, 0.08), (0.095, 0.04), (0.1, 0.0)], (1.55, 0.3, TOP + 0.025), BRASS)
cyl("BellButton", 0.012, 0.04, (1.55, 0.3, TOP + 0.15), BRASS, 16)
BELL = Vector((1.55, 0.3, TOP + 0.08))
for i in range(4):
    lathe("Bottle%d" % i, [(0.0, 0.0), (0.035, 0.0), (0.035, 0.14), (0.015, 0.19), (0.012, 0.24), (0.0, 0.24)], (1.22 + i * 0.1, 0.92, TOP), GLASS)
    cyl("Cap%d" % i, 0.014, 0.03, (1.22 + i * 0.1, 0.92, TOP + 0.25), BRASS, 12)
box("Towel", (0.4, 0.06, 0.32), (-0.75, -0.12, TOP - 0.17), TOWEL, 0.02)
shade_smooth_all()

# lights + camera
sun = link(bpy.data.objects.new("Key", bpy.data.lights.new("Key", "AREA")))
sun.data.energy = 260; sun.data.size = 3.0; sun.data.color = (1.0, 0.86, 0.72)
sun.location = (0.0, -0.6, 3.2); sun.rotation_euler = (math.radians(20), 0, 0)
fill = link(bpy.data.objects.new("Fill", bpy.data.lights.new("Fill", "AREA")))
fill.data.energy = 90; fill.data.size = 4.0; fill.data.color = (0.7, 0.6, 1.0)
fill.location = (-2.5, -2.0, 1.8); fill.rotation_euler = (math.radians(65), 0, math.radians(-50))
cam = link(bpy.data.objects.new("Cam", bpy.data.cameras.new("Cam")))
cam.data.lens = 24
cam.location = (0.0, -2.35, 2.05)
target = Vector((0.0, 0.5, TOP))
cam.rotation_euler = (target - cam.location).to_track_quat("-Z", "Y").to_euler()
sc.camera = cam
sc.render.resolution_x = 1920
sc.render.resolution_y = 700
sc.render.filepath = os.path.join(OUT, "UI_Counter.png")
bpy.ops.render.render(write_still=True)


def px(v):
    p = world_to_camera_view(sc, cam, v)
    return [round(p.x * 1920, 1), round((1 - p.y) * 700, 1)]

anchors = {
    "pans": [px(p) for p in PANS],
    "pan_radius_px": round(abs(px(PANS[0] + Vector((0.22, 0, 0)))[0] - px(PANS[0])[0]), 1),
    "board": [px(BOARD[0]), px(Vector((BOARD[1].x, BOARD[0].y, BOARD[0].z))), px(BOARD[1]), px(Vector((BOARD[0].x, BOARD[1].y, BOARD[0].z)))],
    "bins": [px(b) for b in BINS],
    "plate": px(PLATE),
    "plate_radius_px": round(abs(px(PLATE + Vector((0.25, 0, 0)))[0] - px(PLATE)[0]), 1),
    "bell": px(BELL),
}
json.dump(anchors, open(os.path.join(OUT, "counter_anchors.json"), "w"), indent=1)
print("ANCHORS", json.dumps(anchors))

# ================================================================= 2) chef avatar (head close-up)
sc = reset()
bpy.ops.import_scene.fbx(filepath=CHEF_FBX)
meshes = [o for o in sc.objects if o.type == "MESH"]
mn = Vector((1e9,) * 3); mx = Vector((-1e9,) * 3)
for o in meshes:
    for c in o.bound_box:
        w = o.matrix_world @ Vector(c); mn = Vector(map(min, mn, w)); mx = Vector(map(max, mx, w))
Hh = mx.z - mn.z
head = Vector(((mn.x + mx.x) / 2, (mn.y + mx.y) / 2, mn.z + Hh * 0.84))
cam = link(bpy.data.objects.new("Cam", bpy.data.cameras.new("Cam")))
cam.data.type = "ORTHO"; cam.data.ortho_scale = Hh * 0.36
cam.location = head + Vector((0.25 * Hh, -2.0 * Hh, 0.05 * Hh))
cam.rotation_euler = (head - cam.location).to_track_quat("-Z", "Y").to_euler()
sc.camera = cam
k = link(bpy.data.objects.new("Key", bpy.data.lights.new("Key", "AREA")))
k.data.energy = 60 * Hh * Hh; k.data.size = Hh
k.location = head + Vector((0.6 * Hh, -1.2 * Hh, 0.8 * Hh)); k.rotation_euler = (head - k.location).to_track_quat("-Z", "Y").to_euler()
for m in bpy.data.materials:                                             # Tripo face is inside-out
    m.use_backface_culling = False
sc.render.resolution_x = 320; sc.render.resolution_y = 320
sc.render.filepath = os.path.join(OUT, "UI_Avatar.png")
bpy.ops.render.render(write_still=True)

# ================================================================= 3) submarine icon
sc = reset()
bpy.ops.import_scene.fbx(filepath=SUB_FBX)
meshes = [o for o in sc.objects if o.type == "MESH"]
mn = Vector((1e9,) * 3); mx = Vector((-1e9,) * 3)
for o in meshes:
    for c in o.bound_box:
        w = o.matrix_world @ Vector(c); mn = Vector(map(min, mn, w)); mx = Vector(map(max, mx, w))
ctr = (mn + mx) / 2; size = (mx - mn).length
cam = link(bpy.data.objects.new("Cam", bpy.data.cameras.new("Cam")))
cam.data.type = "ORTHO"; cam.data.ortho_scale = size * 0.85
cam.location = ctr + Vector((size * 0.9, -size * 1.4, size * 0.6))
cam.rotation_euler = (ctr - cam.location).to_track_quat("-Z", "Y").to_euler()
sc.camera = cam
k = link(bpy.data.objects.new("Key", bpy.data.lights.new("Key", "SUN")))
k.data.energy = 4.0; k.rotation_euler = (math.radians(50), 0, math.radians(30))
sc.render.resolution_x = 360; sc.render.resolution_y = 300
sc.render.filepath = os.path.join(OUT, "UI_SubIcon.png")
bpy.ops.render.render(write_still=True)
print("KITCHEN UI DONE")
