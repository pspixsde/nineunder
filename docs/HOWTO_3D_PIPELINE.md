# How to: create 3D models for Nine Under

**Audience:** first-time 3D / ComfyUI.  
**Style:** Comfy Desktop canvas first; Cursor MCP optional for prompts + Blender help.  
Technical reference: `[ASSET_PIPELINE.md](ASSET_PIPELINE.md)`.  
MCP setup: `[MCP_SETUP.md](MCP_SETUP.md)`.

---

## Standardized pipeline (props first)

Characters are harder (retopo, rigging). **Master props before characters.**

```text
Prompt
  →  "Nine Under - Text to Image v1"     (saved Comfy workflow)
  →  concept PNG (one clear object)
  →  "Nine Under - Image to 3D v1"       (Pixal3D default is fine)
  →  GLB in Comfy Shared\output
  →  copy into assets/source/.../raw/selected.glb
  →  Blender polish (scale, origin, selective decimate, …)
  →  export assets/models/<category>/<id>.glb
  →  game load
```


| Saved Comfy workflow              | Role              | Notes                                                                     |
| --------------------------------- | ----------------- | ------------------------------------------------------------------------- |
| **Nine Under - Text to Image v1** | Text → 2D concept | Your improved pre-built template — keep this as the standard              |
| **Nine Under - Image to 3D v1**   | Image → mesh GLB  | Pixal3D default is OK; TRELLIS.2 switch is optional quality/VRAM tradeoff |


All of this is **local and free** when run in Comfy Desktop. Skip paid **Tripo*** / **Meshy*** API nodes. Skip Hunyuan for shippable assets in **EU / UK / KR**.

### Target budgets (game-ready)

From `tools/content/config/budgets.toml`:


| Category    | Triangles     | Texture max                       |
| ----------- | ------------- | --------------------------------- |
| **prop**    | **500–5,000** | **1024** (512 OK for small props) |
| environment | 200–3,000     | 1024                              |
| enemy       | 8k–25k        | 2048                              |
| character   | 15k–30k       | 2048                              |


AI meshes often arrive at **100k–700k+ tris**. That is normal. The Blender section exists to get you into budget **without** destroying silhouette (use vertex-group protected Decimate).

---



## Cursor MCP (optional but recommended)

Configured in your user `~\.cursor\mcp.json` (see `[MCP_SETUP.md](MCP_SETUP.md)`):

- **comfy-cloud** — cloud generation (needs `COMFY_API_KEY`)
- **comfy-mcp** — local Desktop on `:8188`
- **blender** — Blender control (`mcp-for-blender`)

After restarting Cursor, you can ask the agent for prompt help, parameter suggestions, and Blender steps. Keep Local Comfy and/or Blender MCP server running when you want those tools.

---



## One-time setup

- [x] Content pipeline / Blender / Comfy Desktop Local
- [x] Saved workflows: **Text to Image v1**, **Image to 3D v1**
- [x] MCP: `COMFY_API_KEY` set + Cursor restarted (`[MCP_SETUP.md](MCP_SETUP.md)`)
- [x] Blender: enable MCP addon + **Start MCP Server** when using agent control

Shared folders:

```
C:\Users\emilk\AppData\Local\Comfy-Desktop\ComfyUI-Shared\
  models\
  output\          ← Save Image / SaveGLB land here
```

---



# Part A — Text → image

1. Open Comfy Desktop → load **Nine Under - Text to Image v1**.
2. Edit the positive prompt for **one** object (no table, floor, or scene unless you want them in the mesh).
3. Queue a few seeds; keep the clearest silhouette / plain background.
4. Note the PNG under Shared `output\`.

Prompt reminders (append as needed):

```text
single game prop, one object only, centered, full object visible,
plain seamless studio background, isolated, clean silhouette
```

Negative extras:

```text
two objects, multiple, duplicate, side by side, table, floor, room,
environment, watermark, text, logo, cut off, cropped
```

---



# Part B — Image → 3D

1. Load **Nine Under - Image to 3D v1**.
2. **Load Image** ← your concept PNG.
3. Leave **Pixal3D** as default if it already looks good (confirmed OK for the urn).
  Optionally switch to TRELLIS.2 later for comparison (heavier on 8 GB).
4. Queue → wait → confirm **Preview 3D** and a GLB under Shared `output\` (often `output\3d\…`).

Keep the two workflows **separate** for day-to-day use (retry concepts without paying a full 3D run). Merging onto one canvas is optional later.

---



# Part C — Into the project + Blender polish



## C.1 Copy files (Explorer)

Pick an asset id (`prop_` + snake_case), e.g. `prop_ink_urn`.

1. Create:
  ```
   assets\source\prop\prop_ink_urn\concept\
   assets\source\prop\prop_ink_urn\raw\
  ```
2. Copy concept PNG → `concept\`
3. Copy GLB → `raw\selected.glb`

Optional: add `asset.toml` (copy from `prop_ink_urn` or `nu-content new …`) so status / prompt / height are recorded.

## C.2 Blender — import

1. Open **Blender 4.2**.
2. **File → New → General** (or empty scene: delete the default cube).
3. **File → Import → glTF 2.0 (.glb/.gltf)** → select `raw\selected.glb`.
4. Scene units: **Metric**, Length **Metres** (Scene Properties → Units).

If using MCP: start the Blender MCP server (N-panel) so the Cursor agent can help with later steps.

## C.3 Scale to target height & origin at bottom centre



### A. Scale to your target height

1. Select the imported model in the 3D Viewport (left-click — orange outline).
2. Press **N** to open the side panel → **Item** tab.
3. Look at **Dimensions** (X, Y, Z). For upright props, **Z** is height in metres.
4. If wrong size:
  - Press **S** (Scale), move the mouse, left-click to confirm, **or**
  - Type an exact scale factor after **S** (e.g. `S` then `0.5` `Enter`), **or**
  - Edit the Z dimension in the Item panel (Blender scales uniformly if you lock scale).
5. Target examples: small prop `0.3–0.8`, urn `~0.9`, humanoid later `~1.85`.

Tip: after scaling, **Ctrl + A → Apply → Scale** so scale reads `1,1,1` and dimensions stay correct.

### B. Move the origin to the bottom centre

Imported models often have the origin in the middle. Game code expects **bottom centre**.

1. Press **Tab** to enter **Edit Mode** (vertices/edges/faces visible).
2. Press **3** (top number row) for **Face Select**.
3. Select the bottom face(s) of the model (the underside that should sit on the ground).
  If there is no clean bottom face: select the lowest ring of vertices (**1** = Vertex Select) instead.
4. Press **Shift + S** → **Cursor to Selected**.
5. Press **Tab** → back to **Object Mode**.
6. Right-click the object → **Set Origin → Origin to 3D Cursor**.

Optional tidy: **Object → Set Origin → Origin to Geometry** only if you then re-do bottom-centre — prefer the cursor method above.

### C. Snap object to world origin (optional but nice)

1. Object Mode, select model.
2. **Alt + G** (clear location) so the origin sits at the world origin — bottom of the prop at `(0,0,0)`.



## C.4 Triangle count & why naive Decimate hurts

Example from the urn: **~700k → 3500 tris** with a single global Decimate often causes **holes**, **jagged lids**, and **triangle soup** on thin features.

**Rule:** protect detailed / thin parts with **vertex groups**; decimate bulky smooth volumes harder.

### Turn on statistics

Bottom-right status bar → right-click empty area → enable **Scene Statistics**.  
You should see verts / edges / faces / tris for the selection.

### Create vertex groups (protect detail)

1. Select the object → **Tab** (Edit Mode).
2. Select faces/verts that must stay sharp (lid rim, engraved cracks, thin handles, silhouette edges).
  - **C** circle select, **Ctrl + click** grow, **Alt + click** edge loops when useful.
3. In the Object Data properties (green triangle icon) → **Vertex Groups**.
4. Click **+** to add a group, name it e.g. `KEEP_DETAIL`.
5. Click **Assign** (weight `1.0`).
6. Optionally create `DECIMATE_HEAVY` for thick body faces you are happy to melt.



### Decimate with a vertex group

1. **Tab** → Object Mode. Select the object.
2. Properties → **wrench** (Modifiers) → **Add Modifier → Decimate**.
3. Set **Ratio** (start gently: `0.1`–`0.05` from 700k is still huge; iterate).
4. Enable **Vertex Group** on the modifier:
  - Choose `KEEP_DETAIL`.
  - Enable **Invert** if the group means “protect these” (Decimate’s group means “affect these” — so **Invert** = don’t melt the keep group). Confirm in your Blender build: with Invert on, protected verts should stay denser while the body thins.
5. Watch **Face Count** on the modifier and Scene Statistics.
6. Aim props toward **500–5,000** tris. If quality collapses before budget, stop — raise the ratio (less reduction) or paint a larger keep group and try again.
7. When happy: modifier dropdown → **Apply** (or hover modifier → **Ctrl + A**).

If one pass still ruins the lid: undo, grow `KEEP_DETAIL`, or decimate in **two passes** (mild global → then stronger on body-only group).

### Alternative cleanup (when Decimate is not enough)

- **Remesh** (Voxel) then Decimate — can close holes but softens detail.  
- Manual **Dissolve** / **Merge by Distance** in Edit Mode for junk verts.  
- For hero props: retopology (longer; more important for characters later).



## C.5 Materials / UVs (light pass)

1. Check the material slots survived import (Shading workspace).
2. If UVs look broken after heavy remesh: **UV Editing** → select all → **Unwrap** / **Smart UV Project**.
3. Keep textures ≤ budget (**1024** for props unless a hero piece needs 2K).



## C.6 Export for the game

1. Object Mode. Select the prop (and any LODs you created).
2. **File → Export → glTF 2.0 (.glb)**.
3. Settings to prefer:
  - Format: **glTF Binary (.glb)**
  - Transform: **+Y Up**
  - Geometry: apply modifiers (already applied Decimate is fine)
  - Include: meshes + materials; exclude lights/cameras unless needed
4. Save to:
  ```
   assets\models\prop\prop_ink_urn.glb
  ```



## C.7 See it in the game

Rebuild / run Nine Under. Staging copies `assets/` → `data/`.  
Arena I loads two `prop_ink_urn` instances. Add more spawns via code or level empties ([`LEVELS.md`](LEVELS.md)).

---



## C.8 Blender polish — to be continued

The steps above cover the **minimum** game-ready path:

1. Import
2. Scale + bottom-centre origin
3. Selective Decimate with vertex groups
4. Export GLB

Real production modeling will need more (to explore and document later):

- Weighted normals / shade auto-smooth  
- Better UV packing and texture baking from high → low  
- LOD0 / LOD1 / LOD2 meshes  
- Collision helpers (`COL_box` / `COL_capsule`)  
- Texture cleanup (Material Maker, StableProjectorz, hand paint)  
- Character-specific: retopo, `nu_humanoid` rig, skin weights

When you learn a reliable trick, add it under this heading so the HOWTO stays the single source of truth.

---



# Quick checklist

1. **Nine Under - Text to Image v1** → good single-object PNG.
2. **Nine Under - Image to 3D v1** (Pixal3D OK) → GLB in Shared `output`.
3. Copy → `assets\source\prop\<id>\raw\selected.glb` (+ concept PNG).
4. Blender: scale, origin bottom centre, **vertex-group Decimate** to budget.
5. Export → `assets\models\prop\<id>.glb` → run game.

---



# Licence (read once)

- **Pixal3D / TRELLIS.2 / TripoSR / TripoSG**: MIT — OK.  
- **Hunyuan3D 2 community**: **not** for EU / UK / KR shippable assets.  
- Record backend / notes in `asset.toml` when you add one.

---



# Troubleshooting


| Symptom                          | Fix                                                                                 |
| -------------------------------- | ----------------------------------------------------------------------------------- |
| Concept is a whole scene         | Prompt: one object, no table/floor; stronger negatives                              |
| Tripo asks for credits           | Remove node; use **Image to 3D v1**                                                 |
| 700k tris, Decimate destroys lid | Vertex group `KEEP_DETAIL` + Invert; milder ratio; multi-pass                       |
| Origin floats above ground       | Origin to bottom face via 3D cursor                                                 |
| MCP tools missing                | `[MCP_SETUP.md](MCP_SETUP.md)` — restart Cursor; start Local Comfy / Blender server |
| CUDA OOM on 3D                   | Stick with Pixal3D; close other GPU apps; lower template res                        |
| Local crash `0xC0000005`         | NVIDIA driver **580+**, reboot                                                      |


---



# Characters / levels (later)

Same AI loop for organic props. Characters: `[CHARACTERS.md](CHARACTERS.md)`.  
Modular architecture on a 1 m grid: `[LEVELS.md](LEVELS.md)`.