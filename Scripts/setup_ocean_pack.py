# Uses the AlienOceanEnvironment pack:
#  1) M_AbyssSeabedPBR: world-aligned sand (wavy + beach) with the pack's normals, tinted by the generator's vertex colours
#  2) whales in the level, then re-saves L_Abyss so the new scatter is baked into the map
import unreal

eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
MP = unreal.MaterialProperty
TEX = "/Game/AlienOceanEnvironment/Textures/Landscape/"


def log(m):
    unreal.log("[AbyssSetup] " + m)


def ex(mat, cls, x, y):
    return mel.create_material_expression(mat, cls, x, y)


def link(a, ao, b, bi):
    for n in ([bi] + (["", "Input"] if bi in ("", "Input") else [])):
        if mel.connect_material_expressions(a, ao, b, n):
            return True
    unreal.log_warning("[AbyssSetup] link failed %s.%s -> %s.%s" % (a.get_class().get_name(), ao, b.get_class().get_name(), bi))
    return False


def const(mat, v, x, y):
    e = ex(mat, unreal.MaterialExpressionConstant, x, y)
    e.set_editor_property("r", v)
    return e


def mul(mat, a, b, x, y, ao="", bo=""):
    m = ex(mat, unreal.MaterialExpressionMultiply, x, y)
    link(a, ao, m, "A")
    link(b, bo, m, "B")
    return m


def tex_sample(mat, name, uv, x, y, normal=False):
    t = eal.load_asset(TEX + name)
    e = ex(mat, unreal.MaterialExpressionTextureSample, x, y)
    e.set_editor_property("texture", t)
    if normal:
        e.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    link(uv, "", e, "UVs")
    return e


def make_seabed():
    path = "/Game/Abyss/Materials/M_AbyssSeabedPBR"
    if eal.does_asset_exist(path):
        m = eal.load_asset(path)
        mel.delete_all_material_expressions(m)
    else:
        m = tools.create_asset("M_AbyssSeabedPBR", "/Game/Abyss/Materials", unreal.Material, unreal.MaterialFactoryNew())
    m.set_editor_property("two_sided", True)

    wp = ex(m, unreal.MaterialExpressionWorldPosition, -1800, 0)
    xy = ex(m, unreal.MaterialExpressionComponentMask, -1600, 0)
    for c, on in (("r", True), ("g", True), ("b", False), ("a", False)):
        xy.set_editor_property(c, on)
    link(wp, "", xy, "")
    uv1 = ex(m, unreal.MaterialExpressionDivide, -1400, -100)
    link(xy, "", uv1, "A"); link(const(m, 700.0, -1550, 120), "", uv1, "B")
    uv2 = ex(m, unreal.MaterialExpressionDivide, -1400, 250)
    link(xy, "", uv2, "A"); link(const(m, 2300.0, -1550, 400), "", uv2, "B")

    d1 = tex_sample(m, "T_Landscape_Sand_Wavy_Dif", uv1, -1100, -300)
    d2 = tex_sample(m, "T_Landscape_Sand_Beach_Dif", uv2, -1100, -50)
    n1 = tex_sample(m, "T_Landscape_Sand_Wavy_Norm", uv1, -1100, 250, normal=True)
    n2 = tex_sample(m, "T_Landscape_Sand_Beach_Norm", uv2, -1100, 500, normal=True)

    blend = ex(m, unreal.MaterialExpressionNoise, -1100, 800)
    blend.set_editor_property("scale", 0.0015)
    blend.set_editor_property("output_min", 0.0)
    blend.set_editor_property("output_max", 1.0)

    lc = ex(m, unreal.MaterialExpressionLinearInterpolate, -700, -200)
    link(d1, "RGB", lc, "A"); link(d2, "RGB", lc, "B"); link(blend, "", lc, "Alpha")
    vc = ex(m, unreal.MaterialExpressionVertexColor, -700, 50)
    tint = mul(m, vc, const(m, 1.7, -800, 150), -550, 50)
    base = mul(m, lc, tint, -350, -100)
    mel.connect_material_property(base, "", MP.MP_BASE_COLOR)

    ln = ex(m, unreal.MaterialExpressionLinearInterpolate, -700, 400)
    link(n1, "RGB", ln, "A"); link(n2, "RGB", ln, "B"); link(blend, "", ln, "Alpha")
    mel.connect_material_property(ln, "", MP.MP_NORMAL)
    mel.connect_material_property(const(m, 0.9, -350, 300), "", MP.MP_ROUGHNESS)
    mel.recompile_material(m)
    eal.save_loaded_asset(m)
    log("seabed PBR material ok")


def place_whales_and_save():
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    les.load_level("/Game/Maps/L_Abyss")
    for a in eas.get_all_level_actors():
        if a.get_actor_label().startswith("Whale_"):
            eas.destroy_actor(a)
    cls = unreal.EditorAssetLibrary.load_blueprint_class("/Game/AlienOceanEnvironment/Blueprints/BP_BoziWhale")
    if cls:
        for i, (loc, yaw) in enumerate([((-9000, 12000, -2200), 30.0), ((16000, -9000, -6500), 150.0), ((-60000, 6000, -1800), -60.0)]):
            w = eas.spawn_actor_from_class(cls, unreal.Vector(*loc), unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw))
            w.set_actor_label("Whale_%d" % i)
        log("whales placed")
    # touching the seabed reruns its construction script with the new scatter
    for a in eas.get_all_level_actors():
        if a.get_class().get_name() in ("SeabedGenerator", "RestaurantDome", "UnderwaterEnvironment"):
            a.set_editor_property("hidden", False) if False else None
            try:
                a.rerun_construction_scripts()
            except Exception:
                # C++ actors: nudging a property triggers PostEditChange -> OnConstruction
                if a.get_class().get_name() == "SeabedGenerator":
                    a.set_editor_property("scatter_density", a.get_editor_property("scatter_density"))
                elif a.get_class().get_name() == "RestaurantDome":
                    a.set_editor_property("light_scale", a.get_editor_property("light_scale"))
                else:
                    a.set_editor_property("exposure_compensation", a.get_editor_property("exposure_compensation"))
    les.save_current_level()
    log("level re-saved")


make_seabed()
place_whales_and_save()
