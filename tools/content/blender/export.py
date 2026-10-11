"""Headless Blender: open .blend → export glTF GLB with extras / tangents.

Args after `--`:
  input.blend  output.glb  asset_id
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

    args = _argv_after_double_dash()
    if len(args) < 3:
        raise SystemExit("Usage: export.py input.blend output.glb asset_id")

    src, dst, asset_id = args[:3]
    bpy.ops.wm.open_mainfile(filepath=src)

    # Ensure custom props export as extras
    for obj in bpy.context.scene.objects:
        if "nu_asset" not in obj:
            if not obj.name.startswith("COL_"):
                obj["nu_asset"] = asset_id

    bpy.ops.object.select_all(action="SELECT")
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
        export_colors=True,
        export_attributes=True,
        export_extras=True,
        export_yup=True,
        export_animations=True,
        export_skins=True,
        export_morph=False,
    )
    print(f"nu-content export: wrote {dst}")


if __name__ == "__main__":
    main()
