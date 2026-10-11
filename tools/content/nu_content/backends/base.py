from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from typing import Protocol

from nu_content.manifest import AssetManifest


@dataclass
class BackendInfo:
    id: str
    workflow: str
    licence: str
    vram_gb: float
    notes: str
    eu_ok: bool = True


BACKENDS: dict[str, BackendInfo] = {
    "triposg": BackendInfo(
        id="triposg",
        workflow="i23d_triposg_v1.json",
        licence="MIT",
        vram_gb=8.0,
        notes="Default shape backend for 8 GB GPUs",
        eu_ok=True,
    ),
    "triposr": BackendInfo(
        id="triposr",
        workflow="i23d_triposr_v1.json",
        licence="MIT",
        vram_gb=6.0,
        notes="Fast blockouts; rough topology",
        eu_ok=True,
    ),
    "hunyuan3d_mini": BackendInfo(
        id="hunyuan3d_mini",
        workflow="i23d_hunyuan3d_mini_v1.json",
        licence="Hunyuan Community (NOT EU/UK/KR)",
        vram_gb=8.0,
        notes="Only where Community licence grants rights; use offloading on 8 GB",
        eu_ok=False,
    ),
}


def list_backends() -> list[BackendInfo]:
    return list(BACKENDS.values())


def get_backend(backend_id: str) -> BackendInfo:
    if backend_id not in BACKENDS:
        known = ", ".join(BACKENDS)
        raise KeyError(f"Unknown backend {backend_id!r}. Known: {known}")
    return BACKENDS[backend_id]


class ShapeBackend(Protocol):
    def generate(self, manifest: AssetManifest, *, seed: int, concept_image: Path) -> Path: ...
