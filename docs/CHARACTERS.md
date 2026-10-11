# Characters — rigging, animation, and engine skinning

Pipeline overview: [`ASSET_PIPELINE.md`](ASSET_PIPELINE.md).

## `nu_humanoid` skeleton standard

All humanoid playable characters and humanoid enemies share one skeleton so
Mixamo / AccuRIG animations retarget once.

| Convention | Value |
|------------|--------|
| Units | metres, Y-up |
| Bind pose | A-pose |
| Hip height | ~1.0 m for a 1.85 m adult (scale per `height_m` in `asset.toml`) |
| Root | `Root` at world origin (bottom centre of character) |
| Pelvis | `Hips` |
| Naming | Mixamo-style (`mixamorig:Hips`, …) **or** stripped names without prefix — pick one per project and stick to it. Default export: **strip `mixamorig:`** in Blender before export. |

Recommended bone set (minimum):

`Root`, `Hips`, `Spine`, `Spine1`, `Spine2`, `Neck`, `Head`,
`LeftShoulder`, `LeftArm`, `LeftForeArm`, `LeftHand`,
`RightShoulder`, `RightArm`, `RightForeArm`, `RightHand`,
`LeftUpLeg`, `LeftLeg`, `LeftFoot`, `LeftToeBase`,
`RightUpLeg`, `RightLeg`, `RightFoot`, `RightToeBase`.

Store the reference armature as `assets/source/character/nu_humanoid/nu_humanoid.blend`
(create once in Blender / Rigify metarig → generate → rename).

## Workflow

1. **Generate** concept (A-pose sheet) → image-to-3D (`triposg`).
2. **Process** in Blender (`nu-content process`) → remesh / scale.
3. **Retopo** for deformation: QuadriFlow or Instant Meshes → ~15–30k tris.
4. **UV + texture polish** (bake AI colour, StableProjectorz / texture paint).
5. **Auto-rig**
   - Humanoid: [Mixamo](https://www.mixamo.com/) or AccuRIG → download FBX with skin.
   - Or Rigify in Blender (more control).
6. **Retarget** onto `nu_humanoid` (Blender: bone map / Rokoko / Mixamo addon).
7. **Export** GLB with skins + animations (`nu-content export`).
8. Non-humanoid enemies: Rigify by hand; do not force `nu_humanoid`.

## Engine skinning (evaluation)

**Goal:** GPU skinning in the mesh vertex shader + CPU animation sampling.

### Options evaluated

| Option | Pros | Cons | Verdict |
|--------|------|------|---------|
| **ozz-animation** | Battle-tested, small, sampling + blending, glTF→ozz offline tools | Not always in vcpkg; extra cook step to ozz runtime format | **Preferred** when available on vcpkg or vendored under `third_party/` (MIT) |
| **In-house sampler** | No new dep; read glTF animation channels directly | Must implement blending, IK later | Acceptable MVP |
| **Assimp runtime** | Familiar | Heavy; duplicates fastgltf | Reject |

### Planned engine work (after static `ModelAsset`)

1. Extend `Vertex` (or `SkinnedVertex`) with `JOINTS_0` (u16x4) + `WEIGHTS_0` (vec4).
2. Load skins + inverse bind matrices in `loadGltf`.
3. Upload joint matrix palette UBO/SSBO per draw.
4. Vertex shader: `skinnedPos = sum(w_i * joint_i * invBind_i * pos)`.
5. Animation clip player (ozz or in-house) updates palette at 60 Hz sim.

Until that lands, characters can ship as **static A-pose props** for scale/silhouette checks.

## Animation library

Prefer Mixamo locomotion packs (walk, run, idle, attack) retargeted to `nu_humanoid`.
Keep clip names stable: `idle`, `walk`, `run`, `jump`, `melee_light`, `hit`, `death`.
