"""Headless Blender: import raw GLB → clean, scale, UV, LOD, collision → save .blend.

Args after `--`:
  input.glb  output.blend  asset_id  height_m  lods_csv  collision
"""
from __future__ import annotations

import sys
from pathlib import Path


def _argv_after_double_dash() -> list[str]:
    if "--" in sys.argv:
        return sys.argv[sys.argv.index("--") + 1 :]
    return sys.argv[1:]


def main() -> None:
    import bpy
    import bmesh
    from mathutils import Vector

    args = _argv_after_double_dash()
    if len(args) < 6:
        raise SystemExit("Usage: process.py input.glb output.blend asset_id height_m lods collision")

    src, dst, asset_id, height_s, lods_csv, collision = args[:6]
    height_m = float(height_s)
    lod_factors = [float(x) for x in lods_csv.split(",") if x.strip()]

    bpy.ops.wm.read_factory_settings(use_empty=True)

    # Import
    ext = Path(src).suffix.lower()
    if ext in (".glb", ".gltf"):
        bpy.ops.import_scene.gltf(filepath=src)
    elif ext == ".obj":
        bpy.ops.wm.obj_import(filepath=src)
    else:
        raise SystemExit(f"Unsupported input: {src}")

    mesh_objs = [o for o in bpy.context.scene.objects if o.type == "MESH" and not o.name.startswith("COL_")]
    if not mesh_objs:
        raise SystemExit("No mesh objects imported")

    # Join into one
    bpy.ops.object.select_all(action="DESELECT")
    for o in mesh_objs:
        o.select_set(True)
    bpy.context.view_layer.objects.active = mesh_objs[0]
    if len(mesh_objs) > 1:
        bpy.ops.object.join()
    obj = bpy.context.view_layer.objects.active
    obj.name = f"{asset_id}_LOD0"

    # Apply transforms, origin to bottom centre, scale to height
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    bbox = [obj.matrix_world @ Vector(c) for c in obj.bound_box]
    min_v = Vector((min(v.x for v in bbox), min(v.y for v in bbox), min(v.z for v in bbox)))
    max_v = Vector((max(v.x for v in bbox), max(v.y for v in bbox), max(v.z for v in bbox)))
    # Blender is Z-up; glTF import often converts to Z-up. Re-orient to Y-up for engine? 
    # Blender glTF importer uses +Y up when scene is configured; keep Blender Z-up in .blend,
    # export.py writes glTF Y-up.
    size = max_v - min_v
    current_h = max(size.z, 1e-4)
    scale = height_m / current_h
    obj.scale = (scale, scale, scale)
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)

    bbox = [obj.matrix_world @ Vector(c) for c in obj.bound_box]
    min_v = Vector((min(v.x for v in bbox), min(v.y for v in bbox), min(v.z for v in bbox)))
    max_v = Vector((max(v.x for v in bbox), max(v.y for v in bbox), max(v.z for v in bbox)))
    centre_x = (min_v.x + max_v.x) * 0.5
    centre_y = (min_v.y + max_v.y) * 0.5
    obj.location -= Vector((centre_x, centre_y, min_v.z))
    bpy.ops.object.transform_apply(location=True, rotation=False, scale=False)

    # Clean mesh
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.mesh.remove_doubles(threshold=0.0001)
    bpy.ops.mesh.normals_make_consistent(inside=False)
    bpy.ops.object.mode_set(mode="OBJECT")

    # Smart UV project if no UVs
    me = obj.data
    if not me.uv_layers:
        bpy.ops.object.mode_set(mode="EDIT")
        bpy.ops.mesh.select_all(action="SELECT")
        bpy.ops.uv.smart_project(angle_limit=66.0, island_margin=0.02)
        bpy.ops.object.mode_set(mode="OBJECT")

    # Decimate LODs
    for i, factor in enumerate(lod_factors):
        if i == 0:
            continue
        lod = obj.copy()
        lod.data = obj.data.copy()
        lod.name = f"{asset_id}_LOD{i}"
        bpy.context.collection.objects.link(lod)
        mod = lod.modifiers.new(name="Decimate", type="DECIMATE")
        mod.ratio = max(0.05, min(1.0, factor))
        bpy.context.view_layer.objects.active = lod
        bpy.ops.object.modifier_apply(modifier=mod.name)

    # Collision helper empty / mesh
    bpy.context.view_layer.objects.active = obj
    bbox = [obj.matrix_world @ Vector(c) for c in obj.bound_box]
    min_v = Vector((min(v.x for v in bbox), min(v.y for v in bbox), min(v.z for v in bbox)))
    max_v = Vector((max(v.x for v in bbox), max(v.y for v in bbox), max(v.z for v in bbox)))
    dims = max_v - min_v
    mid = (min_v + max_v) * 0.5

    if collision == "capsule":
        bpy.ops.object.empty_add(type="PLAIN_AXES", location=(mid.x, mid.y, mid.z))
        col = bpy.context.view_layer.objects.active
        col.name = "COL_capsule"
        col["nu_collision"] = "capsule"
        col["radius"] = float(max(dims.x, dims.y) * 0.5)
        col["half_height"] = float(max(dims.z * 0.5 - col["radius"], 0.01))
    elif collision == "mesh":
        col = obj.copy()
        col.data = obj.data.copy()
        col.name = "COL_mesh"
        col["nu_collision"] = "mesh"
        bpy.context.collection.objects.link(col)
        # hide render
        col.hide_render = True
    elif collision != "none":
        bpy.ops.object.empty_add(type="CUBE", location=(mid.x, mid.y, mid.z))
        col = bpy.context.view_layer.objects.active
        col.name = "COL_box"
        col.empty_display_size = float(max(dims) * 0.5)
        col["nu_collision"] = "box"
        col["half_extents"] = [float(dims.x * 0.5), float(dims.y * 0.5), float(dims.z * 0.5)]

    # Custom property on root
    obj["nu_asset"] = asset_id

    Path(dst).parent.mkdir(parents=True, exist_ok=True)
    bpy.ops.wm.save_as_mainfile(filepath=dst)
    print(f"nu-content process: saved {dst}")


if __name__ == "__main__":
    main()
