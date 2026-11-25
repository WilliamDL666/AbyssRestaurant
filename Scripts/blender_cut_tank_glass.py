# Blender: the Tripo display tank has its "glass" painted on as an opaque wall, so fish inside would be invisible.
# This removes that wall (the game adds a real transparent cylinder + water) and keeps the base, the lid and the corals.
#   blender -b -P blender_cut_tank_glass.py -- <in.fbx> <out.fbx> <preview.png>
import sys, math
import bpy
import numpy as np
from mathutils import Vector

argv = sys.argv[sys.argv.index("--") + 1:]
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=argv[0])
o = [x for x in bpy.context.scene.objects if x.type == "MESH"][0]
bpy.context.view_layer.objects.active = o
o.select_set(True)
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
me = o.data
n = len(me.vertices)
co = np.empty(n * 3, dtype=np.float32)
me.vertices.foreach_get("co", co)
co = co.reshape(-1, 3)
mn, mx = co.min(0), co.max(0)
up = int(np.argmax(mx - mn))               # the tall axis
ax = [a for a in range(3) if a != up]
h = (co[:, up] - mn[up]) / (mx[up] - mn[up])
c = (mn + mx) / 2
r = np.hypot(co[:, ax[0]] - c[ax[0]], co[:, ax[1]] - c[ax[1]])
print("TANK bounds", mn, mx, "up axis", up)
# radius profile along the height: the glass is the long straight stretch in the middle
for k in range(20):
    m = (h >= k / 20) & (h < (k + 1) / 20)
    if m.any():
        print("  h %.2f..%.2f  r90 %.4f  rmax %.4f  n %d" % (k / 20, (k + 1) / 20, np.percentile(r[m], 90), r[m].max(), m.sum()))
LO, HI = float(argv[3]) if len(argv) > 3 else 0.30, float(argv[4]) if len(argv) > 4 else 0.70
mid = (h > LO) & (h < HI)
rg = np.percentile(r[mid], 80)
wall = mid & (r > rg * 0.86)
print("TANK glass radius", rg, "wall verts", int(wall.sum()))
npoly = len(me.polygons)
ls = np.empty(npoly, dtype=np.int32); lt = np.empty(npoly, dtype=np.int32)
me.polygons.foreach_get("loop_start", ls); me.polygons.foreach_get("loop_total", lt)
lv = np.empty(len(me.loops), dtype=np.int32)
me.loops.foreach_get("vertex_index", lv)
wl = wall[lv].astype(np.int32)
cs = np.concatenate(([0], np.cumsum(wl)))
kill = (cs[ls + lt] - cs[ls]) == lt          # every corner of the face is on the wall
print("TANK faces removed", int(kill.sum()), "of", npoly)
bpy.ops.object.mode_set(mode="EDIT")
bpy.ops.mesh.select_all(action="DESELECT")
bpy.ops.object.mode_set(mode="OBJECT")
me.polygons.foreach_set("select", kill)
bpy.ops.object.mode_set(mode="EDIT")
bpy.ops.mesh.select_mode(type="FACE")
bpy.ops.mesh.delete(type="FACE")
bpy.ops.object.mode_set(mode="OBJECT")
bpy.ops.export_scene.fbx(filepath=argv[1], use_selection=True, path_mode="STRIP", mesh_smooth_type="FACE")
print("TANK glass r=%.4f lo=%.4f hi=%.4f height=%.4f" % (rg, mn[up] + LO * (mx[up] - mn[up]), mn[up] + HI * (mx[up] - mn[up]), mx[up] - mn[up]))

# preview
sc = bpy.context.scene
try:
    sc.render.engine = "BLENDER_EEVEE"
except TypeError:
    sc.render.engine = "BLENDER_EEVEE_NEXT"
w = bpy.data.worlds.new("W"); sc.world = w
w.use_nodes = True
w.node_tree.nodes["Background"].inputs["Strength"].default_value = 1.5
cam = bpy.data.objects.new("Cam", bpy.data.cameras.new("Cam")); sc.collection.objects.link(cam); sc.camera = cam
size = float((mx - mn).max())
ctr = Vector(((mn[0] + mx[0]) / 2, (mn[1] + mx[1]) / 2, (mn[2] + mx[2]) / 2))
d = Vector((1.2, -1.6, 0.9)).normalized()
cam.location = ctr + d * size * 2.2
cam.rotation_euler = (ctr - cam.location).to_track_quat("-Z", "Y").to_euler()
sun = bpy.data.objects.new("Sun", bpy.data.lights.new("Sun", "SUN")); sun.data.energy = 3
sun.rotation_euler = (math.radians(50), 0, math.radians(30)); sc.collection.objects.link(sun)
sc.render.resolution_x = 700; sc.render.resolution_y = 900
sc.render.filepath = argv[2]
bpy.ops.render.render(write_still=True)
