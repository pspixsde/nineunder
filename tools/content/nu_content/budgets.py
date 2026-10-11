from __future__ import annotations

import sys
from dataclasses import dataclass
from pathlib import Path

if sys.version_info >= (3, 11):
    import tomllib
else:
    import tomli as tomllib

from nu_content.paths import category_folder, config_dir


@dataclass
class Budget:
    tris_min: int
    tris_max: int
    texture_max: int


def load_budgets() -> dict[str, Budget]:
    path = config_dir() / "budgets.toml"
    with path.open("rb") as f:
        data = tomllib.load(f)
    out: dict[str, Budget] = {}
    for key, section in data.items():
        if not isinstance(section, dict):
            continue
        out[key] = Budget(
            tris_min=int(section.get("tris_min", 0)),
            tris_max=int(section.get("tris_max", 10**9)),
            texture_max=int(section.get("texture_max", 4096)),
        )
    return out


def budget_for(category: str) -> Budget:
    budgets = load_budgets()
    key = category_folder(category)
    return budgets.get(key, budgets.get("default", Budget(0, 10**9, 4096)))


@dataclass
class BudgetResult:
    ok: bool
    tris: int
    budget: Budget
    messages: list[str]


def check_tris(category: str, tris: int) -> BudgetResult:
    b = budget_for(category)
    messages: list[str] = []
    ok = True
    if tris < b.tris_min:
        ok = False
        messages.append(f"tris {tris} below min {b.tris_min}")
    if tris > b.tris_max:
        ok = False
        messages.append(f"tris {tris} above max {b.tris_max}")
    if ok:
        messages.append(f"tris {tris} within [{b.tris_min}, {b.tris_max}]")
    return BudgetResult(ok=ok, tris=tris, budget=b, messages=messages)


def estimate_glb_tris(glb_path: Path) -> int | None:
    """Best-effort triangle count from a GLB via optional pygltflib-free JSON scan.

    Parses the GLB JSON chunk for accessor/index counts without external deps.
    Returns None if parsing fails.
    """
    try:
        data = glb_path.read_bytes()
    except OSError:
        return None
    if len(data) < 20 or data[0:4] != b"glTF":
        # Maybe .gltf JSON
        try:
            import json

            doc = json.loads(data.decode("utf-8"))
            return _tris_from_gltf_json(doc)
        except Exception:
            return None

    import json
    import struct

    _magic, _version, length = struct.unpack_from("<III", data, 0)
    offset = 12
    while offset + 8 <= length:
        chunk_len, chunk_type = struct.unpack_from("<II", data, offset)
        offset += 8
        chunk = data[offset : offset + chunk_len]
        offset += chunk_len
        if chunk_type == 0x4E4F534A:  # JSON
            # Chunk may be padded with spaces or NULs to 4-byte alignment
            text = chunk.decode("utf-8").rstrip(" \x00")
            doc = json.loads(text)
            return _tris_from_gltf_json(doc)
    return None


def _tris_from_gltf_json(doc: dict) -> int:
    accessors = doc.get("accessors", [])
    total = 0
    for mesh in doc.get("meshes", []):
        for prim in mesh.get("primitives", []):
            if "indices" in prim:
                acc = accessors[prim["indices"]]
                total += int(acc.get("count", 0)) // 3
            else:
                # Non-indexed: POSITION accessor
                attrs = prim.get("attributes", {})
                if "POSITION" in attrs:
                    acc = accessors[attrs["POSITION"]]
                    total += int(acc.get("count", 0)) // 3
    return total
