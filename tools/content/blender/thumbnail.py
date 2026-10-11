"""Headless Blender: render a simple thumbnail of a GLB.

Args after `--`:
  input.glb  output.png
"""
from __future__ import annotations

import math
import sys
from pathlib import Path


def _argv_after_double_dash() -> list[str]:
    if "--" in sys.argv:
        return sys.argv[sys.argv.index("--") + 1 :]
    return sys.argv[1:]


def main() -> None:
    import bpy
    from mathutils import Vector

    args = _argv_after_double_dash()
    if len(args) < 2:
        raise SystemExit("Usage: thumbnail.py input.glb output.png")

    src, dst = args[:2]
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=src)

    meshes = [o for o in bpy.context.scene.objects if o.type == "MESH"]
    if not meshes:
        raise SystemExit("No meshes for thumbnail")

    # Framing
    bbox_pts = []
    for o in meshes:
        bbox_pts.extend(o.matrix_world @ Vector(c) for c in o.bound_box)
    centre = sum(bbox_pts, Vector()) / len(bbox_pts)
    radius = max((p - centre).length for p in bbox_pts)

    # Camera
    bpy.ops.object.camera_add()
    cam = bpy.context.view_layer.objects.active
    bpy.context.scene.camera = cam
    dist = radius * 2.8 + 0.5
    cam.location = centre + Vector((dist * 0.7, -dist, dist * 0.55))
    direction = centre - cam.location
    cam.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()

    # Light
    bpy.ops.object.light_add(type="SUN", location=centre + Vector((2, -3, 5)))
    sun = bpy.context.view_layer.objects.active
    sun.data.energy = 3.0

    scene = bpy.context.scene
    scene.render.engine = "BLENDER_EEVEE_NEXT" if hasattr(bpy.types, "EEVEE_NEXT") else "BLENDER_EEVEE"
    scene.render.resolution_x = 256
    scene.render.resolution_y = 256
    scene.render.filepath = dst
    scene.render.image_settings.file_format = "PNG"
    scene.render.film_transparent = True

    Path(dst).parent.mkdir(parents=True, exist_ok=True)
    bpy.ops.render.render(write_still=True)
    print(f"nu-content thumbnail: {dst}")


if __name__ == "__main__":
    main()
