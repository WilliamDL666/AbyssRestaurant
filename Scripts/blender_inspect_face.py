# Blender: close-up renders of a character's head with backface culling on, to find inverted normals.
# blender -b -P blender_inspect_face.py -- <fbx> <out_dir>
import sys, os, math
import bpy
from mathutils import Vector

argv = sys.argv[sys.argv.index("--") + 1:]
FBX, OUT = argv[0], argv[1]
os.makedirs(OUT, exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=FBX)
meshes = [o for o in bpy.context.scene.objects if o.type == "MESH"]
mn = Vector((1e9,) * 3); mx = Vector((-1e9,) * 3)
for o in meshes:
    for c in o.bound_box:
        w = o.matrix_world @ Vector(c)
        mn = Vector(map(min, mn, w)); mx = Vector(map(max, mx, w))
H = mx.z - mn.z
head = Vector(((mn.x + mx.x) / 2, (mn.y + mx.y) / 2, mn.z + H * 0.86))
# per-part report: parts whose bounds sit around the head
for o in meshes:
    bb = [o.matrix_world @ Vector(c) for c in o.bound_box]
    zc = sum(v.z for v in bb) / 8
    if zc > mn.z + H * 0.75:
        me = o.data
        # normals pointing toward the center = inverted
        ctr = sum((o.matrix_world @ v.co for v in me.vertices), Vector()) / max(len(me.vertices), 1)
        inward = 0
        for p in me.polygons:
            wc = o.matrix_world @ p.center
            n = (o.matrix_world.to_3x3() @ p.normal)
            if n.dot(wc - ctr) < 0:
                inward += 1
        print("HEADPART", o.name, "polys", len(me.polygons), "inward%%=%.0f" % (100.0 * inward / max(len(me.polygons), 1)),
              "mats", [m.name for m in me.materials])
scene = bpy.context.scene
scene.render.engine = "BLENDER_WORKBENCH"
scene.display.shading.color_type = "TEXTURE"
scene.display.shading.show_backface_culling = True
scene.render.resolution_x = 600
scene.render.resolution_y = 600
scene.world = bpy.data.worlds.new("W")
cam = bpy.data.objects.new("Cam", bpy.data.cameras.new("Cam"))
scene.collection.objects.link(cam)
cam.data.type = "ORTHO"
cam.data.ortho_scale = H * 0.3
scene.camera = cam
for name, off in (("face_front", Vector((0, -3, 0))), ("face_side", Vector((3, 0, 0)))):
    cam.location = head + off
    d = head - cam.location
    cam.rotation_euler = d.to_track_quat("-Z", "Y").to_euler()
    scene.render.filepath = os.path.join(OUT, name + ".png")
    bpy.ops.render.render(write_still=True)
