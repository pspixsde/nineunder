"""Export a level .blend to GLB with extras (same as export.py, level-oriented).

Args after `--`:
  input.blend  output.glb  level_id
"""
from __future__ import annotations

import sys

# Re-use export.main
from pathlib import Path

# When run by Blender, this file's directory is on sys.path if invoked correctly.
# Fallback: exec export.py logic inline by importing sibling is unreliable in Blender.
# So duplicate the thin wrapper:

def _argv_after_double_dash() -> list[str]:
    if "--" in sys.argv:
        return sys.argv[sys.argv.index("--") + 1 :]
    return sys.argv[1:]


def main() -> None:
    import bpy

    args = _argv_after_double_dash()
    if len(args) < 3:
        raise SystemExit("Usage: level_export.py input.blend output.glb level_id")

    src, dst, level_id = args[:3]
    bpy.ops.wm.open_mainfile(filepath=src)

    for obj in bpy.context.scene.objects:
        if obj.name.startswith("SPAWN_") and "nu_asset" not in obj:
            obj["nu_asset"] = obj.name[len("SPAWN_") :]
        if obj.name.startswith("COL_") and "nu_collision" not in obj:
            if "capsule" in obj.name.lower():
                obj["nu_collision"] = "capsule"
            elif "mesh" in obj.name.lower():
                obj["nu_collision"] = "mesh"
            else:
                obj["nu_collision"] = "box"
        if "nu_level" not in obj and obj.type == "EMPTY" and obj.name == "LevelRoot":
            obj["nu_level"] = level_id

    Path(dst).parent.mkdir(parents=True, exist_ok=True)
    bpy.ops.export_scene.gltf(
        filepath=dst,
        export_format="GLB",
        use_selection=False,
        export_apply=True,
        export_texcoords=True,
        export_normals=True,
        export_tangents=True,
        export_materials="EXPORT",
        export_extras=True,
        export_yup=True,
        export_animations=False,
        export_skins=False,
    )
    print(f"nu-content level_export: wrote {dst}")


if __name__ == "__main__":
    main()
