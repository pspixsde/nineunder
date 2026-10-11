#!/usr/bin/env python3
"""CI helper: validate a lightweight sample prop GLB triangle budget (no GPU / Blender)."""
from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "tools" / "content"))

from nu_content.budgets import check_tris, estimate_glb_tris  # noqa: E402
from nu_content.manifest import iter_manifests  # noqa: E402
from nu_content.validate import validate_asset  # noqa: E402


def main() -> int:
    sample = ROOT / "assets" / "models" / "prop" / "prop_practice_cube.glb"
    if not sample.exists():
        print("sample prop missing — skip")
        return 0

    tris = estimate_glb_tris(sample)
    print(f"prop_practice_cube tris={tris}")
    if tris is None or tris <= 0:
        print("failed to estimate tris", file=sys.stderr)
        return 1
    result = check_tris("prop", tris)
    print("; ".join(result.messages))
    if not result.ok:
        return 1

    # Manifest-driven validate when asset.toml + export GLB exist
    for m in iter_manifests():
        if not m.export_glb.exists():
            continue
        # Skip oversized AI exports until they are budgeted / LODded
        report = validate_asset(m)
        print(f"{m.id}: {'OK' if report.ok else 'FAIL'} tris={report.tris}")
        for msg in report.messages:
            print(f"  {msg}")
        if not report.ok and m.id == "prop_practice_cube":
            return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
