"""Build the EmberTrial Cinder Blade trial mesh in Blender."""

import math
from pathlib import Path
import sys

import bmesh
import bpy


ASSET_NAME = "SM_ET_CinderBlade"


def add_rings(vertices, polygons, slots, rings, material_slot):
    first_vertex = len(vertices)
    ring_size = len(rings[0])
    for ring in rings:
        vertices.extend(ring)

    faces = [tuple(reversed(range(first_vertex, first_vertex + ring_size)))]
    for level in range(len(rings) - 1):
        lower = first_vertex + level * ring_size
        upper = lower + ring_size
        for corner in range(ring_size):
            next_corner = (corner + 1) % ring_size
            faces.append((lower + corner, lower + next_corner,
                          upper + next_corner, upper + corner))

    last = first_vertex + (len(rings) - 1) * ring_size
    faces.append(tuple(range(last, last + ring_size)))
    polygons.extend(faces)
    slots.extend([material_slot] * len(faces))


def material(name, color, metallic, roughness):
    result = bpy.data.materials.new(name)
    result.diffuse_color = (*color, 1.0)
    if result.node_tree is None:
        result.use_nodes = True
    shader = result.node_tree.nodes.get("Principled BSDF")
    shader.inputs["Base Color"].default_value = result.diffuse_color
    shader.inputs["Metallic"].default_value = metallic
    shader.inputs["Roughness"].default_value = roughness
    return result


def build_mesh():
    # Dimensions are in meters. The grip center is the mesh origin.
    vertices, polygons, slots = [], [], []

    blade_sections = [
        (0.062, 0.000, 0.022, 0.007),
        (0.105, 0.000, 0.027, 0.009),
        (0.352, 0.004, 0.024, 0.007),
        (0.418, 0.009, 0.018, 0.0055),
        (0.470, 0.017, 0.0025, 0.0025),
    ]
    blade_rings = []
    for z, center, half_width, half_depth in blade_sections:
        # A faceted cross section keeps the bevel visible under the trial lighting.
        section = [
            (-half_width, 0.0), (-half_width * 0.58, -half_depth),
            (half_width * 0.58, -half_depth), (half_width, 0.0),
            (half_width * 0.58, half_depth), (-half_width * 0.58, half_depth),
        ]
        blade_rings.append([(center + x, y, z) for x, y in section])
    add_rings(vertices, polygons, slots, blade_rings, 0)

    # Octagonal guard, grip, and cap. Each ring stores Z and the X/Y radii.
    handle_profiles = [
        (0, [(0.049, 0.037, 0.015), (0.055, 0.044, 0.017),
             (0.066, 0.044, 0.017), (0.073, 0.037, 0.015)]),
        (1, [(-0.073, 0.013, 0.011), (-0.060, 0.015, 0.013),
             (0.033, 0.016, 0.013), (0.051, 0.014, 0.012)]),
        (0, [(-0.090, 0.010, 0.010), (-0.087, 0.016, 0.013),
             (-0.071, 0.016, 0.013), (-0.067, 0.013, 0.011)]),
    ]
    angles = [math.tau * corner / 8 for corner in range(8)]
    for slot, profile in handle_profiles:
        rings = [
            [(rx * math.cos(angle), ry * math.sin(angle), z) for angle in angles]
            for z, rx, ry in profile
        ]
        add_rings(vertices, polygons, slots, rings, slot)

    # Small amber inlays on both faces sit proud of the steel.
    for side in (-1, 1):
        inlay_rings = [
            [(0.003 - width, side * near, z), (0.003 + width, side * near, z),
             (0.003 + width, side * far, z), (0.003 - width, side * far, z)]
            for z, width, near, far in (
                (0.115, 0.003, 0.0088, 0.0098),
                (0.143, 0.006, 0.0085, 0.0094),
                (0.264, 0.003, 0.0075, 0.0086),
            )
        ]
        add_rings(vertices, polygons, slots, inlay_rings, 2)

    mesh = bpy.data.meshes.new(ASSET_NAME)
    mesh.from_pydata(vertices, [], polygons)
    mesh.materials.append(material("M_ET_BladeSteel", (0.105, 0.117, 0.138), 0.72, 0.38))
    mesh.materials.append(material("M_ET_BladeWrap", (0.031, 0.025, 0.022), 0.0, 0.78))
    mesh.materials.append(material("M_ET_BladeEmber", (0.46, 0.11, 0.022), 0.18, 0.42))
    for polygon, slot in zip(mesh.polygons, slots):
        polygon.material_index = slot

    editable = bmesh.new()
    editable.from_mesh(mesh)
    bmesh.ops.recalc_face_normals(editable, faces=list(editable.faces))
    editable.to_mesh(mesh)
    editable.free()
    mesh.update()
    return mesh


def main():
    if not bpy.app.background or bpy.data.filepath or "--" not in sys.argv:
        raise RuntimeError("Run in a fresh Blender background process and pass an output folder after --.")
    args = sys.argv[sys.argv.index("--") + 1:]
    if len(args) != 1:
        raise ValueError("Expected exactly one output folder after --.")

    output = Path(args[0]).resolve()
    output.mkdir(parents=True, exist_ok=True)
    blend = output / (ASSET_NAME + ".blend")
    exchange = output / (ASSET_NAME + ".fbx")
    if blend.exists() or exchange.exists():
        raise FileExistsError("Choose an empty output folder to preserve the current model.")

    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    scene.unit_settings.system = "METRIC"
    scene.unit_settings.scale_length = 1.0
    mesh = build_mesh()
    weapon = bpy.data.objects.new(ASSET_NAME, mesh)
    scene.collection.objects.link(weapon)
    weapon.select_set(True)
    bpy.context.view_layer.objects.active = weapon
    bpy.context.view_layer.update()
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    if bpy.ops.uv.smart_project(island_margin=0.02) != {"FINISHED"}:
        raise RuntimeError("Could not generate the weapon UVs.")
    bpy.ops.object.mode_set(mode="OBJECT")
    mesh.calc_loop_triangles()

    if bpy.ops.wm.save_as_mainfile(filepath=str(blend)) != {"FINISHED"}:
        raise RuntimeError(f"Could not save {blend}")
    if bpy.ops.export_scene.fbx(
        filepath=str(exchange), use_selection=True, object_types={"MESH"},
        global_scale=1.0, apply_unit_scale=True, apply_scale_options="FBX_SCALE_UNITS",
        axis_forward="-Y", axis_up="Z", use_triangles=True, bake_anim=False,
    ) != {"FINISHED"}:
        raise RuntimeError(f"Could not export {exchange}")

    size_cm = tuple(round(value * 100, 2) for value in weapon.dimensions)
    print(f"{ASSET_NAME}: XYZ {size_cm} cm; {len(mesh.loop_triangles)} triangles.", flush=True)
    print(f"Saved {blend}\nExported {exchange}", flush=True)


if __name__ == "__main__":
    main()
