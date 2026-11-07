# Blender (headless): procedurally model the restaurant props and export one FBX per prop.
#   blender -b -P blender_make_props.py -- <out_dir>
# Props (meters, Z up, origin noted per prop). Material slot names are used by the game code:
#   SM_JellyLamp   origin = bell center        slots Bell / Glow / Tentacle
#   SM_ShellLamp   origin = bottom center      slots Shell / Pearl
#   SM_CoralBranch origin = bottom center      slots Coral / Rock
#   SM_CoralFan    origin = bottom center      slots Coral / Rock
#   SM_BrainCoral  origin = bottom center      slots Coral
#   SM_Anchor      origin = center, in XZ      slots Metal
import sys, os, math, random
import bpy, bmesh
from mathutils import Vector, Matrix, noise

argv = sys.argv[sys.argv.index("--") + 1:]
OUT = argv[0]
os.makedirs(OUT, exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
scene = bpy.context.scene

PREVIEW = {
    "Bell": (1.0, 0.75, 0.6, 1), "Glow": (1.0, 0.9, 0.5, 1), "Tentacle": (1.0, 0.6, 0.5, 1),
    "Shell": (0.95, 0.85, 0.75, 1), "Pearl": (1.0, 0.95, 0.85, 1),
    "Coral": (0.95, 0.4, 0.5, 1), "Rock": (0.35, 0.3, 0.28, 1), "Metal": (0.6, 0.55, 0.45, 1),
}
MATS = {}


def mat(name):
    if name not in MATS:
        m = bpy.data.materials.new(name)
        m.diffuse_color = PREVIEW.get(name, (0.8, 0.8, 0.8, 1))
        MATS[name] = m
    return MATS[name]


def obj_from_bm(name, bm, slot):
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    o = bpy.data.objects.new(name, me)
    scene.collection.objects.link(o)
    o.data.materials.append(mat(slot))
    return o


def curve_obj(name, splines, bevel, slot, res=3, caps=True):
    """splines: list of [(Vector, radius), ...]; converted to a mesh object."""
    cu = bpy.data.curves.new(name, "CURVE")
    cu.dimensions = "3D"
    cu.bevel_depth = bevel
    cu.bevel_resolution = res
    cu.use_fill_caps = caps
    for pts in splines:
        sp = cu.splines.new("NURBS")
        sp.points.add(len(pts) - 1)
        for i, (p, r) in enumerate(pts):
            sp.points[i].co = (p.x, p.y, p.z, 1.0)
            sp.points[i].radius = r
        sp.use_endpoint_u = True
        sp.order_u = min(4, len(pts))
        sp.resolution_u = 6
    o = bpy.data.objects.new(name, cu)
    scene.collection.objects.link(o)
    dg = bpy.context.evaluated_depsgraph_get()
    me = bpy.data.meshes.new_from_object(o.evaluated_get(dg))
    bpy.data.objects.remove(o)
    mo = bpy.data.objects.new(name, me)
    scene.collection.objects.link(mo)
    mo.data.materials.clear()
    mo.data.materials.append(mat(slot))
    return mo


def add_mod(o, kind, **kw):
    m = o.modifiers.new(kind, kind)
    for k, v in kw.items():
        setattr(m, k, v)
    return m


def finish(name, parts, origin_bottom=True):
    """join parts, apply modifiers, smooth, set origin, export FBX + keep for preview."""
    bpy.ops.object.select_all(action="DESELECT")
    for p in parts:
        p.select_set(True)
        bpy.context.view_layer.objects.active = p
        for m in list(p.modifiers):
            bpy.ops.object.modifier_apply(modifier=m.name)
    bpy.context.view_layer.objects.active = parts[0]
    bpy.ops.object.join()
    o = bpy.context.view_layer.objects.active
    o.name = name
    o.data.name = name
    bpy.ops.object.shade_smooth()
    try:
        bpy.ops.object.shade_auto_smooth(angle=math.radians(50))
    except Exception:
        pass
    # origin
    zs = [v.co.z for v in o.data.vertices]
    if origin_bottom:
        o.data.transform(Matrix.Translation((0, 0, -min(zs))))
    bpy.ops.object.select_all(action="DESELECT")
    o.select_set(True)
    bpy.ops.export_scene.fbx(filepath=os.path.join(OUT, name + ".fbx"), use_selection=True,
                             apply_unit_scale=True, axis_forward="X", axis_up="Z", mesh_smooth_type="FACE")
    dims = o.dimensions
    print("PROP", name, "verts", len(o.data.vertices), "size_m", tuple(round(d, 3) for d in dims),
          "slots", [m.name for m in o.data.materials])
    return o


rng = random.Random(7)

# ------------------------------------------------------------------ jellyfish lamp
def jelly_lamp():
    parts = []
    # bell: dome with scalloped, slightly flared rim and soft radial ribs
    bm = bmesh.new()
    bmesh.ops.create_uvsphere(bm, u_segments=64, v_segments=24, radius=0.45)
    kill = [v for v in bm.verts if v.co.z < -0.12]
    bmesh.ops.delete(bm, geom=kill, context="VERTS")
    for v in bm.verts:
        a = math.atan2(v.co.y, v.co.x)
        h = (v.co.z + 0.12) / 0.57              # 0 rim .. 1 top
        rim = max(0.0, 1.0 - h * 2.5)
        r = 1.0 + 0.018 * math.cos(16 * a) + 0.07 * rim * math.cos(16 * a) + 0.06 * rim
        v.co.x *= r
        v.co.y *= r
        v.co.z = v.co.z * 0.78 - rim * 0.05 * (0.5 + 0.5 * math.cos(16 * a))
    bell = obj_from_bm("Bell", bm, "Bell")
    add_mod(bell, "SOLIDIFY", thickness=0.018, offset=-1)
    add_mod(bell, "SUBSURF", levels=1, render_levels=1)
    parts.append(bell)

    # glowing core inside the bell
    bm = bmesh.new()
    bmesh.ops.create_uvsphere(bm, u_segments=32, v_segments=16, radius=0.2)
    for v in bm.verts:
        v.co.z = v.co.z * 0.75 + 0.06
        a = math.atan2(v.co.y, v.co.x)
        f = 1.0 + 0.08 * math.cos(4 * a)       # four-leaf gonads like a moon jelly
        v.co.x *= f
        v.co.y *= f
    parts.append(obj_from_bm("Core", bm, "Glow"))

    # thin trailing tentacles around the rim
    spl = []
    for t in range(16):
        a0 = 2 * math.pi * t / 16 + rng.uniform(-0.08, 0.08)
        L = rng.uniform(0.9, 1.35)
        twist = rng.uniform(1.5, 3.0) * (1 if t % 2 else -1)
        pts = []
        for i in range(9):
            s = i / 8
            r = 0.43 - 0.08 * s + 0.04 * math.sin(s * 6 + t)
            a = a0 + twist * s * 0.35
            pts.append((Vector((math.cos(a) * r, math.sin(a) * r, -0.15 - L * s)), 1.0 - 0.8 * s))
        spl.append(pts)
    parts.append(curve_obj("Tentacles", spl, 0.008, "Tentacle", res=2))

    # four frilly oral arms from the center
    spl = []
    for t in range(4):
        a0 = math.pi / 2 * t + math.pi / 4
        pts = []
        for i in range(10):
            s = i / 9
            w = 0.06 * math.sin(s * 9 + t)
            a = a0 + 0.5 * math.sin(s * 3 + t)
            r = 0.05 + 0.08 * s + w
            pts.append((Vector((math.cos(a) * r, math.sin(a) * r, -0.05 - 0.85 * s)), 1.0 + 0.6 * math.sin(s * 14) ** 2 - 0.7 * s))
        spl.append(pts)
    arms = curve_obj("OralArms", spl, 0.03, "Tentacle", res=2)
    parts.append(arms)
    return finish("SM_JellyLamp", parts, origin_bottom=False)


# ------------------------------------------------------------------ scallop shell lamp
def shell_half(name, open_deg, flip):
    bm = bmesh.new()
    NU, NV = 14, 40
    grid = []
    for iu in range(NU + 1):
        u = iu / NU
        row = []
        for iv in range(NV + 1):
            v = iv / NV * 2 - 1
            a = v * math.radians(68)
            r = 0.04 + 0.16 * u
            rib = 0.02 * abs(math.sin(9 * math.pi * (v + 1) / 2)) * u
            z = 0.07 * u * u * (1 - 0.5 * v * v) + rib
            x = math.sin(a) * r
            y = math.cos(a) * r - 0.02
            row.append(bm.verts.new((x, y, z if not flip else -z)))
        grid.append(row)
    # the two "ears" of a scallop at the hinge
    for iu in range(NU):
        for iv in range(NV):
            f = [grid[iu][iv], grid[iu][iv + 1], grid[iu + 1][iv + 1], grid[iu + 1][iv]]
            bm.faces.new(f if not flip else list(reversed(f)))
    o = obj_from_bm(name, bm, "Shell")
    add_mod(o, "SOLIDIFY", thickness=0.006, offset=0)
    add_mod(o, "SUBSURF", levels=1, render_levels=1)
    o.rotation_euler = (math.radians(-open_deg if flip else 0), 0, 0)
    bpy.context.view_layer.objects.active = o
    o.select_set(True)
    bpy.ops.object.transform_apply(rotation=True)
    o.select_set(False)
    return o


def shell_lamp():
    parts = []
    bottom = shell_half("ShellBottom", 0, True)
    top = shell_half("ShellTop", 0, False)
    # hinge at y=-0.02: open the top shell
    top.data.transform(Matrix.Translation((0, 0.02, 0)))
    top.data.transform(Matrix.Rotation(math.radians(62), 4, "X"))
    top.data.transform(Matrix.Translation((0, -0.02, 0.0)))
    parts += [bottom, top]
    bm = bmesh.new()
    bmesh.ops.create_icosphere(bm, subdivisions=3, radius=0.045)
    bmesh.ops.translate(bm, verts=bm.verts, vec=(0, 0.06, 0.035))
    parts.append(obj_from_bm("Pearl", bm, "Pearl"))
    # small foot
    bm = bmesh.new()
    bmesh.ops.create_cone(bm, cap_ends=True, segments=24, radius1=0.06, radius2=0.03, depth=0.04)
    bmesh.ops.translate(bm, verts=bm.verts, vec=(0, 0.05, -0.045))
    parts.append(obj_from_bm("Foot", bm, "Shell"))
    return finish("SM_ShellLamp", parts)


# ------------------------------------------------------------------ corals
def rock(radius, seed):
    bm = bmesh.new()
    bmesh.ops.create_icosphere(bm, subdivisions=3, radius=radius)
    off = Vector((seed * 3.1, seed * 1.7, 0))
    for v in bm.verts:
        n = noise.noise(v.co * 6 + off)
        v.co *= 1 + 0.25 * n
        v.co.z = max(v.co.z * 0.55, -0.01)
    return obj_from_bm("Rock", bm, "Rock")


def coral_branch():
    spl = []

    def branch(p, d, L, r, depth):
        pts = [(p.copy(), r)]
        q = p.copy()
        dd = d.copy()
        for i in range(1, 5):
            dd = (dd + Vector((rng.uniform(-0.25, 0.25), rng.uniform(-0.25, 0.25), 0.15))).normalized()
            q = q + dd * (L / 4)
            pts.append((q.copy(), r * (1 - 0.18 * i)))
        spl.append(pts)
        if depth == 0:
            return
        for k in range(rng.choice([2, 3])):
            nd = (dd + Vector((rng.uniform(-0.9, 0.9), rng.uniform(-0.9, 0.9), 0.4))).normalized()
            s = rng.uniform(0.45, 0.8)
            branch(pts[int(s * 4)][0], nd, L * 0.7, r * 0.65, depth - 1)

    for k in range(3):
        a = 2 * math.pi * k / 3
        branch(Vector((math.cos(a) * 0.04, math.sin(a) * 0.04, 0.02)),
               Vector((math.cos(a) * 0.4, math.sin(a) * 0.4, 1)).normalized(), 0.32, 1.0, 3)
    c = curve_obj("Branches", spl, 0.028, "Coral", res=3)
    return finish("SM_CoralBranch", [rock(0.14, 1), c])


def coral_fan():
    spl = []

    def branch(p, ang, L, r, depth):
        pts = [(p.copy(), r)]
        q = p.copy()
        a = ang
        for i in range(1, 5):
            a += rng.uniform(-0.12, 0.12)
            q = q + Vector((math.sin(a), rng.uniform(-0.02, 0.02), math.cos(a))) * (L / 4)
            pts.append((q.copy(), r * (1 - 0.15 * i)))
        spl.append(pts)
        if depth == 0:
            return
        for s in (0.5, 1.0):
            for side in (-1, 1):
                if rng.random() < 0.8:
                    branch(pts[int(s * 4)][0], a + side * rng.uniform(0.25, 0.55), L * 0.68, r * 0.7, depth - 1)

    branch(Vector((0, 0, 0.02)), 0.0, 0.3, 1.0, 4)
    c = curve_obj("Fan", spl, 0.012, "Coral", res=2)
    return finish("SM_CoralFan", [rock(0.1, 2), c])


def brain_coral():
    bm = bmesh.new()
    bmesh.ops.create_icosphere(bm, subdivisions=5, radius=0.28)
    for v in bm.verts:
        p = v.co.normalized()
        ridge = 1 - abs(noise.noise(p * 4.5 + Vector((3, 1, 2))))
        ridge = ridge ** 3
        v.co = p * 0.28 * (1 + 0.07 * ridge + 0.05 * noise.noise(p * 1.5))
        v.co.z = max(v.co.z, -0.02) * 0.8
    o = obj_from_bm("Brain", bm, "Coral")
    return finish("SM_BrainCoral", [o])


# ------------------------------------------------------------------ anchor (lies in the XZ plane)
def anchor():
    parts = []
    # shank
    bm = bmesh.new()
    bmesh.ops.create_cone(bm, cap_ends=True, segments=24, radius1=0.04, radius2=0.032, depth=0.95)
    bmesh.ops.translate(bm, verts=bm.verts, vec=(0, 0, 0.0))
    parts.append(obj_from_bm("Shank", bm, "Metal"))
    # top ring
    bpy.ops.mesh.primitive_torus_add(major_radius=0.085, minor_radius=0.022, location=(0, 0, 0.57),
                                     rotation=(math.radians(90), 0, 0), major_segments=40, minor_segments=12)
    ring = bpy.context.active_object
    ring.data.materials.append(mat("Metal"))
    parts.append(ring)
    # stock (crossbar) with ball ends
    bm = bmesh.new()
    bmesh.ops.create_cone(bm, cap_ends=True, segments=16, radius1=0.028, radius2=0.028, depth=0.5)
    bmesh.ops.rotate(bm, verts=bm.verts, cent=(0, 0, 0), matrix=Matrix.Rotation(math.radians(90), 3, "Y"))
    bmesh.ops.translate(bm, verts=bm.verts, vec=(0, 0, 0.36))
    for x in (-0.25, 0.25):
        sph = bmesh.ops.create_uvsphere(bm, u_segments=16, v_segments=10, radius=0.045)
        bmesh.ops.translate(bm, verts=sph["verts"], vec=(x, 0, 0.36))
    parts.append(obj_from_bm("Stock", bm, "Metal"))
    # curved arms
    pts = []
    for i in range(13):
        a = math.radians(200 + 140 * i / 12)
        pts.append((Vector((math.cos(a) * 0.38, 0, 0.0 + math.sin(a) * 0.38 + 0.0)), 1.0 - 0.35 * abs(i - 6) / 6))
    arms = curve_obj("Arms", [pts], 0.04, "Metal", res=3)
    arms.data.transform(Matrix.Translation((0, 0, -0.1)))
    parts.append(arms)
    # flukes (spade tips) at both arm ends
    for sgn in (-1, 1):
        a = math.radians(200 if sgn < 0 else 340)
        tip = Vector((math.cos(a) * 0.38, 0, math.sin(a) * 0.38 - 0.1))
        bm = bmesh.new()
        bmesh.ops.create_cone(bm, cap_ends=True, segments=3, radius1=0.1, radius2=0.0, depth=0.18)
        bmesh.ops.scale(bm, vec=(1, 0.3, 1), verts=bm.verts)
        bmesh.ops.rotate(bm, verts=bm.verts, cent=(0, 0, 0), matrix=Matrix.Rotation(math.radians(-sgn * 35), 3, "Y"))
        bmesh.ops.translate(bm, verts=bm.verts, vec=tip + Vector((0, 0, 0.06)))
        parts.append(obj_from_bm("Fluke", bm, "Metal"))
    # crown at the bottom
    bm = bmesh.new()
    bmesh.ops.create_uvsphere(bm, u_segments=20, v_segments=12, radius=0.06)
    bmesh.ops.translate(bm, verts=bm.verts, vec=(0, 0, -0.48))
    parts.append(obj_from_bm("Crown", bm, "Metal"))
    return finish("SM_Anchor", parts, origin_bottom=False)


props = [jelly_lamp(), shell_lamp(), coral_branch(), coral_fan(), brain_coral(), anchor()]

# ------------------------------------------------------------------ preview render (all props in a row)
x = 0.0
for o in props:
    w = max(o.dimensions.x, 0.3)
    o.location = (x + w / 2, 0, 0)
    x += w + 0.25
scene.render.engine = "BLENDER_WORKBENCH"
scene.display.shading.light = "STUDIO"
scene.display.shading.color_type = "MATERIAL"
scene.display.shading.show_cavity = True
scene.render.resolution_x = 1800
scene.render.resolution_y = 700
scene.world = bpy.data.worlds.new("W")
cam = bpy.data.objects.new("Cam", bpy.data.cameras.new("Cam"))
scene.collection.objects.link(cam)
cam.data.type = "ORTHO"
cam.data.ortho_scale = x + 0.2
cam.location = (x / 2, -6, 0.1)
cam.rotation_euler = (math.radians(90), 0, 0)
scene.camera = cam
scene.render.filepath = os.path.join(OUT, "preview_front.png")
bpy.ops.render.render(write_still=True)
cam.location = (x / 2, -6, 3.5)
cam.rotation_euler = (math.radians(60), 0, 0)
scene.render.filepath = os.path.join(OUT, "preview_top.png")
bpy.ops.render.render(write_still=True)
print("PROPS DONE")
