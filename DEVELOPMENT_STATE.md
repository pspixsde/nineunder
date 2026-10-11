# Development State — Nine Under

High-level snapshot of where the project is. Update this when a version ships
or when the tech / content footprint changes in a meaningful way.

| Field | Value |
|-------|--------|
| **Game version** | `v0.1.1` |
| **Codename / slice** | Foundation + Arena I sandbox (menu, Imp stub, urns) |
| **Last updated** | 2026-10-11 |
| **Primary platform** | Windows (MSVC / x64) |
| **Engine** | Custom C++20 + Vulkan 1.3 (`nu_engine`) |
| **Genre / camera** | Action-roguelite, third person |
| **Setting** | The Dread Pit (see `LORE.md`) |

Related docs: [`README.md`](README.md) (build), [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md),
[`CHANGELOG.md`](CHANGELOG.md), [`LORE.md`](LORE.md), [`FUTURE_CHANGES.md`](FUTURE_CHANGES.md).

---

## One-line status

**Runnable sandbox:** main menu → Arena I graybox with walk (no jump), two urn
props, immobile Imp + HP bar + death SFX test key. Still pre-combat Stage 0.

---

## What works today (v0.1.1)

### Engine / tooling

- CMake presets (`debug` / `dev` / `release`) + vcpkg manifest deps.
- Module split: `engine/` (game-agnostic) vs `game/` (Nine Under).
- SDL3 window/input (Vulkan feature enabled), fixed 60 Hz sim + variable render.
- Vulkan 1.3 device/swapchain, dynamic rendering, mesh forward pass, fog/lighting hooks.
- Shader pipeline: Slang sources → SPIR-V (`shaders/`, compile helper).
- Jolt: physics world + `CharacterVirtual` capsule controller (no jump).
- ImGui debug UI + simple Play/Quit main menu.
- miniaudio: engine init + one-shot SFX with pitch.
- Tracy / validation / asserts wired per preset.
- Unit test harness (`doctest`) and asset cooker (`--validate`, meshopt, KTX2 dump).
- Content pipeline CLI (`tools/content` / `nu-content`) + Blender/ComfyUI docs.
- Textured `ModelAsset` path; `prop_ink_urn` instances load in Arena I.

### Gameplay / content

- Playable graybox of **Arena I — The Grey Threshold**.
- Main menu → Play / Quit; Esc returns to menu from play.
- Third-person orbit camera + free-cam toggle; WASD / mouse (no jump).
- Immobile **Imp** (360 HP, HP bar, placeholder capsule); **F2** kill/respawn for SFX.
- Two `prop_ink_urn` props placed in the arena.
- No real combat, AI attacks, abilities, loot, or multi-arena flow yet.

### Lore / design docs

- World and Dread Pit notes started in `LORE.md` (Arena I detailed; Arena II sketched).
- Playable character concept notes exist (e.g. Undead Hunter); not implemented as systems.

---

## Setup (developer machine)

Prerequisites and bootstrap are documented in `README.md`. Short path:

1. VS 2022 Build Tools (MSVC + Windows SDK), CMake ≥ 3.28, Ninja, Vulkan SDK, vcpkg, Git LFS.
2. Open an **x64 Native Tools / VS Developer** shell (MSVC `INCLUDE`/`LIB` required).
3. `$env:VCPKG_ROOT = "C:\dev\vcpkg"` (or permanent env).
4. `cmake --preset dev` → `cmake --build --preset dev` → `.\build\dev\game\NineUnder.exe`.

Notes from bring-up:

- Prefer the **system** Vulkan loader; do not ship vcpkg’s `vulkan-1.dll` next to the exe.
- `sdl3` must be built with the `vulkan` feature (declared in `vcpkg.json`).
- ImGui Vulkan backend uses a local overlay under `vcpkg-overlays/imgui` for volk / `VK_NO_PROTOTYPES`.

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

- Arena I is still mostly graybox; urn GLB may still be over triangle budget until Blender LODs land.
- Imp is a non-attacking placeholder (no AI / damage dealing).
- Characters / skinning / animation not wired in-game yet (stubs + docs only).
- No shadows, post stack beyond basic fog/tonemap intent, or VFX.
- No ability systems, inventory, or meta progression.
- Hot-reload and full asset cooking are incomplete.

---

## Version history (state snapshots)

| Version | State summary |
|---------|----------------|
| **v0.1** | Foundation: build/run on Windows, Vulkan graybox Arena I, character move + debug UI. |
| **v0.1.1** | Menu, no jump, Imp stub + death SFX, urn props; removed `prop_ruin_block_a`. |
