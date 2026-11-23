# Materials used on Nanite meshes need bUsedWithNanite, or packaged builds fall back to the default material.
import unreal
eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
for p in ("/Game/Abyss/Kitchen/M_Stove", "/Game/Abyss/Kitchen/M_PrepTable", "/Game/Abyss/Kitchen/M_CookCounter", "/Game/Abyss/Kitchen/M_Shelf"):
    m = eal.load_asset(p)
    if m:
        m.set_editor_property("used_with_nanite", True)
        mel.recompile_material(m)
        eal.save_loaded_asset(m, False)
        unreal.log("[AbyssSetup] nanite usage on %s" % p)
