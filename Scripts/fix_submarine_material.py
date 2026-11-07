# Builds M_SubmarineHull from the model's basecolor texture and assigns it to SM_Submarine.
import os
import unreal

eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
DEST = "/Game/Abyss/Submarine"
TEX_FILE = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()),
                        "SourceArt", "Submarine2", "yellow_sci-fi_vehicle_3d_model.fbm", "yellow_sci-fi_vehicle_3d_model_basecolor.jpg")

# 1) texture
task = unreal.AssetImportTask()
task.set_editor_property("filename", os.path.normpath(TEX_FILE))
task.set_editor_property("destination_path", DEST)
task.set_editor_property("destination_name", "T_SubmarineHull_BaseColor")
task.set_editor_property("automated", True)
task.set_editor_property("replace_existing", True)
task.set_editor_property("save", True)
tools.import_asset_tasks([task])
tex = eal.load_asset(DEST + "/T_SubmarineHull_BaseColor")
unreal.log("[AbyssSetup] hull texture: %s" % tex)

# 2) material
path = DEST + "/M_SubmarineHull"
if eal.does_asset_exist(path):
    mat = eal.load_asset(path)
    mel.delete_all_material_expressions(mat)
else:
    mat = tools.create_asset("M_SubmarineHull", DEST, unreal.Material, unreal.MaterialFactoryNew())
ts = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSample, -500, 0)
ts.set_editor_property("texture", tex)
mel.connect_material_property(ts, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
r = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -300, 200)
r.set_editor_property("r", 0.55)
mel.connect_material_property(r, "", unreal.MaterialProperty.MP_ROUGHNESS)
m = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -300, 300)
m.set_editor_property("r", 0.35)
mel.connect_material_property(m, "", unreal.MaterialProperty.MP_METALLIC)
mel.recompile_material(mat)
eal.save_loaded_asset(mat)

# 3) assign to every slot of the hull mesh
mesh = eal.load_asset(DEST + "/SM_Submarine")
n = len(mesh.static_materials)
for i in range(n):
    mesh.set_material(i, mat)
eal.save_loaded_asset(mesh)
unreal.log("[AbyssSetup] hull material assigned to %d slot(s)" % n)

if "-AbyssSetupQuit" in unreal.SystemLibrary.get_command_line():
    unreal.SystemLibrary.quit_editor()
