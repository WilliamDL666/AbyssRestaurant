# Imports the rigged Tripo/Mixamo chef as /Game/Abyss/Customer/SK_Customer (+ M_Customer with base color + normal).
import os, glob
import unreal

eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
SRC = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "SourceArt", "Customer")
DEST = "/Game/Abyss/Customer"


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
paths = task(os.path.join(SRC, "Customer.fbx"), "SK_Customer", opts)
unreal.log("[AbyssSetup] customer imported: %s" % paths)

import glob as _g
base = (_g.glob(os.path.join(SRC, "*.fbm", "*.jpg")) + _g.glob(os.path.join(SRC, "*.fbm", "*.png")))[0]
task(base, "T_Customer_BaseColor")
tex = eal.load_asset(DEST + "/T_Customer_BaseColor")
ntex = None

mpath = DEST + "/M_Customer"
if eal.does_asset_exist(mpath):
    mat = eal.load_asset(mpath)
    mel.delete_all_material_expressions(mat)
else:
    mat = tools.create_asset("M_Customer", DEST, unreal.Material, unreal.MaterialFactoryNew())
mat.set_editor_property("used_with_skeletal_mesh", True)
mat.set_editor_property("two_sided", True)  # Tripo face part has inverted normals
ts = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSample, -500, 0)
ts.set_editor_property("texture", tex)
mel.connect_material_property(ts, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
r = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -300, 200)
r.set_editor_property("r", 0.6)
mel.connect_material_property(r, "", unreal.MaterialProperty.MP_ROUGHNESS)
mel.recompile_material(mat)
eal.save_loaded_asset(mat)

sk = eal.load_asset(DEST + "/SK_Customer") if eal.does_asset_exist(DEST + "/SK_Customer") else None
if sk:
    mats = sk.get_editor_property("materials")
    for m in mats:
        m.set_editor_property("material_interface", mat)
    sk.set_editor_property("materials", mats)
    eal.save_loaded_asset(sk)
    b = sk.get_bounds()
    unreal.log("[AbyssSetup] SK_Customer sections=%d bounds extent=%s origin=%s" % (len(mats), b.box_extent, b.origin))
else:
    unreal.log_error("[AbyssSetup] SK_Customer missing")
