# Assets

Source art for Nine Under. Binary formats are tracked with **Git LFS** (see `.gitattributes`).

## Layout (suggested)

```
assets/
  models/       # Blender .blend sources + exported .gltf / .glb
  textures/     # PNG / KTX2
  audio/        # wav / ogg
  levels/       # graybox / arena data
```

Runtime staging copies this tree next to `NineUnder.exe` as `data/`.

Until real art lands, the game uses procedural graybox meshes for Arena I.
