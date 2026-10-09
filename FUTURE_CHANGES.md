# Future Changes — Nine Under

Planning workspace for what comes next. Complements `DEVELOPMENT_STATE.md`
(current truth) and `CHANGELOG.md` (what already shipped mechanically).

---

## 1. GameDev Plan

High-level stages to fully develop the game. No task-level detail here — only
what each stage is *for* and what “done” roughly means.  
**Current stage is marked with ▶.** Update the marker when a new version ships.

### ▶ Stage 0 — Foundation (tech vertical slice) — *current (v0.1)*

Stand up a custom engine and prove the stack: window, Vulkan rendering, physics
character, one graybox arena, debug UI, reproducible Windows build.  
**Exit:** a developer can build and walk around Arena I in third person.

### Stage 1 — Core combat & character fantasy

Make the player *feel* like a Pit fighter: attacks, damage, death/respawn rules
for a run, one playable character fantasy implemented (kit + feedback), camera
and controls locked to a fun third-person combat read.  
**Exit:** a short fight encounter is fun without graybox excuses.

### Stage 2 — First arena is a game

Turn Arena I into a designed space: hazards (ink pools), gates/progression beat
with Vareth’s role (even if narrative is light), enemy set, onboarding, win/lose
of the arena. Art can still be gray/proxy, but rules are real.  
**Exit:** clear Arena I end-to-end as a self-contained experience.

### Stage 3 — Roguelite run loop

Session structure: enter Pit → arena sequence hooks → failure → restart/meta
hooks. Draft of between-run or in-run progression (whatever fits the fantasy),
basic menus (boot, pause, run summary).  
**Exit:** die → restart is intentional and readable, not a crash-to-desktop.

### Stage 4 — Content breadth (nine arenas & cast)

Expand toward the Dread Pit fantasy: more arenas (starting with Arena II), enemy
variety, additional playable characters, environmental storytelling. Systems
from earlier stages should *scale* rather than be rewritten.  
**Exit:** multiple arenas and at least two characters feel distinct.

### Stage 5 — Systems depth & juice

Audio, VFX, animation, UI/UX, accessibility, performance budgets, save/settings,
difficulty/feel tuning, tool-side content pipeline (cook, hot reload, data).  
**Exit:** the game feels shippable as a slice people would pay to try.

### Stage 6 — Polish & release

Content complete for the chosen scope, bug/regression pass, packaging,
store/page readiness, final balance.  
**Exit:** release candidate for the first public version.

---

## 2. To Do List

> **Owner workspace.** Do not auto-fill this section.  
> Dump anything here: immediate fixes, backlog, design notes, constraints, open
> questions. Prefer clear directives (“change X so that Y”) over vague wishes.

<!-- Add items below. Suggested tags: [now] [backlog] [design] [bug] [tech] [art] [audio] -->

-

---

## 3. Next Version Changes

| Label | Content|
|--|--|
| **Current version** | `v0.1` |
| **Next version** | `v0.1.1` *(patch — proposed)* |
| **Intent** | Stabilize the foundation and make the Arena I sandbox a better base for Stage 1 combat work. |

> Hybrid workspace: the agent proposes/updates this block when versions move;
> you edit and approve. When locked, this section feeds the next update plan.

### Proposed scope for `v0.1` → `v0.1.1`

- [ ] **Hardening** — Document and/or script the VS-developer-shell requirement; keep Vulkan loader hygiene (no applocal `vulkan-1.dll`); smoke-test path from clean clone.
- [ ] **Arena I sandbox quality** — Clearer graybox read (ink pools as triggers or debug volumes, gate/vale markers, spawn/reset); optional simple ground material variation so space reads in screenshots.
- [ ] **Player feel prep** — Tune capsule/camera defaults; expose a few move/ jump values for iteration; ensure respawn-at-spawn without restarting the process.
- [ ] **Combat stub (thin)** — Minimal damageable target + one melee or hitscan debug attack so Stage 1 has a hook to grow from (can stay ImGui-driven).
- [ ] **Content pipeline check** — Load at least one real glTF mesh (prop or proxy character) through the existing asset path to prove the cooker/runtime loop.
- [ ] **Audio stub** — Play one one-shot / loop to prove miniaudio wiring beyond init.
- [ ] **Docs sync** — On ship: bump `CHANGELOG.md`, refresh `DEVELOPMENT_STATE.md`, advance GameDev Plan marker if a stage actually completed (unlikely for a patch), rewrite this section for `v0.1.1` → next.

### Out of scope for `v0.1.1` (parked)

- Full combat game feel / enemy AI packs  
- Multi-arena run loop / meta progression  
- Final art pass or narrative scripting for Vareth  

### Approval

- [ ] Scope edited / accepted by owner  
- [ ] Ready to turn into an implementation plan  

---

*When `v0.1.1` (or whichever next version you choose) ships: move delivered items
into `CHANGELOG.md`, update `DEVELOPMENT_STATE.md`, and replace this section with
the following hop.*
