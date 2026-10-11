# nu-content

Python CLI for the Nine Under 3D asset pipeline.

```powershell
uv sync
uv run nu-content --help
```

Environment:

| Variable | Default | Meaning |
|----------|---------|---------|
| `NU_BLENDER` | `blender` on PATH | Path to Blender LTS 4.2 |
| `NU_COMFYUI_URL` | `http://127.0.0.1:8188` | ComfyUI HTTP API |
| `NU_GLTF_VALIDATOR` | `gltf_validator` on PATH | Khronos validator CLI |
| `NU_REPO_ROOT` | auto (two levels up) | Nine Under repo root |

Beginner guide: [`docs/HOWTO_3D_PIPELINE.md`](../../docs/HOWTO_3D_PIPELINE.md)  
Reference: [`docs/ASSET_PIPELINE.md`](../../docs/ASSET_PIPELINE.md)  
Setup: `..\..\scripts\setup_content_pipeline.ps1`
