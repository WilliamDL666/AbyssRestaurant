import unreal
eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
for b in ("Stove", "PrepTable"):
    m = eal.load_asset("/Game/Abyss/Kitchen/M_" + b)
    n = mel.get_num_material_expressions(m)
    bc = mel.get_material_property_input_node(m, unreal.MaterialProperty.MP_BASE_COLOR)
    unreal.log("[AbyssSetup] CHK %s exprs=%d basecolor_node=%s" % (b, n, bc))
    t = eal.load_asset("/Game/Abyss/Kitchen/T_%s_basecolor" % b)
    unreal.log("[AbyssSetup] CHK tex %s size=%sx%s srgb=%s" % (b, t.blueprint_get_size_x(), t.blueprint_get_size_y(), t.get_editor_property("srgb")))
    sm = eal.load_asset("/Game/Abyss/Kitchen/SM_" + b)
    unreal.log("[AbyssSetup] CHK mesh mats=%s nanite=%s" % ([s.get_editor_property("material_interface").get_name() if s.get_editor_property("material_interface") else None for s in sm.get_editor_property("static_materials")],
        sm.get_editor_property("nanite_settings").get_editor_property("enabled")))
