from __future__ import annotations

import os
from pathlib import Path


def repo_root() -> Path:
    env = os.environ.get("NU_REPO_ROOT")
    if env:
        return Path(env).resolve()
    # tools/content/nu_content/paths.py → repo root
    return Path(__file__).resolve().parents[3]


def content_root() -> Path:
    return Path(__file__).resolve().parents[1]


def config_dir() -> Path:
    return content_root() / "config"


def blender_scripts_dir() -> Path:
    return content_root() / "blender"


def workflows_dir() -> Path:
    return content_root() / "comfyui" / "workflows"


def assets_dir() -> Path:
    return repo_root() / "assets"


def source_dir() -> Path:
    return assets_dir() / "source"


def models_dir() -> Path:
    return assets_dir() / "models"


def category_folder(category: str) -> str:
    """character.humanoid → character."""
    return category.split(".", 1)[0]


def asset_dir(asset_id: str, category: str | None = None) -> Path:
    if category:
        return source_dir() / category_folder(category) / asset_id
    # Search known category folders
    for child in source_dir().iterdir() if source_dir().exists() else []:
        if not child.is_dir():
            continue
        candidate = child / asset_id
        if candidate.is_dir():
            return candidate
    raise FileNotFoundError(f"Asset folder not found for id={asset_id!r}")


def models_export_path(asset_id: str, category: str) -> Path:
    return models_dir() / category_folder(category) / f"{asset_id}.glb"
