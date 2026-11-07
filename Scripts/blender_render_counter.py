# Blender: render the user's Tripo cook counter (SourceArt/KitchenC) as the cooking-UI counter picture.
#   blender -b -P blender_render_counter.py -- <fbx> <out_png> <grid_png>
# Same framing idea as the old procedural counter: 1920x700, transparent above, counter top seen from a raised front view.
import sys, math
import bpy
from mathutils import Vector

argv = sys.argv[sys.argv.index("--") + 1:]
FBX, OUT, GRID = argv[0], argv[1], argv[2]
bpy.ops.wm.read_factory_settings(use_empty=True)
sc = bpy.context.scene
bpy.ops.import_scene.fbx(filepath=FBX)
ms = [o for o in sc.objects if o.type == "MESH"]
mn = Vector((1e9,) * 3); mx = Vector((-1e9,) * 3)
for o in ms:
    for c in o.bound_box:
        w = o.matrix_world @ Vector(c); mn = Vector(map(min, mn, w)); mx = Vector(map(max, mx, w))
ctr = (mn + mx) / 2
size = mx - mn
print("COUNTER bounds", tuple(mn), tuple(mx))
try:
    sc.render.engine = "BLENDER_EEVEE"
except TypeError:
    sc.render.engine = "BLENDER_EEVEE_NEXT"
sc.render.film_transparent = True
sc.view_settings.view_transform = "Standard"
w = bpy.data.worlds.new("W"); sc.world = w
w.use_nodes = True
w.node_tree.nodes["Background"].inputs["Color"].default_value = (0.55, 0.55, 0.75, 1)
w.node_tree.nodes["Background"].inputs["Strength"].default_value = 0.45
def light(name, kind, energy, loc, rot, color, size=1.0):
    l = bpy.data.objects.new(name, bpy.data.lights.new(name, kind))
    l.data.energy = energy; l.data.color = color
    if kind == "AREA": l.data.size = size
    l.location = loc; l.rotation_euler = rot
    sc.collection.objects.link(l)
L = size.x
light("Key", "AREA", 45 * L * L, ctr + Vector((0, -L * 0.8, L * 1.2)), (math.radians(35), 0, 0), (1.0, 0.9, 0.8), L)
light("Fill", "AREA", 16 * L * L, ctr + Vector((-L, -L, L * 0.5)), (math.radians(60), 0, math.radians(-45)), (0.7, 0.65, 1.0), L)
light("Rim", "AREA", 14 * L * L, ctr + Vector((L * 0.8, L, L * 0.6)), (math.radians(-60), 0, math.radians(150)), (0.6, 0.85, 1.0), L)
cam = bpy.data.objects.new("Cam", bpy.data.cameras.new("Cam")); sc.collection.objects.link(cam)
cam.data.type = "ORTHO"
cam.data.ortho_scale = size.x * 1.22
# front (-Y) at ~32 deg elevation, aimed slightly above the counter top so the top fills the upper half
cam.location = ctr + Vector((0, -L * 2.0, L * 1.25))
aim = ctr + Vector((0, 0, size.z * 0.05))
cam.rotation_euler = (aim - cam.location).to_track_quat("-Z", "Y").to_euler()
cam.data.shift_y = 0.08
sc.camera = cam
sc.render.resolution_x = 1920
sc.render.resolution_y = 900
sc.render.filepath = OUT
bpy.ops.render.render(write_still=True)
print("COUNTER RENDER DONE")
