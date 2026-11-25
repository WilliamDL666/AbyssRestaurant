# Blender: reduce a ~2M-triangle Tripo model so the Nanite build does not run out of memory.
#   blender -b -P blender_decimate.py -- <in.fbx> <out.fbx> <target_tris>
import sys
import bpy
argv = sys.argv[sys.argv.index("--") + 1:]
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=argv[0])
o = [x for x in bpy.context.scene.objects if x.type == "MESH"][0]
bpy.context.view_layer.objects.active = o
o.select_set(True)
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
n = len(o.data.polygons)
m = o.modifiers.new("Dec", "DECIMATE")
m.ratio = min(1.0, float(argv[2]) / max(n, 1))
bpy.ops.object.modifier_apply(modifier=m.name)
bpy.ops.export_scene.fbx(filepath=argv[1], use_selection=True, path_mode="STRIP", mesh_smooth_type="FACE")
print("DECIMATED", n, "->", len(o.data.polygons), [round(d, 3) for d in o.dimensions])
