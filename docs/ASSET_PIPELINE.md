# Asset Pipeline — Nine Under

Local, free pipeline for stylized mid-poly 3D content on an **8 GB VRAM** GPU
(RTX 4060 Laptop class). Concept art and draft meshes come from ComfyUI;
cleanup, polish, and export happen in Blender; the engine consumes validated
glTF 2.0 / GLB under `assets/models/`.

**Beginner how-to (start here):** [`HOWTO_3D_PIPELINE.md`](HOWTO_3D_PIPELINE.md)  
**Cursor MCP (Comfy + Blender):** [`MCP_SETUP.md`](MCP_SETUP.md)

Standard saved Comfy Desktop workflows: **Nine Under - Text to Image v1**,
**Nine Under - Image to 3D v1** (Pixal3D default is fine).

Related: [`assets/README.md`](../assets/README.md),
[`tools/content/`](../tools/content/), [`docs/ARCHITECTURE.md`](ARCHITECTURE.md).

---

## Prerequisites (host machine)

| Tool | Notes |
|------|-------|
| **Blender** LTS **4.2** (pin this version) | Headless scripts under `tools/content/blender/`. Set `NU_BLENDER` to the `blender.exe` path if not on `PATH`. |
| **uv** | Python package manager for `tools/content`. `winget install astral-sh.uv` |
| **ComfyUI** (portable) | Install **outside** the repo. Set `NU_COMFYUI_URL` (default `http://127.0.0.1:8188`). |
| **glTF-Validator** | [Khronos release](https://github.com/KhronosGroup/glTF-Validator/releases). Optional but used by `nu-content validate`. |
| **StableProjectorz** (optional) | Free SD projection painting for texture polish on 8 GB. |
| **Material Maker** (optional) | Tileable environment materials. |
| **Modly** (optional) | Quick exploration GUI; import results with `nu-content import`. |

```powershell
cd tools/content
uv sync
uv run nu-content --help
```

---

## Generator licences (Phase 0 check)

Record the backend and version in every `asset.toml` so assets can be audited.

| Model | Licence | 8 GB fit | Notes |
|-------|---------|----------|-------|
| **TripoSG** | MIT | ~8 GB | **Default** shape backend |
| **TripoSR** | MIT | ~6 GB | Fast blockouts; rough topology |
| **TRELLIS / TRELLIS 2** | MIT | ~24 GB | Too large locally; use HF Space demos → `nu-content import` for hero pieces |
| **Hunyuan3D-2mini** | Community | with offload | **Not granted in EU, UK, or South Korea.** If you are in those regions, do not use Hunyuan for any shippable asset. Prefer TripoSG. |
| **SDXL** | OpenRAIL++ | yes | Concept images |
| **Flux.1 schnell** | Apache 2.0 | GGUF quant | Alternative concepts |

Gaussian splat outputs are **not** game meshes — always convert to mesh / GLB.

---

## Folder layout

```
assets/
  source/<category>/<id>/     # working tree (Git LFS for binaries)
    asset.toml                # manifest (source of truth for status)
    concept/*.png
    raw/selected.glb          # chosen generation candidate
    raw/cand_*.glb            # optional candidates
    <id>.blend                # Blender source after cleaned
  models/<category>/<id>.glb  # exported runtime assets (staged to data/)
  levels/<arena>.blend|.glb
  textures/                   # shared / tileable maps
```

Categories map to budget keys: `character`, `enemy`, `prop`, `environment`.

---

## Manifest (`asset.toml`)

```toml
id = "chr_undead_hunter"
category = "character.humanoid"
status = "concept"            # concept | generated | cleaned | polished | approved
height_m = 1.85

[concept]
workflow = "concept_character_v1"
seed = 1234
prompt = "..."

[generate]
backend = "triposg"
backend_version = "pinned"
seed = 42
selected = "raw/selected.glb"

[export]
lods = [1.0, 0.5, 0.25]
collision = "capsule"         # none | capsule | box | mesh | from_col_nodes
rig = "nu_humanoid"           # empty for static props
```

### Status rules

1. Stages are idempotent and driven by `status`.
2. Once `cleaned`, the `.blend` is the source of truth.
3. Regeneration never overwrites a polished `.blend` unless `--force`, which
   writes a new revision (`<id>_revN.blend`) instead of clobbering.

---

## Naming and conventions

| Prefix | Use |
|--------|-----|
| `chr_` | Playable characters |
| `enm_` | Enemies |
| `prop_` | Props / set dressing |
| `env_<arena>_` | Modular environment pieces |
| `COL_` | Collision-only mesh nodes (not rendered) |

- Units: **metres**, **Y-up**, origin at **bottom centre**.
- Characters: A-pose when generating; retarget to `nu_humanoid` after retopo.
- LOD meshes: suffix `_LOD0` (hero), `_LOD1`, `_LOD2` in the Blender collection.

### Budgets (stylized mid-poly)

Configured in [`tools/content/config/budgets.toml`](../tools/content/config/budgets.toml):

| Category | Tris | Texture |
|----------|------|---------|
| character | 15–30k | 2K |
| enemy | 8–25k | 1–2K |
| prop | 0.5–5k | 512–1K |
| environment (modular) | 0.2–3k | 512–1K, 1 m grid / 4 m wall units |

---

## Workflow commands

```powershell
# Create a new asset folder + manifest
uv run nu-content new prop_ink_urn --category prop

# Import Modly / HF Space / manual GLB into raw/
uv run nu-content import prop_ink_urn path\to\model.glb

# Concept images via ComfyUI (requires server)
uv run nu-content concept prop_ink_urn

# Generate mesh candidates (default: TripoSG)
uv run nu-content generate prop_ink_urn --backend triposg --candidates 3

# Blender clean → .blend
uv run nu-content process prop_ink_urn

# Export runtime GLB + validate budgets
uv run nu-content export prop_ink_urn
uv run nu-content validate prop_ink_urn

# Batch / review
uv run nu-content batch --status generated
uv run nu-content report
uv run nu-content status
```

Manual polish: open `assets/source/.../<id>.blend` in Blender, then
`nu-content export` again. Set `status = "polished"` (or `approved`) when done.

---

## Map design guidance

AI is weak at precise modular kits. Workflow:

1. **Modular kit** — model by hand in Blender on a 1 m grid.
2. **Organic props / hero pieces** — AI generate → Blender clean / polish.
3. **Tileable surfaces** — Material Maker → `assets/textures/`.
4. **Levels** — Blender as level editor. Empties with custom property
   `nu_asset = "prop_ink_urn"` export as glTF `extras` and spawn as prefabs.

---

## Engine consumption

Runtime staging copies `assets/` → `data/` next to the exe.

- Static textured models: `nu::ModelAsset` / `loadGltf` (tangents + PBR maps).
- Collision: `COL_*` nodes → Jolt shapes (box / capsule / mesh).
- Cooker: `nu_asset_cooker <glb> [--validate] [--cook-out <dir>]` (KTX2 + meshopt).
- Dev builds: hot-reload watches `data/models`.

Skinning / animation: see [docs/CHARACTERS.md](CHARACTERS.md).

---

## Style (Dread Pit)

Muted, dark fantasy; sickly organic textures (rot, damp flesh, brown-white mass).
Prompt templates and negative prompts live in
[`tools/content/config/style.toml`](../tools/content/config/style.toml).
Train a project style LoRA later from approved concepts.
