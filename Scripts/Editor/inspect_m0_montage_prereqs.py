"""Read the current player, input mapping and parent AnimGraph without saving assets."""
import json
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
    report["mappings"].append({"action": item.get_editor_property("action").get_path_name()})
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
output.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
unreal.log("M0_MONTAGE_PREREQUISITES_COMPLETE: " + str(output))
