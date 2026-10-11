# Development State — Nine Under

High-level snapshot of where the project is. Update this when a version ships
or when the tech / content footprint changes in a meaningful way.

| Field | Value |
|-------|--------|
| **Game version** | `v0.4.0` |
| **Codename / slice** | Stage 1 kit / feel (full basic + ult stubs) |
| **Last updated** | 2026-10-11 |
| **Primary platform** | Windows (MSVC / x64) |
| **Engine** | Custom C++20 + Vulkan 1.3 (`nu_engine`) |
| **Genre / camera** | Action-roguelite, third person |
| **Setting** | The Dread Pit (see `LORE.md`) |

Related docs: [`README.md`](README.md) (build), [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md),
[`CHANGELOG.md`](CHANGELOG.md), [`LORE.md`](LORE.md), [`FUTURE_CHANGES.md`](FUTURE_CHANGES.md).

---

## One-line status

**Full graybox kit:** Q/E/R/F abilities with icons, Imp two-way fight, settings menu,
damage vignette. Stage 1 still open on combat polish / non-graybox feel.

---

## What works today (v0.4.0)

### Engine / tooling

- CMake presets + vcpkg; `engine/` vs `game/` split.
- Mouse look sensitivity + master audio volume controls.
- Vulkan present sync, GPU idle before teardown; ImGui ability icon textures.
- Jolt character, miniaudio SFX, content pipeline / `ModelAsset` urns.

### Gameplay / content

- Arena I graybox; menus: main / pause / death / settings (General, Audio, Credits).
- Player: 500 HP, LMB fireball, ammo pips, red damage vignette, low-HP tint.
- Abilities: Q melee, E scorched dash, R molten barrier, F ult (burn stacks).
- Imp: roam/aggro/spit (faster), slower retreat, scorch slow + burn DoT.
- Settings: sensitivity, floating damage toggle, volume, Zapsplat credit.

### Lore / design docs

- World notes in `LORE.md`; character fantasy still ahead of final art/animation.

---

## Setup (developer machine)

See `README.md`. Short path: VS Developer shell → `cmake --preset dev` →
`cmake --build --preset dev` → `.\build\dev\game\NineUnder.exe`.

---

## Known gaps / limitations (honest)

- Still graybox capsules; no animation / final VFX stack.
- Imp telegraph / multi-enemy packs not in yet.
- Stage 1 exit (“fun without graybox excuses”) not claimed.

---

## Version history (state snapshots)

| Version | State summary |
|---------|----------------|
| **v0.1** | Foundation graybox Arena I. |
| **v0.1.1** | Menu, Imp stub, urns. |
| **v0.2** | Ranged combat kickoff + pause/death. |
| **v0.3** | Imp AI spit + melee + HUD polish. |
| **v0.4** | Full ability kit + settings + vignette / burn / scorch. |
