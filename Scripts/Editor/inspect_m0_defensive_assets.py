"""Inspect saved M0 clips/Sequences in UE 5.5.4; never modify or save UE assets."""
import json
import math
from pathlib import Path

import unreal


WORKSPACE = Path(__file__).resolve().parents[2]
OUTPUT = WORKSPACE / "Docs/Reports/Raw/M0_05B_AssetInspection.json"
CLIPS = "/Game/EmberTrial/Animations/Practice/Clips/"
SEQUENCES = "/Game/EmberTrial/Animations/Practice/Sequence/"
BONES = ["root", "pelvis", "head", "hand_l", "hand_r", "foot_l", "foot_r"]


def asset_path(obj):
    return obj.get_path_name() if obj else None


def load(name):
    obj = unreal.load_asset(name)
    assert obj is not None, name
    return obj


def vector(value):
    return [float(value.x), float(value.y), float(value.z)]


def transform(value):
    return {"location": vector(value.translation),
            "rotation_xyzw": vector(value.rotation) + [float(value.rotation.w)],
            "scale": vector(value.scale3d)}


def dump(data):
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_text(json.dumps(data, ensure_ascii=False, indent=2), encoding="utf-8")


player = unreal.get_default_object(load("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter").generated_class())
mesh_component = player.get_editor_property("mesh")
mesh = mesh_component.get_skeletal_mesh_asset()
skeleton = mesh.get_editor_property("skeleton")
data = {"engine": unreal.SystemLibrary.get_engine_version(),
        "player": {"mesh": asset_path(mesh), "skeleton": asset_path(skeleton),
                   "anim_class": asset_path(mesh_component.get_editor_property("anim_class"))},
        "clips": {}, "sequences": {}, "anim_graphs": {}}
dump(data)

for name in ["AS_M0_Attack_01", "AS_M0_Dodge_01", "AS_M0_HitResult_01"]:
    clip = load(CLIPS + name)
    frames = unreal.AnimationLibrary.get_num_frames(clip)
    length = unreal.AnimationLibrary.get_sequence_length(clip)
    item = {"asset": asset_path(clip), "skeleton": asset_path(clip.get_editor_property("skeleton")),
            "num_frames": frames, "num_keys": unreal.AnimationLibrary.get_num_keys(clip),
            "length_seconds": length, "derived_sample_rate": frames / length,
            "enable_root_motion": clip.get_editor_property("enable_root_motion"),
            "track_names": [str(n) for n in unreal.AnimationLibrary.get_animation_track_names(clip)],
            "evaluations": {}}
    data["clips"][name] = item
    model = clip.get_editor_property("data_model_interface")
    source_rate = model.get_frame_rate()
    item["source_frame_rate"] = [source_rate.numerator, source_rate.denominator]
    assert item["skeleton"] == asset_path(skeleton), "Skeleton mismatch: " + name
    dump(data)
    for evaluation, kind in [("raw", unreal.AnimDataEvalType.RAW),
                             ("compressed", unreal.AnimDataEvalType.COMPRESSED)]:
        options = unreal.AnimPoseEvaluationOptions()
        options.set_editor_property("evaluation_type", kind)
        options.set_editor_property("optional_skeletal_mesh", mesh)
        options.set_editor_property("extract_root_motion", False)
        samples = []
        for index in range(frames + 1):
            pose = unreal.AnimPoseExtensions.get_anim_pose_at_time(clip, length * index / frames, options)
            assert unreal.AnimPoseExtensions.is_valid(pose), (name, evaluation, index)
            names = {str(n) for n in unreal.AnimPoseExtensions.get_bone_names(pose)}
            assert set(BONES) <= names, (name, names)
            bones = {bone: transform(unreal.AnimPoseExtensions.get_bone_pose(pose, bone, unreal.AnimPoseSpaces.WORLD))
                     for bone in BONES}
            assert all(math.isfinite(v) for t in bones.values() for values in t.values() for v in values)
            samples.append({"index": index, "time": length * index / frames, "bones": bones})
        metrics = {}
        for bone in BONES:
            first = samples[0]["bones"][bone]
            last = samples[-1]["bones"][bone]
            metrics[bone] = {
                "max_position_delta_cm": max(math.dist(first["location"], s["bones"][bone]["location"]) for s in samples),
                "end_position_delta_cm": math.dist(first["location"], last["location"]),
                "max_scale_delta": max(math.dist(first["scale"], s["bones"][bone]["scale"]) for s in samples),
                "max_rotation_delta_degrees": max(math.degrees(2 * math.acos(min(1.0, abs(sum(a*b for a,b in zip(first["rotation_xyzw"], s["bones"][bone]["rotation_xyzw"])))))) for s in samples)}
        item["evaluations"][evaluation] = {"space": "component (AnimPoseSpaces.WORLD)", "metrics": metrics, "samples": samples}
        dump(data)

for name in ["LS_M0_Dodge_01", "LS_M0_HitResult_01", "LS_M0_HitResult_02"]:
    sequence = load(SEQUENCES + name)
    rate = sequence.get_display_rate()
    bindings = []
    for binding in sequence.get_bindings():
        tracks = [{"name": track.get_name(), "class": track.get_class().get_name(),
                   "sections": [section.get_class().get_name() for section in track.get_sections()]}
                  for track in binding.get_tracks()]
        bindings.append({"name": binding.get_name(), "tracks": tracks})
        for track, track_item in zip(binding.get_tracks(), tracks):
            if track.get_class().get_name() != "MovieSceneControlRigParameterTrack":
                continue
            track_item["key_channels"] = []
            for section in track.get_sections():
                for channel in section.get_all_channels():
                    channel_name = str(channel.get_editor_property("channel_name"))
                    selected = ["spine_01_ctrl", "upperarm_l_fk_ctrl", "upperarm_r_fk_ctrl",
                                "lowerarm_l_fk_ctrl", "lowerarm_r_fk_ctrl", "hand_l_fk_ctrl", "hand_r_fk_ctrl"]
                    if not any(control in channel_name for control in selected):
                        continue
                    keys = [{"frame": key.get_time().frame_number.value, "sub_frame": key.get_time().sub_frame,
                             "value": key.get_value()} for key in channel.get_keys()]
                    track_item["key_channels"].append({"channel": channel_name, "keys": keys})
    data["sequences"][name] = {"asset": asset_path(sequence), "display_rate": [rate.numerator, rate.denominator],
                               "requested_asset": SEQUENCES + name,
                               "playback_start": sequence.get_playback_start(), "playback_end": sequence.get_playback_end(),
                               "bindings": bindings}
    dump(data)

# Optional graph inventory. UE reflection exposure differs by property; record any
# inspection limitation instead of altering a Blueprint or assuming its wiring.
for name in ["ABP_Manny", "ABP_Quinn"]:
    blueprint = load("/Game/Characters/Mannequins/Animations/" + name)
    item = {"parent_class_tag": unreal.EditorAssetLibrary.find_asset_data(asset_path(blueprint)).get_tag_value("ParentClass")}
    data["anim_graphs"][name] = item
    graph = unreal.BlueprintEditorLibrary.find_graph(blueprint, "AnimGraph")
    item["graph"] = asset_path(graph)
    if graph:
        try:
            item["nodes"] = [{"name": node.get_name(), "class": node.get_class().get_name()}
                             for node in graph.get_editor_property("nodes")]
        except Exception as error:
            item["node_inventory_limitation"] = str(error)
    dump(data)

# A successful commandlet means extraction succeeded, not that the authored
# animation meets the plan. Keep concrete discrepancies in the report.
expected = json.loads((WORKSPACE / "Docs/Data/M0_05B_DefensivePoseValues.json").read_text(encoding="utf-8"))
data["review_findings"] = []
sequence_names = {"Dodge": "LS_M0_Dodge_01", "HitReact": "LS_M0_HitResult_02"}
for kind, specification in expected["clips"].items():
    sequence_name = sequence_names[kind]
    actual = data["sequences"][sequence_name]
    if actual["playback_end"] != specification["end_exclusive"]:
        data["review_findings"].append({"sequence": sequence_name, "field": "playback_end",
                                        "actual": actual["playback_end"], "expected": specification["end_exclusive"]})
    channels = {c["channel"]: c["keys"] for binding in actual["bindings"] for track in binding["tracks"]
                for c in track.get("key_channels", [])}
    for frame, controls in specification["frames"].items():
        for control, rotation in controls.items():
            for field, values in [("Location", expected["location"]), ("Rotation", rotation), ("Scale", expected["scale"])]:
                for axis, target in zip("XYZ", values):
                    channel = f"{control}.{field}.{axis}"
                    keys = [k for k in channels.get(channel, []) if k["frame"] == int(frame) and k["sub_frame"] == 0]
                    if len(keys) != 1 or abs(keys[0]["value"] - target) > 0.001:
                        data["review_findings"].append({"sequence": sequence_name, "frame": int(frame), "channel": channel,
                                                        "actual": [k["value"] for k in keys], "expected": target})
dump(data)
unreal.log("M0_DEFENSIVE_ASSET_INSPECTION_COMPLETE: " + str(OUTPUT))
unreal.log("M0_REVIEW_FINDINGS: " + str(len(data["review_findings"])))
