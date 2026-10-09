# Agent / contributor guide — Nine Under

## Project intent

Custom C++20 engine + Vulkan 1.3 game. Windows-first. Action-roguelite, third person,
set in the Dread Pit (`LORE.md`).

## Hard rules

1. **Engine never includes game.** `engine/` must not `#include` anything under `game/`.
2. **Lore stays in docs.** Do not hardcode lengthy narrative into source; reference
   arena/character IDs and keep prose in `LORE.md`.
3. **Mechanics changelog.** Player-facing mechanical changes go in `CHANGELOG.md`.
4. **No drive-by refactors.** Match existing style; prefer small, focused diffs.
5. **Do not edit plan files** under `.cursor/plans/` unless the user asks.

## Code style

- C++20, MSVC `/W4 /permissive-`.
- Naming: `CamelCase` types, `camelBack` functions/variables, `m_` or trailing `_`
  for private members (see `.clang-tidy`).
- Prefer `std::` and explicit ownership (`unique_ptr`, handles) over raw owning pointers.
- Headers under `engine/` use the `nu/` include prefix (e.g. `#include "nu/core/log.hpp"`).

## Build expectations

- Configure with CMake presets (`debug` / `dev` / `release`).
- Manifest-mode vcpkg (`vcpkg.json`); do not vendor libraries under `third_party/`
  unless they are unavailable on vcpkg.
- Shaders live in `shaders/` as Slang; SPIR-V is a build artifact.

## Module ownership

| Module | Responsibility |
|--------|----------------|
| `core` | Types, asserts, logging, time, memory, jobs |
| `platform` | Window, input, filesystem (SDL3) |
| `render` | Vulkan device, swapchain, pipelines, materials |
| `assets` | Handles, glTF, textures, shader cache |
| `scene` | ECS (EnTT), transforms |
| `physics` | Jolt wrappers |
| `audio` | miniaudio wrappers |
| `ui` | Dear ImGui (debug) |
| `math` | glm config + helpers |
| `game` | Player, combat, enemies, arenas, run loop |

## When adding features

- Prefer fixed-timestep simulation (60 Hz) with interpolated render.
- Hot-reload shaders/textures in `dev` builds when practical.
- Keep validation layers clean in `debug`/`dev`.
