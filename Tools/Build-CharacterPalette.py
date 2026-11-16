"""Create the four EmberTrial character palettes in Unreal Editor."""

import math

import unreal as ue


OUTPUT = "/Game/EmberTrial/Art/Characters/Palettes"
QUINN = "/Game/Characters/Mannequins/Materials/Instances/Quinn"
PALETTES = {
    # Each role specifies both material slots' armor tints and a shared accent color.
    "Player": ((0.34, 0.53, 0.66), (0.54, 0.71, 0.80), (0.95, 0.49, 0.15)),
    "Melee": ((0.62, 0.27, 0.09), (0.84, 0.44, 0.16), (0.93, 0.23, 0.07)),
    "Ranged": ((0.17, 0.37, 0.22), (0.32, 0.63, 0.43), (0.72, 0.83, 0.41)),
    "Guardian": ((0.23, 0.055, 0.095), (0.47, 0.13, 0.20), (1.00, 0.66, 0.26)),
}
ACCENT_BRIGHTNESS = (1.25, 2.5, 5.0)


def verify(instance, parent, colors, brightness):
    assert instance.get_editor_property("parent") == parent, instance.get_path_name()
    for parameter, expected in colors.items():
        actual = GRAPH.get_material_instance_vector_parameter_value(instance, parameter)
        assert all(math.isclose(getattr(actual, channel), value, abs_tol=1e-5)
                   for channel, value in zip("rgb", expected)), (parameter, actual)
    for parameter, expected in brightness.items():
        actual = GRAPH.get_material_instance_scalar_parameter_value(instance, parameter)
        assert math.isclose(actual, expected, abs_tol=1e-5), (parameter, actual)


def main():
    editor = ue.EditorAssetLibrary
    tools = ue.AssetToolsHelpers.get_asset_tools()
    parents = [ue.load_asset(f"{QUINN}/MI_Quinn_{slot:02}") for slot in (1, 2)]
    assert all(isinstance(parent, ue.MaterialInstanceConstant) for parent in parents)
    brightness = {f"LogoLayer{index}_Brightness": value
                  for index, value in enumerate(ACCENT_BRIGHTNESS)}

    for role, (tint_one, tint_two, accent) in PALETTES.items():
        for slot, (parent, tint) in enumerate(zip(parents, (tint_one, tint_two)), start=1):
            colors = {"Tint": tint, **{f"LogoLayer{index}_Color": accent for index in range(3)}}
            name = f"MI_ET_{role}_Quinn_{slot:02}"
            path = f"{OUTPUT}/{name}"
            if editor.does_asset_exist(path):
                instance = editor.load_asset(path)
                verify(instance, parent, colors, brightness)
                continue

            instance = tools.create_asset(name, OUTPUT, ue.MaterialInstanceConstant,
                                          ue.MaterialInstanceConstantFactoryNew())
            assert instance, path
            GRAPH.set_material_instance_parent(instance, parent)
            # UE 5.5's vector setter returns false even after it writes the value.
            for parameter, color in colors.items():
                GRAPH.set_material_instance_vector_parameter_value(
                    instance, parameter, ue.LinearColor(*color, 1.0))
            for parameter, value in brightness.items():
                GRAPH.set_material_instance_scalar_parameter_value(instance, parameter, value)
            GRAPH.update_material_instance(instance)
            verify(instance, parent, colors, brightness)
            assert editor.save_loaded_asset(instance), path
            ue.log(f"[M7-03] Saved {path}")

    ue.log("[M7-03] Verified four palettes on both Quinn material slots.")


GRAPH = ue.MaterialEditingLibrary
if __name__ == "__main__":
    main()
