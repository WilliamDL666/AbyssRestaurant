# After arrange_lux_round.py moved the bar to the dome center: move the kitchen stations with it and rebuild both domes.
import unreal
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
DX, DY = 380.8 - (-745.0), -996.75 - (-294.0)   # bar shift used by arrange_lux_round.py
les.load_level("/Game/Maps/L_Abyss")
for a in eas.get_all_level_actors():
    if a.get_actor_label() in ("Restaurant_Interior", "Restaurant_Dome"):
        for prop, (x, y) in (("lux_slice", (-700.0, -300.0)), ("lux_cook", (-900.0, -300.0)), ("lux_plate", (-520.0, -300.0))):
            a.set_editor_property(prop, unreal.Vector(x + DX, y + DY, 0.0))
        a.set_editor_property("preview_tier", 1)
        a.set_editor_property("preview_tier", 0)
        unreal.log("[AbyssSetup] stations moved on %s" % a.get_actor_label())
unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
