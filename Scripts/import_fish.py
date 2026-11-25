# Imports the Blender fish / den (Scripts/blender_make_fish.py) into /Game/Abyss/Fish, plus the new stew dish,
# pot picture and seasoning icons. Fish material slots keep their Blender names (Back/Belly/Stripe/Fin/Eye, Rock/Coral/Glow);
# the game code assigns colored material instances per slot.
import os, glob
import unreal

tools = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary
ROOT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())


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


def mesh_opts(materials):
    o = unreal.FbxImportUI()
    o.set_editor_property("import_mesh", True)
    o.set_editor_property("import_as_skeletal", False)
    o.set_editor_property("import_materials", materials)
    o.set_editor_property("import_textures", False)
    o.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)
    sm = o.get_editor_property("static_mesh_import_data")
    sm.set_editor_property("combine_meshes", True)
    sm.set_editor_property("auto_generate_collision", False)
    sm.set_editor_property("generate_lightmap_u_vs", False)
    return o


for f in sorted(glob.glob(os.path.join(ROOT, "SourceArt", "Fish", "SM_*.fbx"))):
    name = os.path.splitext(os.path.basename(f))[0]
    m = run(f, "/Game/Abyss/Fish", name, mesh_opts(False))
    slots = [str(s.get_editor_property("material_slot_name")) for s in m.get_editor_property("static_materials")] if m else []
    unreal.log("[AbyssSetup] fish %s slots=%s origin=%s extent=%s" % (name, slots, m.get_bounds().origin if m else None, m.get_bounds().box_extent if m else None))

for f in sorted(glob.glob(os.path.join(ROOT, "SourceArt", "Aquarium", "SM_*.fbx"))):
    name = os.path.splitext(os.path.basename(f))[0]
    m = run(f, "/Game/Abyss/Aquarium", name, mesh_opts(False))
    unreal.log("[AbyssSetup] aquarium %s slots=%s" % (name, [str(s.get_editor_property("material_slot_name")) for s in m.get_editor_property("static_materials")] if m else None))

f = os.path.join(ROOT, "SourceArt", "Food", "models", "SM_Dish_Stew.fbx")
m = run(f, "/Game/Abyss/Food", "SM_Dish_Stew", mesh_opts(True))
unreal.log("[AbyssSetup] food SM_Dish_Stew ok=%s" % (m is not None))

UI = os.path.join(ROOT, "SourceArt", "Food", "ui")
for base in ["UI_Pot", "UI_Dish_4", "UI_Ing_8", "UI_Bait", "UI_Fish_0", "UI_Fish_1", "UI_Fish_2"] + ["UI_Sea_%d" % i for i in range(1, 8)]:
    tex = run(os.path.join(UI, base + ".png"), "/Game/Abyss/UI", "T_" + base[3:])
    if tex:
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
        tex.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
        tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
        tex.set_editor_property("never_stream", True)
        eal.save_loaded_asset(tex)
    unreal.log("[AbyssSetup] ui %s ok=%s" % (base, tex is not None))
