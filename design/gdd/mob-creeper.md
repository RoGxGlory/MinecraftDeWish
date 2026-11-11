# GDD — Mob: Creeper

| Field | Value |
|---|---|
| Status | Draft (2026-11-11) |
| System owner | game-designer / ai-programmer |
| Related | `Docs/MOB_REGISTRY.md`, `Docs/SYSTEMS_REFERENCE.md` §10–11, `design/gdd/items.md` §3.7, `design/gdd/weather-system.md` §3.4, `design/art/art-direction.md` §5 |
| Source of truth for code | `AMobCharacter`, `UMobSpawnerSystem`, `EMobType::Creeper` |

---

## 1. Overview

The Creeper is the game's silent ambush stalker: a swollen, moss-covered organic
form that approaches the player without sound, then detonates when within 3 m.
It is the only mob that **destroys terrain** — its explosion breaks all voxel
blocks in a 3-block radius sphere and deals up to 43 HP at ground zero (lethal
to a full-health player). Its role is to **punish inattention and reward
awareness**: the hissing fuse is the tell, and a player who spots it has 1.5 s
to flee beyond 6 m and cancel the detonation. Creepers do not burn in sunlight,
drop Gunpowder (and no other loot), and award 5 XP on death — but if they
detonate, they drop no item loot, only XP.

## 2. Player Fantasy

> *"It was quiet. That was the warning."*

The player should feel that the Creeper is **the consequence of not looking
behind you**. Where the Zombie is a frontal threat and the Skeleton is a ranged
threat, the Creeper is the threat that **was already there**. The fantasy is the
**heart-stopping hiss**: the moment the player realizes they are standing next
to something that is about to explode. The Creeper teaches the player to scan
their surroundings, to keep a buffer of open space, and to never mine with their
back to an unlit cave. A Creeper that detonates in an unlit tunnel and
destroys a wall is a memorable, terrifying event — by design.

## 3. Detailed Rules

### 3.1 Identity and stats

| Stat | Value | Notes |
|---|---|---|
| Health | 20 HP (10 hearts) | Same as Zombie/Skeleton |
| Attack damage | Up to 43 HP at ground zero (lethal) | Radial; scales with distance from blast center |
| Movement speed | 220 cm/s | Slowest after Zombie (230); deliberate, stalking pace |
| Detection range | 16 m (1600 cm) | Same as Zombie/Spider |
| Attack range (fuse trigger) | 3.0 m (300 cm) | Enters Explode state within this distance |
| Fuse duration | 1.5 s (`ExplosionFuseTimer`) | Hissing countdown before detonation |
| Fuse cancel distance | 6.0 m (600 cm) | If player flees beyond this, fuse cancels, returns to Chase |
| Explosion radius | 3 blocks (sphere: `Dx² + Dy² + Dz² ≤ 9`) | Voxel destruction radius |
| Capsule | Standard humanoid (low-poly, swollen form) | Non-standard silhouette (art-direction §5) |
| `bBurnsInSunlight` | `false` | **Does not burn** — active day and night |
| `bCanClimbWalls` | `false` | — |
| `bExplodes` | `true` | Self-detonates near player |

### 3.2 Spawn rules

- **Light level:** spawn only where `CalculateLightLevel < 7`.
- **Surface:** valid solid surface.
- **Distance from player:** between `MinSpawnDistance = 24 m` and
  `MaxSpawnDistance = 128 m`.
- **Time of day:** night and dark interiors at any time. Because Creepers do not
  burn, a Creeper spawned at night can survive into daytime on the surface.
- **Cap:** shared `MaxMobCap = 20`.
- **Despawn:** beyond 128 m.

### 3.3 AI behavior

```
Idle → Wander (silent, slow)
     → Chase (player detected within DetectionRange)
     → Explode (player within AttackRange = 3 m → fuse begins)
        → Detonate (fuse expires → radial damage + voxel destruction + self-destruct)
        → Cancel (player flees beyond 6 m → return to Chase)
```

- **Idle / Wander:** slow, silent movement. Creepers do not make ambient noise
  while wandering. They do not hiss until the fuse begins.
- **Chase:** move toward the player at `MovementSpeed` (220 cm/s). No strafing,
  no dodge. The Creeper closes distance deliberately.
- **Explode (fuse):** when the player is within `AttackRange` (3 m), the Creeper
  stops moving, begins hissing, and swells visually (sodium cracks glow from
  within — art-direction §5). The fuse timer counts down for `FuseDuration` (1.5 s).
  - **Cancel:** if the player moves beyond `FuseCancelDistance` (6 m) during the
    fuse, the hissing stops, the swelling reverses, and the Creeper returns to
    Chase. The player "escaped."
  - **Detonate:** if the fuse expires, the Creeper detonates (§3.4).
- **Post-detonation:** the Creeper is destroyed. No corpse, no item loot (only
  XP). The explosion is the event.

### 3.4 Detonation

When the fuse expires:

1. **Radial damage:** applies damage to the player based on distance from the
   blast center:

```
Damage = MaxDamage * (1 - Distance / ExplosionRadius)   // linear falloff
```

   where `MaxDamage = 43 HP` at ground zero (`Distance = 0`), and `ExplosionRadius = 3 blocks`
   (300 cm). At 3 blocks distance, damage = 0.

   | Distance from center | Damage |
   |---|---|
   | 0 blocks (ground zero) | 43 HP (lethal) |
   | 1 block | ~28.7 HP |
   | 2 blocks | ~14.3 HP |
   | 3 blocks | 0 HP |

2. **Voxel destruction:** all blocks within the sphere `Dx² + Dy² + Dz² ≤ 9`
   are broken (converted to air). This includes the block the Creeper is standing
   on and surrounding terrain. The destruction is **not** affected by block
   durability — Creepers ignore hardness.

3. **Self-destruction:** the Creeper is removed from the world.

4. **XP drop:** 5 XP (spawns `AXPOrbPickup`). No item loot.

5. **Visual/audio:** explosion flash, debris, sound. (Creeper sodium flash per
   art-direction §11 priority 9.)

### 3.5 Drops and XP

- **On death by player (killed before detonation):** `Item.Gunpowder × 1` (100 %)
  + 5 XP. No other loot.
- **On detonation:** no item loot, 5 XP only.
- **On death by other causes (sunlight — N/A, Creepers don't burn; fall damage):**
  same as player kill: Gunpowder + XP.

### 3.6 Weather effects

| Weather | Spawn rate mult. | Aggression mult. | Effect on Creeper |
|---|---|---|---|
| Clear | 1.0 | 1.0 | Baseline |
| Overcast | 1.0 | 1.0 | Baseline |
| Rain | 0.7 | 0.8 | Fewer spawns; fuse duration +0.3 s (wet gunpowder) |
| Fog | 1.3 | 1.2 | More spawns; harder to spot the hiss/swell tell |
| Ash Storm | 1.5 | 1.5 | Most dangerous; spawn surge, fuse -0.2 s (dry powder) |

**Rain interaction:** wet gunpowder slows the fuse (`1.5 → 1.8 s`), giving the
player slightly more time to escape. This is a small mercy.

**Ash Storm interaction:** dry conditions accelerate the fuse (`1.5 → 1.3 s`),
making detonation faster. Combined with the spawn surge, Ash Storm is the most
dangerous weather for Creeper encounters.

## 4. Formulas

### 4.1 Explosion damage falloff

```
Damage(d) = MaxDamage * max(0, 1 - d / R)
```

where `MaxDamage = 43`, `R = 300 cm` (3 blocks), `d = distance from blast center in cm`.

At `d = 0`: `Damage = 43`.
At `d = 100` (1 block): `Damage = 43 * (1 - 100/300) = 43 * 0.667 ≈ 28.7`.
At `d = 200` (2 blocks): `Damage = 43 * (1 - 200/300) = 43 * 0.333 ≈ 14.3`.
At `d = 300` (3 blocks): `Damage = 0`.

### 4.2 Voxel destruction (sphere)

```
Destroyed = { (x,y,z) : (x-cx)² + (y-cy)² + (z-cz)² ≤ 9 }   // in block coordinates
```

All blocks in this set are set to air, regardless of durability. Bedrock is
**not** destroyed (absolute rule, shared with Angel breaching).

### 4.3 Fuse timer

```
FuseTimer starts at FuseDuration (1.5 s)
FuseTimer -= DeltaTime each tick
if FuseTimer <= 0 → Detonate
if Distance(Player, Creeper) > FuseCancelDistance (6 m) → Cancel fuse, return to Chase
```

Weather-adjusted fuse:

```
EffectiveFuse = FuseDuration + WeatherFuseModifier
```

where `WeatherFuseModifier`: Clear/Overcast = 0, Rain = +0.3, Fog = 0, Ash Storm = -0.2.

### 4.4 Detection

```
Detected = (Distance(Player, Creeper) <= DetectionRange)
         AND (LineOfSight(Player, Creeper) == true)
         AND (LightLevelAt(Creeper) < 7 OR IsNight)
```

Note: Creepers do **not** burn in sunlight, but they still **spawn** only in dark
areas. The spawn gate is the same as other mobs; the activity gate is not.

### 4.5 Spawn eligibility

Same as Zombie (§4.4 of mob-zombie.md), with the same weather multipliers.

### 4.6 XP award

```
XP = 5   (constant, whether killed or detonated)
```

## 5. Edge Cases

- **Creeper detonates inside a player-built structure:** destroys all blocks
  in the 3-block sphere, including walls, floors and roofs. This can breach a
  base. This is intended — Creepers are a structural threat, not just a damage
  threat.
- **Creeper detonates on Bedrock:** Bedrock blocks in the sphere are not
  destroyed. The explosion still deals damage and destroys non-Bedrock blocks.
- **Creeper detonates next to another Creeper:** chain reaction — the second
  Creeper's fuse is triggered by the explosion (treated as a noise/damage event
  within DetectionRange). Multiple Creepers can chain-detonate.
- **Creeper detonates next to a Zombie/Skeleton/Spider:** the explosion deals
  radial damage to nearby mobs too. Other mobs take damage and can die.
- **Player flees during fuse:** if the player reaches 6 m before 1.5 s, the
  fuse cancels. The Creeper returns to Chase. The player "escaped." This is
  the intended counterplay.
- **Player is at exactly 6 m:** cancel triggers at `> 6.0 m` (strictly greater).
  At exactly 6.0 m, the fuse continues.
- **Creeper in a 1-block-wide corridor:** the explosion destroys the corridor
  walls, potentially opening a path. This can be used tactically (by the player,
  if they can bait a Creeper) or catastrophically.
- **Creeper detonates underwater:** explosion still deals damage and destroys
  blocks. No special water interaction in v1.
- **Creeper killed during fuse:** if the player kills the Creeper during the
  1.5 s fuse (before detonation), it drops Gunpowder + XP (not just XP). This
  rewards fast reaction.
- **Creeper spawns at dawn on the surface:** survives (no burn). Wanders
  silently. Player may not notice until it is within 3 m.
- **Multiple Creepers in one area:** each detonates independently. A group of
  3 Creepers can level a significant area.
- **Creeper hiss audible range:** the hiss is audible within ~16 m (same as
  detection range). A player who hears the hiss has ~1.5 s to react.
- **Weather transition during fuse:** fuse duration is set at fuse start based
  on current weather. Weather changes mid-fuse do not adjust the timer.
- **Creeper explosion and Angel interaction:** if an Angel is within the
  explosion radius, it takes damage (if mortal) but is not destroyed by the
  blast. The explosion does not affect Angel observation state.

## 6. Dependencies

| System | Why | Status |
|---|---|---|
| `UMobSpawnerSystem` | Spawn cycle, light level, despawn | Implemented |
| `AMobCharacter` | Health, damage, explosion, drops, XP | Implemented |
| `UDayNightCycleSystem` | Day/night, weather multipliers | Implemented |
| `UPlayerVitalSystem` | Damage application, death | Implemented |
| `UBaublesSystem` | Sword damage (melee counter before detonation) | Implemented |
| Voxel destruction | `BreakBlockAtVoxel` / sphere break | Implemented (via `AWorldGenerator`) |
| Item layer (`design/gdd/items.md`) | Gunpowder drop | Not implemented (block proxies today) |
| `AXPOrbPickup` | XP drop | Implemented |
| `NavigationSystem` | Pathing | Implemented |
| Audio system | Hiss, explosion sound | Partial |
| VFX system | Swell, sodium cracks, explosion flash | Partial |

## 7. Tuning Knobs

| Knob | Default | Range | Effect |
|---|---|---|---|
| `Health` | 20 | 10–40 | Survivability (before detonation) |
| `MaxExplosionDamage` | 43 | 20–60 | Lethality at ground zero |
| `ExplosionRadius` | 3 blocks | 2–5 | Destruction radius |
| `MovementSpeed` | 220 cm/s | 150–350 | Stalk pressure |
| `DetectionRange` | 1600 cm | 800–3200 | How early the player is noticed |
| `FuseTriggerRange` | 300 cm | 200–500 | Distance to start fuse |
| `FuseDuration` | 1.5 s | 0.8–3.0 | Countdown time |
| `FuseCancelDistance` | 600 cm | 400–1000 | Escape distance |
| `BurnRate` | N/A | — | Creepers do not burn |
| `LightLevelThreshold` | 7 | 0–15 | Darkness required to spawn |
| `GunpowderDropCount` | 1 | 0–3 | Loot on kill (not detonation) |
| `XPDropped` | 5 | 1–15 | Progression reward |
| `RainFuseModifier` | +0.3 s | 0–0.6 | Wet gunpowder slowdown |
| `AshStormFuseModifier` | -0.2 s | -0.5–0 | Dry powder speedup |
| `HissAudibleRange` | 1600 cm | 800–3200 | How far the hiss can be heard |

## 8. Acceptance Criteria

1. A Creeper spawns only where light level < 7, between 24 m and 128 m from the
   player, on a solid surface, while total mob count < 20.
2. A Creeper does **not** burn in sunlight; it remains active on the surface
   during daytime.
3. A Creeper enters the Explode state when the player is within 3 m, beginning a
   1.5 s fuse with an audible hiss and visual swell.
4. If the player moves beyond 6 m during the fuse, the fuse cancels and the
   Creeper returns to Chase.
5. If the fuse expires, the Creeper detonates: deals up to 43 HP radial damage
   (linear falloff to 0 at 3 blocks) and destroys all blocks in a 3-block sphere
   (except Bedrock).
6. A Creeper that detonates drops no item loot, only 5 XP.
7. A Creeper killed by the player before detonation drops 1 Gunpowder + 5 XP.
8. A Creeper destroyed during its fuse (killed mid-countdown) drops Gunpowder + XP.
9. Creepers do not climb walls or break blocks (only the explosion does).
10. In Rain weather, the fuse duration increases by 0.3 s.
11. In Ash Storm weather, the fuse duration decreases by 0.2 s.
12. Loot rolls are deterministic given a fixed seed.
13. A Creeper despawns when further than 128 m from the player.