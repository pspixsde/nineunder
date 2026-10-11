# Future Changes — Nine Under

Planning workspace for what comes next. Complements `DEVELOPMENT_STATE.md`
(current truth) and `CHANGELOG.md` (what already shipped mechanically).

---

## 1. GameDev Plan

High-level stages to fully develop the game. No task-level detail here — only
what each stage is *for* and what “done” roughly means.  
**Current stage is marked with ▶.** Update the marker when a new version ships.

### ▶ Stage 0 — Foundation (tech vertical slice) — *current (v0.1.1)*

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



- Update gpu drivers: "Your NVIDIA driver (561.00) may be too old for the bundled PyTorch. Recommended minimum: 580. Please consider updating your NVIDIA drivers if you run into issues."
- add 3 characters
- add a skill tree with major and minor skills
- add an inventory with 8 active items 3 of which are active
- 3 basic abilities and 1 ultimate ability per character
- expand lore
- Decimate / LOD `prop_ink_urn` into prop budget (500–5k tris) — current GLB is still heavy

---



## 3. Next Version Changes


| Label               | Content                                                                                         |
| ------------------- | ----------------------------------------------------------------------------------------------- |
| **Current version** | `v0.1.1`                                                                                        |
| **Next version**    | `v0.2` *(proposed — Stage 1 combat kickoff)*                                                    |
| **Intent**          | Start real combat fantasy: player damage/attacks, Imp can die from gameplay (not only F2).      |


> Hybrid workspace: the agent proposes/updates this block when versions move;
> you edit and approve. When locked, this section feeds the next update plan.



### Proposed scope for `v0.1.1` → `v0.2`

- [ ] Player melee attack (simple hit volume / cooldown)
- [ ] Imp takes damage from player attacks; remove temporary F2 kill once combat works
- [ ] Player HP + death / respawn stub for Arena I
- [ ] Hit feedback (flash / number / SFX placeholder)
- [ ] **Docs sync** — On ship: bump `CHANGELOG.md`, refresh `DEVELOPMENT_STATE.md`, advance GameDev Plan marker if Stage 1 exit criteria are met, rewrite this section for the following hop.



### Out of scope for `v0.2` (parked)

- Full enemy AI packs / multi-enemy encounters  
- Multi-arena run loop / meta progression  
- Final art pass or narrative scripting for Vareth



### Approval

- [ ] Scope edited / accepted by owner  
- [ ] Ready to turn into an implementation plan  

---

*When the next version ships: move delivered items into* `CHANGELOG.md`*, update*
`DEVELOPMENT_STATE.md`*, and replace this section with the following hop.*
