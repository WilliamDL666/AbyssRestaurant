# Sets both restaurant domes to the same radius and rebuilds them.
import os, unreal
R = float(os.environ.get("ABYSS_RADIUS", "2200"))
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les.load_level("/Game/Maps/L_Abyss")
for a in eas.get_all_level_actors():
    if a.get_actor_label() in ("Restaurant_Interior", "Restaurant_Dome"):
        a.set_editor_property("radius", R)
        a.set_editor_property("preview_tier", 1); a.set_editor_property("preview_tier", 0)
unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
unreal.log("[AbyssSetup] dome radius -> %.0f" % R)
