# Changelog

Game mechanics and notable engine changes. Lore and flavor live in `LORE.md`.

Format: [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

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
