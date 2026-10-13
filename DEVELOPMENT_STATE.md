# Development State — Nine Under

High-level snapshot of where the project is. Update this when a version ships
or when the tech / content footprint changes in a meaningful way.

| Field | Value |
|-------|--------|
| **Game version** | `v0.6.0` |
| **Codename / slice** | Stage 1 → arena rules (graybox) |
| **Last updated** | 2026-10-12 |
| **Primary platform** | Windows (MSVC / x64) |
| **Engine** | Custom C++20 + Vulkan 1.3 (`nu_engine`) |
| **Genre / camera** | Action-roguelite, third person |
| **Setting** | The Dread Pit (see `LORE.md`) |

Related docs: [`README.md`](README.md) (build), [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md),
[`CHANGELOG.md`](CHANGELOG.md), [`LORE.md`](LORE.md), [`FUTURE_CHANGES.md`](FUTURE_CHANGES.md),
[`BALANCE.md`](BALANCE.md).

---

## One-line status

**Arena I as an editable graybox encounter:** map editor, next-level marker, ink
slow, real skill-tree effects, expanded floor. Still graybox art / Stage 1 feel.

---

## What works today (v0.6.0)

### Engine / tooling

- CMake presets + vcpkg; latched key presses; character move-speed multiplier.
- GUI scale (ImGui font global scale + menu/HUD sizing).
- Free-cam look with RMB when cursor is visible (map editor).

### Gameplay / content

- Arena I expanded graybox; map data in `assets/maps/arena_i.json`.
- Map Editor: place/move Imps, ink pools, ruins; Spawn + Next Level (non-deletable).
- Play: spawn from map, ink slow 50%, next-level → main menu.
- Skill tree (TAB, pauses): directional dash, barrier reload, dash momentum, barrier nova.
- Imp telegraph / aggro bang while aggroed; damage always aggros.
- Kit: Q / Shift / E / Space, R reload, Nox + Pure Nox economy stub.

### Lore / design docs

- World notes in `LORE.md`; `BALANCE.md` for numeric tuning proposals.

---

## Setup (developer machine)

See `README.md`. Short path: VS Developer shell → `cmake --preset dev` →
`cmake --build --preset dev` → `.\build\dev\game\NineUnder.exe`.

---

## Known gaps / limitations (honest)

- Still graybox; multi-enemy AI packs / full Stage 2 arena rules unfinished.
- Pure Nox still a stub (+1 per Imp kill) — economy not final.
- Map editor does not live-rebuild ruin physics mid-session (collisions from last play/build).

---

## Version history (state snapshots)

| Version | State summary |
|---------|----------------|
| **v0.1** | Foundation graybox Arena I. |
| **v0.1.1** | Menu, Imp stub, urns. |
| **v0.2** | Ranged combat kickoff + pause/death. |
| **v0.3** | Imp AI spit + melee + HUD polish. |
| **v0.4** | Full ability kit + settings + vignette / burn / scorch. |
| **v0.5** | Telegraph, hit feel, input reliability, HUD/economy stubs. |
| **v0.6** | Map editor, skill effects, ink slow, expanded arena. |
