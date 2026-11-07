# Blender: textured 3/4 preview of a Tripo FBX + its bounds.  blender -b -P blender_preview_model.py -- <fbx> <out.png>
import sys, math
import bpy
from mathutils import Vector

argv = sys.argv[sys.argv.index("--") + 1:]
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=argv[0])
sc = bpy.context.scene
ms = [o for o in sc.objects if o.type == "MESH"]
mn = Vector((1e9,) * 3); mx = Vector((-1e9,) * 3)
tris = 0
for o in ms:
    tris += len(o.data.polygons)
    for c in o.bound_box:
        w = o.matrix_world @ Vector(c); mn = Vector(map(min, mn, w)); mx = Vector(map(max, mx, w))
print("MODEL meshes", len(ms), "polys", tris, "size", tuple(round(v, 3) for v in (mx - mn)), "mats", [m.name for m in bpy.data.materials])
ctr = (mn + mx) / 2; size = (mx - mn).length
sc.render.engine = "BLENDER_WORKBENCH"
sc.display.shading.color_type = "TEXTURE"
sc.display.shading.light = "STUDIO"
sc.render.resolution_x = 700; sc.render.resolution_y = 700
sc.world = bpy.data.worlds.new("W")
cam = bpy.data.objects.new("C", bpy.data.cameras.new("C")); sc.collection.objects.link(cam)
cam.data.type = "ORTHO"; cam.data.ortho_scale = size * 0.9
cam.location = ctr + Vector((size * 0.8, -size * 1.4, size * 0.7))
cam.rotation_euler = (ctr - cam.location).to_track_quat("-Z", "Y").to_euler()
sc.camera = cam
sc.render.filepath = argv[1]
bpy.ops.render.render(write_still=True)
