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

### ▶ Stage 1 — Core combat & character fantasy — *current (v0.4)*

Make the player *feel* like a Pit fighter: attacks, damage, death/respawn rules
for a run, one playable character fantasy implemented (kit + feedback), camera
and controls locked to a fun third-person combat read.  
**Exit:** a short fight encounter is fun without graybox excuses.  
*(v0.4: full Q/E/R/F kit + settings. Still graybox; feel/telegraphs/art remain.)*

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
- add items

---



## 3. Next Version Changes


| Label               | Content                                                                                         |
| ------------------- | ----------------------------------------------------------------------------------------------- |
| **Current version** | `v0.4.0`                                                                                        |
| **Next version**    | `v0.5` *(proposed — Stage 1 feel / readability)*                                                |
| **Intent**          | Make the Imp fight readable and punchy without leaving graybox.                                 |


> Hybrid workspace: the agent proposes/updates this block when versions move;
> you edit and approve. When locked, this section feeds the next update plan.


### Proposed scope for `v0.4` → `v0.5`

- make the imp model briefly flash yellow in order to indicate that it's about to attack
- in drafts folder there is "imp_windup.wav" - use it as the windup sound before it attacks. randomize pitch to avoid monotonous sounds 
- Light combat feel: hitstop or camera nudge on melee / fireball / ult hits
- remove the unlimited ammo feature from the barrier ability
- Tuning pass: create an md document with all current balance and numeric values (cooldowns, damage, ability effects, buffs, debuffs, etc.) along with suggested changes by the agent. It's going to be a hybrid workspace heavily maintained by the owner and implemented only if requested. The aim is to finetune the conceptual parts of the game and balance it for more enjoyable gameplay.
- all abilities and perhaps the base attacks in the game work inconsistently. An ability while not on cooldown might not trigger even after pressing it 5 times. This might be tied to the simultaneous movement in the process hintering the flow, but from what I saw sometimes even when you stay perfectly still and try to press an ability, it takes many presses to make it work. Investigate this issue and fix it. Let's tighten all processes that are involved in the flow of the game - everything needs to run smoothly
- scale HUD elements up. they are too small even on a big monitor. add a setting in general tab where you can scale it up or down (good example: Minecraft)
- burn ticks from ulti hits and second ability scorched ground happen way too much. floating numbers show many 1s dropping like a waterfall - fine tune that and make the burn effect ticks slower 
- below all abilities show which key is bind to them for clarity. ability 1 should be Q, ability 2 should be SHIFT, 3 should be E, ulti should be SPACEBAR.
- add the possibility to reload even when you are not completely out of ammo (similar to shooter games) - use R key for that
- move "Nine Under - Arena I" helper box to the right upper corner. in the freed up corner let's place 2 boxes showing the currency player currently has. The currencies will be called "Nox" and "Pure Nox". Killing an imp gives you one Nox. For now let's make it so that as soon as you kill one, you get it. Below those boxes place an icon to open the skill tree. below the icon also show the key bind that opens it - it will be TAB (unbind it from the current "show mouse" feature and use something else for it). Skill tree will have 2 kinds of skills: Minor and Major ones. to power up minor ones Noxes will be used, for major ones - Pure Noxes. For now put 2 skills as placeholders that don't do anything.



### Out of scope for `v0.5` (parked)

- Full enemy AI packs / multi-enemy encounters  
- Multi-arena run loop / meta progression  
- Final art pass or narrative scripting for Vareth



### Approval

- [x] Scope edited / accepted by owner  
- [x] Ready to turn into an implementation plan  

---

*When the next version ships: move delivered items into* `CHANGELOG.md`*, update*
`DEVELOPMENT_STATE.md`*, and replace this section with the following hop.*
