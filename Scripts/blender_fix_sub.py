# Blender (headless): straighten the Tripo submarine, optionally add a cockpit glass dome, render checks, export FBX.
# blender -b -P blender_fix_sub.py -- <in_fbx> <out_dir> <yaw_deg> [glass cx cy cz rx ry rz]
#   yaw_deg: rotation about Z so the nose points +X
#   glass: center + radii (meters, after straightening). The dome is the front (+X) half of an ellipsoid.
import sys, os, math
import bpy, bmesh
from mathutils import Vector, Matrix

argv = sys.argv[sys.argv.index("--") + 1:]
FBX, OUT, YAW = argv[0], argv[1], float(argv[2])
GLASS = [float(v) for v in argv[3:9]] if len(argv) >= 9 else None
os.makedirs(OUT, exist_ok=True)

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=FBX)
meshes = [o for o in bpy.context.scene.objects if o.type == "MESH"]
hull = meshes[0]
for o in bpy.context.scene.objects:
    o.select_set(o in meshes)
bpy.context.view_layer.objects.active = hull
if len(meshes) > 1:
    bpy.ops.object.join()
hull = bpy.context.view_layer.objects.active

# bake transforms, then straighten + recenter (XY centered, bottom at z=0)
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
hull.data.transform(Matrix.Rotation(math.radians(YAW), 4, "Z"))
vs = [v.co for v in hull.data.vertices]
mn = Vector((min(v.x for v in vs), min(v.y for v in vs), min(v.z for v in vs)))
mx = Vector((max(v.x for v in vs), max(v.y for v in vs), max(v.z for v in vs)))
c = (mn + mx) / 2
hull.data.transform(Matrix.Translation(Vector((-c.x, -c.y, -mn.z))))
hull.data.update()
vs = [v.co for v in hull.data.vertices]
mn = Vector((min(v.x for v in vs), min(v.y for v in vs), min(v.z for v in vs)))
mx = Vector((max(v.x for v in vs), max(v.y for v in vs), max(v.z for v in vs)))
print("STRAIGHT_BOUNDS_MIN", tuple(round(v, 4) for v in mn))
print("STRAIGHT_BOUNDS_MAX", tuple(round(v, 4) for v in mx))
hull.data.materials[0].name = "Hull"

# smooth shading (auto smooth by angle) to soften the low-poly look
bpy.ops.object.shade_smooth()
try:
    bpy.ops.object.shade_auto_smooth(angle=math.radians(35))
except Exception as e:
    print("auto smooth:", e)

if GLASS:
    cx, cy, cz, rx, ry, rz = GLASS
    bpy.ops.mesh.primitive_uv_sphere_add(segments=48, ring_count=24, radius=1.0, location=(0, 0, 0))
    g = bpy.context.active_object
    g.name = "CockpitGlass"
    bm = bmesh.new()
    bm.from_mesh(g.data)
    # keep only the front-upper quarter (x >= 0, z >= 0) -> canopy over the arched opening
    bmesh.ops.delete(bm, geom=[v for v in bm.verts if v.co.x < -1e-4 or v.co.z < -1e-4], context="VERTS")
    bm.to_mesh(g.data)
    bm.free()
    g.data.transform(Matrix.Diagonal((rx, ry, rz, 1.0)))
    g.data.transform(Matrix.Translation(Vector((cx, cy, cz))))
    bpy.ops.object.shade_smooth()
    gm = bpy.data.materials.new("Glass")
    gm.use_nodes = True
    bsdf = gm.node_tree.nodes.get("Principled BSDF")
    if bsdf:
        bsdf.inputs["Base Color"].default_value = (0.6, 0.85, 1.0, 1.0)
        bsdf.inputs["Alpha"].default_value = 0.25
        bsdf.inputs["Roughness"].default_value = 0.05
    gm.diffuse_color = (0.55, 0.85, 1.0, 0.35)
    gm.blend_method = "BLEND" if hasattr(gm, "blend_method") else gm.blend_method
    g.data.materials.append(gm)

# ---- renders
scene = bpy.context.scene
scene.render.engine = "BLENDER_WORKBENCH"
scene.display.shading.light = "STUDIO"
scene.display.shading.color_type = "TEXTURE" if not GLASS else "MATERIAL"
scene.display.shading.show_xray = False
scene.render.resolution_x = 1000
scene.render.resolution_y = 1000
scene.world = bpy.data.worlds.new("W")
if GLASS:
    # textured hull + coloured glass: use per-object colour so the glass stands out
    scene.display.shading.color_type = "TEXTURE"
    gm.diffuse_color = (0.3, 0.8, 1.0, 1.0)

size = mx - mn
ctr = (mn + mx) / 2
cam_data = bpy.data.cameras.new("Cam")
cam_data.type = "ORTHO"
cam_data.ortho_scale = max(size) * 1.15
cam = bpy.data.objects.new("Cam", cam_data)
scene.collection.objects.link(cam)
scene.camera = cam
print("ORTHO_SCALE", cam_data.ortho_scale, "CENTER", tuple(round(v, 4) for v in ctr))
D = max(size) * 3.0
views = {
    # image right = +X, up = +Z
    "side": (Vector((0, -D, 0)), (math.radians(90), 0, 0)),
    # image right = +Y... camera at +X looking -X: right = +Y, up = +Z
    "front": (Vector((D, 0, 0)), (math.radians(90), 0, math.radians(90))),
    # image right = +X, up = +Y
    "top": (Vector((0, 0, D)), (0, 0, 0)),
    "persp34": None,
}
for name, v in views.items():
    if v is None:
        cam_data.type = "PERSP"
        cam.location = ctr + Vector((max(size) * 1.6, -max(size) * 1.6, max(size) * 1.0))
        d = ctr - cam.location
        cam.rotation_euler = d.to_track_quat("-Z", "Y").to_euler()
    else:
        cam.location = ctr + v[0]
        cam.rotation_euler = v[1]
    scene.render.filepath = os.path.join(OUT, name + ".png")
    bpy.ops.render.render(write_still=True)
    print("RENDERED", name)

if GLASS:
    out_fbx = os.path.join(OUT, "SM_Submarine.fbx")
    for o in bpy.context.scene.objects:
        o.select_set(o.type == "MESH")
    bpy.ops.export_scene.fbx(filepath=out_fbx, use_selection=True, path_mode="COPY", embed_textures=True,
                             apply_unit_scale=True, axis_forward="X", axis_up="Z", mesh_smooth_type="FACE")
    print("EXPORTED", out_fbx)
