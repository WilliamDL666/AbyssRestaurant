# Thinner underwater fog so the (now 66 m wide) restaurant dome reads from the dock.
import unreal
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les.load_level("/Game/Maps/L_Abyss")
for a in eas.get_all_level_actors():
    if a.get_class().get_name() == "UnderwaterEnvironment":
        a.set_editor_property("fog_density", 0.016)
        unreal.log("[AbyssSetup] fog density -> %s" % a.get_editor_property("fog_density"))
les.save_current_level()
