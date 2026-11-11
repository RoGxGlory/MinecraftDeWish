# GDD — Player Vitals (Health, XP, Level, Death, Respawn)

| Field | Value |
|---|---|
| Status | Draft (2026-11-11) |
| System owner | game-designer / systems-designer / gameplay-programmer |
| Related | `Docs/SYSTEMS_REFERENCE.md` §8, `Docs/GAME_DESIGN.md` §6.1 (historical), `design/gdd/mob-spawning.md`, `design/gdd/mob-zombie.md`, `design/gdd/mob-skeleton.md`, `design/gdd/mob-spider.md`, `design/gdd/mob-creeper.md`, `design/gdd/items-loot.md`, `design/gdd/save-load.md`, `design/gdd/weather-system.md` |
| Source of truth for current code | `UPlayerVitalSystem` (`PlayerVitalSystem.h/.cpp`), `AXPOrbPickup`, `AMobCharacter`, `BP_HorrorCharacter` |
| Decisions | D10 (UI direction: vignette + audio, no health bars) |

---

## 1. Overview

`UPlayerVitalSystem` is a `UActorComponent` attached to the player pawn and is the
single authority for everything that keeps the player alive and progressing:
**health**, **natural regeneration**, **damage intake**, **healing**, **experience
(XP)**, **leveling**, **death**, and **respawn**. No other system mutates player
health directly; mobs, explosions, fall damage and environmental hazards all
route through `ApplyDamage`, and recovery routes through `Heal` / `Respawn`.

The system is deliberately small and event-driven. It holds no `Tick` of its own
for combat; instead it exposes four delegates — `OnHealthChanged`,
`OnXPChanged`, `OnLevelUp`, `OnPlayerDied` — that the HUD, audio, mob AI and
save system subscribe to. Regeneration is the only per-frame concern and is
driven by a timer, not `Tick` (per the project's no-`Tick`-unless-necessary
rule).

In one sentence: **the vitals system turns damage, healing, XP and death into
broadcast events, so the rest of the game reacts without ever touching health
directly.**

---

## 2. Player Fantasy

> *"You are not a health bar. You are a heartbeat."*

The player must never see a number that says "you have 14 HP left." Per decision
D10, health is communicated through **sensation, not UI**: the screen edges
bloom red when hurt, the world desaturates as death approaches, and a low
heartbeat thumps in the audio when the player is one hit from dying. The fantasy
is **fragility made legible** — the player *feels* how close they are to death
without ever being told.

Three beats the vitals system must deliver:

- **The first hit.** The player, fresh in the world, takes a Zombie punch. The
  vignette flashes, a wet thud plays, and the world tilts slightly. They learn,
  instantly, that this game hurts and that health is precious.
- **The regen relief.** The player, bleeding after a fight, ducks behind a
  wall. Over the next few seconds the red fades and the heartbeat slows. They
  learn that *time* is a resource — survive 4 seconds and you get a heart back.
- **The death and return.** The player dies to a Creeper. The world freezes,
  the camera ragdolls, a death stinger plays, and after a beat they wake up on
  the surface with full health — but the XP they were carrying is still theirs.
  They learn that death is a setback, not a reset, and that the surface is the
  safe harbour.

The system must never feel like a spreadsheet. A player who cannot explain
"why did I die there?" or "why didn't I regen?" is a design failure. Every
state change (damage, regen, level-up, death, respawn) must be traceable to a
visible or audible cue.

---

## 3. Detailed Rules

### 3.1 System identity

`UPlayerVitalSystem` (`UActorComponent`), created as a subobject of the player
pawn (`BP_HorrorCharacter` / `AHorrorCharacter`):

| Property | Default | Unit | Meaning |
|---|---|---|---|
| `MaxHealth` | 20.0 | HP | Maximum health |
| `CurrentHealth` | 20.0 | HP | Current health |
| `bRegenEnabled` | true | — | Master switch for natural regeneration |
| `RegenRate` | 0.25 | HP/s | Natural regeneration rate (1 HP every 4 s) |
| `CurrentXP` | 0 | XP | Experience points accumulated this level |
| `XPToNextLevel` | 7 | XP | XP required to reach the next level |
| `Level` | 0 | levels | Current player level |
| `XPProgress` | 0.0 | 0..1 | Normalised progress toward next level (CurrentXP / XPToNextLevel) |
| `bIsAlive` | true | — | Alive/dead state (derived; true iff CurrentHealth > 0) |

All tunable values are `EditAnywhere` / data-driven. No magic numbers in code.

### 3.2 Health

- Health is a floating-point value in `[0, MaxHealth]`.
- Health never exceeds `MaxHealth` (clamped on heal and regen) and never goes
  below `0` (clamped on damage).
- The player starts a new game / respawn at `CurrentHealth = MaxHealth`.
- Health is the only vital that gates survival: when `CurrentHealth <= 0` the
  player dies (§3.5).

### 3.3 Damage intake

`ApplyDamage(float DamageAmount, AActor* DamageSource)` is the **only** entry
point for reducing health. All damage — mob melee, Creeper detonation, fall
damage, fire/lava, suffocation — must call it. Direct attribute mutation is
forbidden.

Rules:
1. If `DamageAmount <= 0`, the call is a no-op (returns current health). Negative
   damage is never interpreted as healing (use `Heal`).
2. If the player is already dead (`bIsAlive == false`), the call is a no-op.
3. Otherwise: `CurrentHealth = max(0, CurrentHealth - DamageAmount)`.
4. Broadcast `OnHealthChanged(CurrentHealth, MaxHealth, DamageAmount)` where
   `DamageAmount` is the **positive** damage applied (clamped to the health that
   was actually lost, so overkill does not report more than `MaxHealth`).
5. If `CurrentHealth <= 0`: trigger death (§3.5).
6. Return `CurrentHealth`.

`DamageSource` is recorded for attribution (which mob, which explosion) and
may be null for environmental damage. It is not dereferenced without `IsValid()`.

#### 3.3.1 Damage types (design taxonomy)

| Damage type | Source | Notes |
|---|---|---|
| Melee | Zombie, Skeleton, Spider | Per-mob `MeleeDamage` (see mob GDDs) |
| Explosion (radial) | Creeper detonation | Radial falloff; max damage at centre |
| Fall | Gravity / drop | Threshold-based; not yet implemented (see §6) |
| Fire / Lava | Environment | Damage-over-time while in hazard volume |
| Suffocation | Environment | When head is inside a solid block |
| Angel | Weeping Angel (future) | Per `design/gdd/angels.md` |

Only Melee and Explosion are wired in v1; the rest are documented intent and
flagged as not-yet-implemented (§6).

### 3.4 Healing

`Heal(float HealAmount)` restores health up to `MaxHealth`.

Rules:
1. If `HealAmount <= 0`, no-op.
2. If dead, no-op (a dead player cannot be healed; use `Respawn`).
3. `CurrentHealth = min(MaxHealth, CurrentHealth + HealAmount)`.
4. Broadcast `OnHealthChanged(CurrentHealth, MaxHealth, -HealAmount)` (negative
   `DamageAmount` signals healing).
5. Return `CurrentHealth`.

Healing sources in v1: natural regeneration (§3.6), possibly food/potions (out
of scope for this GDD — see `design/gdd/items.md`).

### 3.5 Death

Triggered when `CurrentHealth` reaches `0` after `ApplyDamage`.

Death sequence:
1. Set `bIsAlive = false`, `CurrentHealth = 0`.
2. Broadcast `OnPlayerDied()` (once; guarded against re-entry).
3. Enter the **dead state**: ragdoll / freeze the player pawn (visual + physics),
   disable input and interaction, stop regeneration.
4. Mobs that were targeting the player drop aggro (they return to patrol/idle
   per their StateTree).
5. XP and level are **retained** (no XP loss on death in v1). See §7 tuning
   knob `XPLossOnDeath`.
6. The player's inventory is **retained** in v1 (no item drop on death). See
   §6 / `design/gdd/save-load.md` for future options.

The dead state persists until `Respawn` is called (by the death screen / timer).

### 3.6 Natural regeneration

While alive, regen runs on a timer (default 1.0 s tick) rather than `Tick`:

```
if bIsAlive and bRegenEnabled and CurrentHealth < MaxHealth:
    CurrentHealth = min(MaxHealth, CurrentHealth + RegenRate * dt)
    broadcast OnHealthChanged(CurrentHealth, MaxHealth, 0.0f)   // 0 = passive, no damage event
```

- `RegenRate = 0.25 HP/s` → 1 HP every 4 s.
- Regen only applies while `CurrentHealth < MaxHealth`; at full health it is a
  no-op (no broadcast, no work).
- Regen pauses while dead.
- v1 has **no damage cooldown**: regen begins immediately after the last damage.
  A "regen delay after damage" gate is a documented future enhancement (§7
  `RegenDelayAfterDamage`), not implemented.

### 3.7 Experience (XP) and leveling

`AddXP(int32 Amount)` awards experience and advances levels.

Rules:
1. If `Amount <= 0`, no-op.
2. If dead, XP is still credited (progress is not lost on death), but
   `OnLevelUp` may be deferred? v1 credits immediately; level-up broadcasts
   immediately. (Flagged as a tuning decision; see §7.)
3. `CurrentXP += Amount`.
4. While `CurrentXP >= XPToNextLevel`: level up:
   a. `CurrentXP -= XPToNextLevel`
   b. `Level += 1`
   c. `XPToNextLevel = 7 + (Level × 3)` (formula uses the **new** level)
   d. Recompute `XPProgress = CurrentXP / XPToNextLevel`
   e. Broadcast `OnLevelUp(Level)`
   f. Repeat (a player can gain multiple levels from a single large XP award).
5. After the loop, broadcast `OnXPChanged(CurrentXP, XPToNextLevel, Level)`.
6. `XPProgress = CurrentXP / XPToNextLevel` (clamped to `[0, 1)`).

The XP curve is `XPToNextLevel(L) = 7 + (L × 3)`, where `L` is the **current**
level before the next level-up. Concretely:

| Current level L | XP to reach L+1 |
|---|---|
| 0 | 7 |
| 1 | 10 |
| 2 | 13 |
| 3 | 16 |
| 10 | 37 |

#### 3.7.1 XP sources

- `AXPOrbPickup` contact deposits XP into `UPlayerVitalSystem::AddXP` (orb
  value from `FMobDropEntry` / mob GDDs).
- Future: quest rewards, crafting, exploration (out of scope v1).
- XP is **not** granted by mining blocks in v1 (mining grants nothing); only
  mob kills and pickups do.

### 3.8 Respawn

`Respawn(const FVector& SpawnLocation)` restores the player to a playable state
at `SpawnLocation`.

Rules:
1. `CurrentHealth = MaxHealth`.
2. `bIsAlive = true`.
3. Move the pawn to `SpawnLocation` (surface of terrain; the caller — death
   screen / spawn manager — is responsible for choosing a safe surface point).
4. Reset the dead-state (un-ragdoll, re-enable input/interaction).
5. Broadcast `OnHealthChanged(MaxHealth, MaxHealth, 0.0f)`.
6. Regeneration resumes.
7. XP and level are unchanged.

`SpawnLocation` is provided by the caller; the vitals system does not compute
it. The death/respawn flow should pick the world spawn point or the last safe
surface (see `design/gdd/save-load.md`).

### 3.9 Delegates

| Delegate | Signature | Fired when |
|---|---|---|
| `OnHealthChanged` | `(float NewHealth, float MaxHealth, float DamageAmount)` | Any health change: damage (positive), heal (negative), regen (0), respawn |
| `OnXPChanged` | `(int32 CurrentXP, int32 XPToNextLevel, int32 Level)` | XP gained or level-up (after `OnLevelUp`) |
| `OnLevelUp` | `(int32 NewLevel)` | A level boundary is crossed |
| `OnPlayerDied` | `()` | Death triggered (once per death) |

Consumers: HUD vignette/audio (health), XP popup & level indicator (XP/level),
mob AI (death → drop aggro), save system (persist on change), achievements
(future).

### 3.10 HUD / UI direction (D10)

Per decision D10, the UI shows **no health bar and no numeric HP**. Instead:

- **Damage vignette**: red radial gradient at screen edges; opacity driven by
  `1 - (CurrentHealth / MaxHealth)`, with a brief flash on damage.
- **Low-health audio**: heartbeat loop whose volume/pitch scales with missing
  health; triggers below ~30% health (6 HP).
- **Death screen**: full-screen overlay on `OnPlayerDied`, with respawn prompt.
- **XP / level**: small, non-intrusive indicator (orb fill + level number) that
  pulses on `OnXPChanged` / `OnLevelUp`. Not a persistent bar; appears on gain
  and fades.
- **Damage number popups** (optional, off by default): floating numbers at the
  hit location for debugging; disabled in release per D10's "no bars" spirit.

The HUD is a **consumer** of the delegates; it never owns game state and never
mutates vitals.

---

## 4. Formulas

### 4.1 Health clamp

```
CurrentHealth = clamp(CurrentHealth, 0, MaxHealth)
```

Applied after every damage, heal, regen tick and respawn.

### 4.2 Damage application

```
if DamageAmount <= 0 or not bIsAlive: return CurrentHealth
lost = min(DamageAmount, CurrentHealth)
CurrentHealth = CurrentHealth - lost          // clamped to >= 0
broadcast OnHealthChanged(CurrentHealth, MaxHealth, lost)
if CurrentHealth <= 0: trigger death
return CurrentHealth
```

### 4.3 Heal application

```
if HealAmount <= 0 or not bIsAlive: return CurrentHealth
gained = min(HealAmount, MaxHealth - CurrentHealth)
CurrentHealth = CurrentHealth + gained
broadcast OnHealthChanged(CurrentHealth, MaxHealth, -gained)
return CurrentHealth
```

### 4.4 Natural regeneration (per tick)

```
dt = time since last regen tick (s)
if bIsAlive and bRegenEnabled and CurrentHealth < MaxHealth:
    CurrentHealth = min(MaxHealth, CurrentHealth + RegenRate * dt)
    if CurrentHealth changed: broadcast OnHealthChanged(CurrentHealth, MaxHealth, 0.0)
```

With `RegenRate = 0.25`: time to full from 0 HP = `MaxHealth / RegenRate` =
`20 / 0.25 = 80 s`.

### 4.5 XP to next level

```
XPToNextLevel(L) = 7 + (L * 3)     // L = current level
```

### 4.6 Cumulative XP to reach level N

```
TotalXP(N) = sum_{i=0}^{N-1} (7 + 3*i)
           = 7*N + 3 * N*(N-1)/2
```

Examples: Level 1 → 7 XP; Level 2 → 7 + 10 = 17 XP; Level 10 →
`70 + 3*45 = 205` XP.

### 4.7 XP gain and level-up loop

```
if Amount <= 0: return
CurrentXP += Amount
while CurrentXP >= XPToNextLevel and XPToNextLevel > 0:
    CurrentXP -= XPToNextLevel
    Level += 1
    XPToNextLevel = 7 + (Level * 3)
    broadcast OnLevelUp(Level)
XPProgress = CurrentXP / XPToNextLevel   // 0 <= XPProgress < 1
broadcast OnXPChanged(CurrentXP, XPToNextLevel, Level)
```

### 4.8 Death trigger

```
if CurrentHealth <= 0 and bIsAlive:
    bIsAlive = false
    CurrentHealth = 0
    broadcast OnPlayerDied()
    enter dead state (ragdoll/freeze, disable input)
```

### 4.9 Respawn

```
CurrentHealth = MaxHealth
bIsAlive = true
teleport pawn to SpawnLocation
exit dead state
broadcast OnHealthChanged(MaxHealth, MaxHealth, 0.0)
```

### 4.10 Vignette intensity (HUD, informational)

```
VignetteOpacity = 1 - (CurrentHealth / MaxHealth)     // 0 at full, 1 at death
LowHealthFlag   = CurrentHealth <= 0.3 * MaxHealth    // 6 HP at default
```

---

## 5. Edge Cases

- **Damage of 0 or negative**: `ApplyDamage` is a no-op; no broadcast, no death
  check. Negative damage never heals.
- **Heal of 0 or negative**: no-op.
- **Overkill damage** (e.g. 100 damage at 5 HP): `lost` is clamped to remaining
  health (5); `OnHealthChanged` reports 5, not 100. Death triggers once.
- **Damage while dead**: no-op; prevents double death / re-broadcast.
- **Heal while dead**: no-op; a dead player must be respawned, not healed.
- **Heal at full health**: `gained = 0`; broadcast reports 0 (or no broadcast —
  implementation should skip zero-delta broadcasts to avoid HUD spam).
- **Regen at full health**: no-op; no broadcast.
- **Regen while dead**: disabled.
- **Multiple level-ups in one `AddXP`**: loop handles it; `OnLevelUp` fires
  once per level crossed, `OnXPChanged` fires once at the end.
- **`AddXP(0)` or negative**: no-op.
- **XP overflow / very large award**: loop terminates because `XPToNextLevel`
  grows linearly (`+3` per level); no infinite loop. Guard `XPToNextLevel > 0`.
- **`MaxHealth` changed at runtime**: `CurrentHealth` is clamped to the new max;
  if the new max is lower than current health, health is reduced (no death
  triggered by the clamp itself; death only via `ApplyDamage`).
- **`MaxHealth` set to 0 or negative**: treated as configuration error; clamp
  to a minimum of 1 to avoid division-by-zero in `XPProgress`-style ratios and
  vignette math. Log a warning.
- **Respawn at an unsafe location** (inside a block, in lava): the vitals system
  does not validate; the caller (spawn manager / death screen) must pick a safe
  surface point. If invalid, the player may suffocate / burn — that is the
  caller's bug, not the vitals system's.
- **Death during a Creeper detonation / explosion chain**: `OnPlayerDied` fires
  once; subsequent damage calls are no-ops. The death screen appears after the
  current frame's damage resolution.
- **Player dies while a mob is mid-attack**: the mob's attack completes its
  animation but deals no further damage (player is dead). Mob returns to
  patrol/idle.
- **Regen timer and pause**: regen uses game time, not wall clock; pausing the
  game pauses regen. No regen during loading screens.
- **Level 0 start**: a brand-new player is level 0 and needs 7 XP for level 1.
  No "level 1 at spawn" special case.
- **XP orb picked up while dead**: XP is credited (progress retained) but no
  level-up fanfare until respawn? v1 credits immediately; level-up broadcasts
  immediately. Flagged in §7 (`LevelUpWhileDead`).
- **Concurrent `ApplyDamage` calls in one frame** (e.g. two mobs hitting): each
  is processed in order; death triggers on the call that crosses 0; later calls
  in the same frame are no-ops.

---

## 6. Dependencies

| System | Why | Status |
|---|---|---|
| `UPlayerVitalSystem` | This document's subject | Implemented |
| Player pawn (`BP_HorrorCharacter` / `AHorrorCharacter`) | Hosts the component; ragdoll/freeze, input gating | Implemented (basic) |
| `AMobCharacter` | Damage source (melee); reads player health indirectly via damage application | Implemented |
| `AXPOrbPickup` | Deposits XP via `AddXP` on contact | Implemented |
| `UBaublesSystem` | Sword damage feeds mob melee (indirect) | Implemented |
| HUD / vignette widget (`WBP_HUD_Vitals` or equivalent) | Consumer of `OnHealthChanged`, `OnXPChanged`, `OnLevelUp`, `OnPlayerDied` | Not implemented (D10 direction set; widget TBD) |
| Audio (heartbeat, death stinger) | Consumer of health/low-health/death events | Not implemented |
| `design/gdd/mob-*.md` | Per-mob damage values, XP drop amounts | Draft |
| `design/gdd/items-loot.md` | XP orb values, drop tables | Draft |
| `design/gdd/save-load.md` | Persistence of health/XP/level on save | Draft |
| `design/gdd/angels.md` | Angel damage (future) | Draft |
| Fall damage / fire / suffocation | Environmental damage types | Not implemented (§3.3.1) |
| Death screen / respawn manager | Calls `Respawn` with a safe surface point | Not implemented |

Bidirectional note: `mob-spawning.md` §6 lists `UPlayerVitalSystem` as an
indirect dependency (damage/death on the player side); this document lists the
mob system as a damage source.

---

## 7. Tuning Knobs

| Knob | Default | Range | Effect |
|---|---|---|---|
| `MaxHealth` | 20.0 | 1–100 | Health ceiling; higher = tankier |
| `RegenRate` | 0.25 HP/s | 0.0–2.0 | Natural heal speed; 0 disables regen |
| `bRegenEnabled` | true | — | Master regen switch (e.g. for "hardcore" mode) |
| `RegenTickInterval` | 1.0 s | 0.1–2.0 | Timer cadence for regen (not `Tick`) |
| `RegenDelayAfterDamage` | 0.0 s (disabled) | 0.0–10.0 | Future: seconds after damage before regen resumes |
| `XPBase` (in `7 + Level×3`) | 7 | 1–20 | Base XP per level; raises early-game pacing |
| `XPSlope` (in `7 + Level×3`) | 3 | 0–10 | XP growth per level; 0 = flat curve |
| `XPLossOnDeath` | 0 (none) | 0–100% | Future: fraction of XP lost on death |
| `LevelUpWhileDead` | true (credit immediately) | — | Whether XP/level-up applies while dead |
| `LowHealthThreshold` | 0.3 (30%) | 0.1–0.5 | Health fraction below which heartbeat audio triggers |
| `DamageVignetteFlashDuration` | 0.15 s | 0.05–0.5 | Brief flash on damage |
| `RespawnDelay` | 3.0 s | 0.0–10.0 | Death-screen-to-respawn delay (caller-side) |
| `MinHealthForClamp` | 1.0 | 1–5 | Safety floor for `MaxHealth` to avoid div-by-zero |

All knobs are `EditAnywhere` and data-driven. No knob value may be hardcoded in
C++.

---

## 8. Acceptance Criteria

1. A new player starts with `CurrentHealth = MaxHealth = 20`, `Level = 0`,
   `CurrentXP = 0`, `XPToNextLevel = 7`, `bIsAlive = true`.
2. `ApplyDamage(5, source)` reduces health to 15, broadcasts
   `OnHealthChanged(15, 20, 5)`, and does not trigger death.
3. `ApplyDamage(100, source)` at 5 HP reduces health to 0, broadcasts
   `OnHealthChanged(0, 20, 5)` (clamped lost), and triggers `OnPlayerDied()`
   exactly once.
4. `ApplyDamage(0)` or negative is a no-op: no health change, no broadcast, no
   death.
5. `ApplyDamage` while dead is a no-op.
6. `Heal(5)` at 15 HP raises health to 20, broadcasts
   `OnHealthChanged(20, 20, -5)`.
7. `Heal` while dead is a no-op.
8. `Heal` at full health results in no change (and no zero-delta broadcast).
9. Natural regen at 0.25 HP/s raises health from 0 to 20 in 80 s; at full
   health regen does nothing.
10. Regen does not run while dead.
11. `AddXP(7)` at level 0 sets `Level = 1`, `CurrentXP = 0`,
    `XPToNextLevel = 10`, and fires `OnLevelUp(1)` then `OnXPChanged(0, 10, 1)`.
12. `AddXP(17)` at level 0 sets `Level = 2`, `CurrentXP = 0`,
    `XPToNextLevel = 13` (two level-ups: 7 then 10), firing `OnLevelUp` twice.
13. `AddXP(0)` or negative is a no-op.
14. `Respawn(location)` sets `CurrentHealth = MaxHealth`, `bIsAlive = true`,
    moves the pawn, and broadcasts `OnHealthChanged(MaxHealth, MaxHealth, 0)`.
    XP and level are unchanged.
15. `XPToNextLevel(L) = 7 + L*3` for L = 0, 1, 2, … (verified for at least
    L = 0..10).
16. `TotalXP(10) = 205` (cumulative formula).
17. The HUD shows no health bar and no numeric HP; vignette opacity equals
    `1 - CurrentHealth/MaxHealth`; heartbeat audio triggers below 30% health.
18. `OnPlayerDied` fires exactly once per death; subsequent damage calls in the
    same frame are no-ops.
19. All tunable values (`MaxHealth`, `RegenRate`, XP base/slope, thresholds)
    are `EditAnywhere` and not hardcoded in C++.
20. An automation test exists for: damage clamp, heal clamp, death trigger,
    regen rate, XP curve, multi-level-up, respawn restoration, and the
    no-op edge cases (zero/negative damage, damage-while-dead).