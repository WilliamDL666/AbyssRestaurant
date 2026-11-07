# Logs what is in /Game/AlienOceanEnvironment: classes, mesh bounds, material params. Output -> Saved/ocean_pack.txt
import os
import unreal

eal = unreal.EditorAssetLibrary
ROOT = "/Game/AlienOceanEnvironment"
out = []
for p in sorted(eal.list_assets(ROOT, recursive=True, include_folder=False)):
    path = p.split(".")[0]
    data = eal.find_asset_data(path)
    cls = str(data.asset_class_path.asset_name)
    line = "%s | %s" % (cls, path)
    if cls in ("StaticMesh",):
        a = eal.load_asset(path)
        b = a.get_bounds()
        mats = [str(m.material_interface.get_path_name()) if m.material_interface else "None" for m in a.static_materials]
        line += " | extent=(%.0f,%.0f,%.0f) origin=(%.0f,%.0f,%.0f) | %s" % (b.box_extent.x, b.box_extent.y, b.box_extent.z, b.origin.x, b.origin.y, b.origin.z, ";".join(mats))
    elif cls in ("MaterialInstanceConstant",):
        a = eal.load_asset(path)
        try:
            parent = a.get_editor_property("parent").get_path_name()
        except Exception:
            parent = "?"
        sc = [str(x) for x in unreal.MaterialEditingLibrary.get_scalar_parameter_names(a)]
        vc = [str(x) for x in unreal.MaterialEditingLibrary.get_vector_parameter_names(a)]
        line += " | parent=%s | scalars=%s | vectors=%s" % (parent, sc, vc)
    elif cls in ("Material",):
        a = eal.load_asset(path)
        try:
            line += " | domain=%s blend=%s" % (a.get_editor_property("material_domain"), a.get_editor_property("blend_mode"))
        except Exception:
            pass
    out.append(line)
dst = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "ocean_pack.txt")
open(dst, "w", encoding="utf-8").write("\n".join(out))
unreal.log("[AbyssSetup] ocean pack inventory: %d assets -> %s" % (len(out), dst))
