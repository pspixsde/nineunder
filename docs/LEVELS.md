# Levels — Blender as level editor

Arena layout is authored in Blender and exported to glTF. Graybox code in
`game/src/arena_greybox.cpp` remains the fallback until a level GLB is present.

## Conventions

| Item | Rule |
|------|------|
| File | `assets/levels/arena_01.blend` → export `assets/levels/arena_01.glb` |
| Units | metres, Y-up on export |
| Grid | 1 m; wall modules 4 m |
| Ground | mesh named `ENV_ground` (static collision from mesh or `COL_*`) |
| Collision | empties / meshes named `COL_box`, `COL_capsule`, `COL_mesh` |
| Prefab spawns | empty named `SPAWN_<asset_id>` e.g. `SPAWN_prop_ink_urn` |

Custom properties (exported as glTF `extras`):

- `nu_asset` on a mesh or empty → spawn that runtime model ID at the node transform.
- `nu_collision` = `box` \| `capsule` \| `mesh` on `COL_*` nodes.

## Authoring loop

1. Build modular kit pieces (`env_arena01_*`) by hand on the grid.
2. Place kit + AI organic props in the arena `.blend`.
3. Add `SPAWN_` empties for repeated prefabs.
4. Export:

```powershell
# From tools/content (with Blender on PATH / NU_BLENDER set)
blender --background assets/levels/arena_01.blend --python tools/content/blender/export.py -- assets/levels/arena_01.blend assets/levels/arena_01.glb arena_01
```

Or use `nu-content` once level manifests are added (same export script).

5. Engine loads the level GLB, draws embedded meshes, spawns `SPAWN_*` via
   `ModelAsset` registry, and creates Jolt shapes from `COL_*`.

## Hot reload

In dev builds the game watches `data/models/**/*.glb` and reloads changed
props (see `AssetHotReload`). Level GLB hot-reload can follow the same pattern.
