# Imports the user's Tripo kitchen models (SourceArt/Kitchen*/Model.fbx) as Nanite static meshes with PBR materials.
#   KitchenA -> /Game/Abyss/Kitchen/SM_Stove      KitchenB -> /Game/Abyss/Kitchen/SM_PrepTable
import os, glob
import unreal

eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
PROJ = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
DEST = "/Game/Abyss/Kitchen"
MODELS = {"KitchenA": "Stove", "KitchenB": "PrepTable", "KitchenC": "CookCounter", "KitchenD": "Shelf"}
import os as _os
if _os.environ.get("ABYSS_ONLY"): MODELS = {k: v for k, v in MODELS.items() if v in _os.environ["ABYSS_ONLY"].split(",")}


def run(path, name, opts=None):
    t = unreal.AssetImportTask()
    t.set_editor_property("filename", os.path.normpath(path))
    t.set_editor_property("destination_path", DEST)
    t.set_editor_property("destination_name", name)
    t.set_editor_property("automated", True)
    t.set_editor_property("replace_existing", True)
    t.set_editor_property("save", True)
    if opts:
        t.set_editor_property("options", opts)
    tools.import_asset_tasks([t])
    return eal.load_asset(DEST + "/" + name)


for folder, base in MODELS.items():
    src = os.path.join(PROJ, "SourceArt", folder)
    fbx = os.path.join(src, "Model.fbx")
    if not os.path.exists(fbx):
        continue
    o = unreal.FbxImportUI()
    o.set_editor_property("import_mesh", True)
    o.set_editor_property("import_as_skeletal", False)
    o.set_editor_property("import_materials", False)
    o.set_editor_property("import_textures", False)
    o.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)
    sm = o.get_editor_property("static_mesh_import_data")
    sm.set_editor_property("combine_meshes", True)
    sm.set_editor_property("generate_lightmap_u_vs", False)
    sm.set_editor_property("auto_generate_collision", False)
    try:
        sm.set_editor_property("build_nanite", True)
    except Exception:
        pass
    mesh = run(fbx, "SM_" + base, o)

    texs = {}
    for kind in ("basecolor", "normal", "roughness", "metallic"):
        f = glob.glob(os.path.join(src, "Model.fbm", "*_%s.jpg" % kind))
        if not f:
            continue
        t = run(f[0], "T_%s_%s" % (base, kind))
        if kind != "basecolor":
            t.set_editor_property("srgb", False)
        if kind == "normal":
            t.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
        eal.save_loaded_asset(t)
        texs[kind] = t

    mpath = DEST + "/M_" + base
    if eal.does_asset_exist(mpath):
        mat = eal.load_asset(mpath)
        mel.delete_all_material_expressions(mat)
    else:
        mat = tools.create_asset("M_" + base, DEST, unreal.Material, unreal.MaterialFactoryNew())
    y = 0
    for kind, prop, out in (("basecolor", unreal.MaterialProperty.MP_BASE_COLOR, "RGB"), ("normal", unreal.MaterialProperty.MP_NORMAL, "RGB"),
                            ("roughness", unreal.MaterialProperty.MP_ROUGHNESS, "R"), ("metallic", unreal.MaterialProperty.MP_METALLIC, "R")):
        if kind not in texs:
            continue
        ts = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSample, -500, y)
        ts.set_editor_property("texture", texs[kind])
        if kind == "normal":
            ts.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
        elif kind != "basecolor":
            ts.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
        mel.connect_material_property(ts, out, prop)
        y += 260
    mel.recompile_material(mat)
    eal.save_loaded_asset(mat)
    if mesh:
        mesh.set_material(0, mat)   # (setting static_materials did not persist)
        eal.save_loaded_asset(mesh, False)
        b = mesh.get_bounds()
        unreal.log("[AbyssSetup] kitchen %s imported, extent=%s origin=%s" % (base, b.box_extent, b.origin))
