# Re-imports the GPT-made food pictures that replaced the Blender renders (same texture names).
import os
import unreal
tools = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary
UI = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "SourceArt", "Food", "ui")
names = ["Ing_%d" % i for i in range(9)] + ["Fillet_%d" % i for i in range(3)] + ["Dish_%d" % i for i in range(5)] + ["Steak_Raw", "Steak_Cooked", "Steak_Burnt", "Plate", "Knife"]
for n in names:
    t = unreal.AssetImportTask()
    t.set_editor_property("filename", os.path.normpath(os.path.join(UI, "UI_" + n + ".png")))
    t.set_editor_property("destination_path", "/Game/Abyss/UI")
    t.set_editor_property("destination_name", "T_" + n)
    t.set_editor_property("automated", True)
    t.set_editor_property("replace_existing", True)
    t.set_editor_property("save", True)
    tools.import_asset_tasks([t])
    tex = eal.load_asset("/Game/Abyss/UI/T_" + n)
    if tex:
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
        tex.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
        tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
        tex.set_editor_property("never_stream", True)
        eal.save_loaded_asset(tex)
    unreal.log("[AbyssSetup] food ui %s ok=%s" % (n, tex is not None))
