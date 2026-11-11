# GDD — Mob Spawning System

| Field | Value |
|---|---|
| Status | Draft (2026-11-11) |
| System owner | game-designer / systems-designer / ai-programmer |
| Related | `Docs/MOB_REGISTRY.md`, `Docs/SYSTEMS_REFERENCE.md` §10–11, `design/gdd/mob-zombie.md`, `design/gdd/mob-skeleton.md`, `design/gdd/mob-spider.md`, `design/gdd/mob-creeper.md`, `design/gdd/weather-system.md`, `design/gdd/lighting-power.md`, `design/gdd/light-and-power.md` §3.1, `design/gdd/angels.md` |
| Source of truth for code | `UMobSpawnerSystem`, `AMobCharacter`, `UDayNightCycleSystem`, `AWorldGenerator` |

---

## 1. Overview

`UMobSpawnerSystem` is the ecological engine that decides **where and when** hostile
creatures enter the world. It is a `UActorComponent` created as a subobject of
`AWorldGenerator` and is the single authority for hostile-mob population. It owns
four concerns:

1. **Spawn cycle** — every `SpawnInterval` (2.0 s) it attempts to introduce new
   mobs into dark, valid locations within 24–128 blocks of the player.
2. **Spawn gating** — a candidate location must satisfy light, distance, surface,
   cap and weather rules before a mob is instantiated.
3. **Despawn** — mobs further than 128 m from the player are removed, freeing
   cap slots and keeping the world from becoming a zoo.
4. **Population cap** — a global ceiling of `MaxMobCap = 20` hostile mobs, shared
   across all four types (Zombie, Skeleton, Spider, Creeper).

The spawner is the reason darkness is never safe: it converts every unlit cave
mouth, unlit cellar and shadowed corner into a potential encounter. It does not
decide *how* a mob behaves once spawned — that is `AMobCharacter` and the StateTree
(`ST_Mob_Behavior`). It only decides *whether* a spawn happens and *where*.

The spawner is intentionally simple and deterministic: given the same world state,
seed and elapsed time, it produces the same population. This makes balance
tunable and automation-testable.

---

## 2. Player Fantasy

> *"The torch is not decoration. It is a border."*

The player should feel that **darkness is a resource the world spends against them**.
Every unlit block is a potential spawn point, and the spawner makes that threat
legible: the player learns, through a single death, that a dark cave mouth at
dusk is not an invitation but a trap. The fantasy is the **lit perimeter** — the
moment the player realises that placing torches is not optional decoration but
the primary defensive act of the early game.

Three beats the spawner must deliver:

- **The first spawn.** The player, at night, hears a groan behind them. They turn:
  a Zombie has appeared at the edge of their torch's light. They learn that the
  darkness *behind* them was never empty.
- **The torch lesson.** The player places a torch at a cave entrance; spawns stop
  there and resume one block further in. They learn the 5-block protection radius
  by experiment, not by tooltip.
- **The cap relief.** The player, surrounded by 20 mobs, realises that no new ones
  appear until some die or despawn. The world is hostile but **finite**; the
  player can win by attrition if they hold the line.

The spawner must never feel arbitrary. A spawn that the player cannot explain
("why did that appear *there*?") is a design failure. Every spawn must be
traceable to a dark, valid, in-range surface.

---

## 3. Detailed Rules

### 3.1 System identity

`UMobSpawnerSystem` (`UActorComponent`), created by `AWorldGenerator`:

| Property | Default | Unit | Meaning |
|---|---|---|---|
| `MaxMobCap` | 20 | mobs | Global ceiling on live hostile mobs |
| `MinSpawnDistance` | 2400.0 | cm (24 blocks) | Closest a mob may spawn to the player |
| `MaxSpawnDistance` | 12800.0 | cm (128 blocks) | Farthest a mob may spawn; also the despawn distance |
| `SpawnInterval` | 2.0 | s | Time between spawn cycles |
| `TorchLightRadius` | 5 | blocks | Torch protection radius for spawn gating |
| `DespawnDistance` | 12800.0 | cm (128 blocks) | Distance beyond which mobs despawn (separate from MaxSpawnDistance) |
| `bSpawningEnabled` | true | - | Master switch for the spawn system |

All properties are `EditAnywhere` and data-driven (no magic numbers in code). They are
the top-level tuning knobs; per-mob stats live in `DT_Mobs` / the mob GDDs.

### 3.2 Spawn cycle

`ProcessSpawnCycle()` is called every `SpawnInterval` seconds (driven by a timer,
not `Tick`). One cycle performs the following:

1. **Count** the live hostile mobs in the world (`CurrentMobCount`).
2. **Compute the attempt budget** for this cycle:
   ```
   BaseAttempts  = RandRange(1, 4)                                    // current code
   SpawnAttempts = max(0, round(BaseAttempts * CurrentMobSpawnRateMultiplier))
   ```
   There is no upper clamp in the current implementation; the loop also stops
    as soon as `ActiveMobs.Num() >= MaxMobCap`. A future `MaxSpawnAttemptsPerCycle`
    clamp (design intent: 8) may be added for tuning, but is not enforced today.
   (See §3.10 for the weather multiplier.)
3. **For each attempt** (up to `Attempts`), repeat until a spawn succeeds or the
   budget is exhausted:
   a. Pick a random player (single-player: the one player). In multiplayer, run
      the cycle per player with a shared global cap.
   b. Pick a random candidate point in the annulus
      `[MinSpawnDistance, MaxSpawnDistance]` around the player (current code draws the distance uniformly in radius,
      which biases spawns toward the outer ring; an area-weighted pick is design
      intent - see §3.5 / §4.4).
   c. Snap the candidate to the nearest valid surface (see §3.6).
   d. Evaluate `CanSpawn` (§3.3). If false, discard and try again.
   e. Roll a mob type from the spawn-weight table (§3.8).
   f. Instantiate the mob via `SpawnMob(MobClass, Location)` and increment
      `CurrentMobCount`.
   g. If `CurrentMobCount >= MaxMobCap`, stop the cycle immediately.
4. **Record** the cycle timestamp for the next interval.

A cycle that spawns zero mobs is normal and expected (e.g. the player is
standing in full daylight on a flat plain). The cycle never spawns more than one
mob per successful attempt, and never exceeds the cap.

### 3.3 Spawn conditions (`CanSpawn`)

A candidate location is eligible **only if all** of the following hold:

```
CanSpawn(candidate, player) =
    LightLevel(candidate) < LightLevelThreshold          // default threshold 7
    AND MinSpawnDistance <= Distance(player, candidate) <= MaxSpawnDistance
    AND ValidSurface(candidate)                          // §3.6
    AND CurrentMobCount < MaxMobCap
    AND NOT WithinTorchRadius(candidate)                 // §3.4 / §3.11
    AND MobTypeEligible(candidate, timeOfDay, weather)  // §3.7
```

- `LightLevelThreshold = 7` (Minecraft-style; matches `AngelLightThreshold` in
  `design/gdd/angels.md` §4 and `design/gdd/light-and-power.md` §3.1).
- Distance is measured in blocks (1 block = 100 cm) between the player's feet
  and the candidate spawn point.
- `MobTypeEligible` is **design intent, not yet enforced by code**. The current
  implementation relies on the light gate alone: an exposed surface during daytime
  has `LightLevel = 15` (§3.4), so burning mobs cannot spawn there. A shaded
  surface during daytime has `LightLevel < 7` and *does* allow burning mobs today.
  The stricter "burning mobs excluded from daytime surfaces even in shadow"
  rule is a design target for a future revision (see §3.7 and §8).

### 3.4 Light level calculation

`CalculateLightLevel(const FIntVector& VoxelPos) → int32` in `[0, 15]`, combining
torch proximity with sky exposure, exactly as documented in
`Docs/SYSTEMS_REFERENCE.md` §10:

```
LightLevel(p) = max( SkyLight(p), TorchLight(p) )   // already in [0, 15]

SkyLight(p)   = SkyExposure(p) ? (IsDay() ? 15 : 4) : 0
                // SkyExposure: true if no solid opaque block above p
                // Daytime exposed surface = 15; nighttime exposed = 4 (moonlight)
                // WeatherLightFactor: from weather-system.md §3.4 (Ambient light column)

TorchLight(p) = HasNearbyTorch(p) ? 14 : 0
                // Binary: a torch within the radius sets light to 14 (>= 7)
                // No distance falloff beyond the radius in the current code
```

Consequences:

- A torch within `TorchLightRadius` (5) blocks sets the light level to 14
  (`≥ 7`), so **no mob spawns within 5 blocks of a torch**. This is the "torch
  avoidance (5 blocks)" rule.
- Beyond 5 blocks from any torch, torch light is treated as 0 for spawn gating
  (the torch's contribution is ignored), so a dark corridor 6 blocks from a
  torch is a valid spawn zone. This is a deliberate simplification; raising
  `TorchLightRadius` widens protection.
- Sky exposure dominates during the day: an open surface at noon has `SkyLight = 15`,
  so no surface spawns. At night, `SkyLight = 4` (moonlight), which is still `< 7`,
  so surface spawns remain possible at night on open ground.
- **Weather and light (current implementation):** `CalculateLightLevel` does *not*
  consult weather today. Weather affects spawning only through the attempt-count
  multiplier (§3.10) and, indirectly, aggression. Modulating sky light by
  weather (`WeatherLightFactor`) is design intent for a future revision; until
  then, Fog/Ash Storm spawn zones are widened only by the 1.3x/1.5x attempt
  multiplier, not by darker light levels.

### 3.5 Distance (annulus)

Spawn distance is measured from the player's feet position to the candidate spawn
point, in blocks:

```
d = |candidate - player|   (blocks, 1 block = 100 cm)

Eligible distance:  MinSpawnDistance <= d <= MaxSpawnDistance
                    24 blocks <= d <= 128 blocks
```

- The inner bound (24) prevents mobs spawning on top of the player or just
  outside their view; the player always has a short "safe bubble" around them.
- The outer bound (128) keeps spawns within the loaded/streamed world and
  within the despawn radius.
- Boundary values are **inclusive**: a spawn at exactly 24 or exactly 128 blocks
  is allowed.
- In the current implementation the candidate distance is drawn uniformly over
  *radius* (`FRandRange(MinSpawnDistance, MaxSpawnDistance)`), which biases
  spawns toward the outer ring. An area-weighted pick is design intent for a
  future revision; see §4.4.

### 3.6 Valid surface

A candidate snaps to a valid surface before eligibility is checked. A valid
surface is:

- The **top face** of a solid, opaque block (not air, not liquid, not a torch,
  not a non-solid like leaves/grass in their non-solid state — leaves are
  treated as solid for surface purposes but spawns on leaf canopies are allowed
  and intended).
- The block **above** the surface is air for at least 2 blocks of vertical
  headroom (mob height + clearance). A 1-block ceiling disqualifies the spot.
- The block **below** the surface is solid (no floating spawns).
- The surface is **not** liquid (water/lava). Mobs do not spawn in liquid.
- The surface is **not** inside a block (no embedded spawns).
- For **Spider** spawns specifically, the surface must be horizontal (Spiders
  do not spawn on vertical walls; they climb onto walls only after spawning).

Surfaces that qualify: cave floors, dungeon floors, unlit room floors, the
underside of overhangs (if the block above is solid and headroom exists), leaf
canopies, ruined structure floors. Surfaces that do not: water, lava, torches,
single-block-thick platforms over void (no support below), the top of a
Creeper crater rim if headroom < 2.

### 3.7 Time of day and mob eligibility

The spawn gate is light-level based, but two extra rules apply:

- **Burning mobs (Zombie, Skeleton):** design intent is that they may spawn on
  the **surface** only when `IsNight()` is true OR the candidate is enclosed
  (sky exposure = 0). During the day, an exposed surface is rejected for these
  types. **Current implementation:** this is enforced indirectly - an exposed
  daytime surface has `LightLevel = 15` (§3.4), so the light gate rejects it.
  A shaded daytime surface (e.g. under a tree canopy) has `LightLevel < 7` and
  *does* allow burning mobs today. The stricter exclusion is a design target.
  They may still spawn in caves and interiors at any time.
- **Non-burning mobs (Spider, Creeper):** no time-of-day restriction. They spawn
  wherever the light gate passes, day or night, surface or cave. Because they do
  not burn, a Spider or Creeper spawned in a dark cave at night can wander onto
  the surface at dawn and survive — this is intended and is their daytime threat
  niche.

This rule is what makes **Spiders and Creepers the daytime surface threat** while
**Zombies and Skeletons are the nighttime/cave threat**, without needing a
separate "time of day" spawn table.

### 3.8 Mob type selection (spawn weights)

When a spawn is approved, the mob type is chosen by a weighted roll. Proposed
default weights (tunable per biome/weather in a future revision):

| Mob type | Base weight | Notes |
|---|---|---|
| Zombie | 40 | Baseline, most common |
| Skeleton | 25 | Ranged threat, slightly rarer |
| Spider | 20 | Agile threat, cave/surface |
| Creeper | 15 | Silent ambush, rarest |

Weights are normalised to sum 100. A weight of 0 disables a type (useful for
biome-specific tables later). Weather does not change the *mix* in v1 — only the
*rate* (§3.10). If a future biome system is added, weights become a per-biome
table; the global default above remains the fallback.

### 3.9 Cap management

- `MaxMobCap = 20` is a **global** ceiling across all four hostile types and all
  players (in multiplayer, a single shared cap; per-player caps are a future
  option).
- `CurrentMobCount` is recomputed at the start of each cycle by counting live
  `AMobCharacter` instances (or maintained incrementally via spawn/despawn/death
  events — incremental is preferred for performance, with a periodic recount as
  a safety net).
- When `CurrentMobCount >= MaxMobCap`, the cycle spawns nothing. No attempt
  budget is consumed.
- Death (player kill, sunlight burn, explosion, fall) decrements the count.
  Despawn decrements the count.
- **Despawned mobs drop no loot and no XP** (Minecraft-style). Only deaths
  (player kills and environmental deaths such as sunlight burn) yield loot and
  XP. This prevents players from farming by walking away and back.
- Optional per-type sub-caps (`MaxZombies`, `MaxSkeletons`, etc.) are **not**
  enforced in v1; the global cap is the only gate. They are documented as tuning
  knobs for a future revision if one type dominates.

### 3.10 Weather modifiers

Weather (from `design/gdd/weather-system.md` §3.4) modulates the spawn cycle via
`UDayNightCycleSystem::GetMobSpawnRateMultiplier()` and
`GetMobAggressionMultiplier()`:

| Weather | Spawn rate mult. | Aggression mult. | Effect on spawning |
|---|---|---|---|
| Clear | 1.0 | 1.0 | Baseline |
| Overcast | 1.0 | 1.0 | Baseline |
| Rain | 0.7 | 0.8 | Fewer spawns; wet surfaces (Spider climbing reduced) |
| Fog | 1.3 | 1.2 | More spawns; spawn zones widened by lower ambient light |
| Ash Storm | 1.5 | 1.5 | Spawn surge; most dangerous weather |

- The spawn-rate multiplier scales the **attempt budget** (§3.2), not the per-
  spawn probability. A 1.5x multiplier turns a 4-attempt base roll into 6 attempts.
- The aggression multiplier scales **pursuit speed** (`MovementSpeed *
  CurrentMobAggressionMultiplier`, clamped to 1.5×) and is applied by
  `AMobCharacter`, not the spawner. The spawner only reads the spawn-rate
  multiplier.
- Weather transitions are smooth (30–60 s lerp); the multiplier lerps with them.
  No instant snap.
- Weather also lowers ambient light in the weather system (weather-system.md §3.4),
  but `CalculateLightLevel` does not consume it today (§3.4). Until that is wired,
  Fog and Ash Storm are dangerous through the attempt-count multiplier only, not
  through darker light levels.

### 3.11 Torch avoidance (summary)

A torch protects a 5-block radius from spawning. This is enforced through the
light-level gate (§3.4): within 5 blocks of a torch, `LightLevel >= 9 >= 7`, so
`CanSpawn` is false. No separate "torch exclusion list" is needed; the light
calculation is the single source of truth. Raising `TorchLightRadius` widens the
protected zone; lowering it narrows it.

### 3.12 Day/night and sunlight burning

Sunlight burning is a property of `AMobCharacter`, not the spawner, but the
spawner's population is shaped by it:

- Mobs with `bBurnsInSunlight = true` (Zombie, Skeleton) ignite and lose
  `BurnRate = 1 HP/s` while under open sky during daytime. They do not flee
  from sunlight; they die if they cannot reach shade. Burn-death yields loot
  and XP (a death is a death).
- Mobs with `bBurnsInSunlight = false` (Spider, Creeper) are fully active in
  any light once spawned.
- The spawner does **not** remove burning mobs at dawn. They burn and die
  naturally (or reach shade). This produces the "scattering at dawn" spectacle
  and frees cap slots through death, not despawn.
- Day/night is provided by `UDayNightCycleSystem` (`IsDay()`, `IsNight()`,
  `GetSkyLightIntensity()`).

### 3.13 Despawn

`ProcessDespawn()` is called every tick of `UMobSpawnerSystem` (tick interval
0.5 s), not on a separate 5-second timer. It removes a mob when:


```
Despawn(mob) if Distance(player, mob) > MaxSpawnDistance   // > 128 blocks
```

- Boundary is **strictly greater**: a mob at exactly 128 blocks is not despawned.
- Despawn is silent: no loot, no XP, no death animation (the mob simply
  ceases to exist). **Current implementation is single-player** (`GetFirstPlayerController`);
  the multiplayer "despawn only when no player is within 128 blocks" rule is design intent.
- Burning mobs and Creeper fuses are **not** specially protected today: a mob beyond
  128 blocks is despawned regardless of state. Protecting burning mobs (letting them
  finish burning) and in-progress Creeper detonations is design intent for a future
  revision.
- Despawn frees a cap slot for the next spawn cycle.

---

## 4. Formulas

### 4.1 Spawn cycle interval

```
CyclePeriod = SpawnInterval = 2.0 s
NextCycleTime = LastCycleTime + CyclePeriod
```

### 4.2 Light level

```
LightLevel(p) = max( SkyLight(p), TorchLight(p) )          // already in [0, 15]

SkyLight(p)   = SkyExposure(p) ? (IsDay() ? 15 : 4) : 0

TorchLight(p) = HasNearbyTorch(p) ? 14 : 0
```

Variables:

| Variable | Default | Unit | Meaning |
|---|---|---|---|
| `SkyExposure(p)` | bool | — | True if no solid opaque block above p |
| `IsDay()` | bool | — | From `UDayNightCycleSystem` (TimeOfDay in [0, 0.5)) |
| `HasNearbyTorch(p)` | bool | — | Any torch block within `TorchLightRadius` blocks (Euclidean sphere) |
| `TorchLightRadius` | 5 | blocks | Torch consideration radius |

### 4.3 Spawn eligibility predicate

```
CanSpawn(candidate, player, world) =
    LightLevel(candidate) < LightLevelThreshold
    AND MinSpawnDistance <= Distance(player, candidate) <= MaxSpawnDistance
    AND ValidSurface(candidate)
    AND CurrentMobCount < MaxMobCap
    AND MobTypeEligible(mobType, candidate, TimeOfDay)
```

### 4.4 Distance

```
d = sqrt( (cx - px)^2 + (cy - py)^2 + (cz - pz)^2 )   // in blocks
Eligible: 24 <= d <= 128

Note: the current implementation draws d ~ Uniform(Min, Max) (uniform in radius),
which biases candidates toward the outer ring. An area-weighted pick
(d ~ sqrt(Uniform(Min^2, Max^2))) is design intent for a future revision.
```

### 4.5 Cap

```
if CurrentMobCount >= MaxMobCap: no spawn this cycle
```

### 4.6 Effective attempt budget (weather-modulated)

```
BaseAttempts   = RandRange(1, 4)                                   // current code
SpawnAttempts  = max(0, round(BaseAttempts * CurrentMobSpawnRateMultiplier))

// No upper clamp in the current implementation; the loop also stops when
// ActiveMobs.Num() >= MaxMobCap. A MaxSpawnAttemptsPerCycle clamp (design intent: 8)
// is a future tuning addition.
```

Example: with a base roll of 4 and Ash Storm (1.5x): `round(4 * 1.5) = 6` attempts.

### 4.7 Weather-modulated spawn rate (combined view)

```
EffectiveSpawnRate = BaseSpawnRate * CurrentMobSpawnRateMultiplier
EffectivePursuitSpeed = MovementSpeed * CurrentMobAggressionMultiplier   // clamped to 1.5x
```

where `BaseSpawnRate` is implicit in the attempt budget (§4.6).

### 4.8 Sunlight burn time-to-death

```
BurnTTD = Health / BurnRate = Health / 1.0 = Health seconds
```

Zombie (20 HP) and Skeleton (20 HP) die in ~20 s in open daylight at full health.
Spider and Creeper are immune (`BurnRate = N/A`).

### 4.9 Despawn

```
Despawn if Distance(player, mob) > MaxSpawnDistance   // > 128 blocks
```

### 4.10 Spawn-weight roll

```
type = weighted_random_choice(Weights, rng)
// Weights: Zombie 40, Skeleton 25, Spider 20, Creeper 15 (sum 100)
// rng is seeded for determinism in tests
```

### 4.11 Determinism note

All random rolls (candidate point, type selection, surface snap jitter) use a
seeded RNG keyed on `(worldSeed, cycleIndex, playerIndex)`. This makes spawn
outcomes reproducible for automation tests and replay/debug. Production uses a
non-deterministic seed per world (world seed).

---

## 5. Edge Cases

- **Player stands in full daylight on a flat plain at noon:** `SkyLight = 15`
  everywhere in range, so `LightLevel >= 7` for all candidates → no spawns. The
  player is safe on the surface at noon. This is intended.
- **Player enters a cave:** sky exposure drops to 0; `LightLevel` is driven by
  torches only. Unlit cave sections spawn mobs. This is the core cave threat.
- **Player places a torch at a cave mouth:** the 5-block radius becomes
  `LightLevel >= 9` → no spawns there. Spawns resume just outside the radius.
  The player learns the protection radius by experiment.
- **Cap reached (20 mobs):** cycle spawns nothing until deaths or despawns free
  slots. A player who kills mobs faster than the spawner replaces them can
  clear an area. A player who does not will face a steady 20-mob pressure.
- **Player in a 1-block-wide corridor:** candidates snap to the corridor floor;
  spawns are valid if dark and in range. A Skeleton in a 1-block corridor
  cannot strafe (per its GDD) but still fires. This is intended kill-funnel
  gameplay.
- **Player builds an unlit room (base):** if light level < 7 inside, spawns are
  valid. This is intended — dark rooms attract mobs. Place torches or electric
  lamps (`design/gdd/light-and-power.md`). A lit room (`LightLevel >= 7`) is
  safe.
- **Player builds a room with a 1-block ceiling:** no valid surface (headroom <
  2) → no spawns inside. This is a cheap (if cramped) shelter trick, but
  Creepers exploding nearby can breach it.
- **Spawn candidate lands in water/lava:** rejected by `ValidSurface`; the snap
  tries adjacent surfaces. If none qualify, the attempt fails.
- **Spawn candidate lands inside a block:** rejected; snap to nearest air-above-
  solid. If none, attempt fails.
- **Player at world edge / border:** annulus is clipped to loaded chunks; spawns
  only where chunks exist. No spawns in void.
- **Multiple players (multiplayer):** each player gets a spawn cycle in their
  radius, but the cap is shared. A mob spawned for player A counts against the
  global cap for player B. Despawn requires no player within 128 blocks.
- **Weather transition mid-cycle:** the multiplier lerps; the cycle uses the
  current (interpolated) value. No instant snap.
- **Spawn during a Creeper detonation:** the cycle runs independently; a
  detonation does not pause spawning. A chain of Creepers can keep an area
  dangerous.
- **Mob spawned at exactly 24 or 128 blocks:** allowed (inclusive bounds).
- **Mob at exactly 128 blocks during despawn check:** not despawned (strictly
  greater). It gets one more chance to approach.
- **Burning mob at dawn on the surface:** burns and dies (or reaches shade). No
  despawn; death yields loot. Cap slot freed on death.
- **Spider/Creeper spawned in a cave at night, wanders out at dawn:** survives
  (no burn). This is intended — they are the daytime surface threat. The player
  must deal with them.
- **Torch radius change at runtime (`TorchLightRadius` edited):** takes effect
  on the next cycle. No retroactive re-evaluation of existing mobs.
- **Cap set to 0:** no spawns ever; existing mobs despawn/die normally. Useful
  for peaceful mode (a future toggle).
- **Spawn weight sum = 0 (all weights zero):** no spawns; cycle is a no-op.
  Treated as a configuration error; log a warning.
- **Determinism:** with a fixed seed, candidate positions and type rolls are
  reproducible. Used by automation tests.

---

## 6. Dependencies

| System | Why | Status |
|---|---|---|
| `UMobSpawnerSystem` | This document's subject | Implemented |
| `AMobCharacter` | Instantiated by `SpawnMob`; health, damage, burn, drops, XP, despawn-safe | Implemented |
| `UDayNightCycleSystem` | `IsDay/IsNight`, weather multipliers, sky light, time of day | Implemented |
| `AWorldGenerator` | Voxel queries (solid/air/liquid), chunk loading, surface snap, `CalculateLightLevel` host | Implemented |
| `NavigationSystem` | Spawned mobs path on the navmesh (if missing, mob wanders in place) | Implemented (basic) |
| `DT_Mobs` / `MobClasses` | Blueprint class resolution per type (`BP_Mob_*`) | Implemented |
| `UPlayerVitalSystem` | Damage/death on the player side (indirect) | Implemented |
| `design/gdd/mob-zombie.md` | Per-mob stats, behavior, drops | Draft |
| `design/gdd/mob-skeleton.md` | Per-mob stats, behavior, drops | Draft |
| `design/gdd/mob-spider.md` | Per-mob stats, behavior, drops | Draft |
| `design/gdd/mob-creeper.md` | Per-mob stats, behavior, drops | Draft |
| `design/gdd/weather-system.md` | Weather states, multipliers, transitions | Draft |
| `design/gdd/light-and-power.md` | Light levels, torches, electric lamps (future) | Draft (0% implemented) |
| `design/gdd/angels.md` | Angel light threshold (shared = 7) | Draft |
| Item layer (`design/gdd/items.md`) | Mob drops (currently block proxies) | Not implemented |

---

## 7. Tuning Knobs

| Knob | Default | Range | Effect |
|---|---|---|---|
| `MaxMobCap` | 20 | 0–60 | Population ceiling; 0 = peaceful |
| `MinSpawnDistance` | 2400 cm (24) | 1200–4800 | Safe bubble around player |
| `MaxSpawnDistance` | 12800 cm (128) | 6400–25600 | Spawn/despawn radius |
| `SpawnInterval` | 2.0 s | 1.0–5.0 | Cycle frequency |
| `TorchLightRadius` | 5 blocks | 3–8 | Torch protection radius |
| `LightLevelThreshold` | 7 | 0–15 | Darkness required to spawn |
| `BaseSpawnAttemptsPerCycle` | 4 | 1–8 | Base attempts per cycle |
| `MaxSpawnAttemptsPerCycle` | 8 | 4–16 | Attempt cap (weather can raise to this) |
| `DespawnCheckInterval` | 5.0 s | 1.0–10.0 | Despawn cadence |
| `HeadroomRequired` | 2 blocks | 1–3 | Vertical clearance for a valid surface |
| `BurnRate` (per mob) | 1 HP/s | 0.5–2.0 | Sunlight lethality (Zombie/Skeleton) |
| `ZombieSpawnWeight` | 40 | 0–100 | Spawn mix |
| `SkeletonSpawnWeight` | 25 | 0–100 | Spawn mix |
| `SpiderSpawnWeight` | 20 | 0–100 | Spawn mix |
| `CreeperSpawnWeight` | 15 | 0–100 | Spawn mix |
| `WeatherSpawnRateMultiplier` | 0.7 / 1.0 / 1.0 / 1.3 / 1.5 | — | Clear/Overcast/Rain/Fog/Ash Storm |
| `WeatherAggressionMultiplier` | 1.0 / 1.0 / 0.8 / 1.2 / 1.5 | — | Clear/Overcast/Rain/Fog/Ash Storm |
| `SurfaceSpawnDayGate` | true | — | Burning mobs excluded from daytime surface spawns |

All knobs are `EditAnywhere` on `UMobSpawnerSystem` (or in `DT_Mobs` for per-mob
weights) and should be exposed to designers via the details panel. No knob
value may be hardcoded in C++.

---

## 8. Acceptance Criteria

1. A spawn occurs only where `CalculateLightLevel < 7`, between 24 and 128
   blocks from the player, on a valid solid surface with ≥ 2 blocks of headroom,
   while `CurrentMobCount < 20`.
2. No mob spawns within 5 blocks of a torch (torch light level ≥ 9 ≥ threshold).
3. The spawn cycle runs every 2.0 s and attempts up to 4 spawns (6 in Ash
   Storm), never exceeding the cap.
4. A mob is despawned when it is further than 128 blocks from the player; a mob
   at exactly 128 blocks is not despawned.
5. Despawned mobs drop no loot and no XP.
6. Zombie and Skeleton do not spawn on an exposed daytime surface (LightLevel = 15).
   Excluding them from shaded daytime surfaces is a design target (see §3.7).
7. Spider and Creeper spawn wherever the light gate passes, day or night.
8. A burning mob (Zombie/Skeleton) under open sky during daytime ignites and
   loses 1 HP/s; it dies in ~20 s at full health and yields loot + XP.
9. A Spider or Creeper spawned in a dark cave at night can survive on the
   surface at dawn.
10. Weather multipliers apply: Rain 0.7× spawn rate, Fog 1.3×, Ash Storm 1.5×;
    aggression multipliers per the weather table.
11. With a fixed seed, candidate positions and type rolls are reproducible.
12. With `MaxMobCap = 0`, no mobs spawn; existing mobs despawn/die normally.
13. A spawn at exactly 24 blocks and at exactly 128 blocks from the player is
    allowed (inclusive bounds).
14. An unlit player-built room (`LightLevel < 7`) is a valid spawn zone; a lit
    room (`LightLevel >= 7`) is not.
15. (Design target) A Creeper mid-detonation (fuse active) is not despawned; the
    detonation completes. Current implementation despawns by distance regardless
    of fuse state.