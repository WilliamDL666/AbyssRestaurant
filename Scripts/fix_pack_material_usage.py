# Pack materials we render through (H)ISM components need the "Used with Instanced Static Meshes" flag,
# otherwise standalone/packaged builds fall back to the default grey material.
import unreal

eal = unreal.EditorAssetLibrary
fixed = 0
for p in eal.list_assets("/Game/AlienOceanEnvironment/Materials", recursive=True, include_folder=False):
    path = p.split(".")[0]
    data = eal.find_asset_data(path)
    if str(data.asset_class_path.asset_name) != "Material":
        continue
    m = eal.load_asset(path)
    if m.get_editor_property("material_domain") != unreal.MaterialDomain.MD_SURFACE:
        continue
    if not m.get_editor_property("used_with_instanced_static_meshes"):
        m.set_editor_property("used_with_instanced_static_meshes", True)
        unreal.MaterialEditingLibrary.recompile_material(m)
        eal.save_loaded_asset(m)
        fixed += 1
        unreal.log("[AbyssSetup] ISM usage enabled: %s" % path)
unreal.log("[AbyssSetup] pack materials fixed: %d" % fixed)
