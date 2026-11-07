# Imports the Blender-authored diver animations onto SK_Diver's skeleton (/Game/Abyss/Diver/A_Diver_*).
import os, glob
import unreal

tools = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary
SRC = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "SourceArt", "DiverAnims")
DEST = "/Game/Abyss/Diver"
sk = eal.load_asset(DEST + "/SK_Diver")
skel = sk.get_editor_property("skeleton")
for f in sorted(glob.glob(os.path.join(SRC, "A_Diver_*.fbx"))):
    name = os.path.splitext(os.path.basename(f))[0]
    opts = unreal.FbxImportUI()
    opts.set_editor_property("import_mesh", False)
    opts.set_editor_property("import_as_skeletal", True)
    opts.set_editor_property("import_animations", True)
    opts.set_editor_property("import_materials", False)
    opts.set_editor_property("import_textures", False)
    opts.set_editor_property("skeleton", skel)
    opts.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_ANIMATION)
    ad = opts.get_editor_property("anim_sequence_import_data")
    ad.set_editor_property("animation_length", unreal.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME)
    ad.set_editor_property("remove_redundant_keys", False)
    t = unreal.AssetImportTask()
    t.set_editor_property("filename", os.path.normpath(f))
    t.set_editor_property("destination_path", DEST)
    t.set_editor_property("destination_name", name)
    t.set_editor_property("automated", True)
    t.set_editor_property("replace_existing", True)
    t.set_editor_property("save", True)
    t.set_editor_property("options", opts)
    tools.import_asset_tasks([t])
    paths = list(t.get_editor_property("imported_object_paths"))
    unreal.log("[AbyssSetup] diver anim %s -> %s" % (name, paths))
    # the importer may name it after the take; make sure it ends up as DEST/name
    for p in paths:
        a = eal.load_asset(p.split(".")[0])
        if isinstance(a, unreal.AnimSequence) and a.get_name() != name:
            eal.rename_asset(p.split(".")[0], DEST + "/" + name)
    a = eal.load_asset(DEST + "/" + name)
    unreal.log("[AbyssSetup] %s ok=%s length=%s" % (name, a is not None, a.get_editor_property("sequence_length") if a else None))
