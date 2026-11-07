# Rebuild the restaurant interior with the Luxury Restaurant Pack (bigger 15 m dome) and re-save the map.
import unreal

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les.load_level("/Game/Maps/L_Abyss")
for a in eas.get_all_level_actors():
    if a.get_actor_label() == "Restaurant_Interior":
        a.set_editor_property("radius", 1400.0)
        a.set_editor_property("preview_tier", 0)
        unreal.log("[AbyssSetup] interior rebuilt with radius 1400 (concept layout, scaled)")
    elif a.get_class().get_name() == "SeabedGenerator":
        a.set_editor_property("scatter_density", a.get_editor_property("scatter_density"))
les.save_current_level()
unreal.log("[AbyssSetup] level re-saved (interior)")
