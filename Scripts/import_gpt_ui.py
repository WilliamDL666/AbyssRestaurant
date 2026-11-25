# Imports the GPT-made UI art (A_/B_/C_/D_/E_*.png in SourceArt/Food/ui) into /Game/Abyss/UI as T_<name>.
import os, glob
import unreal

tools = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary
UI = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "SourceArt", "Food", "ui")
files = []
for p in "ABCDE":
    files += sorted(glob.glob(os.path.join(UI, p + "_*.png")))
for f in files:
    base = os.path.splitext(os.path.basename(f))[0]
    t = unreal.AssetImportTask()
    t.set_editor_property("filename", os.path.normpath(f))
    t.set_editor_property("destination_path", "/Game/Abyss/UI")
    t.set_editor_property("destination_name", "T_" + base)
    t.set_editor_property("automated", True)
    t.set_editor_property("replace_existing", True)
    t.set_editor_property("save", True)
    tools.import_asset_tasks([t])
    tex = eal.load_asset("/Game/Abyss/UI/T_" + base)
    if tex:
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
        tex.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
        tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
        tex.set_editor_property("never_stream", True)
        eal.save_loaded_asset(tex)
    unreal.log("[AbyssSetup] gpt ui %s ok=%s" % (base, tex is not None))
