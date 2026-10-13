# Changelog

Game mechanics and notable engine changes. Lore and flavor live in `LORE.md`.

Format: [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

## [0.6.0] — 2026-10-12

### Added

- Visual skill tree (Start → two Minors → two Majors) with real effects:
  - Minor 1 (2 Nox): dash follows move input direction
  - Minor 2 (2 Nox): barrier expiry instantly refills ammo
  - Major 1 (5 Pure Nox): +50% move speed for 2s after dash
  - Major 2 (5 Pure Nox): barrier expiry nova (50 dmg + knockback)
- Map Editor (main menu): free cam, place/move Imps / ink pools / ruins, compulsory Spawn + Next Level markers, Imp aggro rings; saves `assets/maps/arena_i.json`
- Next Level marker returns to main menu on touch
- Ink pools slow the player by 50% while standing in them
- GUI scale setting (menus + HUD); replaces HUD-only scale

### Changed

- Arena walkable area expanded (larger floor / wall pushed back)
- Imp `!` stays while aggroed; any damage aggroes even outside radius
- Skill tree opens paused (no camera look); Imp kills grant +1 Nox and +1 Pure Nox (stub)

### Removed

- Upper-right Arena I helper panel
- **F1** free-cam and **F2** mouse-toggle binds from gameplay

## [0.5.0] — 2026-10-12

### Added

- Imp attack telegraph: yellow flash + `imp_windup.wav` (randomized pitch) before spit.
- Light combat feel: brief hitstop + camera pitch nudge on melee / fireball / ult hits.
- HUD scale slider (General settings); default scale 1.5.
- Keybind labels under ability slots (**Q** / **SHIFT** / **E** / **SPACE**).
- Manual reload with **R** (mid-clip, shooter-style).
- Currencies: **Nox** (+1 on Imp kill) and **Pure Nox** (stub); top-left HUD boxes.
- Skill tree overlay (**TAB**): placeholder Minor / Major skills (no effects yet).
- `BALANCE.md` hybrid tuning workspace (current numbers + suggested changes).

### Changed

- Ability binds: melee **Q**, scorched dash **Shift**, barrier **E**, ult **Space** (was Q/E/R/F).
- Show-mouse toggle moved from **Tab** to **F2**.
- Helper panel moved to upper-right; currencies / skill-tree entry occupy upper-left.
- Burn and scorch DoT floating numbers tick every 0.5s instead of ~0.1s chunks.
- Input: key press latches survive frames with no fixed step (fixes missed ability presses).

### Removed

- Molten barrier no longer grants unlimited ammo / skips reload while active.

## [0.4.0] — 2026-10-11

### Added

- Ability kit: **Q** melee (icon), **E** scorched dash (6s CD), **R** molten barrier (16s CD), **F** ult (45s CD).
- Ult buff (10s): faster fire/reload, 80 dmg shots, stackable burn (2s / 10 DPS).
- Scorched trail: 50% enemy slow + 10 DPS for 3s; barrier absorbs spit up to 50% max HP / 4s and skips reload.
- Settings (main + pause): General (mouse sensitivity, damage numbers), Audio (volume), Credits (Zapsplat).
- Damage red vignette (flash on hit; persistent when low HP).
- Ability icons from `assets/ui/icons/`.

### Changed

- Imp retreat speed slower than chase so melee approach is viable.
- Imp spit projectile speed increased.

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

