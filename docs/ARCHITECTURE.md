# Architecture

## Goals

- Custom engine for flexibility (combat feel, arena streaming, hot reload).
- Clear split: **engine** (reusable) vs **game** (Nine Under–specific).
- Windows / MSVC first; platform code isolated under `platform/`.

## Module dependency graph

```
game → engine
         ├── core
         ├── math
         ├── platform  → core
         ├── render    → core, platform, math
         ├── assets    → core, render
         ├── scene     → core, math
         ├── physics   → core, math, scene
         ├── audio     → core
         └── ui        → core, render, platform
```

`engine` must never depend on `game`.

## Main loop

Fixed simulation at **60 Hz**, variable render with interpolation:

```
while running:
  pump input / events
  accumulate wall time
  while accumulator >= dt:
    fixedUpdate(dt)   # physics, gameplay
    accumulator -= dt
  alpha = accumulator / dt
  variableUpdate(frameDt)
  render(alpha)
  present
```

## Rendering (v0)

- Vulkan 1.3: dynamic rendering, synchronization2, descriptor indexing (bindless).
- Forward / clustered-forward path with depth prepass.
- One directional light + cascaded shadows (shadows may land after M3).
- PBR metallic-roughness (glTF 2.0).
- Tonemap + exponential fog (Arena I mist / permanent dawn).

Frame graph-lite: passes declare resource reads/writes; barriers via sync2.

## Assets

- Source: `assets/` (Git LFS). Runtime data copied/cooked to `build/<preset>/data/`.
- Formats: glTF 2.0 / GLB, KTX2 where available, Slang → SPIR-V.
- Handles: generational IDs. Hot reload for shaders (and textures when wired) in `dev`.

## Physics

- Jolt Physics.
- Player: `CharacterVirtual` capsule, keyboard/mouse or gamepad.
- Collision layers: static, dynamic, character, trigger (ink pools later).

## Threading (initial)

- Main thread: input, simulation, submit.
- Optional worker pool in `core` for asset decode / cooking (expand later).
- Tracy zones on hot paths.

## Naming & includes

- Public headers: `engine/<module>/include/nu/<module>/...`
- Implementation: `engine/<module>/src/...`
- Include as `#include "nu/core/log.hpp"`.

## Build presets

| Preset | Optimizations | Asserts | Validation | ASan |
|--------|---------------|---------|------------|------|
| debug  | Off           | On      | On         | On   |
| dev    | On            | On      | On         | Off  |
| release| On            | Off     | Off        | Off  |
