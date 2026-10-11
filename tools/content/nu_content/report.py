from __future__ import annotations

import html
from datetime import datetime, timezone
from pathlib import Path

from nu_content.budgets import check_tris, estimate_glb_tris
from nu_content.manifest import AssetManifest, iter_manifests
from nu_content.paths import assets_dir
from nu_content.blender_runner import run_blender_script


def ensure_thumbnail(m: AssetManifest) -> Path | None:
    thumb = m.root / "concept" / f"{m.id}_thumb.png"
    if thumb.exists():
        return thumb
    glb = m.export_glb if m.export_glb.exists() else (m.selected_raw if m.selected_raw.exists() else None)
    if glb is None:
        return None
    try:
        run_blender_script("thumbnail.py", str(glb), str(thumb))
    except Exception:
        return None
    return thumb if thumb.exists() else None


def build_html_report(out_path: Path | None = None) -> Path:
    manifests = iter_manifests()
    out_path = out_path or (assets_dir() / "report.html")
    rows: list[str] = []
    for m in manifests:
        tris = estimate_glb_tris(m.export_glb) if m.export_glb.exists() else None
        budget_ok = "—"
        if tris is not None:
            br = check_tris(m.category, tris)
            budget_ok = "PASS" if br.ok else "FAIL"
        thumb = ensure_thumbnail(m)
        thumb_html = (
            f'<img src="{html.escape(thumb.as_uri())}" width="128" height="128"/>'
            if thumb
            else "—"
        )
        rows.append(
            "<tr>"
            f"<td>{thumb_html}</td>"
            f"<td>{html.escape(m.id)}</td>"
            f"<td>{html.escape(m.category)}</td>"
            f"<td>{html.escape(m.status)}</td>"
            f"<td>{tris if tris is not None else '—'}</td>"
            f"<td>{budget_ok}</td>"
            f"<td>{html.escape(str(m.export_glb.relative_to(assets_dir())) if m.export_glb.exists() else 'missing')}</td>"
            "</tr>"
        )

    doc = f"""<!DOCTYPE html>
<html><head><meta charset="utf-8"/><title>Nine Under asset report</title>
<style>
body {{ font-family: Segoe UI, sans-serif; background:#1a1b1e; color:#ddd; margin:2rem; }}
table {{ border-collapse: collapse; width:100%; }}
th, td {{ border:1px solid #333; padding:0.5rem; text-align:left; vertical-align:middle; }}
th {{ background:#2a2b30; }}
tr:nth-child(even) {{ background:#222327; }}
.pass {{ color:#8f8; }} .fail {{ color:#f88; }}
</style></head><body>
<h1>Nine Under — asset report</h1>
<p>Generated {datetime.now(timezone.utc).strftime('%Y-%m-%d %H:%M UTC')} — {len(manifests)} assets</p>
<table>
<thead><tr><th>Thumb</th><th>ID</th><th>Category</th><th>Status</th><th>Tris</th><th>Budget</th><th>Export</th></tr></thead>
<tbody>
{''.join(rows)}
</tbody></table>
</body></html>
"""
    out_path.write_text(doc, encoding="utf-8")
    return out_path
