# Assigns M_Stove / M_PrepTable to their meshes (the import left WorldGridMaterial) and reports texture source sizes.
import unreal
eal = unreal.EditorAssetLibrary
for b in ("Stove", "PrepTable"):
    sm = eal.load_asset("/Game/Abyss/Kitchen/SM_" + b)
    mat = eal.load_asset("/Game/Abyss/Kitchen/M_" + b)
    for i in range(sm.get_num_sections(0) if hasattr(sm, "get_num_sections") else 1):
        sm.set_material(i, mat)
    sm.set_material(0, mat)
    eal.save_loaded_asset(sm, False)
    for k in ("basecolor", "normal", "roughness", "metallic"):
        t = eal.load_asset("/Game/Abyss/Kitchen/T_%s_%s" % (b, k))
        unreal.log("[AbyssSetup] FIX %s %s size=%dx%d" % (b, k, t.blueprint_get_size_x(), t.blueprint_get_size_y()))
    unreal.log("[AbyssSetup] FIX %s mats now %s" % (b, [s.get_editor_property("material_interface").get_name() for s in sm.get_editor_property("static_materials")]))
