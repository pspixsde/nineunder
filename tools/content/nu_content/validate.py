from __future__ import annotations

import json
import os
import shutil
import subprocess
from dataclasses import dataclass, field
from pathlib import Path

from nu_content.budgets import check_tris, estimate_glb_tris
from nu_content.manifest import AssetManifest


@dataclass
class ValidationReport:
    asset_id: str
    ok: bool
    messages: list[str] = field(default_factory=list)
    tris: int | None = None


def find_gltf_validator() -> str | None:
    env = os.environ.get("NU_GLTF_VALIDATOR")
    if env and Path(env).exists():
        return env
    return shutil.which("gltf_validator")


def run_gltf_validator(glb: Path) -> list[str]:
    exe = find_gltf_validator()
    if not exe:
        return ["glTF-Validator not installed (optional) — skipped"]
    proc = subprocess.run(
        [exe, str(glb), "-o", "-"],
        check=False,
        capture_output=True,
        text=True,
    )
    msgs: list[str] = []
    if proc.returncode != 0:
        msgs.append(f"gltf_validator exit {proc.returncode}")
        if proc.stdout:
            msgs.append(proc.stdout.strip()[:2000])
        if proc.stderr:
            msgs.append(proc.stderr.strip()[:1000])
    else:
        msgs.append("gltf_validator: OK")
        # Try to surface issue counts from JSON if present
        try:
            data = json.loads(proc.stdout)
            issues = data.get("issues", {})
            err = issues.get("numErrors", 0)
            warn = issues.get("numWarnings", 0)
            if err:
                msgs.append(f"validator errors: {err}")
            if warn:
                msgs.append(f"validator warnings: {warn}")
        except Exception:
            pass
    return msgs


def validate_asset(m: AssetManifest) -> ValidationReport:
    report = ValidationReport(asset_id=m.id, ok=True)
    glb = m.export_glb
    if not glb.exists():
        report.ok = False
        report.messages.append(f"missing export: {glb}")
        return report

    report.messages.extend(run_gltf_validator(glb))
    if any("gltf_validator exit" in msg for msg in report.messages):
        report.ok = False

    tris = estimate_glb_tris(glb)
    report.tris = tris
    if tris is None:
        report.messages.append("could not estimate triangle count")
    else:
        br = check_tris(m.category, tris)
        report.messages.extend(br.messages)
        if not br.ok:
            report.ok = False

    # Soft checks
    if m.status in ("polished", "approved") and not m.blend_path.exists():
        report.ok = False
        report.messages.append("polished/approved asset missing .blend source")

    return report


def validate_all() -> list[ValidationReport]:
    from nu_content.manifest import iter_manifests

    return [validate_asset(m) for m in iter_manifests() if m.export_glb.exists()]
