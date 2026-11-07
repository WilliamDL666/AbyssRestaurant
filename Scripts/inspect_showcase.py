# Read-only: summarize the Luxury_Restaurant_Showcase map (bounds, actor classes, furniture, lights).
import os, collections
import unreal

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les.load_level("/Game/Luxury_Restaurant_Pack/Levels/Luxury_Restaurant_Showcase")
out = []
classes = collections.Counter()
meshes = collections.Counter()
mn = [1e18] * 3; mx = [-1e18] * 3
furniture = []
others = []
for a in eas.get_all_level_actors():
    cn = a.get_class().get_name()
    classes[cn] += 1
    if cn == "StaticMeshActor":
        smc = a.static_mesh_component
        sm = smc.static_mesh if smc else None
        n = sm.get_name() if sm else "None"
        meshes[n] += 1
        o, e = a.get_actor_bounds(False)
        if e.x < 5000 and e.y < 5000:
            for i, (c, x) in enumerate(((o.x, e.x), (o.y, e.y), (o.z, e.z))):
                mn[i] = min(mn[i], c - x); mx[i] = max(mx[i], c + x)
        low = n.lower()
        if any(k in low for k in ("chair", "table", "bar", "counter", "sofa", "booth", "stool", "kitchen")):
            l = a.get_actor_location()
            furniture.append("%s  %s  (%.0f, %.0f, %.0f) yaw=%.0f" % (a.get_actor_label(), n, l.x, l.y, l.z, a.get_actor_rotation().yaw))
    else:
        l = a.get_actor_location()
        others.append("%s  [%s]  (%.0f, %.0f, %.0f)" % (a.get_actor_label(), cn, l.x, l.y, l.z))
out.append("BOUNDS min=%s max=%s" % (mn, mx))
out.append("CLASSES " + repr(classes.most_common()))
out.append("MESHES " + repr(meshes.most_common(120)))
out.append("---- NON-STATICMESH ACTORS")
out += others
out.append("---- FURNITURE (%d)" % len(furniture))
out += furniture
p = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "showcase_inspect.txt")
open(p, "w", encoding="utf-8").write("\n".join(out))
unreal.log("[AbyssSetup] showcase inspected -> " + p)
