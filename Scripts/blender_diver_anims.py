# Blender (headless): keyframe the diver's swim animations on its Mixamo rig and export one FBX per action.
#   blender -b -P blender_diver_anims.py -- <diver.fbx> <out_dir>
# Actions (30 fps, looping):
#   A_Diver_Swim  - prone flutter kick, arms relaxed along the body, head up looking ahead
#   A_Diver_Idle  - hovering: slow scissor kick, hands sculling, gentle breathing
#   A_Diver_Aim   - arms extended ahead (over the head in mesh space) holding a tool; used as an arm overlay
# Rotations are authored in armature space: X = character's left, -Y = forward (face), Z = up.
import sys, os, math
import bpy
from mathutils import Vector, Matrix, Quaternion

argv = sys.argv[sys.argv.index("--") + 1:]
FBX, OUT = argv[0], argv[1]
os.makedirs(OUT, exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=FBX, automatic_bone_orientation=False)
scene = bpy.context.scene
scene.render.fps = 30
arm = [o for o in scene.objects if o.type == "ARMATURE"][0]
bpy.context.view_layer.objects.active = arm
for a in list(bpy.data.actions):
    bpy.data.actions.remove(a)

PREFIX = ""
for b in arm.data.bones:
    if b.name.endswith("Hips"):
        PREFIX = b.name[:-4]


def B(short):
    return PREFIX + short


LEFT, FWD, UP = Vector((1, 0, 0)), Vector((0, -1, 0)), Vector((0, 0, 1))
order = []
def walk(b):
    order.append(b)
    for c in b.children:
        walk(c)
for b in arm.data.bones:
    if b.parent is None:
        walk(b)


def apply_pose(deltas):
    """deltas: {short_name: [(axis, deg), ...]} applied in armature space, parent first."""
    posed = {}
    for b in order:
        rest = b.matrix_local
        if b.parent:
            inherited = posed[b.parent.name] @ b.parent.matrix_local.inverted() @ rest
        else:
            inherited = rest.copy()
        desired = inherited
        short = b.name[len(PREFIX):]
        if short in deltas:
            h = inherited.to_translation()
            R = Matrix.Identity(4)
            for axis, deg in deltas[short]:
                R = Matrix.Rotation(math.radians(deg), 4, axis) @ R
            desired = Matrix.Translation(h) @ R @ Matrix.Translation(-h) @ inherited
        posed[b.name] = desired
        pb = arm.pose.bones[b.name]
        pb.rotation_mode = "QUATERNION"
        pb.matrix_basis = inherited.inverted() @ desired
        pb.location = (0, 0, 0)


def arms_down(extra_l=0.0, extra_r=0.0, fwd_l=0.0, fwd_r=0.0):
    # T-pose arms along +-X -> down along the body; positive fwd swings the hand toward the face side
    return {
        "LeftArm": [(FWD, -72 - extra_l), (LEFT, -fwd_l)],
        "RightArm": [(FWD, 72 + extra_r), (LEFT, -fwd_r)],
    }


def key_all(frame):
    for pb in arm.pose.bones:
        pb.keyframe_insert("rotation_quaternion", frame=frame)
        pb.keyframe_insert("location", frame=frame)


def make_action(name, frames, pose_fn):
    act = bpy.data.actions.new(name)
    arm.animation_data_create()
    arm.animation_data.action = act
    for f in range(frames + 1):
        t = f / frames  # 0..1 loop phase
        apply_pose(pose_fn(t))
        key_all(f)
    return act


def merge(*ds):
    out = {}
    for d in ds:
        for k, v in d.items():
            out.setdefault(k, []).extend(v)
    return out


S = lambda x: math.sin(2 * math.pi * x)
C = lambda x: math.cos(2 * math.pi * x)


def swim(t):
    k = S(t)
    # flutter kick from the hips: legs swing face-side/back-side (mesh forward axis); knees bend on the down-beat
    legs = {
        "LeftUpLeg": [(LEFT, 15 * k + 5)],
        "RightUpLeg": [(LEFT, -15 * k + 5)],
        "LeftLeg": [(LEFT, 6 + 15 * max(0.0, S(t - 0.15)))],
        "RightLeg": [(LEFT, 6 + 15 * max(0.0, -S(t - 0.15)))],
        # pointed toes so the fins trail in line with the legs
        "LeftFoot": [(LEFT, 72 + 8 * S(t - 0.25))],
        "RightFoot": [(LEFT, 72 - 8 * S(t - 0.25))],
    }
    body = {
        "Spine": [(LEFT, 3 * S(t + 0.2))],
        "Spine1": [(LEFT, 2 * S(t + 0.3))],
        # prone: the head tilts back to look ahead
        "Neck": [(LEFT, -35)],
        "Head": [(LEFT, -28)],
    }
    arms = arms_down(extra_l=12, extra_r=12, fwd_l=-12, fwd_r=-12)
    arms["LeftForeArm"] = [(UP, -12)]
    arms["RightForeArm"] = [(UP, 12)]
    return merge(legs, body, arms)


def idle(t):
    k = S(t)
    legs = {
        "LeftUpLeg": [(LEFT, 12 * k - 10)],
        "RightUpLeg": [(LEFT, -12 * k - 10)],
        "LeftLeg": [(LEFT, 25 + 12 * max(0.0, k))],
        "RightLeg": [(LEFT, 25 + 12 * max(0.0, -k))],
        "LeftFoot": [(LEFT, 60)],
        "RightFoot": [(LEFT, 60)],
    }
    body = {
        "Spine": [(LEFT, 2 * S(t * 2))],
        "Neck": [(LEFT, -18)],
        "Head": [(LEFT, -14), (UP, 6 * S(t))],
    }
    # hands sculling in front of the chest
    scull = 18 * S(t * 2)
    arms = {
        "LeftArm": [(FWD, -55), (LEFT, -35), (UP, scull)],
        "RightArm": [(FWD, 55), (LEFT, -35), (UP, -scull)],
        "LeftForeArm": [(UP, -45)],
        "RightForeArm": [(UP, 45)],
        "LeftHand": [(FWD, 20 * S(t * 2))],
        "RightHand": [(FWD, -20 * S(t * 2))],
    }
    return merge(legs, body, arms)


def aim(t):
    # both arms reach ahead of the head (over the head in mesh space), hands together on the tool
    return {
        "LeftShoulder": [(FWD, -10)],
        "RightShoulder": [(FWD, 10)],
        "LeftArm": [(FWD, 78), (LEFT, 14)],
        "RightArm": [(FWD, -78), (LEFT, 14)],
        "LeftForeArm": [(UP, -14)],
        "RightForeArm": [(UP, 8)],
        "Neck": [(LEFT, -35)],
        "Head": [(LEFT, -28)],
    }


acts = {
    "A_Diver_Swim": make_action("A_Diver_Swim", 30, swim),
    "A_Diver_Idle": make_action("A_Diver_Idle", 60, idle),
    "A_Diver_Aim": make_action("A_Diver_Aim", 10, aim),
}

# ---- preview: prone side view, a few frames per action
scene.render.engine = "BLENDER_WORKBENCH"
scene.display.shading.color_type = "TEXTURE"
scene.render.resolution_x = 700
scene.render.resolution_y = 450
scene.world = bpy.data.worlds.new("W")
cam = bpy.data.objects.new("Cam", bpy.data.cameras.new("Cam"))
scene.collection.objects.link(cam)
cam.data.type = "ORTHO"
scene.camera = cam
meshes = [o for o in scene.objects if o.type == "MESH"]
H = max(o.dimensions.z for o in meshes) or 1.0
cam.data.ortho_scale = 1.35 * max(o.dimensions.z for o in meshes) * (arm.scale.z if arm.scale.z else 1)
for name, act in acts.items():
    arm.animation_data.action = act
    prone = name != "A_Diver_Idle" or True
    # lay the rig face-down for the preview (head toward +Y on screen-left)
    arm.rotation_euler = (math.radians(90), 0, 0)
    bpy.context.view_layer.update()
    for f in ([0, 8, 15, 23] if name != "A_Diver_Aim" else [0]):
        scene.frame_set(f)
        mn = Vector((1e9,) * 3); mx = Vector((-1e9,) * 3)
        dg = bpy.context.evaluated_depsgraph_get()
        for o in meshes:
            for c in o.evaluated_get(dg).bound_box:
                w = o.matrix_world @ Vector(c)
                mn = Vector(map(min, mn, w)); mx = Vector(map(max, mx, w))
        ctr = (mn + mx) / 2
        cam.data.ortho_scale = max(mx.y - mn.y, mx.z - mn.z) * 1.2
        cam.location = ctr + Vector((4, 0, 0))
        cam.rotation_euler = (math.radians(90), 0, math.radians(90))
        scene.render.filepath = os.path.join(OUT, "prev_%s_%02d.png" % (name, f))
        bpy.ops.render.render(write_still=True)
    arm.rotation_euler = (0, 0, 0)

# ---- export: one FBX per action (armature + mesh, baked, no leaf bones)
for name, act in acts.items():
    arm.animation_data.action = act
    scene.frame_start = 0
    scene.frame_end = int(act.frame_range[1])
    bpy.ops.object.select_all(action="DESELECT")
    arm.select_set(True)
    for o in meshes:
        o.select_set(True)
    bpy.ops.export_scene.fbx(filepath=os.path.join(OUT, name + ".fbx"), use_selection=True,
                             add_leaf_bones=False, bake_anim=True, bake_anim_use_all_actions=False,
                             bake_anim_use_nla_strips=False, bake_anim_force_startend_keying=True,
                             bake_anim_simplify_factor=0.0, armature_nodetype="NULL", primary_bone_axis="Y",
                             secondary_bone_axis="X")
    print("EXPORTED", name, act.frame_range[:])
print("ANIMS DONE")
