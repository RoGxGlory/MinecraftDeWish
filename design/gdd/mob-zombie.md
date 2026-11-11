# GDD — Mob: Zombie

| Field | Value |
|---|---|
| Status | Draft (2026-11-11) |
| System owner | game-designer / ai-programmer |
| Related | `Docs/MOB_REGISTRY.md`, `Docs/SYSTEMS_REFERENCE.md` §10–11, `design/gdd/items.md` §3.7, `design/gdd/weather-system.md` §3.4, `design/art/art-direction.md` §5 |
| Source of truth for code | `AMobCharacter`, `UMobSpawnerSystem`, `EMobType::Zombie` |

---

## 1. Overview

The Zombie is the game's baseline hostile mob: a slow, hunched undead melee aggressor
that spawns in dark areas and pursues the player on sight. It is the simplest expression
of the "world fights back" fantasy — a creature of flesh and bone (not stone, not voxel)
that the player can outrun, out-light, or out-fight. Its role is to teach the player
the core loop of light management and melee combat before the Skeleton, Spider and
Creeper escalate the threat. Zombies burn in sunlight, drop Rotten Flesh, and award
5 XP on death.

## 2. Player Fantasy

> *"It is slow, but it does not stop."*

The player should feel that the Zombie is **relentless but fair**. It is the mob that
punishes carelessness, not skill: a player who leaves a dark cave mouth unlit will
meet one at the door. A player who places a torch sees it burn. The Zombie is the
first "I survived the night" trophy — defeating one is not an accomplishment, but
surviving a small group of them at dusk is the first real test of the player's
light-and-weapon loop.

## 3. Detailed Rules

### 3.1 Identity and stats

| Stat | Value | Notes |
|---|---|---|
| Health | 20 HP (10 hearts) | Same as Skeleton; tougher than Spider (16) |
| Attack damage | 3 HP (1.5 hearts) per hit | Melee only |
| Movement speed | 230 cm/s | Slowest humanoid; slower than Skeleton (250) |
| Detection range | 16 m (1600 cm) | Line of sight + noise; standard hostile radius |
| Attack range | 1.5 m (150 cm) | Touching distance |
| Attack cooldown | 1.0 s | One hit per second at contact |
| Capsule | Standard humanoid (matches `SK_Mob_Base` 6-bone rig) | — |
| `bBurnsInSunlight` | `true` | Ignites and loses 1 HP/s in open sky during daytime |
| `bCanClimbWalls` | `false` | Cannot scale vertical blocks |
| `bExplodes` | `false` | — |

### 3.2 Spawn rules

- **Spawner:** `UMobSpawnerSystem::ProcessSpawnCycle()` (every `SpawnInterval = 2.0 s`).
- **Light level:** spawn only where `CalculateLightLevel < 7` (combined torch + sky
  exposure, 0–15).
- **Surface:** valid solid surface (top face of a block, not inside a block, not
  floating).
- **Distance from player:** between `MinSpawnDistance = 24 m` and `MaxSpawnDistance = 128 m`.
- **Time of day:** spawns at night and in dark caves/interiors at any time.
  Daytime surface spawns are suppressed because the light level is too high; cave
  spawns remain possible.
- **Cap:** total hostile mob cap `MaxMobCap = 20` across all types.
- **Despawn:** `ProcessDespawn()` removes mobs further than 128 m from the player.

### 3.3 AI behavior

State machine (shared with Skeleton/Spider/Creeper family, see `Docs/MOB_REGISTRY.md`):

```
Idle → Wander (patrol random nearby point)
     → Chase (player detected within DetectionRange)
     → Attack (player within AttackRange, cooldown elapsed)
```

- **Idle / Wander:** slow random walk within a small radius of spawn point; plays
  idle animation. Transitions to Chase on player detection.
- **Chase:** move directly toward the player at `MovementSpeed`, pathing around
  obstacles via `NavigationSystem`. No strafing, no dodge.
- **Attack:** when within `AttackRange`, deal `AttackDamage` every `AttackCooldown`.
  Knockback is minimal (vanilla-style small push).
- **Loss of target:** if the player leaves `DetectionRange` for > 3 s, return to
  Wander at the last known position, then Idle.
- **Sunlight burn:** while under open sky during daytime, ignite (visual flame) and
  take 1 HP/s. The mob does not flee from sunlight; it simply dies if it cannot
  reach shade. This is intentional — Zombies are too slow to escape.

### 3.4 Drops and XP

Per the item layer (`design/gdd/items.md` §3.7):

- **Guaranteed:** `Item.RottenFlesh × 1–2` (100 %).
- **Rolled:** `Item.ScrapMetal` at 5 % chance (1 item).
- **Experience:** 5 XP (spawns `AXPOrbPickup`).

Drop rolls are deterministic given an explicit seed (automation-test requirement).

### 3.5 Weather effects

Per `design/gdd/weather-system.md` §3.4, the `CurrentMobAggressionMultiplier` and
`CurrentMobSpawnRateMultiplier` modulate Zombie behavior:

| Weather | Spawn rate mult. | Aggression mult. | Effect on Zombie |
|---|---|---|---|
| Clear | 1.0 | 1.0 | Baseline |
| Overcast | 1.0 | 1.0 | Baseline |
| Rain | 0.7 | 0.8 | Fewer spawns; slightly slower pursuit |
| Fog | 1.3 | 1.2 | More spawns; faster pursuit, harder to spot |
| Ash Storm | 1.5 | 1.5 | Most dangerous; spawns surge, pursuit aggressive |

Zombies do not gain new abilities in any weather; only spawn density and pursuit
speed scale.

## 4. Formulas

### 4.1 Time to kill (player, fist)

Player fist damage is 1 HP/hit. Against a Zombie (20 HP):

```
TTK_fist = Health / PlayerFistDamage = 20 / 1 = 20 hits
```

With a sword (`UBaublesSystem::GetAttackDamage()`):

```
TTK_sword = Health / SwordDamage
```

where `SwordDamage` is the equipped sword's `GetAttackDamage()` (default fist = 1.0).

### 4.2 Sunlight burn time-to-death

```
BurnTTD = Health / BurnRate = 20 / 1 = 20 s
```

A Zombie entering open daylight dies in ~20 s unless it reaches shade.

### 4.3 Detection

```
Detected = (Distance(Player, Zombie) <= DetectionRange)
         AND (LineOfSight(Player, Zombie) == true)   // no opaque block between
         AND (LightLevelAt(Zombie) < 7 OR IsNight)
```

### 4.4 Spawn eligibility

```
CanSpawn = (LightLevel < 7)
         AND (MinSpawnDistance <= Distance(Player, Candidate) <= MaxSpawnDistance)
         AND (SolidSurface(Candidate))
         AND (CurrentMobCount < MaxMobCap)
         AND (SpawnInterval elapsed)
```

### 4.5 Weather-modulated spawn rate

```
EffectiveSpawnRate = BaseSpawnRate * CurrentMobSpawnRateMultiplier
EffectivePursuitSpeed = MovementSpeed * CurrentMobAggressionMultiplier   (clamped to 1.5×)
```

### 4.6 XP award

```
XP = 5  (constant, see MOB_REGISTRY)
```

## 5. Edge Cases

- **Zombie in daylight under a tree canopy:** `CalculateLightLevel` combines sky
  exposure with torch proximity. A tree canopy lowers sky exposure; if the
  resulting light level is ≥ 7, no burn. If < 7, burn applies even at noon.
  This makes tree shade a valid (temporary) refuge.
- **Zombie knocked into water:** no special drowning rule in v1; Zombies do not
  drown (they are already dead). They continue to pursue.
- **Zombie blocked by a 1-block wall:** paths around it; does not break blocks
  (unlike the Angel). A closed door stops it indefinitely.
- **Zombie spawn inside a player-built structure:** if light level < 7 inside
  (unlit interior), spawns are valid. This is intended — dark rooms attract
  Zombies. Place torches.
- **Two Zombies stacking on one block:** allowed; they do not push each other.
  Combat resolution is per-mob.
- **Zombie dies from sunlight burn:** still drops loot and XP (death is death).
- **Player kills Zombie mid-burn:** loot and XP drop normally.
- **Zombie chasing a player who logs out:** despawned by `ProcessDespawn` if beyond
  128 m; otherwise continues to wander.
- **Weather transition during chase:** pursuit speed lerps with the aggression
  multiplier over the transition duration (30–60 s). No instant snap.

## 6. Dependencies

| System | Why | Status |
|---|---|---|
| `UMobSpawnerSystem` | Spawn cycle, light level, despawn | Implemented |
| `AMobCharacter` | Health, damage, sunlight burn, drops, XP | Implemented |
| `UDayNightCycleSystem` | Day/night, weather multipliers | Implemented |
| `UPlayerVitalSystem` | Damage application, death | Implemented |
| `UBaublesSystem` | Sword damage bonus | Implemented |
| Item layer (`design/gdd/items.md`) | Rotten Flesh, Scrap Metal drops | Not implemented (block proxies today) |
| `AXPOrbPickup` | XP drop | Implemented |
| `NavigationSystem` | Pathing | Implemented |

## 7. Tuning Knobs

| Knob | Default | Range | Effect |
|---|---|---|---|
| `Health` | 20 | 10–40 | Survivability |
| `AttackDamage` | 3 | 1–6 | Threat level |
| `MovementSpeed` | 230 cm/s | 150–350 | Chase pressure |
| `DetectionRange` | 1600 cm | 800–3200 | How early the player is noticed |
| `AttackRange` | 150 cm | 100–250 | Melee reach |
| `AttackCooldown` | 1.0 s | 0.5–2.0 | Hit frequency |
| `BurnRate` | 1 HP/s | 0.5–2 | Sunlight lethality |
| `LightLevelThreshold` | 7 | 0–15 | Darkness required to spawn |
| `MinSpawnDistance` | 2400 cm | 1200–4800 | Spawn proximity floor |
| `MaxSpawnDistance` | 12800 cm | 6400–25600 | Spawn proximity ceiling |
| `MaxMobCap` | 20 | 10–40 | Population density |
| `SpawnInterval` | 2.0 s | 1–5 | Spawn frequency |
| `TorchLightRadius` | 5 blocks | 3–8 | Light protection radius |
| `DetectionLossTimeout` | 3.0 s | 1–10 | How long before giving up chase |
| `RottenFleshDropMin/Max` | 1 / 2 | 0–4 | Loot quantity |
| `ScrapMetalDropChance` | 0.05 | 0–0.2 | Rare loot rate |
| `XPDropped` | 5 | 1–15 | Progression reward |

## 8. Acceptance Criteria

1. A Zombie spawns only where light level < 7, between 24 m and 128 m from the player,
   on a solid surface, while total mob count < 20.
2. A Zombie detects the player within 16 m with line of sight and enters Chase.
3. A Zombie deals 3 HP per hit with a 1.0 s cooldown at 1.5 m range.
4. A Zombie under open sky during daytime ignites and loses 1 HP/s; it dies in 20 s
   at full health if it cannot reach shade.
5. A Zombie drops 1–2 Rotten Flesh (100 %) and Scrap Metal at 5 % chance, plus
   5 XP, on death.
6. A Zombie does not spawn during daytime on the surface (light level ≥ 7).
7. A Zombie does not climb walls or break blocks.
8. Loot rolls are deterministic given a fixed seed.
9. Weather multipliers (spawn rate, aggression) apply per the weather system table.
10. A Zombie despawns when further than 128 m from the player.