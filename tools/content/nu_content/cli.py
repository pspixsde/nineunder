from __future__ import annotations

import shutil
from pathlib import Path

import click
from rich.console import Console
from rich.table import Table

from nu_content import __version__
from nu_content.backends.base import list_backends
from nu_content.blender_runner import run_blender_script
from nu_content.generate import import_glb, run_concept, run_generate
from nu_content.manifest import AssetManifest, create_manifest, iter_manifests, set_status
from nu_content.naming import validate_id
from nu_content.paths import models_dir, models_export_path
from nu_content.report import build_html_report
from nu_content.validate import validate_all, validate_asset

console = Console()


def _load(asset_id: str) -> AssetManifest:
    try:
        return AssetManifest.load_id(asset_id)
    except FileNotFoundError as e:
        raise click.ClickException(str(e)) from e


@click.group()
@click.version_option(__version__, prog_name="nu-content")
def main() -> None:
    """Nine Under content pipeline."""


@main.command("new")
@click.argument("asset_id")
@click.option("--category", "-c", required=True, help="e.g. prop, character.humanoid, enemy, environment")
@click.option("--height", default=1.0, show_default=True, help="Target height in metres")
@click.option("--prompt", default="", help="Seed prompt for concept generation")
def cmd_new(asset_id: str, category: str, height: float, prompt: str) -> None:
    """Create a new asset folder + asset.toml."""
    errs = validate_id(asset_id, category)
    if errs:
        raise click.ClickException("; ".join(errs))
    try:
        m = create_manifest(asset_id, category, height_m=height, prompt=prompt)
    except FileExistsError as e:
        raise click.ClickException(str(e)) from e
    console.print(f"[green]Created[/] {m.root}")


@main.command("import")
@click.argument("asset_id")
@click.argument("glb_path", type=click.Path(exists=True, path_type=Path))
@click.option("--force", is_flag=True, help="Allow import when status is cleaned/polished/approved")
def cmd_import(asset_id: str, glb_path: Path, force: bool) -> None:
    """Import a Modly / HF / manual GLB into raw/selected.glb."""
    m = _load(asset_id)
    try:
        dest = import_glb(m, glb_path, force=force)
    except RuntimeError as e:
        raise click.ClickException(str(e)) from e
    console.print(f"[green]Imported[/] → {dest}")


@main.command("status")
@click.argument("asset_id", required=False)
def cmd_status(asset_id: str | None) -> None:
    """Show status for one asset or a table of all."""
    if asset_id:
        m = _load(asset_id)
        console.print(f"{m.id}  category={m.category}  status={m.status}")
        console.print(f"  root={m.root}")
        console.print(f"  blend={'yes' if m.blend_path.exists() else 'no'}")
        console.print(f"  raw={'yes' if m.selected_raw.exists() else 'no'}")
        console.print(f"  export={'yes' if m.export_glb.exists() else 'no'}")
        return

    table = Table(title="Nine Under assets")
    table.add_column("ID")
    table.add_column("Category")
    table.add_column("Status")
    table.add_column("Export")
    for m in iter_manifests():
        table.add_row(m.id, m.category, m.status, "yes" if m.export_glb.exists() else "no")
    console.print(table)


@main.command("set-status")
@click.argument("asset_id")
@click.argument("status", type=click.Choice(["concept", "generated", "cleaned", "polished", "approved"]))
def cmd_set_status(asset_id: str, status: str) -> None:
    m = _load(asset_id)
    set_status(m, status)
    console.print(f"[green]{m.id}[/] → {status}")


@main.command("concept")
@click.argument("asset_id")
@click.option("--seed", type=int, default=None)
def cmd_concept(asset_id: str, seed: int | None) -> None:
    """Generate concept images via ComfyUI."""
    m = _load(asset_id)
    try:
        path = run_concept(m, seed=seed)
    except Exception as e:
        raise click.ClickException(str(e)) from e
    console.print(f"[green]Concept[/] {path}")


@main.command("generate")
@click.argument("asset_id")
@click.option("--backend", default="triposg", show_default=True)
@click.option("--candidates", default=1, show_default=True)
@click.option("--seed", type=int, default=None)
@click.option("--force", is_flag=True)
def cmd_generate(asset_id: str, backend: str, candidates: int, seed: int | None, force: bool) -> None:
    """Image-to-3D via ComfyUI backends."""
    m = _load(asset_id)
    try:
        paths = run_generate(m, backend_id=backend, candidates=candidates, seed=seed, force=force)
    except Exception as e:
        raise click.ClickException(str(e)) from e
    console.print(f"[green]Generated[/] {len(paths)} candidate(s)")


@main.command("backends")
def cmd_backends() -> None:
    table = Table(title="Shape backends")
    table.add_column("ID")
    table.add_column("VRAM")
    table.add_column("Licence")
    table.add_column("EU/UK/KR OK")
    table.add_column("Notes")
    for b in list_backends():
        table.add_row(b.id, f"{b.vram_gb:g} GB", b.licence, "yes" if b.eu_ok else "NO", b.notes)
    console.print(table)


@main.command("process")
@click.argument("asset_id")
@click.option("--force", is_flag=True, help="Create a revision .blend instead of refusing overwrite")
def cmd_process(asset_id: str, force: bool) -> None:
    """Blender headless clean of raw GLB into a .blend."""
    m = _load(asset_id)
    if not m.selected_raw.exists():
        raise click.ClickException(f"Missing raw mesh: {m.selected_raw}")
    if m.blend_path.exists() and m.status in ("polished", "approved") and not force:
        raise click.ClickException("Refusing to overwrite polished/approved .blend (pass --force for revision)")

    blend_out = m.blend_path
    if m.blend_path.exists() and force:
        rev = 1
        while True:
            candidate = m.root / f"{m.id}_rev{rev}.blend"
            if not candidate.exists():
                blend_out = candidate
                break
            rev += 1
        console.print(f"[yellow]Writing revision[/] {blend_out.name}")

    lods = ",".join(str(x) for x in m.export.get("lods", [1.0, 0.5, 0.25]))
    collision = str(m.export.get("collision", "box"))
    try:
        run_blender_script(
            "process.py",
            str(m.selected_raw),
            str(blend_out),
            m.id,
            str(m.height_m),
            lods,
            collision,
        )
    except Exception as e:
        raise click.ClickException(str(e)) from e

    if blend_out == m.blend_path and m.status in ("concept", "generated"):
        set_status(m, "cleaned")
    console.print(f"[green]Processed[/] → {blend_out}")


@main.command("export")
@click.argument("asset_id")
def cmd_export(asset_id: str) -> None:
    """Export runtime GLB from .blend into assets/models/."""
    m = _load(asset_id)
    if not m.blend_path.exists():
        raise click.ClickException(f"Missing blend: {m.blend_path}")
    out = models_export_path(m.id, m.category)
    out.parent.mkdir(parents=True, exist_ok=True)
    try:
        run_blender_script("export.py", str(m.blend_path), str(out), m.id)
    except Exception as e:
        raise click.ClickException(str(e)) from e
    console.print(f"[green]Exported[/] → {out}")


@main.command("validate")
@click.argument("asset_id", required=False)
@click.option("--all", "all_assets", is_flag=True, help="Validate every exported GLB")
def cmd_validate(asset_id: str | None, all_assets: bool) -> None:
    """Run glTF-Validator + triangle budgets."""
    if all_assets or asset_id is None:
        reports = validate_all()
        if not reports:
            console.print("No exported assets to validate.")
            return
        failed = 0
        for r in reports:
            mark = "[green]OK[/]" if r.ok else "[red]FAIL[/]"
            console.print(f"{mark} {r.asset_id} tris={r.tris}")
            for msg in r.messages:
                console.print(f"    {msg}")
            if not r.ok:
                failed += 1
        if failed:
            raise click.ClickException(f"{failed} asset(s) failed validation")
        return

    m = _load(asset_id)
    r = validate_asset(m)
    for msg in r.messages:
        console.print(msg)
    if not r.ok:
        raise click.ClickException("validation failed")
    console.print("[green]OK[/]")


@main.command("batch")
@click.option("--status", "status_filter", default=None, help="Only assets with this status")
@click.option("--step", type=click.Choice(["process", "export", "validate"]), default="process")
def cmd_batch(status_filter: str | None, step: str) -> None:
    """Run the next pipeline step on matching assets."""
    assets = iter_manifests()
    if status_filter:
        assets = [m for m in assets if m.status == status_filter]
    console.print(f"Batch {step} on {len(assets)} asset(s)")
    ctx = click.get_current_context()
    for m in assets:
        console.print(f"— {m.id}")
        if step == "process":
            ctx.invoke(cmd_process, asset_id=m.id, force=False)
        elif step == "export":
            ctx.invoke(cmd_export, asset_id=m.id)
        elif step == "validate":
            ctx.invoke(cmd_validate, asset_id=m.id, all_assets=False)


@main.command("report")
@click.option("--out", type=click.Path(path_type=Path), default=None)
def cmd_report(out: Path | None) -> None:
    """HTML contact sheet with budgets."""
    path = build_html_report(out)
    console.print(f"[green]Report[/] → {path}")


@main.command("make-primitive")
@click.argument("asset_id")
@click.option("--category", "-c", default="prop")
@click.option("--size", default=1.0, help="Cube half-extent / size in metres")
def cmd_make_primitive(asset_id: str, category: str, size: float) -> None:
    """Create a minimal textured cube GLB (no Blender/ComfyUI) for pipeline smoke tests."""
    from nu_content.make_primitive import write_textured_cube_glb

    errs = validate_id(asset_id, category)
    if errs:
        raise click.ClickException("; ".join(errs))
    try:
        m = AssetManifest.load_id(asset_id)
    except FileNotFoundError:
        m = create_manifest(asset_id, category, height_m=size)

    raw = m.root / "raw" / "selected.glb"
    raw.parent.mkdir(parents=True, exist_ok=True)
    write_textured_cube_glb(raw, size=size)
    m.generate["backend"] = "primitive"
    m.generate["selected"] = "raw/selected.glb"
    set_status(m, "generated")

    # Also drop a copy under models/ for engine smoke without Blender
    out = models_export_path(m.id, m.category)
    out.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(raw, out)
    console.print(f"[green]Primitive[/] {raw} and {out}")


if __name__ == "__main__":
    main()
