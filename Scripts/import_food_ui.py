# Imports the Blender food models (/Game/Abyss/Food) and the cooking-UI pictures (/Game/Abyss/UI).
import os, glob
import unreal

tools = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary
SRC = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "SourceArt", "Food")


def run(path, dest, name, opts=None):
    t = unreal.AssetImportTask()
    t.set_editor_property("filename", os.path.normpath(path))
    t.set_editor_property("destination_path", dest)
    t.set_editor_property("destination_name", name)
    t.set_editor_property("automated", True)
    t.set_editor_property("replace_existing", True)
    t.set_editor_property("save", True)
    if opts:
        t.set_editor_property("options", opts)
    tools.import_asset_tasks([t])
    return eal.load_asset(dest + "/" + name)


for f in ([] if os.environ.get("ABYSS_UI_ONLY") else sorted(glob.glob(os.path.join(SRC, "models", "SM_*.fbx")))):
    name = os.path.splitext(os.path.basename(f))[0]
    o = unreal.FbxImportUI()
    o.set_editor_property("import_mesh", True)
    o.set_editor_property("import_as_skeletal", False)
    o.set_editor_property("import_materials", True)   # flat colors from Blender -> UE materials
    o.set_editor_property("import_textures", False)
    o.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)
    sm = o.get_editor_property("static_mesh_import_data")
    sm.set_editor_property("combine_meshes", True)
    sm.set_editor_property("auto_generate_collision", False)
    sm.set_editor_property("generate_lightmap_u_vs", False)
    m = run(f, "/Game/Abyss/Food", name, o)
    unreal.log("[AbyssSetup] food %s ok=%s" % (name, m is not None))

for f in sorted((glob.glob(os.path.join(SRC, "ui", "UI_*.png")) + glob.glob(os.path.join(SRC, "ui", "S_*.png")) + glob.glob(os.path.join(SRC, "ui", "K_*.png")) + glob.glob(os.path.join(SRC, "ui", "H_*.png")))):
    base = os.path.splitext(os.path.basename(f))[0]
    name = "T_" + (base[3:] if base.startswith("UI_") else base)
    tex = run(f, "/Game/Abyss/UI", name)
    if tex:
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
        tex.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
        tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
        tex.set_editor_property("never_stream", True)
        eal.save_loaded_asset(tex)
    unreal.log("[AbyssSetup] ui %s ok=%s" % (name, tex is not None))
