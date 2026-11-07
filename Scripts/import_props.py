# Imports the Blender-made props (Scripts/blender_make_props.py) into /Game/Abyss/Props.
# Material slots keep their Blender names (Bell/Glow/Tentacle/Shell/Pearl/Coral/Rock/Metal);
# the game code assigns colored material instances per slot.
import os, glob
import unreal

tools = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary
SRC = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "SourceArt", "Props")
DEST = "/Game/Abyss/Props"

for f in sorted(glob.glob(os.path.join(SRC, "SM_*.fbx"))):
    name = os.path.splitext(os.path.basename(f))[0]
    opts = unreal.FbxImportUI()
    opts.set_editor_property("import_mesh", True)
    opts.set_editor_property("import_as_skeletal", False)
    opts.set_editor_property("import_materials", False)
    opts.set_editor_property("import_textures", False)
    opts.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)
    sm = opts.get_editor_property("static_mesh_import_data")
    sm.set_editor_property("combine_meshes", True)
    sm.set_editor_property("generate_lightmap_u_vs", False)
    sm.set_editor_property("auto_generate_collision", False)
    t = unreal.AssetImportTask()
    t.set_editor_property("filename", os.path.normpath(f))
    t.set_editor_property("destination_path", DEST)
    t.set_editor_property("destination_name", name)
    t.set_editor_property("automated", True)
    t.set_editor_property("replace_existing", True)
    t.set_editor_property("save", True)
    t.set_editor_property("options", opts)
    tools.import_asset_tasks([t])
    m = eal.load_asset(DEST + "/" + name)
    slots = [s.get_editor_property("material_slot_name") for s in m.get_editor_property("static_materials")] if m else []
    unreal.log("[AbyssSetup] prop %s slots=%s bounds=%s" % (name, slots, m.get_bounds().box_extent if m else None))
