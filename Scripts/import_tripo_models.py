# Imports the user's second batch of Tripo models as Nanite static meshes with PBR materials (textures capped at 2K):
#   NewA (display tank, glass wall cut by blender_cut_tank_glass.py) -> /Game/Abyss/Aquarium/SM_TankModel
#   NewB (booth sofa) -> /Game/Abyss/Props/SM_Booth      NewC (seafood fridge) -> /Game/Abyss/Kitchen/SM_SeafoodFridge
#   NewD (bluefin tuna) -> /Game/Abyss/Fish/SM_TunaModel
import os, glob
import unreal

eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
PROJ = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
MODELS = [("NewA", "ModelCut.fbx", "/Game/Abyss/Aquarium", "TankModel"), ("NewB", "ModelLow.fbx", "/Game/Abyss/Props", "Booth"),
          ("NewC", "ModelLow.fbx", "/Game/Abyss/Kitchen", "SeafoodFridge"), ("NewD", "ModelLow.fbx", "/Game/Abyss/Fish", "TunaModel")]
only = os.environ.get("ABYSS_ONLY")
if only:
    MODELS = [m for m in MODELS if m[3] in only.split(",")]


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


for folder, fbxname, dest, base in MODELS:
    src = os.path.join(PROJ, "SourceArt", folder)
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
    mesh = run(os.path.join(src, fbxname), dest, "SM_" + base, o)

    fbm = os.path.join(src, "Model.fbm")
    pick = {"basecolor": [f for f in glob.glob(os.path.join(fbm, "tripo_image_*_0.jpg"))],
            "normal": glob.glob(os.path.join(fbm, "NormalGL_*.jpg")),
            "roughness": glob.glob(os.path.join(fbm, "*_Roughness.jpg")),
            "metallic": glob.glob(os.path.join(fbm, "*_Metallic.jpg"))}
    texs = {}
    for kind, files in pick.items():
        if not files:
            continue
        t = run(files[0], dest, "T_%s_%s" % (base, kind))
        if not t:
            continue
        t.set_editor_property("max_texture_size", 2048)
        if kind != "basecolor":
            t.set_editor_property("srgb", False)
        if kind == "normal":
            t.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
        eal.save_loaded_asset(t)
        texs[kind] = t

    mpath = dest + "/M_" + base
    if eal.does_asset_exist(mpath):
        mat = eal.load_asset(mpath)
        mel.delete_all_material_expressions(mat)
    else:
        mat = tools.create_asset("M_" + base, dest, unreal.Material, unreal.MaterialFactoryNew())
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
    for flag in ("used_with_nanite", "used_with_static_lighting"):
        try:
            mat.set_editor_property(flag, True)
        except Exception:
            pass
    # the cut tank and the fish are seen from the inside / both sides
    mat.set_editor_property("two_sided", base in ("TankModel",))
    mel.recompile_material(mat)
    eal.save_loaded_asset(mat)
    if mesh:
        mesh.set_material(0, mat)
        eal.save_loaded_asset(mesh, False)
        b = mesh.get_bounds()
        unreal.log("[AbyssSetup] tripo %s imported, extent=%s origin=%s" % (base, b.box_extent, b.origin))
    else:
        unreal.log("[AbyssSetup] tripo %s FAILED" % base)
