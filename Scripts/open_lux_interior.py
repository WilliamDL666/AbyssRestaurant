# Makes the restaurant copy open-plan (the glass dome is its only shell) and gives the exterior dome the same layout.
#  - L_LuxInterior: delete ceilings / hanging ceiling decor / walls / windows / doors / wall frames, lamps and mirrors
#  - Restaurant_Dome (exterior): radius 3300, lux layout (a second copy of the map is streamed in at runtime)
import collections
import unreal

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
DROP = ("Ceiling", "Wall_Set", "Window", "Wall_Frame", "Lamp_Wall", "Mirror", "Light_Spot", "Wood_Beam", "Door")


def log(m):
    unreal.log("[AbyssSetup] " + m)


les.load_level("/Game/Abyss/Maps/L_LuxInterior")
gone = collections.Counter()
for a in eas.get_all_level_actors():
    if a.get_class().get_name() != "StaticMeshActor":
        continue
    sm = a.static_mesh_component.static_mesh
    n = sm.get_name() if sm else ""
    hit = next((k for k in DROP if k in n), None)
    if hit:
        gone[hit] += 1
        eas.destroy_actor(a)
unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
log("open-plan: removed %s" % dict(gone))

les.load_level("/Game/Maps/L_Abyss")
inner = outer = None
for a in eas.get_all_level_actors():
    if a.get_actor_label() == "Restaurant_Interior":
        inner = a
    elif a.get_actor_label() == "Restaurant_Dome":
        outer = a
outer.set_editor_property("radius", inner.get_editor_property("radius"))
outer.set_editor_property("lux_origin", inner.get_editor_property("lux_origin"))
outer.set_editor_property("use_lux_level", True)
inner.set_editor_property("preview_tier", 1)
inner.set_editor_property("preview_tier", 0)
unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
log("exterior dome now radius %.0f with the lux layout" % outer.get_editor_property("radius"))
