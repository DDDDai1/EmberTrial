"""Build the EmberTrial relic altar mesh and export its neutral FBX."""

import math
from pathlib import Path
import sys

import bpy
from mathutils import Quaternion, Vector


ASSET_NAME = "SM_ET_RelicAltar"


def make_material(name, color, metallic, roughness):
    material = bpy.data.materials.new(name)
    material.diffuse_color = (*color, 1.0)
    if material.node_tree is None:
        material.use_nodes = True
    shader = material.node_tree.nodes.get("Principled BSDF")
    shader.inputs["Base Color"].default_value = material.diffuse_color
    shader.inputs["Metallic"].default_value = metallic
    shader.inputs["Roughness"].default_value = roughness
    return material


def outline(half_width, half_depth, corner):
    x, y = half_width, half_depth
    return [
        (-x + corner, -y), (x - corner, -y), (x, -y + corner), (x, y - corner),
        (x - corner, y), (-x + corner, y), (-x, y - corner), (-x, -y + corner),
    ]


def make_loft(name, rings, material=None, closed=False):
    size = len(rings[0])
    vertices = [point for ring in rings for point in ring]
    faces = [] if closed else [tuple(reversed(range(size)))]
    for layer in range(len(rings) if closed else len(rings) - 1):
        lower = layer * size
        upper = (layer + 1) % len(rings) * size
        for edge in range(size):
            next_edge = (edge + 1) % size
            faces.append((lower + edge, lower + next_edge, upper + next_edge, upper + edge))
    if not closed:
        top = (len(rings) - 1) * size
        faces.append(tuple(range(top, top + size)))

    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(vertices, [], faces)
    mesh.update()
    if material is not None:
        mesh.materials.append(material)
    part = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(part)
    return part


def horizontal_profile(name, levels, material=None, center=(0.0, 0.0)):
    center_x, center_y = center
    rings = [
        [(x + center_x, y + center_y, z) for x, y in outline(width, depth, corner)]
        for z, width, depth, corner in levels
    ]
    return make_loft(name, rings, material)


def front_badge(name, points, depth, material):
    back, front = depth
    return make_loft(name, [[(x, y, z) for x, z in points] for y in (back, front)], material)


def build_altar():
    stone = make_material("M_ET_AltarBasalt", (0.052, 0.061, 0.074), 0.0, 0.79)
    bronze = make_material("M_ET_AltarBronze", (0.236, 0.114, 0.052), 0.72, 0.43)
    ember = make_material("M_ET_AltarEmber", (0.61, 0.194, 0.046), 0.18, 0.40)

    # In meters. The root lies on the ground; the engraved face looks along -Y.
    foot = horizontal_profile("Foot", [
        (0.000, 0.590, 0.350, 0.105), (0.032, 0.620, 0.380, 0.105),
        (0.104, 0.620, 0.380, 0.105), (0.149, 0.540, 0.320, 0.089),
        (0.200, 0.511, 0.300, 0.086), (0.230, 0.460, 0.278, 0.075),
    ], stone)
    base_band = horizontal_profile("Base band", [
        (0.149, 0.542, 0.323, 0.089), (0.164, 0.548, 0.327, 0.091),
        (0.179, 0.532, 0.314, 0.085),
    ], bronze)
    body = horizontal_profile("Tapered body", [
        (0.189, 0.461, 0.277, 0.081), (0.281, 0.407, 0.254, 0.078),
        (0.500, 0.364, 0.235, 0.073), (0.723, 0.387, 0.239, 0.081),
        (0.870, 0.482, 0.282, 0.093),
    ], stone)
    rim = horizontal_profile("Shoulder band", [
        (0.794, 0.471, 0.268, 0.073), (0.824, 0.504, 0.291, 0.074),
        (0.852, 0.525, 0.310, 0.090),
    ], bronze)
    top = horizontal_profile("Ritual table", [
        (0.839, 0.465, 0.270, 0.083), (0.907, 0.600, 0.347, 0.097),
        (0.971, 0.600, 0.347, 0.097), (1.020, 0.559, 0.314, 0.092),
    ], stone)
    parts = [foot, base_band, body, rim, top]

    # Cut actual wells in the slab, then frame and mark the two relic seats.
    for center_x in (-0.241, 0.241):
        center = (center_x, 0.009)
        cutter = horizontal_profile("Well cutter", [
            (0.928, 0.119, 0.098, 0.039), (0.976, 0.136, 0.116, 0.044),
            (1.020, 0.163, 0.142, 0.052), (1.070, 0.168, 0.147, 0.052),
        ], center=center)
        recess = top.modifiers.new("Relic socket", "BOOLEAN")
        recess.operation = "DIFFERENCE"
        recess.solver = "EXACT"
        recess.object = cutter
        bpy.context.view_layer.objects.active = top
        bpy.ops.object.modifier_apply(modifier=recess.name)
        bpy.data.objects.remove(cutter, do_unlink=True)

        rings = [
            [(x + center[0], y + center[1], z) for x, y in outline(width, depth, corner)]
            for z, width, depth, corner in (
                (1.016, 0.206, 0.183, 0.071),
                (1.025, 0.204, 0.181, 0.071),
                (1.025, 0.166, 0.145, 0.055),
                (1.016, 0.160, 0.138, 0.054),
            )
        ]
        parts.append(make_loft("Relic socket rim", rings, bronze, closed=True))
        angles = [-math.pi / 2, 0.0, math.pi / 2, math.pi]
        sigil = [
            [(center[0] + radius * math.cos(angle), center[1] + radius * math.sin(angle), z)
             for angle in angles]
            for z, radius in ((0.927, 0.048), (0.936, 0.045))
        ]
        parts.append(make_loft("Relic socket sigil", sigil, ember))

    # Two side braces frame the split ember mark on the player-facing panel.
    for center_x in (-0.381, 0.381):
        parts.append(horizontal_profile("Obsidian brace", [
            (0.179, 0.111, 0.095, 0.047), (0.253, 0.109, 0.092, 0.039),
            (0.665, 0.073, 0.061, 0.030), (0.836, 0.095, 0.064, 0.032),
        ], stone, center=(center_x, -0.189)))
    parts.append(front_badge("Bronze escutcheon", [
        (-0.235, 0.423), (0.0, 0.310), (0.235, 0.423),
        (0.191, 0.687), (0.0, 0.765), (-0.192, 0.687),
    ], (-0.222, -0.264), bronze))
    parts.append(front_badge("Stone face", [
        (-0.205, 0.438), (0.0, 0.342), (0.205, 0.438),
        (0.165, 0.665), (0.0, 0.734), (-0.165, 0.665),
    ], (-0.260, -0.271), stone))
    parts.append(front_badge("Upper ember", [
        (-0.078, 0.526), (0.0, 0.557), (0.077, 0.526), (0.0, 0.674),
    ], (-0.270, -0.278), ember))
    parts.append(front_badge("Lower ember", [
        (-0.078, 0.502), (0.0, 0.392), (0.077, 0.502), (0.0, 0.531),
    ], (-0.270, -0.278), ember))
    for material in (stone, bronze, ember):
        if material.name not in foot.data.materials:
            foot.data.materials.append(material)

    for part in parts:
        part.select_set(True)
    bpy.context.view_layer.objects.active = foot
    bpy.ops.object.join()
    foot.name = ASSET_NAME
    foot.data.name = ASSET_NAME
    return foot


def main():
    if not bpy.app.background or bpy.data.filepath or "--" not in sys.argv:
        raise RuntimeError("Run in a fresh Blender background process with an output directory after --.")
    args = sys.argv[sys.argv.index("--") + 1:]
    if len(args) != 1:
        raise ValueError("Expected one output directory after --.")
    output = Path(args[0]).resolve()
    output.mkdir(parents=True, exist_ok=True)
    blend = output / f"{ASSET_NAME}.blend"
    exchange = output / f"{ASSET_NAME}.fbx"
    if blend.exists() or exchange.exists():
        raise FileExistsError("Use a fresh output directory to preserve the current altar.")

    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    scene.unit_settings.system = "METRIC"
    scene.unit_settings.scale_length = 1.0
    altar = build_altar()
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.mesh.normals_make_consistent(inside=False)
    bpy.ops.uv.smart_project(island_margin=0.018)
    bpy.ops.object.mode_set(mode="OBJECT")
    altar.data.calc_loop_triangles()
    for screen in bpy.data.screens:
        for area in screen.areas:
            if area.type == "VIEW_3D":
                view = area.spaces.active
                view.shading.color_type = "MATERIAL"
                view.region_3d.view_location = (0, 0, 0.48)
                view.region_3d.view_distance = 2.6
                view.region_3d.view_rotation = Quaternion(
                    (Vector((0, 0, 0.50)) - Vector((1.7, -2.9, 2.1))).to_track_quat("-Z", "Y")
                )

    if bpy.ops.wm.save_as_mainfile(filepath=str(blend)) != {"FINISHED"}:
        raise RuntimeError(f"Could not save {blend}")
    if bpy.ops.export_scene.fbx(
        filepath=str(exchange), use_selection=True, object_types={"MESH"},
        global_scale=1.0, apply_unit_scale=True, apply_scale_options="FBX_SCALE_UNITS",
        axis_forward="-Y", axis_up="Z", use_triangles=True, bake_anim=False,
    ) != {"FINISHED"}:
        raise RuntimeError(f"Could not export {exchange}")
    size_cm = tuple(round(value * 100, 2) for value in altar.dimensions)
    print(f"{ASSET_NAME}: XYZ {size_cm} cm; {len(altar.data.loop_triangles)} triangles.", flush=True)
    print(f"Saved {blend}\nExported {exchange}", flush=True)


if __name__ == "__main__":
    main()
