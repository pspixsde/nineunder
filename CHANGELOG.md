# Changelog

Game mechanics and notable engine changes. Lore and flavor live in `LORE.md`.

Format: [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

## [0.3.0] — 2026-10-11

### Added

- Imp roam + aggro AI: notice bang, mid-range chase, parabolic spit (40 damage).
- Player melee ability (**Q**): 100 damage + knockback, 3s cooldown; ability slot HUD (3 basic + ult).
- Ammo pips (6 yellow rectangles) above HP bar with fill-on-reload.
- FPS counter (upper right); fireball trail VFX.
- Distant Imp spawn so aggro radius is testable on approach.

### Fixed

- Quit-time Vulkan validation: wait for GPU idle before freeing meshes/descriptor sets.

### Removed

- **F2** player-kill debug bind.

## [0.2.0] — 2026-10-11

### Added

- Player ranged attack (LMB): forward projectile, 60 damage, fire rate, reload after 6 shots.
- Player HP (500) with bottom-left HP bar; center-screen attack crosshair.
- Imp takes damage from projectiles; hit flash, floating damage numbers, `fireball_hit.wav`.
- Pause menu (Esc: Resume / Main Menu) and death menu (Play Again / Main Menu).
- **F2** now kills the player (opens death menu).

### Fixed

- Vulkan validation: ImGui loader no longer queries instance-level entry points via `vkGetDeviceProcAddr`.
- Swapchain present semaphore reuse (per-image `renderFinished` + image-in-flight fences); reduces shutdown hitch.
- Imp world HP bar Y drift when orbiting the camera (Vulkan NDC Y mapping).

### Removed

- Temporary **F2** Imp kill/respawn audio test.

## [0.1.1] — 2026-10-11

### Added

- Main menu with **Play** / **Quit** (Esc from play returns to menu).
- Immobile **Imp** enemy placeholder (360 HP, world-space HP bar, capsule mesh).
- Temporary **F2** kill/respawn for Imp audio testing.
- Imp death SFX (`assets/audio/sfx/imp_death.wav`) with random pitch shift.
- Two Arena I instances of `prop_ink_urn` from the authored urn GLB.
- `AudioEngine::playSound` / `update` one-shot playback helpers.



### Changed

- Jump removed (Space no longer launches the character).
- Sample runtime prop is now `prop_ink_urn` (practice cube remains for cooker CI).



### Removed

- `prop_ruin_block_a` sample asset and all game load paths for it.



## [0.1.0] — 2026-10-10



### Added

- Initial project scaffold: CMake + vcpkg, `nu_engine` / `NineUnder` split.
- M0–M3 setup path: SDL3 window, Vulkan 1.3 bring-up, glTF/PBR pipeline hooks,
Jolt character sandbox, graybox Arena I scene.
- **3D content pipeline** (`tools/content` / `nu-content`): asset manifests,
budgets, Blender headless process/export, ComfyUI workflow stubs (TripoSG /
TripoSR / Hunyuan3D-mini), validate + HTML report.
- Textured static `ModelAsset` (tangents, PBR maps, `COL_*` collision,
`SPAWN_*` prefabs), material descriptor set, cooker `--validate` / meshopt /
KTX2 dump, dev model hot-reload.
- Docs: `ASSET_PIPELINE.md`, `CHARACTERS.md` (`nu_humanoid` + ozz evaluation),
`LEVELS.md`, `HOWTO_3D_PIPELINE.md`, `MCP_SETUP.md`.

