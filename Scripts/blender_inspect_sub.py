# Blender (headless): import the submarine FBX, print its bounds, render ortho views for inspection.
# blender -b -P blender_inspect_sub.py -- <fbx> <out_dir>
import sys, os, math
import bpy
from mathutils import Vector

argv = sys.argv[sys.argv.index("--") + 1:]
FBX, OUT = argv[0], argv[1]
os.makedirs(OUT, exist_ok=True)

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=FBX)
meshes = [o for o in bpy.context.scene.objects if o.type == "MESH"]
mn = Vector((1e9, 1e9, 1e9)); mx = Vector((-1e9, -1e9, -1e9))
for o in meshes:
    for c in o.bound_box:
        w = o.matrix_world @ Vector(c)
        mn = Vector((min(mn.x, w.x), min(mn.y, w.y), min(mn.z, w.z)))
        mx = Vector((max(mx.x, w.x), max(mx.y, w.y), max(mx.z, w.z)))
print("MESHES", [(o.name, len(o.data.polygons), [m.name for m in o.data.materials]) for o in meshes])
print("BOUNDS_MIN", tuple(round(v, 4) for v in mn))
print("BOUNDS_MAX", tuple(round(v, 4) for v in mx))
center = (mn + mx) / 2
size = mx - mn

scene = bpy.context.scene
scene.render.engine = "BLENDER_WORKBENCH"
scene.display.shading.light = "STUDIO"
scene.display.shading.color_type = "TEXTURE"
scene.display.shading.show_cavity = True
scene.render.resolution_x = 1000
scene.render.resolution_y = 1000
scene.render.film_transparent = False
world = bpy.data.worlds.new("W"); scene.world = world

cam_data = bpy.data.cameras.new("Cam")
cam_data.type = "ORTHO"
cam_data.ortho_scale = max(size) * 1.15
cam = bpy.data.objects.new("Cam", cam_data)
scene.collection.objects.link(cam)
scene.camera = cam

# grid overlay helper: small marker spheres along axes every 10% for reading coordinates
D = max(size) * 2.0
views = {
    "side_from_negY": (Vector((0, -D, 0)), (math.radians(90), 0, 0)),
    "side_from_posX": (Vector((D, 0, 0)), (math.radians(90), 0, math.radians(90))),
    "top_from_posZ": (Vector((0, 0, D)), (0, 0, 0)),
    "front_from_negX": (Vector((-D, 0, 0)), (math.radians(90), 0, math.radians(-90))),
    "front_from_posY": (Vector((0, D, 0)), (math.radians(90), 0, math.radians(180))),
}
for name, (off, rot) in views.items():
    cam.location = center + off
    cam.rotation_euler = rot
    scene.render.filepath = os.path.join(OUT, name + ".png")
    bpy.ops.render.render(write_still=True)
    print("RENDERED", name)
