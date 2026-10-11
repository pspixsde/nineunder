# Development State — Nine Under

High-level snapshot of where the project is. Update this when a version ships
or when the tech / content footprint changes in a meaningful way.

| Field | Value |
|-------|--------|
| **Game version** | `v0.3.0` |
| **Codename / slice** | Stage 1 combat depth (two-way Imp fight) |
| **Last updated** | 2026-10-11 |
| **Primary platform** | Windows (MSVC / x64) |
| **Engine** | Custom C++20 + Vulkan 1.3 (`nu_engine`) |
| **Genre / camera** | Action-roguelite, third person |
| **Setting** | The Dread Pit (see `LORE.md`) |

Related docs: [`README.md`](README.md) (build), [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md),
[`CHANGELOG.md`](CHANGELOG.md), [`LORE.md`](LORE.md), [`FUTURE_CHANGES.md`](FUTURE_CHANGES.md).

---

## One-line status

**Two-way combat sandbox:** Imp roams/aggros and spits; player fires + melee (Q),
HP/death/pause, ability HUD. Still graybox; Stage 1 exit not fully met.

---

## What works today (v0.3.0)

### Engine / tooling

- CMake presets (`debug` / `dev` / `release`) + vcpkg manifest deps.
- Module split: `engine/` (game-agnostic) vs `game/` (Nine Under).
- SDL3 window/input, fixed 60 Hz sim + variable render.
- Vulkan 1.3 device/swapchain; GPU idle before resource teardown; present sync per image.
- Jolt character controller (no jump); `setPosition` reset.
- ImGui menus + gameplay HUD; miniaudio one-shots.
- Content pipeline CLI + textured `ModelAsset` / `prop_ink_urn` in Arena I.

### Gameplay / content

- Arena I graybox with urn props.
- Main / pause / death menus; FPS counter; crosshair; HP bar; ammo pips; ability slots.
- Player: 500 HP, LMB fireball (60 dmg, clip 6 + reload, trail VFX), **Q** melee (100 + knockback, 3s CD).
- Imp: roam → aggro (exclamation), mid-range hold, parabolic spit (40 dmg); distant spawn.
- No multi-enemy AI packs, ult/abilities 2–3, loot, or multi-arena flow yet.

### Lore / design docs

- World notes in `LORE.md`; character fantasy docs exist but kit is still stub-level.

---

## Setup (developer machine)

Prerequisites and bootstrap are documented in `README.md`. Short path:

1. VS 2022 Build Tools (MSVC + Windows SDK), CMake ≥ 3.28, Ninja, Vulkan SDK, vcpkg, Git LFS.
2. Open an **x64 Native Tools / VS Developer** shell (MSVC `INCLUDE`/`LIB` required).
3. `$env:VCPKG_ROOT = "C:\dev\vcpkg"` (or permanent env).
4. `cmake --preset dev` → `cmake --build --preset dev` → `.\build\dev\game\NineUnder.exe`.

---

## Repo layout (mental model)

| Path | Role |
|------|------|
| `engine/` | Reusable engine modules (`core`, `platform`, `render`, …) |
| `game/` | `NineUnder.exe` — arenas, run rules, characters |
| `shaders/` | Slang + SPIR-V |
| `assets/` | Source art (Git LFS) |
| `tools/` | Cookers / helpers |
| `tests/` | Engine unit tests |
| `docs/` | Architecture and conventions |
| `vcpkg-overlays/` | Local port patches (e.g. ImGui + volk) |

Hard rule: **engine never includes game.**

---

## Known gaps / limitations (honest)

- Arena I still graybox; Imp/player are capsules.
- Ability slots 2–3 and ultimate are UI placeholders only.
- No shadows / rich VFX / animation / skinning in-game yet.
- Hot-reload and full asset cooking remain incomplete.

---

## Version history (state snapshots)

| Version | State summary |
|---------|----------------|
| **v0.1** | Foundation: build/run on Windows, Vulkan graybox Arena I, character move + debug UI. |
| **v0.1.1** | Menu, no jump, Imp stub + death SFX, urn props; removed `prop_ruin_block_a`. |
| **v0.2** | Ranged combat kickoff: player fireball + HP/death, Imp damage feedback, pause menu. |
| **v0.3** | Two-way fight: Imp roam/aggro/spit, melee ability, HUD polish, quit Vulkan fix. |
