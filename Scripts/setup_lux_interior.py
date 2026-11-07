# Puts the Luxury Restaurant showcase map inside the undersea glass dome:
#  1) duplicate it to /Game/Abyss/Maps/L_LuxInterior and strip its sky / sun / fog / clouds / post / backdrops
#  2) stream it into L_Abyss (always loaded) centered under Restaurant_Interior, floor aligned to the dome floor
#  3) switch Restaurant_Interior to the "lux level" layout and enlarge the dome to cover the building
import os, collections
import unreal

eal = unreal.EditorAssetLibrary
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
SRC = "/Game/Luxury_Restaurant_Pack/Levels/Luxury_Restaurant_Showcase"
DST = "/Game/Abyss/Maps/L_LuxInterior"
DOME_RADIUS = 3300.0


def log(m):
    unreal.log("[AbyssSetup] " + m)


# ---------------------------------------------------------------- 1) clean copy
if not os.path.exists(os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_content_dir()), "Abyss", "Maps", "L_LuxInterior.umap")):
    # duplicate + save only; loading a freshly duplicated world in the same session trips the GC leak check
    eal.duplicate_asset(SRC, DST)
    eal.save_asset(DST, only_if_is_dirty=False)
    log("duplicated showcase -> " + DST + " (run the script again to finish)")
    raise RuntimeError("first pass done (expected)")
les.load_level(DST)
kill_classes = {"DirectionalLight", "SkyAtmosphere", "ExponentialHeightFog", "VolumetricCloud", "SkyLight",
                "PostProcessVolume", "SphereReflectionCapture", "LightmassImportanceVolume", "PlayerStart"}
killed = collections.Counter()
mn = [1e18] * 3; mx = [-1e18] * 3
floor_tops = collections.Counter()
for a in eas.get_all_level_actors():
    cn = a.get_class().get_name()
    if cn in kill_classes:
        killed[cn] += 1
        eas.destroy_actor(a)
        continue
    if cn == "StaticMeshActor":
        o, e = a.get_actor_bounds(False)
        if e.x > 5000 or e.y > 5000:
            killed["backdrop:" + a.get_actor_label()] += 1
            eas.destroy_actor(a)
            continue
        for i, (c, x) in enumerate(((o.x, e.x), (o.y, e.y), (o.z, e.z))):
            mn[i] = min(mn[i], c - x); mx[i] = max(mx[i], c + x)
        sm = a.static_mesh_component.static_mesh
        if sm and sm.get_name().startswith("SM_LUX_Floor"):
            floor_tops[int(round(o.z + e.z))] += 1
les.save_current_level()
if __import__("os").environ.get("ABYSS_COPY_ONLY"):
    log("fresh lux copy cleaned")
    raise RuntimeError("copy-only pass done (expected)")
center = ((mn[0] + mx[0]) / 2, (mn[1] + mx[1]) / 2)
main_floor = min(floor_tops.most_common(3))[0] if floor_tops else 0
log("lux cleaned: killed=%s bounds=%s..%s center=%s floor tops=%s -> main floor z=%d" % (dict(killed), mn, mx, center, floor_tops.most_common(5), main_floor))

# ---------------------------------------------------------------- 2) stream into L_Abyss
les.load_level("/Game/Maps/L_Abyss")
world = unreal.EditorLevelLibrary.get_editor_world()
dome = None
for a in eas.get_all_level_actors():
    if a.get_actor_label() == "Restaurant_Interior":
        dome = a
if dome is None:
    raise RuntimeError("Restaurant_Interior not found")
dl = dome.get_actor_location()
offset = unreal.Vector(dl.x - center[0], dl.y - center[1], dl.z - main_floor)
xf = unreal.Transform(location=offset, rotation=unreal.Rotator(0, 0, 0), scale=unreal.Vector(1, 1, 1))

existing = None
for sl in unreal.EditorLevelUtils.get_levels(world):
    pass
for st in world.get_streaming_levels() if hasattr(world, "get_streaming_levels") else []:
    if st and str(st.get_world_asset_package_name()).endswith("L_LuxInterior"):
        existing = st
if existing:
    existing.set_editor_property("level_transform", xf)
    log("updated existing streaming level transform")
else:
    try:
        st = unreal.EditorLevelUtils.add_level_to_world_with_transform(world, DST, unreal.LevelStreamingAlwaysLoaded, xf)
    except Exception as ex:
        log("add_level_to_world_with_transform failed (%s), trying add_level_to_world" % ex)
        st = unreal.EditorLevelUtils.add_level_to_world(world, DST, unreal.LevelStreamingAlwaysLoaded)
        if st:
            st.set_editor_property("level_transform", xf)
    log("streaming level added: %s" % st)

# ---------------------------------------------------------------- 3) dome settings
dome.set_editor_property("radius", DOME_RADIUS)
dome.set_editor_property("use_lux_level", True)
dome.set_editor_property("lux_origin", unreal.Vector(center[0], center[1], float(main_floor)))
dome.set_editor_property("preview_tier", 0)
# adding a sublevel makes it the "current" level, so save every dirty map/package (persistent included)
unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
log("L_Abyss saved: lux interior at offset %s, dome radius %.0f" % (offset, DOME_RADIUS))
