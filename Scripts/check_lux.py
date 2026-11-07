import unreal
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les.load_level("/Game/Maps/L_Abyss")
w = unreal.EditorLevelLibrary.get_editor_world()
for a in eas.get_all_level_actors():
    if a.get_actor_label() == "Restaurant_Interior":
        unreal.log("[AbyssSetup] CHECK dome radius=%s lux=%s origin=%s level=%s" % (a.get_editor_property("radius"), a.get_editor_property("use_lux_level"), a.get_editor_property("lux_origin"), a.get_level().get_path_name()))
for l in unreal.EditorLevelUtils.get_levels(w):
    unreal.log("[AbyssSetup] CHECK level %s" % l.get_path_name())
