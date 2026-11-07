# Blender (headless): food models (FBX) + rendered UI pictures (PNG) for the cooking minigame.
#   blender -b -P blender_make_food.py -- <out_dir>
# Writes <out>/models/SM_*.fbx  and  <out>/ui/*.png
import sys, os, math, random
import bpy, bmesh
from mathutils import Vector, Matrix

argv = sys.argv[sys.argv.index("--") + 1:]
OUT = argv[0]
MOD = os.path.join(OUT, "models"); UI = os.path.join(OUT, "ui")
os.makedirs(MOD, exist_ok=True); os.makedirs(UI, exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
scene = bpy.context.scene
rng = random.Random(3)

# ------------------------------------------------------------------ materials
MATS = {}
def pmat(name, color, rough=0.5, metal=0.0, sss=0.0, wood=False, stripes=None, noise_dark=0.0, emit=0.0):
    if name in MATS:
        return MATS[name]
    m = bpy.data.materials.new(name)
    m.diffuse_color = (*color, 1)
    m.use_nodes = True
    nt = m.node_tree
    bsdf = nt.nodes.get("Principled BSDF")
    bsdf.inputs["Base Color"].default_value = (*color, 1)
    bsdf.inputs["Roughness"].default_value = rough
    bsdf.inputs["Metallic"].default_value = metal
    if sss > 0 and "Subsurface Weight" in bsdf.inputs:
        bsdf.inputs["Subsurface Weight"].default_value = sss
    if emit > 0:
        bsdf.inputs["Emission Color"].default_value = (*color, 1)
        bsdf.inputs["Emission Strength"].default_value = emit
    col_out = None
    if wood or stripes or noise_dark > 0:
        tc = nt.nodes.new("ShaderNodeTexCoord")
        ramp = nt.nodes.new("ShaderNodeValToRGB")
        if wood:
            w = nt.nodes.new("ShaderNodeTexWave")
            w.wave_type = "BANDS"; w.bands_direction = "X"
            w.inputs["Scale"].default_value = 6.0
            w.inputs["Distortion"].default_value = 6.0
            w.inputs["Detail"].default_value = 3.0
            nt.links.new(tc.outputs["Object"], w.inputs["Vector"])
            src = w.outputs["Fac"]
            a, b = color, tuple(c * 0.62 for c in color)
        elif stripes:
            w = nt.nodes.new("ShaderNodeTexWave")
            w.wave_type = "BANDS"; w.bands_direction = "Y"
            w.inputs["Scale"].default_value = stripes[0]
            w.inputs["Distortion"].default_value = 1.5
            nt.links.new(tc.outputs["Object"], w.inputs["Vector"])
            src = w.outputs["Fac"]
            a, b = color, stripes[1]
        else:
            n = nt.nodes.new("ShaderNodeTexNoise")
            n.inputs["Scale"].default_value = 30.0
            nt.links.new(tc.outputs["Object"], n.inputs["Vector"])
            src = n.outputs["Fac"]
            a, b = color, tuple(c * (1 - noise_dark) for c in color)
        ramp.color_ramp.elements[0].color = (*b, 1)
        ramp.color_ramp.elements[1].color = (*a, 1)
        if stripes:
            ramp.color_ramp.elements[0].position = 0.75
            ramp.color_ramp.elements[1].position = 0.9
            ramp.color_ramp.elements[0].color = (*a, 1)
            ramp.color_ramp.elements[1].color = (*b, 1)
        nt.links.new(src, ramp.inputs["Fac"])
        nt.links.new(ramp.outputs["Color"], bsdf.inputs["Base Color"])
    MATS[name] = m
    return m

M = {
    "Rice": pmat("MF_Rice", (0.93, 0.92, 0.86), 0.7, noise_dark=0.12),
    "Yellowtail": pmat("MF_Yellowtail", (0.98, 0.78, 0.62), 0.35, sss=0.3, stripes=(14.0, (0.95, 0.55, 0.5))),
    "Tuna": pmat("MF_TunaRed", (0.72, 0.1, 0.14), 0.35, sss=0.2, stripes=(10.0, (0.92, 0.6, 0.62))),
    "Seared": pmat("MF_Seared", (0.38, 0.2, 0.1), 0.6, noise_dark=0.35),
    "Grouper": pmat("MF_Grouper", (0.95, 0.9, 0.82), 0.45, noise_dark=0.06),
    "Grill": pmat("MF_Grill", (0.12, 0.06, 0.03), 0.7),
    "Lemon": pmat("MF_Lemon", (0.98, 0.82, 0.1), 0.4),
    "LemonFlesh": pmat("MF_LemonFlesh", (1.0, 0.93, 0.45), 0.3, sss=0.4),
    "Greens": pmat("MF_Greens", (0.25, 0.62, 0.15), 0.5),
    "Kelp": pmat("MF_Kelp", (0.1, 0.4, 0.12), 0.3),
    "Wasabi": pmat("MF_Wasabi", (0.55, 0.75, 0.25), 0.6, noise_dark=0.15),
    "Sesame": pmat("MF_Sesame", (0.95, 0.88, 0.7), 0.5),
    "Plate": pmat("MF_Plate", (0.95, 0.95, 0.93), 0.15),
    "PlateRim": pmat("MF_PlateRim", (0.12, 0.3, 0.6), 0.2),
    "Slate": pmat("MF_Slate", (0.08, 0.08, 0.09), 0.6, noise_dark=0.3),
    "Bowl": pmat("MF_Bowl", (0.1, 0.22, 0.45), 0.2),
    "Wood": pmat("MF_Wood", (0.62, 0.42, 0.24), 0.55, wood=True),
    "Steel": pmat("MF_Steel", (0.32, 0.33, 0.35), 0.35, metal=1.0, noise_dark=0.25),
    "Blade": pmat("MF_Blade", (0.78, 0.8, 0.84), 0.25, metal=0.35),
    "Handle": pmat("MF_Handle", (0.18, 0.09, 0.05), 0.5, wood=True),
    "Raw": pmat("MF_RawSteak", (0.85, 0.38, 0.36), 0.35, sss=0.2, stripes=(9.0, (0.95, 0.7, 0.68))),
}


def obj(name, bm, mat):
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me); bm.free()
    o = bpy.data.objects.new(name, me)
    scene.collection.objects.link(o)
    o.data.materials.append(M[mat] if isinstance(mat, str) else mat)
    return o


def mod(o, kind, **kw):
    m = o.modifiers.new(kind, kind)
    for k, v in kw.items():
        setattr(m, k, v)
    return m


def rbox(name, size, mat, bevel=0.3, segs=3, loc=(0, 0, 0), sub=0):
    bm = bmesh.new()
    bmesh.ops.create_cube(bm, size=1.0)
    bmesh.ops.scale(bm, vec=size, verts=bm.verts)
    bmesh.ops.translate(bm, vec=loc, verts=bm.verts)
    o = obj(name, bm, mat)
    mod(o, "BEVEL", width=min(size) * bevel, segments=segs)
    if sub:
        mod(o, "SUBSURF", levels=sub, render_levels=sub)
    return o


def lathe(name, profile, mat, segs=64, mat2=None, split_at=None, split_end=999):
    """profile: [(r, z)], revolved around Z."""
    bm = bmesh.new()
    rings = []
    for (r, z) in profile:
        ring = [bm.verts.new((r * math.cos(2 * math.pi * i / segs), r * math.sin(2 * math.pi * i / segs), z)) for i in range(segs)]
        rings.append(ring)
    for k in range(len(rings) - 1):
        for i in range(segs):
            f = bm.faces.new([rings[k][i], rings[k][(i + 1) % segs], rings[k + 1][(i + 1) % segs], rings[k + 1][i]])
            if mat2 is not None and split_at is not None and split_at <= k < split_end:
                f.material_index = 1
    o = obj(name, bm, mat)
    if mat2:
        o.data.materials.append(M[mat2])
    return o


def ellipsoid(name, r, mat, loc=(0, 0, 0), sub=2):
    bm = bmesh.new()
    bmesh.ops.create_icosphere(bm, subdivisions=sub, radius=1.0)
    bmesh.ops.scale(bm, vec=r, verts=bm.verts)
    bmesh.ops.translate(bm, vec=loc, verts=bm.verts)
    return obj(name, bm, mat)


def join(name, parts, smooth=True):
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
    zs = [v.co.z for v in o.data.vertices]
    o.data.transform(Matrix.Translation((0, 0, -min(zs))))
    return o


# ------------------------------------------------------------------ pieces
def rice_block(name, loc=(0, 0, 0), size=(0.058, 0.026, 0.02)):
    o = rbox(name, size, "Rice", bevel=0.45, segs=4, loc=(loc[0], loc[1], loc[2] + size[2] / 2))
    mod(o, "SUBSURF", levels=1)
    d = mod(o, "DISPLACE", strength=0.0018)
    t = bpy.data.textures.new(name + "_n", "CLOUDS")
    t.noise_scale = 0.004
    d.texture = t
    return o


def fish_slice(name, mat, loc=(0, 0, 0), L=0.075, W=0.03, T=0.006, drape=0.012):
    bm = bmesh.new()
    bmesh.ops.create_grid(bm, x_segments=16, y_segments=6, size=0.5)
    bmesh.ops.scale(bm, vec=(L, W, 1), verts=bm.verts)
    for v in bm.verts:
        x = v.co.x / (L / 2)
        v.co.z = drape * (1 - x * x) + 0.002 * math.sin(v.co.y * 200)
        v.co.y *= 1 - 0.15 * abs(x) ** 3
    bmesh.ops.translate(bm, vec=loc, verts=bm.verts)
    o = obj(name, bm, mat)
    mod(o, "SOLIDIFY", thickness=T, offset=-1)
    mod(o, "SUBSURF", levels=1)
    return o


def lemon_wedge(name, loc=(0, 0, 0), yaw=0.0, s=1.0):
    bm = bmesh.new()
    bmesh.ops.create_uvsphere(bm, u_segments=32, v_segments=16, radius=1.0)
    bmesh.ops.scale(bm, vec=(0.035 * s, 0.022 * s, 0.022 * s), verts=bm.verts)
    kill = [v for v in bm.verts if v.co.z < -1e-5 or v.co.y < -1e-5]
    bmesh.ops.delete(bm, geom=kill, context="VERTS")
    edges = [e for e in bm.edges if e.is_boundary]
    res = bmesh.ops.edgeloop_fill(bm, edges=edges) if edges else {"faces": []}
    for f in res.get("faces", []):
        f.material_index = 1
    bmesh.ops.rotate(bm, verts=bm.verts, cent=(0, 0, 0), matrix=Matrix.Rotation(math.radians(-90), 3, "X"))
    bmesh.ops.rotate(bm, verts=bm.verts, cent=(0, 0, 0), matrix=Matrix.Rotation(yaw, 3, "Z"))
    bmesh.ops.translate(bm, vec=loc, verts=bm.verts)
    o = obj(name, bm, "Lemon")
    o.data.materials.append(M["LemonFlesh"])
    return o


def leaf(name, loc, yaw, s=1.0, mat="Greens"):
    bm = bmesh.new()
    bmesh.ops.create_grid(bm, x_segments=10, y_segments=6, size=0.5)
    for v in bm.verts:
        x, y = v.co.x + 0.5, v.co.y
        w = math.sin(math.pi * min(max(x, 0), 1)) * 0.55
        v.co.x = x * 0.05 * s
        v.co.y = y * 2 * w * 0.025 * s
        v.co.z = 0.012 * s * (x ** 2) + 0.006 * s * (abs(y) * 2) ** 2 + 0.002 * math.sin(x * 20)
    bmesh.ops.rotate(bm, verts=bm.verts, cent=(0, 0, 0), matrix=Matrix.Rotation(yaw, 3, "Z"))
    bmesh.ops.translate(bm, vec=loc, verts=bm.verts)
    o = obj(name, bm, mat)
    mod(o, "SOLIDIFY", thickness=0.0012)
    return o


def greens(name, loc, n=5, s=1.0):
    return [leaf("%s_%d" % (name, i), (loc[0] + rng.uniform(-0.01, 0.01), loc[1] + rng.uniform(-0.01, 0.01), loc[2] + i * 0.002),
                 2 * math.pi * i / n + rng.uniform(-0.3, 0.3), s * rng.uniform(0.8, 1.1)) for i in range(n)]


def sesame(name, center, radius, n=24, z=0.0):
    parts = []
    for i in range(n):
        a = rng.uniform(0, 2 * math.pi); r = radius * math.sqrt(rng.random())
        e = ellipsoid("%s_%d" % (name, i), (0.0022, 0.0013, 0.0009), "Sesame",
                      (center[0] + math.cos(a) * r, center[1] + math.sin(a) * r, z + rng.uniform(0, 0.002)), sub=1)
        e.rotation_euler = (0, 0, rng.uniform(0, 3.14))
        parts.append(e)
    return parts


def steak_block(name, mat_top, mat_side, loc=(0, 0, 0), size=(0.07, 0.04, 0.02), grill=False):
    parts = [rbox(name + "_core", size, mat_side, bevel=0.25, segs=3, loc=(loc[0], loc[1], loc[2] + size[2] / 2))]
    parts.append(rbox(name + "_crust", (size[0] * 1.01, size[1] * 1.01, size[2] * 0.18), mat_top, bevel=0.4, segs=2,
                      loc=(loc[0], loc[1], loc[2] + size[2] * 0.95)))
    if grill:
        for g in range(4):
            gx = loc[0] - size[0] * 0.36 + g * size[0] * 0.24
            b = rbox(name + "_g%d" % g, (0.005, size[1] * 0.8, 0.002), "Grill", bevel=0.4, segs=1)
            b.rotation_euler = (0, 0, math.radians(25))
            b.location = (gx, loc[1], loc[2] + size[2] * 1.05)
            parts.append(b)
    return parts


def plate(name, R=0.13):
    prof = [(0.0, 0.0), (R * 0.55, 0.0), (R * 0.62, 0.004), (R * 0.68, 0.008), (R * 0.85, 0.014), (R * 0.97, 0.02), (R, 0.021),
            (R * 0.985, 0.023), (R * 0.84, 0.017), (R * 0.66, 0.011), (R * 0.6, 0.009), (0.0, 0.009)]
    return lathe(name, prof, "Plate", mat2="PlateRim", split_at=5, split_end=8)


def bowl(name, R=0.09):
    prof = [(0.0, 0.0), (R * 0.45, 0.0), (R * 0.5, 0.006), (R * 0.8, 0.02), (R * 0.97, 0.045), (R, 0.055),
            (R * 0.94, 0.056), (R * 0.9, 0.047), (R * 0.74, 0.024), (R * 0.44, 0.012), (0.0, 0.01)]
    return lathe(name, prof, "Bowl")


def kelp_pile(name, center, n=14, radius=0.05, z=0.012):
    parts = []
    for i in range(n):
        a = rng.uniform(0, 2 * math.pi)
        bm = bmesh.new()
        bmesh.ops.create_grid(bm, x_segments=20, y_segments=2, size=0.5)
        L = rng.uniform(0.05, 0.08)
        for v in bm.verts:
            x = v.co.x + 0.5
            v.co.x = (x - 0.5) * L
            v.co.y = v.co.y * 0.007
            v.co.z = 0.006 * math.sin(x * 14 + i) + 0.012 * math.sin(x * math.pi)
        bmesh.ops.rotate(bm, verts=bm.verts, cent=(0, 0, 0), matrix=Matrix.Rotation(a, 3, "Z"))
        r = radius * math.sqrt(rng.random()) * 0.6
        bmesh.ops.translate(bm, vec=(center[0] + math.cos(a) * r, center[1] + math.sin(a) * r, z + i * 0.0015), verts=bm.verts)
        o = obj("%s_%d" % (name, i), bm, "Kelp")
        mod(o, "SOLIDIFY", thickness=0.0015)
        parts.append(o)
    return parts


def wasabi(name, loc):
    o = ellipsoid(name, (0.012, 0.01, 0.009), "Wasabi", (loc[0], loc[1], loc[2] + 0.004), sub=3)
    d = mod(o, "DISPLACE", strength=0.002)
    t = bpy.data.textures.new(name + "_n", "CLOUDS"); t.noise_scale = 0.005; d.texture = t
    return o


def knife(name):
    bm = bmesh.new()
    bmesh.ops.create_cube(bm, size=1.0)
    bmesh.ops.scale(bm, vec=(0.045, 0.004, 0.25), verts=bm.verts)
    bmesh.ops.translate(bm, vec=(0.0225, 0, 0.125), verts=bm.verts)
    for v in bm.verts:
        if v.co.z > 0.2:
            v.co.x = 0.045 - (0.045 - v.co.x) * 0.25   # tip curves toward the spine
        if v.co.x < 0.01:
            v.co.y *= 0.2                               # thin cutting edge
    o = obj(name + "_blade", bm, "Blade")
    h = rbox(name + "_handle", (0.035, 0.016, 0.12), "Handle", bevel=0.4, segs=3, loc=(0.0245, 0, -0.065))
    return [o, h]


# ------------------------------------------------------------------ dishes (models)
def build_dishes():
    out = {}
    # 0: yellowtail nigiri on a slate board
    p = [rbox("Board", (0.24, 0.11, 0.012), "Slate", bevel=0.3, segs=2, loc=(0, 0, 0.006))]
    for i, x in enumerate((-0.04, 0.035)):
        p.append(rice_block("Rice%d" % i, (x, 0, 0.012)))
        p.append(fish_slice("Fish%d" % i, "Yellowtail", (x, 0, 0.032), drape=0.01))
    p.append(wasabi("Wasabi", (0.095, -0.03, 0.012)))
    p += greens("Shiso", (0.095, 0.03, 0.012), n=2, s=0.7)
    out["SM_Dish_Nigiri"] = join("SM_Dish_Nigiri", p)
    # 1: tuna teppanyaki: seared tuna slices fanned on a plate, lemon + greens
    p = [plate("Plate1")]
    for i in range(4):
        sl = steak_block("Tuna%d" % i, "Seared", "Tuna", (-0.04 + i * 0.022, -0.015 + i * 0.006, 0.01), (0.02, 0.05, 0.02))
        for s in sl:
            s.rotation_euler = (0, math.radians(-12), math.radians(20))
        p += sl
    p.append(lemon_wedge("Lemon1", (0.06, 0.045, 0.01), 0.6))
    p += greens("Greens1", (0.05, -0.05, 0.01), n=5)
    out["SM_Dish_Tuna"] = join("SM_Dish_Tuna", p)
    # 2: grouper teppanyaki: grilled white fillet, lemon, greens, sesame
    p = [plate("Plate2")]
    p += steak_block("Grouper", "Grouper", "Grouper", (-0.01, 0.0, 0.01), (0.11, 0.055, 0.022), grill=True)
    p.append(lemon_wedge("Lemon2", (0.07, 0.05, 0.01), -0.4))
    p += greens("Greens2", (0.065, -0.05, 0.01), n=6)
    p += sesame("Ses2", (-0.01, 0.0), 0.025, n=30, z=0.034)
    out["SM_Dish_Grouper"] = join("SM_Dish_Grouper", p)
    # 3: kelp salad in a blue bowl
    p = [bowl("Bowl3")]
    p += kelp_pile("Kelp3", (0, 0), n=16, radius=0.06, z=0.014)
    p += sesame("Ses3", (0, 0), 0.03, n=34, z=0.05)
    out["SM_Dish_Kelp"] = join("SM_Dish_Kelp", p)
    return out


def export(o, name):
    bpy.ops.object.select_all(action="DESELECT")
    o.select_set(True)
    bpy.context.view_layer.objects.active = o
    bpy.ops.export_scene.fbx(filepath=os.path.join(MOD, name + ".fbx"), use_selection=True, apply_unit_scale=True,
                             axis_forward="X", axis_up="Z", mesh_smooth_type="FACE", path_mode="STRIP")
    print("FOOD", name, tuple(round(d, 3) for d in o.dimensions), [m.name for m in o.data.materials])


dishes = build_dishes()
for n, o in dishes.items():
    export(o, n)

# ------------------------------------------------------------------ renders
try:
    scene.render.engine = "BLENDER_EEVEE"
except TypeError:
    scene.render.engine = "BLENDER_EEVEE_NEXT"
scene.render.film_transparent = True
scene.view_settings.view_transform = "Standard"
scene.view_settings.look = "None" if hasattr(scene.view_settings, "look") else None
world = bpy.data.worlds.new("W"); scene.world = world
world.use_nodes = True
world.node_tree.nodes["Background"].inputs["Color"].default_value = (0.55, 0.6, 0.7, 1)
world.node_tree.nodes["Background"].inputs["Strength"].default_value = 0.35
sun = bpy.data.objects.new("Sun", bpy.data.lights.new("Sun", "SUN"))
sun.data.energy = 2.2; sun.data.angle = math.radians(12)
sun.rotation_euler = (math.radians(40), math.radians(-25), math.radians(30))
scene.collection.objects.link(sun)
fill = bpy.data.objects.new("Fill", bpy.data.lights.new("Fill", "AREA"))
fill.data.energy = 8; fill.data.size = 1.0
fill.location = (-0.6, -0.6, 0.8); fill.rotation_euler = (math.radians(50), 0, math.radians(-45))
scene.collection.objects.link(fill)
cam = bpy.data.objects.new("Cam", bpy.data.cameras.new("Cam")); scene.collection.objects.link(cam)
scene.camera = cam
cam.data.type = "ORTHO"


def hide_all():
    for o in scene.objects:
        if o.type == "MESH":
            o.hide_render = True


def bounds(objs):
    mn = Vector((1e9,) * 3); mx = Vector((-1e9,) * 3)
    for o in objs:
        for c in o.bound_box:
            w = o.matrix_world @ Vector(c)
            mn = Vector(map(min, mn, w)); mx = Vector(map(max, mx, w))
    return mn, mx


def render(objs, name, w, h, view="persp", pad=1.15):
    hide_all()
    for o in objs:
        o.hide_render = False
    mn, mx = bounds(objs)
    ctr = (mn + mx) / 2
    scene.render.resolution_x = w; scene.render.resolution_y = h
    if view == "top":
        cam.location = ctr + Vector((0, 0, 2))
        cam.rotation_euler = (0, 0, 0)
        dx, dy = mx.x - mn.x, mx.y - mn.y
        size = max(dx, dy * w / h) if w >= h else max(dy, dx * h / w)
    else:
        d = Vector((0, -1.0, 0.85)).normalized()
        cam.location = ctr + d * 2
        cam.rotation_euler = (math.radians(90 - 40.4), 0, 0)
        size = max(mx.x - mn.x, (mx.y - mn.y + mx.z - mn.z) * w / h)
    cam.data.ortho_scale = size * pad
    scene.render.filepath = os.path.join(UI, name + ".png")
    bpy.ops.render.render(write_still=True)
    print("RENDER", name)


# dish pictures (order ticket / served)
for i, n in enumerate(["SM_Dish_Nigiri", "SM_Dish_Tuna", "SM_Dish_Grouper", "SM_Dish_Kelp"]):
    render([dishes[n]], "UI_Dish_%d" % i, 384, 384)

# ingredient icons (index == EIngredient)
def icon(idx, parts):
    o = join("Icon%d" % idx, parts)
    render([o], "UI_Ing_%d" % idx, 256, 256, pad=1.25)

icon(0, [rice_block("IRice", (0, 0, 0), (0.06, 0.035, 0.03))])
icon(1, [fish_slice("ISlice", "Yellowtail", (0, 0, 0), L=0.09, W=0.04, drape=0.012)])
icon(2, steak_block("ISteak", "Raw", "Raw", (0, 0, 0), (0.08, 0.05, 0.025)))
icon(3, [lemon_wedge("ILemon", (0, 0, 0), 0.5, s=1.4)])
icon(4, greens("IGreens", (0, 0, 0), n=6, s=1.2))
icon(5, kelp_pile("IKelp", (0, 0), n=12, radius=0.05, z=0.0))
icon(6, [wasabi("IWasabi", (0, 0, 0))])
icon(7, sesame("ISes", (0, 0), 0.02, n=40, z=0.0))

# minigame scenes (top-down)
board = rbox("UIBoard", (0.9, 0.3, 0.04), "Wood", bevel=0.12, segs=3)
render([board], "UI_Board", 1200, 400, view="top", pad=1.02)


def fillet(name, mat):
    bm = bmesh.new()
    bmesh.ops.create_grid(bm, x_segments=40, y_segments=10, size=0.5)
    for v in bm.verts:
        x = v.co.x + 0.5
        width = 0.11 * (0.35 + 0.65 * math.sin(math.pi * min(1.0, 0.15 + x * 0.9)))
        v.co.x = (x - 0.5) * 0.7
        v.co.y = v.co.y * 2 * width
        v.co.z = 0.02 * (1 - (v.co.y / max(width, 1e-4)) ** 2) * math.sin(math.pi * min(1.0, x * 1.05))
    o = obj(name, bm, mat)
    mod(o, "SOLIDIFY", thickness=0.012, offset=-1)
    mod(o, "SUBSURF", levels=1)
    return o

for i, mat in enumerate(["Yellowtail", "Tuna", "Grouper"]):
    f = join("Fillet%d" % i, [fillet("Fillet%d" % i, mat)])
    render([f], "UI_Fillet_%d" % i, 1024, 320, view="top", pad=1.04)

kn = join("Knife", knife("Knife"))
kn.rotation_euler = (math.radians(90), 0, 0)
bpy.context.view_layer.update()
print("KNIFE", kn.dimensions[:], [m.name for m in kn.data.materials], bounds([kn]))
render([kn], "UI_Knife", 160, 512, view="top", pad=1.05)

teppan = rbox("Teppan", (0.8, 0.36, 0.03), "Steel", bevel=0.05, segs=2)
render([teppan], "UI_Teppan", 1100, 500, view="top", pad=1.02)
for nm, mats, grill in (("UI_Steak_Raw", ("Raw", "Raw"), False), ("UI_Steak_Cooked", ("Seared", "Seared"), True)):
    st = join(nm, steak_block(nm, mats[0], mats[1], (0, 0, 0), (0.2, 0.09, 0.03), grill=grill))
    render([st], nm, 512, 256, view="top", pad=1.06)

pl = join("UIPlate", [plate("UIPlateM", 0.13)])
render([pl], "UI_Plate", 512, 512, view="top", pad=1.02)
print("FOOD DONE")
