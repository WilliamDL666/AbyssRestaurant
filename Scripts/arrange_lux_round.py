# Re-arranges the (open-plan) restaurant copy for the round glass dome.
#   bar + kitchen cluster -> dome center; each table set (table + chairs + tableware + pendant lights + umbrella)
#   -> concentric rings around the bar with four aisles; raised floors flattened; rectangular floors/stairs/railings removed
#   (the dome builds a round floor); leftover decor -> a ring along the glass.
# Lux-map coordinates. The dome center is LUX_ORIGIN (same value as Restaurant_*.lux_origin).
import math, collections
import unreal

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
LUX_ORIGIN = (380.8, -996.75)
FLOOR_Z = 70.0
BAR_BOX = (-1300.0, -260.0, -650.0, 60.0)   # xmin, xmax, ymin, ymax of the bar / kitchen area
BAR_CENTER = (-745.0, -294.0)
RINGS = [560.0, 900.0, 1240.0, 1580.0, 1900.0]
AISLES = [0.0, 90.0, 180.0, 270.0]
AISLE_HALF = 160.0
AISLE_HALF_KITCHEN = 520.0                          # half width (cm) of each aisle at every ring
DECOR_RING = 1990.0


def log(m):
    unreal.log("[AbyssSetup] " + m)


les.load_level("/Game/Luxury_Restaurant_Pack/Levels/Luxury_Restaurant_Showcase")
orig = {}
for a in eas.get_all_level_actors():
    orig[a.get_name()] = (a.get_actor_location(), a.get_actor_rotation())
les.load_level("/Game/Abyss/Maps/L_LuxInterior")
actors = eas.get_all_level_actors()
for a in actors:
    o = orig.get(a.get_name())
    if o:
        a.set_actor_location(o[0], False, False)
        a.set_actor_rotation(o[1], False)
items = []
for a in actors:
    cn = a.get_class().get_name()
    if cn == "GroupActor":
        eas.destroy_actor(a)
        continue
    l = a.get_actor_location()
    name = ""
    ext = unreal.Vector(0, 0, 0)
    if cn == "StaticMeshActor":
        sm = a.static_mesh_component.static_mesh
        name = sm.get_name() if sm else ""
        _, ext = a.get_actor_bounds(False)
    items.append({"a": a, "cls": cn, "n": name, "x": l.x, "y": l.y, "z": l.z, "ext": ext, "set": None})

removed = collections.Counter()
keep = []
for it in items:
    n = it["n"]
    if it["cls"] == "StaticMeshActor" and (n.startswith("SM_LUX_Floor") or "Stair" in n or "Railing" in n or n == "Cube"):
        removed[n.split("_")[2] if n.startswith("SM_LUX_") and len(n.split("_")) > 2 else n] += 1
        eas.destroy_actor(it["a"])
    else:
        keep.append(it)
items = keep


def in_bar(it):
    return BAR_BOX[0] <= it["x"] <= BAR_BOX[1] and BAR_BOX[2] <= it["y"] <= BAR_BOX[3]


tables = [it for it in items if it["n"].startswith("SM_LUX_Table") and not in_bar(it)]
for i, t in enumerate(tables):
    t["set"] = i
    t["members"] = [t]
    t["raised"] = t["z"] >= 105.0


def nearest_table(it, maxd, extra_from_table=False):
    best, bd = None, 1e9
    for t in tables:
        d = math.hypot(it["x"] - t["x"], it["y"] - t["y"])
        lim = maxd + (max(t["ext"].x, t["ext"].y) if extra_from_table else 0.0)
        if d < lim and d < bd:
            best, bd = t, d
    return best


bar = []
decor = []
for it in items:
    if it.get("members") is not None:
        continue
    n, cls = it["n"], it["cls"]
    if in_bar(it):
        bar.append(it)
        continue
    t = None
    if cls in ("PointLight", "SpotLight", "RectLight"):
        t = nearest_table(it, 200.0)
        if t is None:
            eas.destroy_actor(it["a"]); removed["light"] += 1
            continue
    elif "Chair" in n or "Seat" in n:
        t = nearest_table(it, 170.0)
    elif "Umbrella" in n:
        t = nearest_table(it, 260.0)
    elif cls == "StaticMeshActor" and max(it["ext"].x, it["ext"].y) < 60.0:
        t = nearest_table(it, 25.0, extra_from_table=True)   # tableware on the table top
    if t is not None:
        t["members"].append(it)
    elif cls == "StaticMeshActor":
        decor.append(it)


def move(it, nx, ny, dz, dyaw):
    a = it["a"]
    a.set_actor_location(unreal.Vector(nx, ny, it["z"] + dz), False, False)
    if dyaw:
        r = a.get_actor_rotation()
        a.set_actor_rotation(unreal.Rotator(r.roll, r.pitch, r.yaw + dyaw), False)


# ---- bar cluster to the center; whatever would stand in the kitchen line behind it (dome -Y) goes away
BAR_LOCAL = (1180.0, -250.0)                       # serving / cashier counter on the right, like the reference
dx, dy = LUX_ORIGIN[0] + BAR_LOCAL[0] - BAR_CENTER[0], LUX_ORIGIN[1] + BAR_LOCAL[1] - BAR_CENTER[1]
KITCHEN_CLEAR = (-99999.0, -99998.0, 0.0, 0.0)    # (nothing to clear now)
for it in bar:
    nx, ny = it["x"] + dx, it["y"] + dy
    lx, ly = nx - LUX_ORIGIN[0], ny - LUX_ORIGIN[1]
    if KITCHEN_CLEAR[0] < lx < KITCHEN_CLEAR[1] and KITCHEN_CLEAR[2] < ly < KITCHEN_CLEAR[3]:
        eas.destroy_actor(it["a"]); removed["kitchen_clear"] += 1
        continue
    move(it, nx, ny, -50.0 if it["z"] >= 115.0 and it["cls"] == "StaticMeshActor" and it["z"] < 140 else 0.0, 0.0)

# ---- table sets on rings
def footprint(t):
    r = 60.0
    for m in t["members"]:
        if m["cls"] == "StaticMeshActor":
            r = max(r, math.hypot(m["x"] - t["x"], m["y"] - t["y"]) + max(m["ext"].x, m["ext"].y) * 0.6)
    return min(r, 330.0)

order = sorted(tables, key=lambda t: (footprint(t), t["n"]))
# candidate spots on rings, skipping the kitchen (back, -Y), the bar (right), the entrance aisle (front, +Y)
def blocked(x, y, fr):
    if y < -480.0 + fr and abs(x) < 820.0 + fr:                        # kitchen zone
        return True
    bx, by = BAR_LOCAL
    if abs(x - bx) < 640.0 + fr and abs(y - by) < 460.0 + fr:          # serving counter
        return True
    if y > 0 and abs(x) < 260.0 + fr:                                  # entrance aisle
        return True
    if abs(y) < 160.0 + fr and x < 0:                                  # side aisle to the left booths
        return True
    return False
slots = []
for ri, R in enumerate(RINGS):
    n = int(2 * math.pi * R / 380.0)
    for k in range(n):
        a = 2 * math.pi * (k + 0.5 * (ri % 2)) / n
        slots.append((R, a))
placed = 0
used = []
for t in order:
    fr = footprint(t)
    pick = None
    for si, (R, a) in enumerate(slots):
        if si in used:
            continue
        x, y = math.cos(a) * R, math.sin(a) * R
        if R + fr > 2010.0 or blocked(x, y, fr * 0.7):
            continue
        pick = si
        break
    if pick is None:
        for m in t["members"]:
            eas.destroy_actor(m["a"])
        removed["no_room"] += 1
        continue
    used.append(pick)
    R, ang = slots[pick]
    cx, cy = LUX_ORIGIN[0] + math.cos(ang) * R, LUX_ORIGIN[1] + math.sin(ang) * R
    target_yaw = math.degrees(ang) + 90.0
    dyaw = target_yaw - t["a"].get_actor_rotation().yaw
    if not ("Rectangle" in t["n"]):
        dyaw = round(dyaw / 45.0) * 45.0
    c, s_ = math.cos(math.radians(dyaw)), math.sin(math.radians(dyaw))
    dz = -50.0 if t["raised"] else 0.0
    for mm in t["members"]:
        ox, oy = mm["x"] - t["x"], mm["y"] - t["y"]
        move(mm, cx + ox * c - oy * s_, cy + ox * s_ + oy * c, dz, dyaw)
    placed += 1

# ---- leftover decor along the glass
for i, it in enumerate(decor):
    ang = 2 * math.pi * (i + 0.5) / max(len(decor), 1)
    nx, ny = LUX_ORIGIN[0] + math.cos(ang) * DECOR_RING, LUX_ORIGIN[1] + math.sin(ang) * DECOR_RING
    dz = -50.0 if it["z"] >= 105.0 else 0.0
    move(it, nx, ny, dz, math.degrees(ang) + 180.0 - it["a"].get_actor_rotation().yaw)

les.save_current_level()   # moving actors from Python does not dirty the package, so save explicitly
log("round layout: %d table sets on rings, bar cluster %d actors, decor %d, removed %s (bar moved by %.0f, %.0f)" %
    (placed, len(bar), len(decor), dict(removed), dx, dy))
