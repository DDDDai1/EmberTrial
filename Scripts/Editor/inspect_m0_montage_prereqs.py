"""Read the current player, input mapping and parent AnimGraph without saving assets."""
import json
import re
from pathlib import Path
import unreal


output = Path(__file__).resolve().parents[2] / "Docs/Reports/Raw/M0_05C1_Prerequisites.json"
player_bp = unreal.load_asset("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter")
player = unreal.get_default_object(player_bp.generated_class())
mesh = player.get_editor_property("mesh")
mapping = player.get_editor_property("default_mapping_context")
report = {"engine": unreal.SystemLibrary.get_engine_version(),
          "player_mesh": mesh.get_skeletal_mesh_asset().get_path_name(),
          "anim_class": mesh.get_editor_property("anim_class").get_path_name(),
          "mapping_context": mapping.get_path_name(), "mappings": [], "blueprints": {}}
for item in mapping.get_editor_property("mappings"):
    action = item.get_editor_property("action")
    report["mappings"].append({
        "action": action.get_path_name() if action else None,
        "key": str(unreal.InputLibrary.key_get_display_name(item.get_editor_property("key"))),
        "mapping_triggers": [str(x) for x in item.get_editor_property("triggers")],
        "mapping_modifiers": [str(x) for x in item.get_editor_property("modifiers")],
        "action_type": str(action.get_editor_property("value_type")) if action else None,
        "action_triggers": [str(x) for x in action.get_editor_property("triggers")] if action else [],
        "action_modifiers": [str(x) for x in action.get_editor_property("modifiers")] if action else [],
        "trigger_when_paused": action.get_editor_property("trigger_when_paused") if action else None,
    })
report["practice_defaults"] = {}
report["montages"] = {}
for name in ["PracticeAttackMontage", "PracticeDodgeMontage", "PracticeHitMontage", "bPauseMenuOpen"]:
    try:
        value = player.get_editor_property(name)
        report["practice_defaults"][name] = value.get_path_name() if isinstance(value, unreal.Object) else str(value)
        if isinstance(value, unreal.AnimMontage):
            report["montages"][name] = {
                "asset": value.get_path_name(),
                "skeleton": value.get_editor_property("skeleton").get_path_name(),
                "auto_blend_out": value.get_editor_property("enable_auto_blend_out"),
                "blend_out_trigger_time": value.get_editor_property("blend_out_trigger_time"),
                "rate_scale": value.get_editor_property("rate_scale"),
                "blend_in_seconds": value.get_editor_property("blend_in").get_editor_property("blend_time"),
                "blend_out_seconds": value.get_editor_property("blend_out").get_editor_property("blend_time"),
            }
    except Exception as error:
        report["practice_defaults"][name] = str(error)
for name in ["ABP_Manny", "ABP_Quinn"]:
    blueprint = unreal.load_asset("/Game/Characters/Mannequins/Animations/" + name)
    graph = unreal.BlueprintEditorLibrary.find_graph(blueprint, "AnimGraph")
    entry = {"graph": graph.get_path_name() if graph else None, "nodes": []}
    report["blueprints"][name] = entry
    if not graph:
        continue
    for node in unreal.ObjectIterator(unreal.EdGraphNode):
        if node.get_outer() != graph:
            continue
        item = {"class": node.get_class().get_name(), "name": node.get_name()}
        if item["class"] == "AnimGraphNode_Slot":
            slot = node.get_editor_property("node")
            item["slot_name"] = str(slot.get_editor_property("slot_name"))
            item["always_update_source_pose"] = slot.get_editor_property("always_update_source_pose")
        entry["nodes"].append(item)
output.parent.mkdir(parents=True, exist_ok=True)
graph = unreal.BlueprintEditorLibrary.find_graph(player_bp, "TryPlayPracticeMontage")
task = unreal.AssetExportTask()
task.object = graph
task.exporter = unreal.ObjectExporterT3D()
task.filename = str(output.with_name("M0_05C1_PlaybackGraph.t3d"))
task.automated = True
task.prompt = False
task.replace_identical = True
report["graph_text_exported"] = unreal.Exporter.run_asset_export_task(task)
if report["graph_text_exported"]:
    # UE's text exporter includes pin connections that Python reflection does
    # not expose directly. Only read the export; never import it into assets.
    graph_text = Path(task.filename).read_text(encoding="utf-8-sig")
    report["playback_graph"] = {}
    current = None
    for line in graph_text.splitlines():
        node_match = re.match(r'   Begin Object Name="([^"]+)"', line)
        if node_match:
            current = {"pins": {}}
            report["playback_graph"][node_match[1]] = current
        elif current is not None:
            if line.strip().startswith(("FunctionReference=", "VariableReference=")):
                current["reference"] = line.strip()
            if "CustomProperties Pin " in line:
                name_match = re.search(r'PinName="([^"]+)"', line)
                linked = re.search(r'LinkedTo=\(([^)]+)\)', line)
                default = re.search(r',DefaultValue="([^"]*)"', line)
                current["pins"][name_match[1]] = {
                    "links": linked[1].rstrip(",").split(",") if linked else [],
                    "default": default[1] if default else None,
                }
output.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
unreal.log("M0_MONTAGE_PREREQUISITES_COMPLETE: " + str(output))
