# Re-runs Restaurant_Interior's construction (after C++ changes) and saves L_Abyss.
import unreal
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les.load_level("/Game/Maps/L_Abyss")
for a in eas.get_all_level_actors():
    if a.get_actor_label() == "Restaurant_Interior":
        a.set_editor_property("preview_tier", 1)
        a.set_editor_property("preview_tier", 0)
        unreal.log("[AbyssSetup] dome rebuilt (lux=%s radius=%s)" % (a.get_editor_property("use_lux_level"), a.get_editor_property("radius")))
unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
