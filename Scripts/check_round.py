import math, collections
import unreal
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les.load_level("/Game/Abyss/Maps/L_LuxInterior")
O = (380.8, -996.75)
rings = collections.Counter()
zs = collections.Counter()
for a in eas.get_all_level_actors():
    if a.get_class().get_name() != "StaticMeshActor":
        continue
    sm = a.static_mesh_component.static_mesh
    if sm and sm.get_name().startswith("SM_LUX_Table"):
        l = a.get_actor_location()
        rings[int(round(math.hypot(l.x - O[0], l.y - O[1]) / 100.0) * 100)] += 1
        zs[int(l.z)] += 1
unreal.log("[AbyssSetup] ROUND table radii=%s z=%s" % (sorted(rings.items()), zs.most_common(6)))
fl = unreal.load_asset("/Game/Luxury_Restaurant_Pack/Meshes/SM_LUX_Floor_2X2")
if fl:
    b = fl.get_bounding_box()
    unreal.log("[AbyssSetup] ROUND floor2x2 box min=%s max=%s" % (b.min, b.max))
