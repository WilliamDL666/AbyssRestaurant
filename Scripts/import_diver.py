# Imports the rigged Tripo/Mixamo diver as /Game/Abyss/Diver/SK_Diver (+ its texture material).
import os, glob
import unreal

eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
SRC = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "SourceArt", "Diver")
DEST = "/Game/Abyss/Diver"


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
opts.set_editor_property("import_animations", True)
opts.set_editor_property("import_materials", False)
opts.set_editor_property("import_textures", False)
opts.set_editor_property("create_physics_asset", False)
opts.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_SKELETAL_MESH)
paths = task(os.path.join(SRC, "diver.fbx"), "SK_Diver", opts)
unreal.log("[AbyssSetup] diver imported: %s" % paths)

tex_file = glob.glob(os.path.join(SRC, "*.fbm", "*.jpg"))[0]
task(tex_file, "T_Diver_BaseColor")
tex = eal.load_asset(DEST + "/T_Diver_BaseColor")

mpath = DEST + "/M_Diver"
if eal.does_asset_exist(mpath):
    mat = eal.load_asset(mpath)
    mel.delete_all_material_expressions(mat)
else:
    mat = tools.create_asset("M_Diver", DEST, unreal.Material, unreal.MaterialFactoryNew())
mat.set_editor_property("used_with_skeletal_mesh", True)
ts = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSample, -500, 0)
ts.set_editor_property("texture", tex)
mel.connect_material_property(ts, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
r = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -300, 200)
r.set_editor_property("r", 0.45)
mel.connect_material_property(r, "", unreal.MaterialProperty.MP_ROUGHNESS)
mel.recompile_material(mat)
eal.save_loaded_asset(mat)

sk = None
for p in paths:
    a = eal.load_asset(p.split(".")[0])
    if isinstance(a, unreal.SkeletalMesh):
        sk = a
if sk is None and eal.does_asset_exist(DEST + "/SK_Diver"):
    sk = eal.load_asset(DEST + "/SK_Diver")
if sk:
    mats = sk.get_editor_property("materials")
    for m in mats:
        m.set_editor_property("material_interface", mat)
    sk.set_editor_property("materials", mats)
    eal.save_loaded_asset(sk)
    b = sk.get_bounds()
    unreal.log("[AbyssSetup] SK_Diver bounds extent=%s origin=%s" % (b.box_extent, b.origin))
