# Imports the Blender-designed kitchen (Scripts/blender_make_kitchen_set.py) as /Game/Abyss/Kitchen/SM_KitchenSet.
# Material slots keep their Blender names; the game assigns a material instance per slot.
import os
import unreal
tools = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary
f = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "SourceArt", "KitchenSet", "SM_KitchenSet.fbx")
o = unreal.FbxImportUI()
o.set_editor_property("import_mesh", True)
o.set_editor_property("import_as_skeletal", False)
o.set_editor_property("import_materials", False)
o.set_editor_property("import_textures", False)
o.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)
sm = o.get_editor_property("static_mesh_import_data")
sm.set_editor_property("combine_meshes", True)
sm.set_editor_property("auto_generate_collision", False)
sm.set_editor_property("generate_lightmap_u_vs", False)
t = unreal.AssetImportTask()
t.set_editor_property("filename", os.path.normpath(f))
t.set_editor_property("destination_path", "/Game/Abyss/Kitchen")
t.set_editor_property("destination_name", "SM_KitchenSet")
t.set_editor_property("automated", True)
t.set_editor_property("replace_existing", True)
t.set_editor_property("save", True)
t.set_editor_property("options", o)
tools.import_asset_tasks([t])
m = eal.load_asset("/Game/Abyss/Kitchen/SM_KitchenSet")
unreal.log("[AbyssSetup] kitchen set slots=%s origin=%s extent=%s" % ([str(s.get_editor_property("material_slot_name")) for s in m.get_editor_property("static_materials")] if m else None, m.get_bounds().origin if m else None, m.get_bounds().box_extent if m else None))
