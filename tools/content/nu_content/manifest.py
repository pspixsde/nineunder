from __future__ import annotations

import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

if sys.version_info >= (3, 11):
    import tomllib
else:
    import tomli as tomllib

import tomli_w

from nu_content.paths import asset_dir, category_folder, models_export_path, source_dir

STATUSES = ("concept", "generated", "cleaned", "polished", "approved")


@dataclass
class AssetManifest:
    id: str
    category: str
    status: str = "concept"
    height_m: float = 1.0
    concept: dict[str, Any] = field(default_factory=dict)
    generate: dict[str, Any] = field(default_factory=dict)
    export: dict[str, Any] = field(default_factory=dict)
    notes: str = ""

    @property
    def root(self) -> Path:
        return asset_dir(self.id, self.category)

    @property
    def blend_path(self) -> Path:
        return self.root / f"{self.id}.blend"

    @property
    def selected_raw(self) -> Path:
        selected = self.generate.get("selected", "raw/selected.glb")
        return self.root / selected

    @property
    def export_glb(self) -> Path:
        return models_export_path(self.id, self.category)

    def toml_path(self) -> Path:
        return self.root / "asset.toml"

    def to_dict(self) -> dict[str, Any]:
        data: dict[str, Any] = {
            "id": self.id,
            "category": self.category,
            "status": self.status,
            "height_m": self.height_m,
        }
        if self.concept:
            data["concept"] = self.concept
        if self.generate:
            data["generate"] = self.generate
        if self.export:
            data["export"] = self.export
        if self.notes:
            data["notes"] = self.notes
        return data

    def save(self) -> None:
        self.root.mkdir(parents=True, exist_ok=True)
        (self.root / "concept").mkdir(exist_ok=True)
        (self.root / "raw").mkdir(exist_ok=True)
        with self.toml_path().open("wb") as f:
            tomli_w.dump(self.to_dict(), f)

    @classmethod
    def load(cls, path: Path) -> AssetManifest:
        with path.open("rb") as f:
            data = tomllib.load(f)
        return cls(
            id=data["id"],
            category=data["category"],
            status=data.get("status", "concept"),
            height_m=float(data.get("height_m", 1.0)),
            concept=dict(data.get("concept", {})),
            generate=dict(data.get("generate", {})),
            export=dict(data.get("export", {})),
            notes=str(data.get("notes", "")),
        )

    @classmethod
    def load_id(cls, asset_id: str) -> AssetManifest:
        root = asset_dir(asset_id)
        return cls.load(root / "asset.toml")


def create_manifest(
    asset_id: str,
    category: str,
    *,
    height_m: float = 1.0,
    prompt: str = "",
) -> AssetManifest:
    cat_root = category_folder(category)
    root = source_dir() / cat_root / asset_id
    if (root / "asset.toml").exists():
        raise FileExistsError(f"Asset already exists: {root}")

    m = AssetManifest(
        id=asset_id,
        category=category,
        status="concept",
        height_m=height_m,
        concept={
            "prompt": prompt,
            # Desktop-ready SD1.5 workflow (see tools/content/comfyui/workflows/)
            "workflow": "concept_prop_desktop_v1" if cat_root == "prop" else "concept_character_v1",
        },
        generate={"backend": "triposg", "selected": "raw/selected.glb"},
        export={
            "lods": [1.0, 0.5, 0.25],
            "collision": "box" if cat_root == "prop" else "capsule",
            "rig": "nu_humanoid" if cat_root in ("character", "enemy") else "",
        },
    )
    m.save()
    return m


def iter_manifests() -> list[AssetManifest]:
    results: list[AssetManifest] = []
    src = source_dir()
    if not src.exists():
        return results
    for toml in sorted(src.rglob("asset.toml")):
        results.append(AssetManifest.load(toml))
    return results


def set_status(m: AssetManifest, status: str) -> None:
    if status not in STATUSES:
        raise ValueError(f"Invalid status {status!r}; expected one of {STATUSES}")
    # Never downgrade polished/approved via normal pipeline without force (caller checks)
    m.status = status
    m.save()
