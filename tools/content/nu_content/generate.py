from __future__ import annotations

import shutil
from pathlib import Path

from rich.console import Console

from nu_content.backends.base import get_backend
from nu_content.comfy_client import ComfyClient, patch_workflow
from nu_content.manifest import AssetManifest, set_status

console = Console()


def latest_concept_image(m: AssetManifest) -> Path | None:
    concept = m.root / "concept"
    if not concept.exists():
        return None
    images = sorted(concept.glob("*.png")) + sorted(concept.glob("*.jpg"))
    return images[-1] if images else None


def run_concept(m: AssetManifest, *, seed: int | None = None) -> Path:
    workflow_name = m.concept.get("workflow", "concept_prop_v1")
    prompt = m.concept.get("prompt", m.id.replace("_", " "))
    use_seed = seed if seed is not None else int(m.concept.get("seed", 42))

    with ComfyClient() as client:
        if not client.ping():
            raise RuntimeError(
                f"ComfyUI not reachable at {client.base}. Start ComfyUI and set NU_COMFYUI_URL."
            )
        wf = client.load_workflow(workflow_name)
        wf = patch_workflow(
            wf,
            {
                "PROMPT": prompt,
                "SEED": use_seed,
                "ASSET_ID": m.id,
            },
        )
        prompt_id = client.queue_prompt(wf)
        console.print(f"[cyan]Queued concept[/] {prompt_id}")
        hist = client.wait_prompt(prompt_id)
        files = client.collect_output_files(hist)
        if not files:
            raise RuntimeError("ComfyUI returned no concept images")
        out_dir = m.root / "concept"
        out_dir.mkdir(exist_ok=True)
        saved: Path | None = None
        for i, meta in enumerate(files):
            data = client.download_view(meta["filename"], meta.get("subfolder", ""), meta.get("type", "output"))
            dest = out_dir / f"{m.id}_concept_{i:02d}.png"
            dest.write_bytes(data)
            saved = dest
            console.print(f"  saved {dest}")
        m.concept["seed"] = use_seed
        m.save()
        assert saved is not None
        return saved


def run_generate(
    m: AssetManifest,
    *,
    backend_id: str = "triposg",
    candidates: int = 1,
    seed: int | None = None,
    force: bool = False,
) -> list[Path]:
    if m.status in ("cleaned", "polished", "approved") and not force:
        raise RuntimeError(
            f"Asset status is {m.status!r}; regenerating would bypass .blend. Pass --force to create candidates only."
        )

    info = get_backend(backend_id)
    if not info.eu_ok:
        console.print(
            f"[yellow]Warning:[/] backend {backend_id} licence may not grant rights in EU/UK/KR. "
            "Confirm before shipping."
        )

    concept = latest_concept_image(m)
    if concept is None:
        raise FileNotFoundError(f"No concept image in {m.root / 'concept'}. Run concept first or drop a PNG there.")

    base_seed = seed if seed is not None else int(m.generate.get("seed", 42))
    raw_dir = m.root / "raw"
    raw_dir.mkdir(exist_ok=True)
    written: list[Path] = []

    with ComfyClient() as client:
        if not client.ping():
            raise RuntimeError(f"ComfyUI not reachable at {client.base}")
        uploaded = client.upload_image(concept)
        for i in range(candidates):
            cand_seed = base_seed + i
            wf = client.load_workflow(info.workflow)
            wf = patch_workflow(
                wf,
                {
                    "IMAGE": uploaded,
                    "SEED": cand_seed,
                    "ASSET_ID": m.id,
                },
            )
            prompt_id = client.queue_prompt(wf)
            console.print(f"[cyan]Queued {backend_id} cand {i}[/] seed={cand_seed} id={prompt_id}")
            hist = client.wait_prompt(prompt_id)
            files = client.collect_output_files(hist)
            mesh_files = [f for f in files if f["filename"].lower().endswith((".glb", ".gltf", ".obj"))]
            if not mesh_files:
                # Some nodes dump GLB as generic files
                mesh_files = files
            if not mesh_files:
                raise RuntimeError(f"No mesh outputs for candidate {i}")
            meta = mesh_files[0]
            data = client.download_view(meta["filename"], meta.get("subfolder", ""), meta.get("type", "output"))
            dest = raw_dir / f"cand_{i:02d}.glb"
            dest.write_bytes(data)
            written.append(dest)
            console.print(f"  saved {dest}")

    # Select first candidate as selected by default
    selected = raw_dir / "selected.glb"
    shutil.copy2(written[0], selected)
    m.generate["backend"] = backend_id
    m.generate["backend_version"] = "workflow:" + info.workflow
    m.generate["seed"] = base_seed
    m.generate["selected"] = "raw/selected.glb"
    m.save()
    if m.status == "concept":
        set_status(m, "generated")
    return written


def import_glb(m: AssetManifest, src: Path, *, select: bool = True, force: bool = False) -> Path:
    if not src.exists():
        raise FileNotFoundError(src)
    if m.status in ("cleaned", "polished", "approved") and not force:
        raise RuntimeError(
            f"Asset status is {m.status!r}; import refused without --force "
            "(will not overwrite polished .blend; raw/ only)."
        )
    raw_dir = m.root / "raw"
    raw_dir.mkdir(exist_ok=True)
    dest = raw_dir / "selected.glb" if select else raw_dir / src.name
    shutil.copy2(src, dest)
    m.generate["backend"] = m.generate.get("backend", "manual")
    m.generate["backend_version"] = m.generate.get("backend_version", "import")
    m.generate["selected"] = str(dest.relative_to(m.root)).replace("\\", "/")
    m.save()
    if m.status == "concept":
        set_status(m, "generated")
    return dest
