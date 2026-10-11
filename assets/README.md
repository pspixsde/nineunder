# Assets

Source art for Nine Under. Binary formats are tracked with **Git LFS**
(see `.gitattributes`).

- **Beginner how-to:** [`docs/HOWTO_3D_PIPELINE.md`](../docs/HOWTO_3D_PIPELINE.md)
- **Technical reference:** [`docs/ASSET_PIPELINE.md`](../docs/ASSET_PIPELINE.md)
- **One-time setup:** `.\scripts\setup_content_pipeline.ps1`

## Layout

```
assets/
  source/<category>/<id>/   # working tree: asset.toml, concept/, raw/, .blend
  models/<category>/<id>.glb
  textures/                 # shared / tileable maps (PNG / KTX2)
  audio/                    # wav / ogg
  levels/                   # arena .blend + exported .glb
```

Categories: `character`, `enemy`, `prop`, `environment`.

Runtime staging copies this tree next to `NineUnder.exe` as `data/`.

Until real art lands, the game uses procedural graybox meshes for Arena I and
loads optional authored props from `models/` when present.

## Quick start

```powershell
cd tools/content
uv sync
uv run nu-content new prop_example --category prop
uv run nu-content import prop_example path\to\model.glb
uv run nu-content process prop_example   # needs Blender
uv run nu-content export prop_example
uv run nu-content validate prop_example
```
