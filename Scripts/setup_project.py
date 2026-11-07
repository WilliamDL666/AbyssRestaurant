"""
Abyss Restaurant - one-shot project setup.
Creates the base materials and builds /Game/Maps/L_Abyss with every gameplay actor placed.

Run automatically by build_and_setup.bat, or inside the editor:
  Output Log -> Cmd dropdown -> Python ->  py "F:/ZUOPINJI/AbyssRestaurant/Scripts/setup_project.py"
"""
import unreal

MAT_DIR = "/Game/Abyss/Materials"
MAP_PATH = "/Game/Maps/L_Abyss"
QUIT_WHEN_DONE = "-AbyssSetupQuit" in unreal.SystemLibrary.get_command_line()

mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
MP = unreal.MaterialProperty


def log(msg):
    unreal.log("[AbyssSetup] " + msg)


# ----------------------------------------------------------------- material helpers

def new_material(name):
    path = MAT_DIR + "/" + name
    if eal.does_asset_exist(path):
        # rebuild in place (the map already references it, deleting would break references)
        mat = eal.load_asset(path)
        mel.delete_all_material_expressions(mat)
        return mat
    return tools.create_asset(name, MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())


def ex(mat, cls, x, y):
    return mel.create_material_expression(mat, cls, x, y)


def vparam(mat, name, val, x, y):
    e = ex(mat, unreal.MaterialExpressionVectorParameter, x, y)
    e.set_editor_property("parameter_name", name)
    e.set_editor_property("default_value", unreal.LinearColor(*val))
    return e


def sparam(mat, name, val, x, y):
    e = ex(mat, unreal.MaterialExpressionScalarParameter, x, y)
    e.set_editor_property("parameter_name", name)
    e.set_editor_property("default_value", val)
    return e


def const(mat, v, x, y):
    e = ex(mat, unreal.MaterialExpressionConstant, x, y)
    e.set_editor_property("r", v)
    return e


def link(a, a_out, b, b_in):
    names = [b_in]
    if b_in in ("", "Input"):
        names += ["", "Input"]
    if b_in == "Exp":
        names += ["Exponent"]
    for n in names:
        if mel.connect_material_expressions(a, a_out, b, n):
            return True
    unreal.log_warning("[AbyssSetup] link failed: %s.%s -> %s.%s" % (a.get_class().get_name(), a_out, b.get_class().get_name(), b_in))
    return False


def out(a, a_out, prop):
    mel.connect_material_property(a, a_out, prop)


def mul(mat, a, b, x, y, a_out="", b_out=""):
    m = ex(mat, unreal.MaterialExpressionMultiply, x, y)
    link(a, a_out, m, "A")
    link(b, b_out, m, "B")
    return m


def noise(mat, pos, scale, lo, hi, x, y, voronoi=False):
    n = ex(mat, unreal.MaterialExpressionNoise, x, y)
    n.set_editor_property("scale", scale)
    n.set_editor_property("output_min", lo)
    n.set_editor_property("output_max", hi)
    n.set_editor_property("levels", 2)
    if voronoi:
        try:
            n.set_editor_property("noise_function", unreal.NoiseFunction.NOISEFUNCTION_VORONOI_ALU)
            n.set_editor_property("quality", 1)
            n.set_editor_property("levels", 1)
        except Exception as e:  # enum name differs between versions - simplex is fine
            log("voronoi unavailable, using default noise: %s" % e)
    if pos is not None:
        link(pos, "", n, "Position")
    return n


def finish(mat):
    mel.recompile_material(mat)
    eal.save_loaded_asset(mat)
    log("material ok: " + mat.get_name())


def flag(mat, prop):
    try:
        mat.set_editor_property(prop, True)
    except Exception:
        pass


# ----------------------------------------------------------------- materials

def make_lit():
    m = new_material("M_AbyssLit")
    for f in ("used_with_instanced_static_meshes", "used_with_skeletal_mesh"):
        flag(m, f)
    col = vparam(m, "Color", (1, 1, 1, 1), -900, -200)
    rough = sparam(m, "Roughness", 0.6, -900, 100)
    metal = sparam(m, "Metallic", 0.0, -900, 0)
    emi = sparam(m, "Emissive", 0.0, -900, 250)

    # subtle world-space grime so flat primitives read as real surfaces
    wp = ex(m, unreal.MaterialExpressionWorldPosition, -1300, -450)
    grime = noise(m, wp, 0.02, 0.82, 1.08, -1100, -450)
    base = mul(m, col, grime, -600, -250)
    out(base, "", MP.MP_BASE_COLOR)

    rvar = noise(m, wp, 0.05, 0.85, 1.15, -1100, 150)
    out(mul(m, rough, rvar, -600, 100), "", MP.MP_ROUGHNESS)
    out(metal, "", MP.MP_METALLIC)
    out(mul(m, col, emi, -600, 300), "", MP.MP_EMISSIVE_COLOR)
    finish(m)


def make_glass():
    m = new_material("M_AbyssGlass")
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    m.set_editor_property("two_sided", True)
    try:
        m.set_editor_property("translucency_lighting_mode", unreal.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)
    except Exception as e:
        log("translucency mode: %s" % e)
    col = vparam(m, "Color", (0.6, 0.85, 1, 1), -800, -200)
    op = sparam(m, "Opacity", 0.1, -800, 200)
    out(col, "", MP.MP_BASE_COLOR)
    out(const(m, 0.03, -500, 0), "", MP.MP_ROUGHNESS)
    out(const(m, 1.0, -500, 60), "", MP.MP_SPECULAR)
    fres = ex(m, unreal.MaterialExpressionFresnel, -800, 350)
    fres.set_editor_property("exponent", 4.0)
    lerp = ex(m, unreal.MaterialExpressionLinearInterpolate, -450, 250)
    link(op, "", lerp, "A")
    link(const(m, 0.32, -650, 300), "", lerp, "B")
    link(fres, "", lerp, "Alpha")
    out(lerp, "", MP.MP_OPACITY)
    out(mul(m, col, const(m, 0.02, -650, 500), -450, 450), "", MP.MP_EMISSIVE_COLOR)
    finish(m)


def make_additive():
    m = new_material("M_AbyssAdditive")
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    m.set_editor_property("two_sided", True)
    col = vparam(m, "Color", (1, 0.95, 0.8, 1), -900, -100)
    inten = sparam(m, "Intensity", 0.05, -900, 100)
    df = ex(m, unreal.MaterialExpressionDepthFade, -900, 250)
    try:
        df.set_editor_property("fade_distance_default", 250.0)
    except Exception:
        pass
    a = mul(m, col, inten, -650, 0)
    b = mul(m, a, df, -450, 100)
    # fade near camera so the cone doesn't flash when the camera sits inside it (optional node)
    cdf_cls = getattr(unreal, "MaterialExpressionCameraDepthFade", None)
    if cdf_cls is not None:
        cdf = ex(m, cdf_cls, -900, 400)
        try:
            cdf.set_editor_property("fade_length_default", 600.0)
            cdf.set_editor_property("fade_offset_default", 100.0)
        except Exception:
            pass
        b = mul(m, b, cdf, -250, 200)
    out(b, "", MP.MP_EMISSIVE_COLOR)
    finish(m)


def make_seabed():
    m = new_material("M_AbyssSeabed")
    m.set_editor_property("two_sided", True)
    vc = ex(m, unreal.MaterialExpressionVertexColor, -1100, -300)
    col = vparam(m, "Color", (1, 1, 1, 1), -1100, -100)
    wp = ex(m, unreal.MaterialExpressionWorldPosition, -1500, 100)
    n1 = noise(m, wp, 0.004, 0.7, 1.2, -1200, 100)
    n2 = noise(m, wp, 0.03, 0.85, 1.1, -1200, 300)
    base = mul(m, mul(m, vc, col, -850, -200), mul(m, n1, n2, -950, 200), -600, -100)
    out(base, "", MP.MP_BASE_COLOR)
    out(const(m, 0.95, -500, 150), "", MP.MP_ROUGHNESS)
    finish(m)


def make_caustics():
    m = new_material("M_AbyssCaustics")
    m.set_editor_property("material_domain", unreal.MaterialDomain.MD_LIGHT_FUNCTION)
    wp = ex(m, unreal.MaterialExpressionWorldPosition, -1600, 0)
    t = ex(m, unreal.MaterialExpressionTime, -1600, 250)

    def panned(speed, x, y):
        v = ex(m, unreal.MaterialExpressionConstant3Vector, x - 200, y + 80)
        v.set_editor_property("constant", unreal.LinearColor(speed[0], speed[1], speed[2], 0))
        off = mul(m, t, v, x, y + 80)
        add = ex(m, unreal.MaterialExpressionAdd, x + 200, y)
        link(wp, "", add, "A")
        link(off, "", add, "B")
        return add

    p1 = panned((45.0, 30.0, 0.0), -1300, -100)
    p2 = panned((-35.0, 40.0, 0.0), -1300, 250)
    n1 = noise(m, p1, 0.006, 0.0, 1.0, -900, -100, voronoi=True)
    n2 = noise(m, p2, 0.0085, 0.0, 1.0, -900, 250, voronoi=True)

    def pw(x_in, e, x, y):
        p = ex(m, unreal.MaterialExpressionPower, x, y)
        link(x_in, "", p, "Base")
        link(const(m, e, x - 150, y + 60), "", p, "Exp")
        return p

    s = ex(m, unreal.MaterialExpressionAdd, -400, 50)
    link(pw(n1, 3.0, -650, -100), "", s, "A")
    link(pw(n2, 3.0, -650, 250), "", s, "B")
    scaled = mul(m, s, const(m, 1.6, -500, 300), -250, 100)
    final = ex(m, unreal.MaterialExpressionAdd, -100, 100)
    link(scaled, "", final, "A")
    link(const(m, 0.45, -250, 300), "", final, "B")
    out(final, "", MP.MP_EMISSIVE_COLOR)
    finish(m)


def make_pp_underwater():
    # Post-process: red/green absorption by distance + gentle refraction wobble
    m = new_material("M_PP_Underwater")
    m.set_editor_property("material_domain", unreal.MaterialDomain.MD_POST_PROCESS)
    for enum_name in ("BL_SCENE_COLOR_BEFORE_DOF", "BL_BEFORE_TRANSLUCENCY", "BL_BEFORE_TONEMAPPING"):
        if hasattr(unreal.BlendableLocation, enum_name):
            try:
                m.set_editor_property("blendable_location", getattr(unreal.BlendableLocation, enum_name))
                break
            except Exception:
                continue
    strength = sparam(m, "Strength", 1.0, -1800, 600)
    uv = ex(m, unreal.MaterialExpressionTextureCoordinate, -1800, 0)
    t = ex(m, unreal.MaterialExpressionTime, -1800, 200)

    def mask(src, r, g, b, x, y):
        k = ex(m, unreal.MaterialExpressionComponentMask, x, y)
        k.set_editor_property("r", r)
        k.set_editor_property("g", g)
        k.set_editor_property("b", b)
        k.set_editor_property("a", False)
        link(src, "", k, "")
        return k

    def wave(coord, freq, speed, amp, phase, x, y):
        a = mul(m, coord, const(m, freq, x - 200, y + 60), x, y)
        b = mul(m, t, const(m, speed, x - 200, y + 160), x, y + 120)
        add = ex(m, unreal.MaterialExpressionAdd, x + 150, y)
        link(a, "", add, "A")
        link(b, "", add, "B")
        add2 = ex(m, unreal.MaterialExpressionAdd, x + 300, y)
        link(add, "", add2, "A")
        link(const(m, phase, x + 150, y + 120), "", add2, "B")
        sn = ex(m, unreal.MaterialExpressionSine, x + 450, y)
        link(add2, "", sn, "")
        return mul(m, mul(m, sn, const(m, amp, x + 450, y + 120), x + 600, y), strength, x + 750, y)

    u = mask(uv, True, False, False, -1500, -100)
    v = mask(uv, False, True, False, -1500, 100)
    ox = wave(v, 38.0, 1.4, 0.0012, 0.0, -1300, -200)
    oy = wave(u, 31.0, 1.1, 0.0010, 2.0, -1300, 250)
    off = ex(m, unreal.MaterialExpressionAppendVector, -400, 0)
    link(ox, "", off, "A")
    link(oy, "", off, "B")
    uv2 = ex(m, unreal.MaterialExpressionAdd, -250, 0)
    link(uv, "", uv2, "A")
    link(off, "", uv2, "B")

    st = ex(m, unreal.MaterialExpressionSceneTexture, -100, 0)
    try:
        st.set_editor_property("scene_texture_id", unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0)
    except Exception as e:
        log("scene texture id: %s" % e)
    link(uv2, "", st, "UVs")

    depth = ex(m, unreal.MaterialExpressionSceneDepth, -600, 500)
    mn = ex(m, unreal.MaterialExpressionMin, -450, 500)
    link(depth, "", mn, "A")
    link(const(m, 2500.0, -600, 600), "", mn, "B")
    k = ex(m, unreal.MaterialExpressionConstant3Vector, -600, 700)
    k.set_editor_property("constant", unreal.LinearColor(0.0011, 0.00015, 0.0, 0))
    dk = mul(m, mul(m, mn, k, -300, 550), strength, -150, 550)
    one_plus = ex(m, unreal.MaterialExpressionAdd, 0, 550)
    link(const(m, 1.0, -150, 650), "", one_plus, "A")
    link(dk, "", one_plus, "B")
    trans = ex(m, unreal.MaterialExpressionDivide, 150, 500)
    link(const(m, 1.0, 0, 450), "", trans, "A")
    link(one_plus, "", trans, "B")

    col = ex(m, unreal.MaterialExpressionComponentMask, 100, 0)
    for c, on in (("r", True), ("g", True), ("b", True), ("a", False)):
        col.set_editor_property(c, on)
    if not link(st, "Color", col, ""):
        link(st, "", col, "")
    final = mul(m, col, trans, 350, 100)
    out(final, "", MP.MP_EMISSIVE_COLOR)
    finish(m)


def make_water_surface():
    m = new_material("M_AbyssWaterSurface")
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    m.set_editor_property("two_sided", True)
    wp = ex(m, unreal.MaterialExpressionWorldPosition, -1200, 0)
    t = ex(m, unreal.MaterialExpressionTime, -1200, 250)
    v = ex(m, unreal.MaterialExpressionConstant3Vector, -1200, 350)
    v.set_editor_property("constant", unreal.LinearColor(60.0, 35.0, 0.0, 0))
    pan = ex(m, unreal.MaterialExpressionAdd, -800, 100)
    link(wp, "", pan, "A")
    link(mul(m, t, v, -1000, 300), "", pan, "B")
    n = noise(m, pan, 0.003, 0.0, 1.0, -600, 100, voronoi=True)
    lerp = ex(m, unreal.MaterialExpressionLinearInterpolate, -300, 0)
    c1 = ex(m, unreal.MaterialExpressionConstant3Vector, -500, -150)
    c1.set_editor_property("constant", unreal.LinearColor(0.05, 0.35, 0.45, 0))
    c2 = ex(m, unreal.MaterialExpressionConstant3Vector, -500, -50)
    c2.set_editor_property("constant", unreal.LinearColor(1.2, 1.6, 1.7, 0))
    link(c1, "", lerp, "A")
    link(c2, "", lerp, "B")
    link(n, "", lerp, "Alpha")
    out(mul(m, lerp, const(m, 2.0, -300, 200), -100, 0), "", MP.MP_EMISSIVE_COLOR)
    out(const(m, 0.85, -100, 200), "", MP.MP_OPACITY)
    finish(m)


def import_submarine():
    import os
    fbx = os.path.normpath(os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "SourceArt", "Submarine", "sci-fi_vehicle_3d_model.fbx"))
    if not os.path.exists(fbx):
        log("no submarine fbx at %s - keeping placeholder hull" % fbx)
        return
    dest = "/Game/Abyss/Submarine"
    if eal.does_asset_exist(dest + "/SM_Submarine"):
        log("submarine already imported")
        return
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", fbx)
    task.set_editor_property("destination_path", dest)
    task.set_editor_property("destination_name", "SM_Submarine")
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    opts = unreal.FbxImportUI()
    opts.set_editor_property("import_mesh", True)
    opts.set_editor_property("import_as_skeletal", False)
    opts.set_editor_property("import_materials", True)
    opts.set_editor_property("import_textures", True)
    opts.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)
    sm = opts.get_editor_property("static_mesh_import_data")
    sm.set_editor_property("combine_meshes", True)
    sm.set_editor_property("generate_lightmap_u_vs", False)
    sm.set_editor_property("auto_generate_collision", False)
    task.set_editor_property("options", opts)
    tools.import_asset_tasks([task])
    log("submarine imported: %s" % list(task.get_editor_property("imported_object_paths")))


# ----------------------------------------------------------------- level

def spawn(cls_name, loc, yaw=0.0, label=None, props=None):
    cls = unreal.load_class(None, "/Script/AbyssRestaurant." + cls_name)
    if cls is None:
        raise RuntimeError("C++ class not found: %s (did the module compile?)" % cls_name)
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    actor = eas.spawn_actor_from_class(cls, unreal.Vector(*loc), unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw))
    if props:
        # set_editor_property fires PostEditChangeProperty, which reruns OnConstruction
        for k, v in props.items():
            actor.set_editor_property(k, v)
    if label:
        actor.set_actor_label(label)
    return actor


def build_level():
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if eal.does_asset_exist(MAP_PATH):
        les.load_level(MAP_PATH)
        eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
        for a in eas.get_all_level_actors():
            eas.destroy_actor(a)
    else:
        if not les.new_level(MAP_PATH):
            raise RuntimeError("could not create " + MAP_PATH)

    spawn("UnderwaterEnvironment", (0, 0, -1500), label="Underwater_Environment")
    spawn("SeabedGenerator", (0, 0, 0), label="Seabed")
    spawn("RestaurantDome", (0, 0, -4040), label="Restaurant_Dome")
    # Separate interior space (where you go after docking) - far away in open water
    spawn("RestaurantDome", (-60000, 0, -3000), label="Restaurant_Interior", props={"interior_space": True})
    spawn("FishSchool", (-60000, 2800, -2500), label="FishSchool_Interior", props={
        "num_fish": 90, "wander_range": 600.0,
        "body_color": unreal.LinearColor(0.85, 0.72, 0.12, 1),
        "back_color": unreal.LinearColor(0.12, 0.30, 0.60, 1),
    })
    spawn("FishSpawner", (0, 0, -4000), label="Fish_Spawner")

    schools = [
        ((3500, 5500, -3300), (0.85, 0.72, 0.12, 1), (0.12, 0.30, 0.60, 1), 110),
        ((-6000, 2500, -3200), (0.85, 0.72, 0.12, 1), (0.12, 0.30, 0.60, 1), 90),
        ((7000, -6000, -3400), (0.75, 0.80, 0.85, 1), (0.15, 0.25, 0.40, 1), 70),
        ((-3000, -8000, -3000), (0.95, 0.55, 0.15, 1), (0.90, 0.90, 0.90, 1), 60),
        ((11000, 4000, -4200), (0.55, 0.85, 0.95, 1), (0.10, 0.20, 0.50, 1), 80),
        ((-1500, 9000, -3300), (0.85, 0.72, 0.12, 1), (0.12, 0.30, 0.60, 1), 100),
    ]
    for i, (loc, body, back, n) in enumerate(schools):
        spawn("FishSchool", loc, label="FishSchool_%d" % i, props={
            "num_fish": n,
            "body_color": unreal.LinearColor(*body),
            "back_color": unreal.LinearColor(*back),
        })

    # Player start on the docking pad (GameMode also snaps the sub there)
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    ps = eas.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(2750, 1250, -3600), unreal.Rotator(roll=0.0, pitch=0.0, yaw=-155.0))
    ps.set_actor_label("PlayerStart_Dock")

    les.save_current_level()
    log("level saved: " + MAP_PATH)


def main():
    if not eal.does_directory_exist(MAT_DIR):
        eal.make_directory(MAT_DIR)
    if not eal.does_directory_exist("/Game/Maps"):
        eal.make_directory("/Game/Maps")
    make_lit()
    make_glass()
    make_additive()
    make_seabed()
    make_caustics()
    make_pp_underwater()
    make_water_surface()
    import_submarine()
    build_level()
    log("DONE - open /Game/Maps/L_Abyss and press Play")


try:
    main()
except Exception as e:
    unreal.log_error("[AbyssSetup] FAILED: %s" % e)
    raise
finally:
    if QUIT_WHEN_DONE:
        unreal.SystemLibrary.quit_editor()
