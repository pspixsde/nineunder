# How to: create 3D models for Nine Under

**Audience:** first-time 3D / first-time ComfyUI.  
**Style:** do everything in the **Comfy Desktop canvas** when possible.  
Technical reference: [`ASSET_PIPELINE.md`](ASSET_PIPELINE.md).

---

## What you are building (one loop)

```text
Text prompt  →  2D concept image     (your "Nine Under v1" graph)
                     ↓
              Image → 3D mesh        (built-in Pixal3D / TRELLIS.2 template)
                     ↓
              Save GLB to disk
                     ↓
         (optional) Blender polish → game
```

All AI steps run **locally** and **free**. No Hugging Face website. No Tripo credits.

| Skip these | Why |
|------------|-----|
| **Tripo: Image to Model** (and other Tripo* / Meshy* API nodes) | Cloud + paid credits |
| Hunyuan3D for shippable assets | Licence not granted in **EU / UK / KR** |

---

## One-time setup (already mostly done)

You already have:

- [x] `setup_content_pipeline.ps1`
- [x] Blender 4.2
- [x] Comfy Desktop **Local** install
- [x] SD 1.5 checkpoint (`v1-5-pruned-emaonly.safetensors`)
- [x] Saved workflow **"Nine Under v1"** (text → image)

Still needed for image → 3D:

- [ ] Download **Pixal3D / TRELLIS.2** model pack (Desktop will prompt, or use Manager → Install Models)
- [ ] Optional: **SDXL** checkpoint for nicer concept images

Shared model folder:

```
C:\Users\emilk\AppData\Local\Comfy-Desktop\ComfyUI-Shared\models\
```

Outputs land under:

```
C:\Users\emilk\AppData\Local\Comfy-Desktop\ComfyUI-Shared\output\
```

If Local Comfy crashes with `0xC0000005` / CUDA errors → update NVIDIA driver to **580+** and reboot (see [Troubleshooting](#troubleshooting)).

---

# Part A — Better 2D concepts (fix your urn graph)

Your wiring was fine. The result (two urns, jagged edges) is a **prompt / seed** problem, not a broken graph.

## A.1 Paste these prompts into "Nine Under v1"

**Positive** (replace the whole box):

```text
single game prop, one cracked stone urn only, centered, full object visible,
dark ink stains on weathered stone, dark fantasy, stylized mid-poly,
muted brown grey green palette, smooth silhouette, plain seamless studio background,
product photo of one object, isolated
```

**Negative** (replace the whole box):

```text
two objects, multiple urns, duplicate, twin, side by side, split view,
photorealistic, neon, watermark, text, logo, cluttered background,
jagged edges, broken silhouette, cut off, cropped, incomplete,
deformed, melted, thin middle, weird proportions
```

## A.2 Sampler settings that help on SD 1.5

| Setting | Try |
|---------|-----|
| Seed | Click randomize; generate **4–8** images; keep the clearest single silhouette |
| Steps | 22–28 |
| CFG | **6.5–7.5** (8+ often overcooks / doubles objects) |
| Sampler | `euler` |
| Scheduler | `normal` (or leave `simple` if that already works) |
| Size | 768×768 is fine for SD 1.5 |

## A.3 Rule: one clear object

Image → 3D needs:

- **One** subject, centered  
- Plain / removed background  
- Full silhouette (not cut off)  
- Prefer “ugly but clear” over “pretty but messy”

If you still get two urns: strengthen negative (`two objects, side by side`) and keep rolling seeds.

## A.4 Optional upgrade — SDXL concepts (prettier, still free)

When you want better images than SD 1.5:

1. In Comfy Desktop: **Manager → Install Models** (or the template download UI).
2. Install an **SDXL** base checkpoint (e.g. `sd_xl_base_1.0.safetensors`) into Shared `models\checkpoints\`.
3. Duplicate "Nine Under v1", change Load Checkpoint to the SDXL file.
4. Set Empty Latent to **1024×1024**.
5. Keep the same “single object” prompt style.

VRAM: close the 3D template while generating concepts if you hit out-of-memory.

## A.5 Save the concept image

After Queue:

1. Open the preview / Save Image output.
2. Note the file under Shared `output\` (prefix `ComfyUI` unless you changed it).
3. Optionally rename / copy it somewhere memorable, e.g. `prop_ink_urn_concept.png`.

You do **not** need the terminal for this part.

---

# Part B — Image → 3D inside ComfyUI (free, local)

Your text→image graph **does not** create 3D by itself. 3D is a **second** graph that takes the PNG and builds a mesh.

Desktop already includes native nodes for this (no GitHub install required):

- TRELLIS.2 / Pixal3D pipeline  
- `RemoveBackground` / BiRefNet  
- `Preview3D` / `SaveGLB` (Save 3D Model)

## B.1 Load the official template

1. In Comfy Desktop, open the **Workflow / Template** library.
2. Search: **`Pixal3D & TRELLIS.2`** (or “Image to Model”).
3. Load it (new tab or replace canvas — keep "Nine Under v1" saved so you can reopen it).

## B.2 Download the model pack (one time)

The template needs files under Shared `models\`:

```
diffusion_models/
  trellis_2_int8_convrot.safetensors
  pixal3d_int8_convrot.safetensors
vae/
  trellis_2_shape_vae_bf16.safetensors
  trellis_2_texture_vae_bf16.safetensors
clip_vision/
  dino_v3_L_naf_fp32.safetensors
background_removal/
  birefnet.safetensors
```

Desktop usually offers **Download** when you open the template or via **Manager → Install Models**.  
This pack is large (many GB). Let it finish. You said waiting is OK — good, because on 8 GB VRAM a run can take several minutes.

Official model list / layout: [ComfyUI TRELLIS.2 tutorial](https://docs.comfy.org/tutorials/3d/trellis2).

## B.3 Run image → 3D

1. In the template’s **Load Image** node, pick your concept PNG (the urn you liked).
2. Set **Boolean (Switch to Trellis2)** to **true** for TRELLIS.2 (usually better quality).  
   Leave false to try Pixal3D instead.
3. Keep background removal **on** (BiRefNet) unless your image is already cut out.
4. On 8 GB VRAM, prefer lower resolution / low-VRAM options in the template if present  
   (e.g. start ~512 structure; raise later if it fits).
5. **Queue** and wait. Close games / other GPU apps.
6. Inspect **Preview 3D**. Confirm it saved a mesh (GLB) under Shared `output\`  
   (often `output\3d\…`).

`SaveGLB` / Save 3D Model is the correct end node for a **local** mesh.  
That is different from the paid **Tripo: Image to Model** node — do not add Tripo.

## B.4 If TRELLIS runs out of VRAM

Try in order:

1. Lower resolution / enable any **low VRAM** toggle in the template.  
2. Generate shape only / skip heavy texture stages if the template allows.  
3. Close Blender / browsers using the GPU; reboot once if the driver is wedged.  
4. Fallback (still free, still in Comfy): **Manager → Install Custom Nodes** →  
   search **`Flowty TripoSR`** / `ComfyUI-Flowty-TripoSR` → install → restart →  
   wire `Load Image` → (optional Remove Background) → TripoSR → SaveGLB.  
   Rough but lighter than TRELLIS.

## B.5 “Automate” both stages on the canvas

You do **not** need the CLI for AI.

Practical automation:

1. Keep two saved workflows: **"Nine Under v1"** (concepts) and **"Nine Under i23d"** (TRELLIS template with your defaults).
2. Run concepts → pick best PNG → open i23d → Load Image → Queue.
3. Later (optional): put both graphs on **one** canvas and connect  
   `VAE Decode → IMAGE` into the TRELLIS image input (bypass Load Image) so one Queue does text→image→3D.  
   Do this only after each half works alone.

---

# Part C — Into Nine Under / Blender (minimal)

AI work stops at the GLB in Shared `output\`. The game still wants files under the repo.

### C.1 Copy the GLB (Explorer — no terminal)

1. Create folders if needed:  
   `assets\source\prop\prop_ink_urn\raw\`  
   `assets\source\prop\prop_ink_urn\concept\`
2. Copy your concept PNG → `concept\`
3. Copy the GLB → `raw\selected.glb`

(Optional but useful: add a small `asset.toml` later via `nu-content new`, or copy from `prop_ruin_block_a` and edit the `id` / prompt.)

### C.2 Clean up in Blender UI

1. Open Blender 4.2.  
2. Import the GLB (`File → Import → glTF 2.0`).  
3. Scale so height ≈ your target metres; origin at bottom centre.  
4. Decimate if triangle count is huge (props: aim **500–5000** tris).  
5. Export GLB to:  
   `assets\models\prop\prop_ink_urn.glb`

You can use the headless helpers (`nu-content process` / `export`) later if you want; they are optional once you are comfortable in Blender.

### C.3 See it in the game

Rebuild / run the game as usual. Staging copies `assets/` → `data/`.  
Arena I currently proves loading with `prop_ruin_block_a` — swap that file temporarily or add a spawn (see [`LEVELS.md`](LEVELS.md)).

---

# Quick checklist — your next session

1. Fix prompts in **"Nine Under v1"** (section A.1) → Queue until you get **one** clean urn.  
2. Workflow templates → **Pixal3D & TRELLIS.2** → download models if asked.  
3. Load concept image → Switch to Trellis2 = true → Queue → wait → GLB in Shared `output`.  
4. Copy GLB into `assets\source\prop\…\raw\selected.glb` → polish in Blender → export to `assets\models\prop\`.

---

# Licence (read once)

- **TRELLIS.2 / Pixal3D / TripoSR / TripoSG**: MIT — OK.  
- **Hunyuan3D 2 community**: **not** for EU / UK / KR shippable assets.  
- Record which tool made each asset when you add `asset.toml`.

---

# Troubleshooting

| Symptom | Fix |
|---------|-----|
| Two objects / jagged concept | Stronger “single object” prompts; CFG 6.5–7.5; more seeds |
| KSampler missing positive/negative | Re-wire CONDITIONING outputs into KSampler (wires must snap) |
| VAE Decode missing VAE | Connect Load Checkpoint **VAE** → VAE Decode |
| Tripo asks for credits | Delete that node; use TRELLIS template instead |
| TRELLIS models missing / red | Download pack into Shared `models\` paths above |
| CUDA OOM on TRELLIS | Lower res / low VRAM; or install Flowty-TripoSR |
| Local crash `0xC0000005` | NVIDIA driver **580+**, reboot, relaunch Local |
| No GLB on disk | Check Preview3D + SaveGLB / Save 3D is in the graph and Queue finished |

### Driver crash detail

Desktop’s PyTorch wants a newer driver than old 56x installs. Update via
[NVIDIA App](https://www.nvidia.com/en-us/software/nvidia-app/) or
[GeForce Drivers](https://www.nvidia.com/Download/index.aspx), reboot, confirm with `nvidia-smi`.

---

# Characters / levels (later)

Same AI loop for organic props. Characters need retopo + rig after mesh exists — [`CHARACTERS.md`](CHARACTERS.md).  
Modular architecture: model by hand on a 1 m grid — [`LEVELS.md`](LEVELS.md).
