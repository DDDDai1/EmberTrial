"""Inspect saved B3 Blueprint defaults without editing, compiling or saving assets."""
import json
from pathlib import Path

import unreal


workspace = Path(__file__).resolve().parents[2]
output = workspace / "Docs/Reports/Raw/M1_03B3_DependencyInspection.json"
paths = [
    "/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter",
    "/Game/EmberTrial/Tests/Blueprints/BP_ETHealthProbeCharacter",
    "/Game/EmberTrial/Tests/Blueprints/BP_ETHealthWaitingCharacter",
]


def reference(value):
    if value is None:
        return None
    if isinstance(value, unreal.Object):
        return {
            "path": value.get_path_name(),
            "class": value.get_class().get_path_name(),
            "outer": value.get_outer().get_path_name() if value.get_outer() else None,
        }
    if isinstance(value, (bool, int, float, str)):
        return value
    return str(value)


report = {"engine": unreal.SystemLibrary.get_engine_version(), "assets": {}}
for path in paths:
    blueprint = unreal.load_asset(path)
    if blueprint is None:
        report["assets"][path] = {"error": "Cannot load Blueprint"}
        continue
    generated_class = blueprint.generated_class()
    defaults = unreal.get_default_object(generated_class)
    item = {
        "generated_class": reference(generated_class),
        "defaults": {},
    }
    for prop in [
        "initial_health", "initial_max_health", "ability_system_component",
        "health_attributes", "default_mapping_context", "jump_action",
        "move_action", "look_action", "auto_possess_player", "auto_possess_ai",
    ]:
        try:
            item["defaults"][prop] = reference(defaults.get_editor_property(prop))
        except Exception as error:
            item["defaults"][prop] = {"inspection_error": str(error)}
    item["direct_attribute_subobjects"] = []
    for obj in unreal.ObjectIterator(unreal.AttributeSet):
        if obj.get_outer() == defaults and "HealthAttributeSet" in obj.get_class().get_name():
            item["direct_attribute_subobjects"].append(reference(obj))
    asc = defaults.get_editor_property("ability_system_component")
    if asc is not None:
        try:
            item["asc_default_spawned_attributes"] = [
                reference(attr) for attr in asc.get_editor_property("spawned_attributes")
            ]
        except Exception as error:
            item["asc_default_spawned_attributes"] = {"inspection_error": str(error)}
    report["assets"][path] = item

output.parent.mkdir(parents=True, exist_ok=True)
output.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
unreal.log("[M1-03B3 Inspection] Default-object report written: " + str(output))
