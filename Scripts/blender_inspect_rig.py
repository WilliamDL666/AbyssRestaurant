# Blender: list armature bones / actions of a rigged FBX and render a few frames of its animation.
# blender -b -P blender_inspect_rig.py -- <fbx> <out_dir>
import sys, os, math
import bpy
from mathutils import Vector

argv = sys.argv[sys.argv.index("--") + 1:]
FBX, OUT = argv[0], argv[1]
os.makedirs(OUT, exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=FBX, automatic_bone_orientation=False)

arms = [o for o in bpy.context.scene.objects if o.type == "ARMATURE"]
meshes = [o for o in bpy.context.scene.objects if o.type == "MESH"]
print("OBJECTS", [(o.name, o.type) for o in bpy.context.scene.objects])
for a in arms:
    print("ARMATURE", a.name, "bones", len(a.data.bones), "rot", tuple(round(v, 3) for v in a.rotation_euler), "scale", tuple(round(v, 3) for v in a.scale))
    for b in a.data.bones:
        print("BONE", b.name, "parent=", b.parent.name if b.parent else None,
              "head=", tuple(round(v, 3) for v in (a.matrix_world @ b.head_local)),
              "tail=", tuple(round(v, 3) for v in (a.matrix_world @ b.tail_local)))
print("ACTIONS", [(ac.name, tuple(ac.frame_range)) for ac in bpy.data.actions])
for m in meshes:
    print("MESH", m.name, len(m.data.polygons), [mt.name for mt in m.data.materials])
mn = Vector((1e9,) * 3); mx = Vector((-1e9,) * 3)
for o in meshes:
    for c in o.bound_box:
        w = o.matrix_world @ Vector(c)
        mn = Vector(map(min, mn, w)); mx = Vector(map(max, mx, w))
print("BOUNDS", tuple(round(v, 3) for v in mn), tuple(round(v, 3) for v in mx))

scene = bpy.context.scene
scene.render.engine = "BLENDER_WORKBENCH"
scene.display.shading.color_type = "TEXTURE"
scene.render.resolution_x = 800
scene.render.resolution_y = 800
scene.world = bpy.data.worlds.new("W")
size = mx - mn
ctr = (mn + mx) / 2
cam_data = bpy.data.cameras.new("Cam")
cam_data.type = "ORTHO"
cam_data.ortho_scale = max(size) * 1.3
cam = bpy.data.objects.new("Cam", cam_data)
scene.collection.objects.link(cam)
scene.camera = cam
D = max(size) * 3
fr = (1, 1)
if bpy.data.actions:
    fr = tuple(int(v) for v in bpy.data.actions[0].frame_range)
frames = sorted(set([fr[0], (fr[0] + fr[1]) // 2, fr[1]]))
for name, off, rot in [("front", Vector((0, -D, 0)), (math.radians(90), 0, 0)), ("side", Vector((D, 0, 0)), (math.radians(90), 0, math.radians(90)))]:
    cam.location = ctr + off
    cam.rotation_euler = rot
    for f in frames:
        scene.frame_set(f)
        scene.render.filepath = os.path.join(OUT, "%s_f%03d.png" % (name, f))
        bpy.ops.render.render(write_still=True)
print("DONE")
