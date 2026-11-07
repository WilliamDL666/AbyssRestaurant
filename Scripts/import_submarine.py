# Re-import the submarine (replaces /Game/Abyss/Submarine/SM_Submarine in place):
#  - FBX made by Scripts/blender_fix_sub.py (straightened, nose +X, cockpit glass slot "Glass")
#  - PBR hull material from the Tripo textures (basecolor/normal/roughness/metallic)
#  - Nanite on
import os
import glob
import unreal

eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
PROJ = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SRC = os.path.join(PROJ, "SourceArt", "Submarine4")
SUB_FBX = os.path.join(SRC, "SM_Submarine.fbx")
DEST = "/Game/Abyss/Submarine"


def log(m):
    unreal.log("[AbyssSetup] " + m)


def import_file(path, name, options=None):
    t = unreal.AssetImportTask()
    t.set_editor_property("filename", os.path.normpath(path))
    t.set_editor_property("destination_path", DEST)
    t.set_editor_property("destination_name", name)
    t.set_editor_property("automated", True)
    t.set_editor_property("replace_existing", True)
    t.set_editor_property("save", True)
    if options:
        t.set_editor_property("options", options)
    tools.import_asset_tasks([t])
    return eal.load_asset(DEST + "/" + name)


# ---- mesh
opts = unreal.FbxImportUI()
opts.set_editor_property("import_mesh", True)
opts.set_editor_property("import_as_skeletal", False)
opts.set_editor_property("import_materials", False)
opts.set_editor_property("import_textures", False)
opts.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)
sm = opts.get_editor_property("static_mesh_import_data")
sm.set_editor_property("combine_meshes", True)
sm.set_editor_property("generate_lightmap_u_vs", False)
sm.set_editor_property("auto_generate_collision", False)
try:
    sm.set_editor_property("build_nanite", True)
except Exception:
    pass
mesh = import_file(SUB_FBX, "SM_Submarine", opts)
log("submarine re-imported: %s" % mesh)

# ---- textures
fbm = glob.glob(os.path.join(SRC, "*.fbm"))[0]
def tex(kind, srgb=True, normal=False):
    files = glob.glob(os.path.join(fbm, "*_%s.jpg" % kind)) + glob.glob(os.path.join(fbm, "*_%s.png" % kind))
    if not files:
        return None
    t = import_file(files[0], "T_SubHull_" + kind)
    if normal:
        t.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
        t.set_editor_property("srgb", False)
    elif not srgb:
        t.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
        t.set_editor_property("srgb", False)
    eal.save_loaded_asset(t)
    return t

t_base = tex("basecolor")
t_norm = tex("normal", normal=True)
t_rough = tex("roughness", srgb=False)
t_metal = tex("metallic", srgb=False)

# ---- PBR hull material
path = DEST + "/M_SubmarineHull"
if eal.does_asset_exist(path):
    mat = eal.load_asset(path)
    mel.delete_all_material_expressions(mat)
else:
    mat = tools.create_asset("M_SubmarineHull", DEST, unreal.Material, unreal.MaterialFactoryNew())

def sample(t, x, y, stype=None):
    e = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSample, x, y)
    e.set_editor_property("texture", t)
    if stype is not None:
        e.set_editor_property("sampler_type", stype)
    return e

if t_base:
    mel.connect_material_property(sample(t_base, -600, -200), "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
if t_norm:
    mel.connect_material_property(sample(t_norm, -600, 0, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL), "RGB", unreal.MaterialProperty.MP_NORMAL)
if t_rough:
    mel.connect_material_property(sample(t_rough, -600, 200, unreal.MaterialSamplerType.SAMPLERTYPE_MASKS), "R", unreal.MaterialProperty.MP_ROUGHNESS)
if t_metal:
    mel.connect_material_property(sample(t_metal, -600, 400, unreal.MaterialSamplerType.SAMPLERTYPE_MASKS), "R", unreal.MaterialProperty.MP_METALLIC)
mel.recompile_material(mat)
eal.save_loaded_asset(mat)

# ---- glass
gpath = DEST + "/MI_SubmarineGlass"
glass = eal.load_asset(gpath) if eal.does_asset_exist(gpath) else tools.create_asset("MI_SubmarineGlass", DEST, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
glass.set_editor_property("parent", eal.load_asset("/Game/Abyss/Materials/M_AbyssGlass"))
mel.set_material_instance_vector_parameter_value(glass, "Color", unreal.LinearColor(0.6, 0.85, 1.0, 1.0))
mel.set_material_instance_scalar_parameter_value(glass, "Opacity", 0.1)
eal.save_loaded_asset(glass)

for i, m in enumerate(mesh.static_materials):
    name = str(m.material_slot_name)
    is_glass = "glass" in name.lower()
    mesh.set_material(i, glass if is_glass else mat)
    log("slot %d '%s' -> %s" % (i, name, "glass" if is_glass else "hull PBR"))
eal.save_loaded_asset(mesh)
