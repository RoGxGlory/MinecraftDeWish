# GDD — Mob: Spider

| Field | Value |
|---|---|
| Status | Draft (2026-11-11) |
| System owner | game-designer / ai-programmer |
| Related | `Docs/MOB_REGISTRY.md`, `Docs/SYSTEMS_REFERENCE.md` §10–11, `design/gdd/items.md` §3.7, `design/gdd/weather-system.md` §3.4, `design/art/art-direction.md` §5 |
| Source of truth for code | `AMobCharacter`, `UMobSpawnerSystem`, `EMobType::Spider` |

---

## 1. Overview

The Spider is the game's agile, low-profile arachnid: a wide, flat creature with
eight articulated legs that moves faster than any humanoid mob (300 cm/s) and
can climb vertical voxel walls. It does not burn in sunlight — it is active
day and night — which makes it the mob that **breaks the "light = safe" rule**
on the surface. Its role is to introduce **verticality and unpredictability**:
a Spider that scales a wall and drops onto the player from above is a different
threat from anything that walks on the ground. Spiders drop String (and
occasionally a Spider Eye), award 5 XP, and have the lowest health of the
hostile roster (16 HP).

## 2. Player Fantasy

> *"It is already above you."*

The player should feel that the Spider is **the mob that refuses to stay on the
ground**. Where Zombies and Skeletons are bound by gravity and terrain, the
Spider ignores the player's walls, cliffs and roofs. The fantasy is the **creeping
dread of something in the rafters**: the player must look up, not just ahead. A
Spider is not the most damaging mob, but it is the one most likely to appear
where the player thought they were safe. Its fast attack cooldown (0.8 s) makes
it a stunlock risk in tight spaces.

## 3. Detailed Rules

### 3.1 Identity and stats

| Stat | Value | Notes |
|---|---|---|
| Health | 16 HP (8 hearts) | Lowest of the hostile roster |
| Attack damage | 2 HP (1.0 heart) per hit | Lowest per-hit, but fastest cooldown |
| Movement speed | 300 cm/s | Fastest mob; 30 % faster than Skeleton |
| Detection range | 16 m (1600 cm) | Same as Zombie; shorter than Skeleton (40 m) |
| Attack range | 2.0 m (200 cm) | Slightly longer than Zombie (1.5 m) |
| Attack cooldown | 0.8 s | Fastest attack rate in the roster |
| Capsule | Half-Height 50 cm, Radius 60 cm (wide and low) | Distinct silhouette from humanoids |
| `bBurnsInSunlight` | `false` | **Does not burn** — active day and night on the surface |
| `bCanClimbWalls` | `true` | Scales vertical voxel blocks |
| `bExplodes` | `false` | — |
| Climbing velocity | 0.7 × MoveSpeed = 210 cm/s upward | Applied when forward motion is blocked by a vertical block |

### 3.2 Spawn rules

- **Light level:** spawn only where `CalculateLightLevel < 7` (same as other
  hostile mobs). However, because Spiders do not burn in sunlight, a Spider that
  spawns in a dark cave at night can **wander onto the surface at dawn** and
  survive. This is intended — Spiders are the daytime surface threat.
- **Surface:** valid solid surface (including vertical walls for climbing
  transitions, but spawn is on a horizontal surface).
- **Distance from player:** between `MinSpawnDistance = 24 m` and
  `MaxSpawnDistance = 128 m`.
- **Time of day:** spawns at night and in dark interiors at any time. Unlike
  Zombies/Skeletons, a Spider spawned at night does not die at dawn.
- **Cap:** shared `MaxMobCap = 20`.
- **Despawn:** beyond 128 m.

### 3.3 AI behavior

```
Idle → Wander
     → Chase (player detected within DetectionRange)
     → Attack (player within AttackRange, cooldown elapsed)
     → Climb (forward blocked by vertical block)
```

- **Idle / Wander:** low, scuttling movement; plays idle animation. Spiders
  wander more erratically than humanoids (random direction changes every 1–2 s).
- **Chase:** move directly toward the player at `MovementSpeed`. Does not strafe;
  closes distance aggressively.
- **Attack:** when within `AttackRange`, deal `AttackDamage` every `AttackCooldown`.
  The fast cooldown (0.8 s) makes Spiders dangerous in sustained contact.
- **Climb:** when forward motion is blocked by a vertical voxel block (raycast
  ahead detects a solid block at body height), apply upward climbing velocity
  (`0.7 × MoveSpeed = 210 cm/s`) to scale over the block and continue pursuit
  on the other side or on top. The Spider does not "stick" to walls indefinitely;
  it climbs only while chasing or when a wall blocks its path to a target.
- **Loss of target:** if the player leaves `DetectionRange` for > 2 s, return to
  Wander. (Shorter than Zombie 3 s — Spiders are more easily distracted but
  also re-engage quickly.)
- **No sunlight burn:** Spiders are fully active during daytime on the surface.
  They do not seek shade.

### 3.4 Drops and XP

Per `design/gdd/items.md` §3.7:

- **Guaranteed:** `Item.String × 1–2` (100 %).
- **Rolled:** `Item.SpiderEye` at 33 % chance (1 item).
- **Experience:** 5 XP.

### 3.5 Weather effects

| Weather | Spawn rate mult. | Aggression mult. | Effect on Spider |
|---|---|---|---|
| Clear | 1.0 | 1.0 | Baseline; daytime surface activity is the Spider's niche |
| Overcast | 1.0 | 1.0 | Baseline |
| Rain | 0.7 | 0.8 | Fewer spawns; climbing speed reduced (wet surfaces) |
| Fog | 1.3 | 1.2 | More spawns; harder to spot at ground level |
| Ash Storm | 1.5 | 1.5 | Most dangerous; spawn surge, faster pursuit |

**Rain interaction:** in Rain weather, the Spider's climbing velocity is reduced
by 30 % (`210 → 147 cm/s`) to simulate slippery surfaces. This is a partial
counterbalance.

## 4. Formulas

### 4.1 Time to kill (player, melee)

```
TTK_player = SpiderHealth / PlayerSwordDamage = 16 / SwordDamage
```

At sword damage 4: `16 / 4 = 4 hits`.

### 4.2 Time for Spider to kill player (sustained contact)

```
PlayerTTK_spider = PlayerHealth / (AttackDamage / AttackCooldown)
                 = 20 / (2 / 0.8) = 20 / 2.5 = 8 s
```

A Spider that maintains contact kills a full-health player in ~8 s. This is
the stunlock risk in tight spaces.

### 4.3 Detection

```
Detected = (Distance(Player, Spider) <= DetectionRange)
         AND (LineOfSight(Player, Spider) == true)
         AND (LightLevelAt(Spider) < 7 OR IsNight OR true)   // Spiders ignore the daylight burn, but still spawn in dark
```

Note: Spiders **spawn** only in dark areas (light < 7), but once spawned they
are **active** in any light. The spawn gate is the same; the activity gate is
not.

### 4.4 Climb trigger

```
Climb = (ForwardRaycast hits solid block within 1 body-length)
      AND (ChaseState == true OR WanderState == true)
      AND (BlockAboveIsPassable)
```

Climbing velocity:

```
ClimbVelocity = 0.7 * MovementSpeed = 0.7 * 300 = 210 cm/s   (base)
              = 210 * WeatherClimbFactor                        (Rain: 0.7 → 147 cm/s)
```

### 4.5 Spawn eligibility

Same as Zombie (§4.4 of mob-zombie.md), with the same weather multipliers. The
`bBurnsInSunlight` flag does not affect spawn eligibility.

### 4.6 XP award

```
XP = 5
```

## 5. Edge Cases

- **Spider on a ceiling:** Spiders do not spawn on ceilings, but a climbing
  Spider can reach a ceiling and continue moving along it (inverted). In v1,
  ceiling movement is limited to 2 s before the Spider drops (gravity reasserts).
  This prevents permanent ceiling camping.
- **Spider climbing a player-built wall:** climbs over it and continues pursuit.
  A 1-block wall does not stop a Spider. A 2-block overhang (lip) stops climbing
  because the raycast above is blocked.
- **Spider in a 1-block-wide corridor:** moves normally; attack range (2 m) is
  longer than the corridor width, so it can hit the player around a corner.
- **Spider dropping onto the player from above:** deals normal attack damage on
  contact. No fall damage to the Spider (it lands on the player).
- **Spider in water:** swims at reduced speed (50 %). Does not drown.
- **Spider and Skeleton in the same fight:** Spider closes to melee while
  Skeleton pins at range. The Spider's speed makes it the flanker.
- **Spider dies from player damage:** drops loot and XP normally. (Spiders do
  not burn, so sunlight is not a death cause.)
- **Spider spawn at dawn on the surface:** a Spider that spawned in a dark cave
  at night and wanders out at dawn survives. This is intended — the player must
  deal with it.
- **Spider climbing during Rain:** climbing speed reduced by 30 %. A Spider
  may fail to clear a 2-block wall in Rain and fall back.
- **Multiple Spiders focusing one player:** each attacks independently. Fast
  cooldowns stack; 2 Spiders in contact = 5 HP/s. Dangerous.
- **Spider in an enclosed 1×1 space:** cannot fit (radius 60 cm > 50 cm
  half-width). Will not enter; paths around.

## 6. Dependencies

| System | Why | Status |
|---|---|---|
| `UMobSpawnerSystem` | Spawn cycle, light level, despawn | Implemented |
| `AMobCharacter` | Health, damage, wall climbing, drops, XP | Implemented |
| `UDayNightCycleSystem` | Day/night, weather multipliers | Implemented |
| `UPlayerVitalSystem` | Damage application, death | Implemented |
| `UBaublesSystem` | Sword damage (melee counter) | Implemented |
| Item layer (`design/gdd/items.md`) | String, Spider Eye drops | Not implemented (block proxies today) |
| `AXPOrbPickup` | XP drop | Implemented |
| `NavigationSystem` | Pathing, climb raycast | Implemented (basic) |

## 7. Tuning Knobs

| Knob | Default | Range | Effect |
|---|---|---|---|
| `Health` | 16 | 8–32 | Survivability |
| `AttackDamage` | 2 | 1–4 | Per-hit threat |
| `MovementSpeed` | 300 cm/s | 200–450 | Chase pressure |
| `DetectionRange` | 1600 cm | 800–3200 | How early the player is noticed |
| `AttackRange` | 200 cm | 100–300 | Melee reach |
| `AttackCooldown` | 0.8 s | 0.4–1.5 | Hit frequency |
| `ClimbVelocityFactor` | 0.7 | 0.4–1.0 | Wall-climbing speed |
| `ClimbCeilingTime` | 2.0 s | 0.5–5.0 | Max time on ceiling before dropping |
| `RainClimbFactor` | 0.7 | 0.3–1.0 | Climbing speed in rain |
| `LightLevelThreshold` | 7 | 0–15 | Darkness required to spawn |
| `DetectionLossTimeout` | 2.0 s | 1–5 | Distraction recovery |
| `CapsuleHalfHeight` | 50 cm | 30–70 | Vertical profile |
| `CapsuleRadius` | 60 cm | 40–80 | Horizontal profile |
| `StringDropMin/Max` | 1 / 2 | 0–4 | Loot quantity |
| `SpiderEyeDropChance` | 0.33 | 0–0.6 | Rare loot rate |
| `XPDropped` | 5 | 1–15 | Progression reward |

## 8. Acceptance Criteria

1. A Spider spawns only where light level < 7, between 24 m and 128 m from the
   player, on a solid surface, while total mob count < 20.
2. A Spider does **not** burn in sunlight; it remains active on the surface during
   daytime.
3. A Spider moves at 300 cm/s and attacks for 2 HP every 0.8 s within 2.0 m.
4. A Spider climbs a vertical voxel wall when its forward path is blocked,
   applying 210 cm/s upward velocity.
5. A Spider drops from a ceiling after at most 2.0 s.
6. A Spider cannot enter a 1×1-block space (capsule radius 60 cm).
7. A Spider drops 1–2 String (100 %) and Spider Eye at 33 % chance, plus 5 XP,
   on death.
8. In Rain weather, a Spider's climbing speed is reduced by 30 %.
9. Loot rolls are deterministic given a fixed seed.
10. A Spider despawns when further than 128 m from the player.