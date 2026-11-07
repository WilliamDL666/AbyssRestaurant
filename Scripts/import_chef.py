# Imports the rigged Tripo/Mixamo chef as /Game/Abyss/Chef/SK_Chef (+ M_Chef with base color + normal).
import os, glob
import unreal

eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
SRC = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "SourceArt", "Chef")
DEST = "/Game/Abyss/Chef"


def task(path, name, opts=None):
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
    return list(t.get_editor_property("imported_object_paths"))


opts = unreal.FbxImportUI()
opts.set_editor_property("import_mesh", True)
opts.set_editor_property("import_as_skeletal", True)
opts.set_editor_property("import_animations", False)
opts.set_editor_property("import_materials", False)
opts.set_editor_property("import_textures", False)
opts.set_editor_property("create_physics_asset", False)
opts.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_SKELETAL_MESH)
opts.skeletal_mesh_import_data.set_editor_property("import_meshes_in_bone_hierarchy", True)
paths = task(os.path.join(SRC, "Chef.fbx"), "SK_Chef", opts)
unreal.log("[AbyssSetup] chef imported: %s" % paths)

base = [f for f in glob.glob(os.path.join(SRC, "*.fbm", "*.jpg")) if "basecolor" in f.lower()][0]
norm = [f for f in glob.glob(os.path.join(SRC, "*.fbm", "*.jpg")) if "normal" in f.lower()][0]
task(base, "T_Chef_BaseColor")
task(norm, "T_Chef_Normal")
tex = eal.load_asset(DEST + "/T_Chef_BaseColor")
ntex = eal.load_asset(DEST + "/T_Chef_Normal")
ntex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
ntex.set_editor_property("srgb", False)
eal.save_loaded_asset(ntex)

mpath = DEST + "/M_Chef"
if eal.does_asset_exist(mpath):
    mat = eal.load_asset(mpath)
    mel.delete_all_material_expressions(mat)
else:
    mat = tools.create_asset("M_Chef", DEST, unreal.Material, unreal.MaterialFactoryNew())
mat.set_editor_property("used_with_skeletal_mesh", True)
mat.set_editor_property("two_sided", True)  # Tripo face part has inverted normals
ts = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSample, -500, 0)
ts.set_editor_property("texture", tex)
mel.connect_material_property(ts, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
ns = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSample, -500, 300)
ns.set_editor_property("texture", ntex)
ns.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
mel.connect_material_property(ns, "RGB", unreal.MaterialProperty.MP_NORMAL)
r = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -300, 200)
r.set_editor_property("r", 0.6)
mel.connect_material_property(r, "", unreal.MaterialProperty.MP_ROUGHNESS)
mel.recompile_material(mat)
eal.save_loaded_asset(mat)

sk = eal.load_asset(DEST + "/SK_Chef") if eal.does_asset_exist(DEST + "/SK_Chef") else None
if sk:
    mats = sk.get_editor_property("materials")
    for m in mats:
        m.set_editor_property("material_interface", mat)
    sk.set_editor_property("materials", mats)
    eal.save_loaded_asset(sk)
    b = sk.get_bounds()
    unreal.log("[AbyssSetup] SK_Chef sections=%d bounds extent=%s origin=%s" % (len(mats), b.box_extent, b.origin))
else:
    unreal.log_error("[AbyssSetup] SK_Chef missing")
