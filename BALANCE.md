# Balance & Tuning — Nine Under

Hybrid workspace for numeric feel. **Current** values match the shipped build.
**Suggested** rows are agent proposals only — implement when the owner asks.

Do not treat this file as lore; keep narrative in `LORE.md`.

---

## How to use

1. Edit **Suggested** (and notes) freely.
2. When a change is approved, ask the agent to apply it in code and sync **Current**.
3. Prefer small batches (one system at a time: Imp, fireball, burn, etc.).

---

## Player — vitals & weapon

| Stat | Current | Suggested | Notes |
|------|---------|-----------|-------|
| Max HP | 500 | 500 | |
| Fireball damage | 60 | 55–65 | |
| Fire interval (s) | 0.35 | 0.30–0.35 | |
| Clip size | 6 | 6 | |
| Reload duration (s) | 1.75 | 1.4–1.6 | Manual **R** reload mid-clip (v0.5) |
| Projectile speed | 28 | 28–32 | |
| Projectile lifetime (s) | 3.0 | 3.0 | |

---

## Abilities

| Ability | Bind | Current | Suggested | Notes |
|---------|------|---------|-----------|-------|
| Melee | Q | 100 dmg, 3.0s CD, range 2.6, knockback 3.2 | CD 2.5–3.0 | Cone cos ≥ 0.25 |
| Scorched dash | Shift | 6.0s CD, distance 6, trail 3s | CD 5–6 | |
| Scorch DPS | — | 10 | 8–10 | Slow 50% while in zone |
| Scorch tick interval (s) | — | 0.5 | 0.4–0.6 | Was ~0.1s chunks (v0.4) |
| Molten barrier | E | 16s CD, 4s duration, absorbs 50% max HP | CD 14–16 | No longer infinite ammo (v0.5) |
| Ultimate | Space | 45s CD, 10s buff | CD 40–45 | |
| Ult fire interval (s) | — | 0.12 | 0.12–0.15 | |
| Ult reload (s) | — | 0.65 | 0.65 | |
| Ult fireball damage | — | 80 | 70–80 | Applies burn stacks |
| Burn duration (s) | — | 2.0 | 2.0–2.5 | Stacking |
| Burn DPS | — | 10 | 8–10 | |
| Burn tick interval (s) | — | 0.5 | 0.45–0.6 | Fewer floating “1”s (v0.5) |

---

## Imp

| Stat | Current | Suggested | Notes |
|------|---------|-----------|-------|
| Max HP | 360 | 320–360 | |
| Roam / chase / retreat speed | 1.7 / 4.6 / 2.1 | keep retreat &lt; chase | |
| Aggro radius | 16 | 14–16 | |
| Desired / attack range | 7.5 / 11 | | |
| Attack interval (s) | 1.35 | 1.2–1.5 | After windup completes |
| Windup duration (s) | 0.45 | 0.35–0.55 | Yellow flash + `imp_windup.wav` |
| Spit damage | 40 | 35–45 | |
| Spit speed / gravity | 24 / 16 | | |

---

## Feel / juice

| Stat | Current | Suggested | Notes |
|------|---------|-----------|-------|
| Hitstop light / heavy (s) | 0.035 / 0.055 | tune by ear | Fireball / melee+ult |
| Camera pitch kick | 0.55° / 1.1° | | Recovers quickly |
| Hit flash (s) | 0.18 | | Damage flash (red-ish) |
| HUD scale default | 1.5 | 1.25–1.75 | Settings → General |

---

## Economy & skills (v0.6)

| Stat | Current | Suggested | Notes |
|------|---------|-----------|-------|
| Nox per Imp kill | 1 | 1 | Instant on death |
| Pure Nox per Imp kill | 1 | TBD | Stub so majors are testable |
| Minor: directional dash | 2 Nox | | Dash uses move input dir |
| Minor: barrier reload | 2 Nox | | Instant full clip on barrier end |
| Major: dash momentum | 5 Pure Nox | | +50% move 2s after dash |
| Major: barrier nova | 5 Pure Nox | | 50 dmg / 4m / knockback |
| Ink pool player slow | 50% | 40–60% | While feet in pool |

---

## Open tuning questions

- Is Imp windup (0.45s) readable enough without feeling sluggish?
- Should barrier block reload, or stay ammo-neutral only?
- Target time-to-kill on a single Imp for a fresh player (~seconds)?

---

*Last synced with code: v0.6.0*
