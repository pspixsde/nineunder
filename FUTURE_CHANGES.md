# Future Changes — Nine Under

Planning workspace for what comes next. Complements `DEVELOPMENT_STATE.md`
(current truth) and `CHANGELOG.md` (what already shipped mechanically).

---

## 1. GameDev Plan

High-level stages to fully develop the game. No task-level detail here — only
what each stage is *for* and what “done” roughly means.  
**Current stage is marked with ▶.** Update the marker when a new version ships.

### Stage 0 — Foundation (tech vertical slice) — *done (v0.1 → v0.1.1)*

Stand up a custom engine and prove the stack: window, Vulkan rendering, physics
character, one graybox arena, debug UI, reproducible Windows build.  
**Exit:** a developer can build and walk around Arena I in third person.

### ▶ Stage 1 — Core combat & character fantasy — *current (v0.3)*

Make the player *feel* like a Pit fighter: attacks, damage, death/respawn rules
for a run, one playable character fantasy implemented (kit + feedback), camera
and controls locked to a fun third-person combat read.  
**Exit:** a short fight encounter is fun without graybox excuses.  
*(v0.3: two-way Imp fight + melee slot. Still graybox; abilities 2–3 / ult and
combat juice remain open.)*

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



- Update gpu drivers: "Your NVIDIA driver (561.00) may be too old for the bundled PyTorch. Recommended minimum: 580. Please consider updating your NVIDIA drivers if you run into issues."
- add 3 characters
- add a skill tree with major and minor skills
- add an inventory with 9 items 3 of which are active
- 3 basic abilities and 1 ultimate ability per character
- expand lore
- handle 3d models that are heavy even after decimating - file size. could be textures 
- mention zapsplat in credits
- Tuning pass: fire interval, reload, projectile speed, Imp HP/hitbox feel
-

---



## 3. Next Version Changes


| Label               | Content                                                                                         |
| ------------------- | ----------------------------------------------------------------------------------------------- |
| **Current version** | `v0.3.0`                                                                                        |
| **Next version**    | `v0.4` *(proposed — Stage 1 kit / feel)*                                                        |
| **Intent**          | Fill ability slots 2–3 + ult stub; more combat juice without leaving graybox Stage 1.           |


> Hybrid workspace: the agent proposes/updates this block when versions move;
> you edit and approve. When locked, this section feeds the next update plan.


### Proposed scope for `v0.3` → `v0.4`

- Wire ability slots 2 and 3 (even simple placeholders with cooldowns)
- Ultimate ability stub on the big HUD slot (high cooldown, readable payoff)
- Imp spit telegraph / wind-up so attacks are dodgeable
- Light combat feel pass: hitstop or camera nudge on melee / fireball hit



### Out of scope for `v0.4` (parked)

- Full enemy AI packs / multi-enemy encounters  
- Multi-arena run loop / meta progression  
- Final art pass or narrative scripting for Vareth



### Approval

- [ ] Scope edited / accepted by owner  
- [ ] Ready to turn into an implementation plan  

---

*When the next version ships: move delivered items into* `CHANGELOG.md`*, update*
`DEVELOPMENT_STATE.md`*, and replace this section with the following hop.*
