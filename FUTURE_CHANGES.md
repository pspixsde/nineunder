# Future Changes — Nine Under

Planning workspace for what comes next. Complements `DEVELOPMENT_STATE.md`
(current truth) and `CHANGELOG.md` (what already shipped mechanically).

---

## 1. GameDev Plan

A milestone-based roadmap for developing Nine Under into a complete 3D action
roguelite.

This plan describes **playable outcomes and production capabilities**, not
individual implementation tasks. Detailed work belongs in Next Version Changes.

### How to use this plan

- **▶ marks the current validation target**, not the only area under development.
- A milestone may span several versions. Shipping a version does not
  automatically complete a milestone.
- Complete milestones using demonstrated exit criteria, not feature presence alone.
- Later systems may receive small exploratory implementations when needed to
  resolve significant design or technical risks.
- Do not expand content around systems whose core behavior remains unreliable.
- Milestones are not equal-duration promises. Split oversized milestones into
  bounded, independently testable outcomes.
- Changes to milestone order or intended game scope require owner approval.

### Current position — v0.6.0

The technical foundation and many core combat components exist:
one character kit, Imp combat, skill effects, hazards, and map authoring.

The immediate priority is to validate a dependable combat encounter and complete
Arena I's start → fight → win/lose → exit/retry flow.

Existing systems do not automatically satisfy their corresponding milestone.
Combat quality, progression rules, and production scalability still need evidence.

---

### Track A — Prove the game

#### M0 — Technical foundation — achieved

**Purpose:** establish a reproducible, playable technical foundation.

Includes:
- Windows build and launch workflow.
- Rendering, physics, character movement, camera, and input.
- Basic model/material loading and graybox level support.
- Development diagnostics and basic content tooling.

**Exit:** a developer can build the game and enter the graybox in third person.

#### ▶ M1 — Dependable combat encounter — current validation target

**Purpose:** make the existing character and enemy reliable enough to evaluate
combat honestly.

Includes:
- Reliable attacks, abilities, cooldowns, damage, and hit detection.
- Readable enemy telegraphs and understandable damage feedback.
- Camera and collision behavior suitable for close and ranged combat.
- Defined interactions between slows, temporary buffs, barriers, and upgrades.
- A representative fight with useful positioning and ability decisions.
- Reliable death and immediate retry.
- A focused tuning pass on the existing kit and Imp.

**Exit:**
- Core actions behave consistently under representative combat conditions.
- Enemy attacks are readable and avoidable under intended conditions.
- Camera, collision, and input problems do not routinely invalidate combat.
- The kit offers useful decisions rather than one action dominating every situation.
- Playtesting identifies and addresses the largest combat frustrations.

#### M2 — Arena I completion loop

**Purpose:** turn the graybox into a complete, self-contained encounter.

Includes:
- Explicit encounter start, active, completed, and failed behavior.
- A defined set of enemies required for completion.
- Locked/unlocked exit behavior.
- Clear objectives, completion feedback, and failure feedback.
- Correct resets for retry and fresh sessions.
- An authored encounter using existing enemies, hazards, and cover.
- Editor-to-play consistency, including ruin collision.

**Exit:**
- Start → fight → clear → exit works without debug intervention.
- Death → retry returns the arena to its intended starting state.
- Players can understand the objective and recognize success.
- The played encounter matches the authored map.

#### M3 — First meaningful build progression

**Purpose:** make existing upgrades produce understandable, consequential choices.

Includes:
- Defined skill prerequisites and purchase behavior.
- Complete and testable effects for the existing skills.
- Explicit roles for Nox and Pure Nox.
- An initial reward/cost cadence suited to the available playable sequence.
- Explicit rules for what persists through retry, arena transitions, and new runs.
- Enough opportunity to earn and compare meaningful upgrades.

**Exit:**
- Players can earn, choose, and perceive useful upgrades.
- Available choices have understandable benefits and tradeoffs.
- Purchase validation and progression resets behave consistently.
- Temporary economy rules are documented; a final whole-game economy is not required.

#### M4 — Minimal connected run

**Purpose:** prove the session structure before building many arenas.

Includes:
- Arena I connected to a small second test encounter.
- Intended player/build state carried between encounters.
- Correct cleanup of encounter-local state.
- Defined run failure and run completion.
- Run summary and restart flow.
- Basic onboarding sufficient to navigate the short run.

**Exit:**
- A short multi-encounter run can be completed or failed.
- Transitions, state carryover, cleanup, and restart work consistently.
- No production-quality Arena II is required yet.

#### M5 — Enemy and encounter variety proof

**Purpose:** demonstrate that combat can support different tactical situations.

Includes:
- A complementary enemy role rather than merely a stronger Imp.
- Mixed-enemy encounters and target-priority decisions.
- Navigation, separation, and attack coordination as required by those encounters.
- Hazard and cover combinations.
- Bounded pacing variations or reinforcement beats where useful.

**Exit:**
- Representative encounters require different positioning, timing, or priorities.
- Difficulty does not rely only on increased enemy HP or population.
- Mixed threats remain readable and reasonably fair.

#### M6 — Representative visual and content slice

**Purpose:** prove the intended presentation and production workflow on a small,
representative portion of the game.

Includes:
- A bounded arena portion at representative visual quality.
- Current-character and representative-enemy presentation.
- Animation, materials, lighting, audio, and VFX needed to judge combat readability.
- Model, texture, collision, and runtime performance budgets.
- Import/cook/iteration workflow validation.
- Investigation of oversized assets and texture footprints.

**Exit:**
- One bounded slice demonstrates the intended quality and combat readability.
- Its asset and runtime costs are measured against provisional targets.
- The workflow is repeatable, and major production bottlenecks are understood.

**Note:** investigate expensive pipeline risks earlier when necessary.
This milestone is not permission to postpone animation, audio, or performance
work that current gameplay already needs.

---

### Track B — Prove the systems can expand

#### M7 — Item and inventory vertical slice

**Purpose:** prove the item system before filling it with content.

Includes:
- Confirmed inventory capacity and active-slot rules.
- Item acquisition, use, replacement, and removal.
- Active-item controls, cooldowns, and required HUD feedback.
- Interactions with the character kit and skill progression.
- A small representative set of active/passive items.
- Explicit item persistence and reset behavior.

**Exit:**
- Items create meaningful choices without obscuring the character kit.
- Pickup, use, replacement, and reset flows work consistently.
- The intended inventory structure is proven before producing a large item catalog.

#### M8 — Second-character production proof

**Purpose:** prove that the game supports a distinct playable character without
duplicating its core systems.

Includes:
- One additional character with a distinct combat role.
- A complete kit, upgrade integration, and required presentation.
- Shared input, HUD, progression, and run interfaces where appropriate.
- Full short-run testing and an initial balance pass.

**Exit:**
- Two characters feel meaningfully different.
- Both work through the existing run and progression systems.
- Adding the second character does not require a parallel bespoke implementation.

#### M9 — Arena II production proof

**Purpose:** prove that a distinct arena can be produced using reusable systems.

Includes:
- A complete Arena II layout with a distinct encounter identity.
- Appropriate enemy/hazard combinations and progression beats.
- Rewards and run-transition integration.
- Art, audio, collision, and performance work to the established content standard.
- Authoring workflow improvements justified by actual production friction.

**Exit:**
- Arena II is a complete part of a run, not an isolated loadable scene.
- It offers meaningful differences from Arena I.
- Its production does not require rebuilding foundational systems.

**Ordering note for M7–M9:**
This is the default sequence, not an immutable dependency chain.
The agent may recommend a different order to address a demonstrated design or
technical risk. Explain the tradeoff and obtain owner approval.

---

### Track C — Produce and finish the intended game

#### M10 — First bounded content batch

**Purpose:** begin repeatable production using the proven systems and workflows.

Includes:
- An owner-approved batch of arenas, encounters, enemies, and rewards.
- Additional characters only when their production fits the approved batch.
- Integration into actual runs.
- Per-content presentation, performance, and playtest checks.
- Review of production cost and remaining scope.

**Exit:**
- Every committed part of the batch is playable in context.
- The batch meets the established content completion checklist.
- Production assumptions are reviewed before another large batch begins.

#### M11 — Remaining content batches

**Purpose:** complete the approved arena and character scope in manageable increments.

Replace this milestone with named batches once production cost is understood.

If nine arenas remain the approved scope, a provisional breakdown is:
- Batch A: Arenas III–IV.
- Batch B: Arenas V–VI.
- Batch C: Arenas VII–VIII.
- Batch D: Arena IX and final-run integration.

The first approved batch is delivered under M10; subsequent batches follow here.
Pairings may change. Mechanically unusual or boss-heavy arenas may need their
own batch.

Every batch includes:
- Layout and navigation.
- Encounters and hazards.
- Rewards and progression integration.
- Presentation and narrative beats appropriate to the arena.
- Performance checks.
- Playtesting and regression coverage.

**Exit:**
- All approved content exists and works in the intended run structure.
- No major content remains merely prototyped or individually loadable.
- Arena and character targets are explicit and owner-approved.

#### M12 — Full-game progression and balance

**Purpose:** make the complete game coherent across its intended content and roster.

Includes:
- Run length, arena ordering, and difficulty curve.
- Reward distribution and currency economy.
- Skill/item availability and build variety.
- Character viability across representative encounters.
- Onboarding and difficulty options.
- Any owner-approved between-run progression.
- Full-run playtesting and resolution of major pacing problems.

**Exit:**
- Complete runs provide a coherent progression curve.
- Progression rules and rewards support the intended player decisions.
- Major build, character, and difficulty outliers have been addressed.
- Meta progression is included only if explicitly approved.

#### M13 — Release hardening

**Purpose:** prepare the approved game scope for players without developer assistance.

Includes:
- Crash, state-corruption, and regression fixes.
- Save robustness and compatibility behavior for supported saves.
- Settings persistence and accessibility completion.
- Performance and hardware compatibility checks.
- Credits, asset/audio licenses, and required attribution.
- Packaging, installation, store/page readiness, and public onboarding.
- A written release checklist and known-issues review.

**Exit:**
- A release candidate meets the shipping checklist.
- Players can install, understand, play, and restart the game independently.
- Remaining known issues are explicitly accepted for release.

---

### Continuous quality tracks

These are developed throughout the roadmap, not postponed to a final polish stage:

- **Testing and reliability:** lifecycle rules, regressions, reproducible failures.
- **Readability and feedback:** telegraphs, damage, cooldowns, objectives, outcomes.
- **Animation/audio/VFX:** enough fidelity to evaluate the current gameplay honestly.
- **Performance:** representative measurements and budgets before multiplying content.
- **Accessibility and controls:** supported input, legibility, remapping and options
  appropriate to the current scope.
- **Persistence:** distinguish settings, run state, and long-term progression;
  implement each when required by approved behavior.
- **Authoring:** accurate editor-to-play results and practical iteration speed.
- **Documentation:** current capabilities, temporary rules, limitations, and decisions.

Each milestone requires the relevant amount of these tracks.
Final polish is not required everywhere, but essential behavior cannot be deferred
merely by labeling it polish.

---

## 2. Next-Release Decision Sheet

A concrete guide for choosing the next update.
This is a decision aid, not a second backlog or a fixed release sequence.

### How to use

1. Read from the top and find the earliest result that is not dependable.
2. Use that gap to propose the next release's main outcome.
3. Include later work only when it supports that outcome or resolves a costly risk.
4. Check actual implementation and playtest evidence where available.
5. Treat undocumented behavior as unknown, not automatically missing.
6. Choose a bounded scope with observable acceptance criteria.

| Checkpoint | Concrete changes to look for | Move on when… |
|---|---|---|
| **1. Can we trust the test environment?** | Reliable inputs; matching visuals/collision; repeatable map loading; clean retry; useful diagnostics; fix crashes and severe state leaks. | Tests produce repeatable, believable results. |
| **2. Does the current combat kit hold up?** | Attack/cooldown reliability; hit detection; camera obstruction; enemy telegraphs; avoidable attacks; useful ability roles; slow/buff/barrier interactions. | A representative fight is readable and dependable, and the largest combat frustrations have been addressed. |
| **3. Is Arena I a complete encounter?** | Encounter start; required enemies; objective; locked exit; completion feedback; defeat/retry; reset rules; one deliberately authored fight. | Start → fight → win/lose → exit/retry works without debug intervention or developer explanation. |
| **4. Does upgrading create a choice?** | Existing skill effects; prerequisites; purchase validation; understandable costs; attainable rewards; upgrade timing; currency roles; persistence rules. | Players can earn, choose, and perceive useful upgrades within the playable sequence. |
| **5. Is there a real short run?** | Second test encounter; transitions; intended state carryover; cleanup; failure/reset; run completion; summary; basic onboarding. | A short multi-encounter run can be completed and restarted cleanly. |
| **6. Do encounters require different decisions?** | Complementary enemy role; mixed groups; positioning pressure; hazard combinations; target priorities; pacing variation; necessary AI coordination. | Encounters differ in decisions, not only enemy counts and HP. |
| **7. Can intended content be produced affordably?** | Representative animated assets; model/texture budgets; collision authoring; import/cook workflow; useful lighting/audio/VFX; performance measurements. | One quality slice works, and its workflow and costs are understood. Investigate costly risks earlier. |
| **8. Do items justify their complexity?** | Inventory/active-slot rules; pickup/use/swap; controls; cooldowns; character interactions; reset behavior; a small representative item set. | Items add meaningful choices without muddling the kit or input scheme. |
| **9. Can a second character reuse the game?** | Distinct kit; upgrade integration; shared input/HUD interfaces; animation/content workflow; complete short-run validation. | Character two works without a parallel bespoke implementation. |
| **10. Can a second arena reuse the game?** | Distinct layout and encounter identity; rewards; transitions; authoring; presentation; integration and performance. | Arena II is a complete addition, not another isolated scene. |
| **11. Are we ready to multiply content?** | Bounded arena/enemy/character batches; per-content completion checklist; run integration; production-cost reviews. | Each batch reaches the agreed playable quality before another large batch opens. |
| **12. Does the whole game hold together?** | Full-run pacing; difficulty curve; reward economy; build/character viability; onboarding; any approved meta progression. | Complete runs support the intended experience across the approved content. |
| **13. Is it ready for strangers?** | Save robustness; settings; accessibility; hardware coverage; performance; credits/licenses; packaging; installation; regression pass. | Players can install, understand, play, and recover from failure without developer assistance. |

### Priority rules

- **Broken existing behavior beats optional breadth.**
- **Readable gameplay beats decorative polish.**
- **Complete one representative example before multiplying content.**
- **Test important interactions before adding more interacting systems.**
- **Prove the production pipeline before committing to content volume.**
- **Resolve high-cost risks early, even when their full milestone comes later.**
- **Do not finalize a whole-game economy around an unrepresentative test encounter.**
- **Every release needs an outcome and evidence—not merely completed tickets.**

### Common traps and better alternatives

| Temptation | Prefer this first |
|---|---|
| Add another character because the current one feels shallow. | Determine whether the existing kit lacks useful interactions, feedback, tuning, or encounter pressure. |
| Add more skills because the tree looks small. | Verify that current skills work, can be earned, and create perceptible choices. |
| Add waves because the arena feels empty. | Author one intentional encounter and determine whether pacing actually needs reinforcements. |
| Redesign the HUD because the game feels unfinished. | Identify the specific information players fail to understand; fix that communication problem. |
| Produce many arenas before the run structure exists. | Connect two small encounters and prove transition, carryover, failure, and restart behavior. |
| Fill the inventory with items immediately. | Prove acquisition, use, replacement, controls, persistence, and a few representative interactions. |
| Import final assets everywhere. | Validate one representative asset/presentation slice and measure its costs. |
| Build a general framework for hypothetical future content. | Support the next concrete use case, then generalize when repeated needs are demonstrated. |
| Carry all unfinished work into the next release. | Reassess whether each item still serves the next release outcome. |

### Current reading — v0.6.0

Based on the documented state:

- **Checkpoint 1:** editor-to-play ruin collision is a known inconsistency.
- **Checkpoint 2:** many combat components exist; quality still needs playtest evidence.
- **Checkpoint 3:** incomplete; the next-level marker remains a completion-rule stub.
- **Checkpoint 4:** skill effects exist, but progression cadence and lifecycle rules
  are not fully established in these documents.
- **Later checkpoints:** mostly future work or unproven.

**Recommended immediate direction:** dependable combat plus a complete Arena I
encounter loop.

Update this reading after each release. Do not mark a checkpoint complete solely
because its related features appear in the changelog.

---



## 2. To Do List

> **Owner workspace.** Do not auto-fill this section.  
> Dump anything here: immediate fixes, backlog, design notes, constraints, open
> questions. Prefer clear directives (“change X so that Y”) over vague wishes.



- overall aim of the MVP - have 3 playable characters, each with their own abilities and skill trees
- add an inventory with 9 items 3 of which are active, items will randomly drop from killed enemies
- each character, location, enemy, interaction requires a section in lore md. Another important requirement is to make the  lore of the characters connected so that they don't feel like disconnected stories
- handle 3d models that are heavy even after decimating - file size. could be textures 



---



## 3. Next Version Changes


| Label               | Content                                                                                         |
| ------------------- | ----------------------------------------------------------------------------------------------- |
| **Current version** | `v0.6.0`                                                                                        |
| **Next version**    | `v0.7` *(proposed — deepen Arena I rules)*                                                      |
| **Intent**          | Push Stage 1→2: clearer win/lose and encounter structure without leaving graybox.               |


> Hybrid workspace: the agent proposes/updates this block when versions move;
> you edit and approve. When locked, this section feeds the next update plan. Find the agent protocol below.


### Proposed scope for `v0.6` → `v0.7`

*(Draft — edit freely. Not approved yet.)*

- Win condition beyond next-level stub (clear Imps / open gate beat)
- Imp respawn / wave stub for sustained fights
- Rebuild ruin physics when leaving map editor
- Pure Nox economy design (replace +1-per-kill stub)



### Out of scope for `v0.7` (parked)

- Full enemy AI packs / multi-enemy encounter design pass  
- Multi-arena run loop / meta progression  
- Final art pass or narrative scripting for Vareth



### Approval

- [ ] Scope edited / accepted by owner 
- [ ] Ready to turn into an implementation plan  

---

## Agent protocol — proposing the next version

The next-version section is an owner-approved planning workspace.
The agent proposes; the owner edits and approves.
Do not treat a proposal as authorization to implement it.

### 1. Reconcile shipped truth

After a release:
- Update CHANGELOG.md with delivered changes only.
- Update DEVELOPMENT_STATE.md with current capabilities and limitations.
- Compare delivered work against the previous approved scope.
- Explicitly identify deferred, partial, or changed items.
- Do not automatically carry unfinished items into the next version.
- Do not auto-edit the owner To Do List; flag stale or ambiguous entries
  separately for owner review.

### 2. Assess the next playable gap

Review:
- The current roadmap milestone and its exit criteria.
- Known bugs, limitations, and playtest observations.
- The complete player flow: start, fight, upgrade, clear, transition,
  die, retry, and quit/resume where supported.
- Interactions and lifecycle rules of recently added systems.
- Authoring and technical problems that block the current milestone.

When repository or test access exists, inspect relevant implementation
and tests rather than relying only on documentation.
Label findings as verified, documented, inferred, or unknown as appropriate.
Do not describe an unverified gap as a confirmed missing feature.

### 3. Generate candidates before choosing scope

Look for candidates across:
- Broken or unreliable existing behavior.
- Missing rules required to complete the current player experience.
- Underdeveloped existing mechanics.
- Readability and onboarding problems.
- Authoring or technical blockers.
- The smallest experiment needed to resolve an important design risk.

Do not fill the release with new features just because they are easy to name.

### 4. Choose one release outcome

Write one sentence describing what the player or developer will be able
to do after this release that they cannot reliably do today.

Prefer work that:
1. Fixes a blocker or invalidates misleading playtest results.
2. Closes a dependency for the current milestone.
3. Completes an existing gameplay loop.
4. Tests an important unresolved design assumption.
5. Improves useful readability or iteration speed.

Defer unrelated polish, speculative infrastructure, and additional content
that multiplies unresolved problems.

### 5. Propose a substantial but bounded scope

Normally present 6–10 meaningful candidates for owner review:
- Clearly separate recommended committed scope from optional additions.
- Adjust the count to work size; do not pad the list or split one feature
  into artificial bullets.
- If capacity is unknown, state the size assumption and provide a cut line.
- A smaller scope is acceptable when the changes are substantial.

For each recommended item, include:
- Change: concrete behavior or capability.
- Why now: the gap, dependency, or evidence that justifies it.
- Done when: observable acceptance criteria.
- Dependencies, uncertainty, and rough size where useful.

Order the work by dependency, not by excitement.

### 6. Explain the tradeoffs

Include:
- Why this is the recommended next release.
- Important candidates deferred and why.
- Decisions requiring owner input.
- A short playtest/regression checklist.
- What evidence would justify advancing the roadmap milestone.

Do not advance a milestone merely because a version shipped.
Advance it when its exit criteria are demonstrated.

### 7. Preserve owner approval

Leave scope unapproved.
Ask only the questions that materially change scope or implementation.
Use explicit provisional assumptions for non-blocking details.

---

*When the next version ships: move delivered items into* `CHANGELOG.md`*, update*
`DEVELOPMENT_STATE.md`, and replace this section with the following hop.*
