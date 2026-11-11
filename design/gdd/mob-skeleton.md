# GDD — Mob: Skeleton

| Field | Value |
|---|---|
| Status | Draft (2026-11-11) |
| System owner | game-designer / ai-programmer |
| Related | `Docs/MOB_REGISTRY.md`, `Docs/SYSTEMS_REFERENCE.md` §10–11, `design/gdd/items.md` §3.7, `design/gdd/weather-system.md` §3.4, `design/art/art-direction.md` §5 |
| Source of truth for code | `AMobCharacter`, `UMobSpawnerSystem`, `EMobType::Skeleton` |

---

## 1. Overview

The Skeleton is the game's ranged undead marksman: a stained, half-dressed bone
creature that engages the player at distance with projectiles rather than melee.
It shares the Zombie's sunlight vulnerability and 20 HP but trades melee damage
for a longer engagement range (15 m) and a slower fire rate (2.0 s). Its role is
to **punish open-ground movement** and to make the player value cover, terrain and
lighting: a Skeleton on a hill at night can pin a player who would otherwise walk
past a Zombie. Skeletons drop Bone (and occasionally String), award 5 XP, and burn
in sunlight.

## 2. Player Fantasy

> *"It does not chase you. It waits for you to step into the open."*

The player should feel that the Skeleton is **a tactical threat, not a brute**.
Where the Zombie is a wall of flesh to be cut down, the Skeleton is a problem to
be solved: break line of sight, close the distance, or take cover. The fantasy is
the **sniper duel in the dark** — the player learns to read terrain, use blocks as
cover, and time their approach. A Skeleton that the player never sees (only hears
the twang) is working as intended.

## 3. Detailed Rules

### 3.1 Identity and stats

| Stat | Value | Notes |
|---|---|---|
| Health | 20 HP (10 hearts) | Same as Zombie/Creeper |
| Attack damage | 4 HP (2.0 hearts) per projectile | Ranged; higher per-hit than Zombie (3) |
| Movement speed | 250 cm/s | Faster than Zombie (230), slower than Spider (300) |
| Detection range | 40 m (4000 cm) | Longest of the humanoid mobs — it sees you first |
| Attack range | 15 m (1500 cm) | Ranged engagement distance |
| Attack cooldown | 2.0 s | One shot every 2 seconds |
| Minimum range | 3 m (300 cm) | Below this, the Skeleton backs away to re-establish range |
| Projectile speed | 25 m/s (2500 cm/s) | Arrow-equivalent; travels in a slight arc |
| Capsule | Standard humanoid (`SK_Mob_Base` 6-bone rig) | — |
| `bBurnsInSunlight` | `true` | Ignites and loses 1 HP/s in open sky during daytime |
| `bCanClimbWalls` | `false` | — |
| `bExplodes` | `false` | — |

### 3.2 Spawn rules

Same spawner as all hostile mobs (`UMobSpawnerSystem`):

- **Light level:** spawn only where `CalculateLightLevel < 7`.
- **Surface:** valid solid surface.
- **Distance from player:** between `MinSpawnDistance = 24 m` and `MaxSpawnDistance = 128 m`.
- **Time of day:** night and dark interiors at any time.
- **Cap:** shared `MaxMobCap = 20`.
- **Despawn:** beyond 128 m.

Skeletons have no special spawn preference (no "high ground" bias in v1), but
their long detection range means a spawn on a hill is effectively a sniper.

### 3.3 AI behavior

```
Idle → Wander
     → Chase (player detected within DetectionRange)
     → Strafe (within AttackRange, maintaining distance)
     → Shoot (cooldown elapsed, line of sight clear)
     → Retreat (player within MinimumRange)
```

- **Idle / Wander:** slow random walk; scans for targets.
- **Chase:** move toward the player until within `AttackRange`, then transition to
  Strafe. If the player is beyond `AttackRange`, keep chasing.
- **Strafe:** lateral movement while keeping the player in front; maintains a
  preferred distance of ~10–14 m. Tries to keep a block or terrain feature between
  itself and the player when possible (simple cover-seeking: move perpendicular to
  the player's line of advance).
- **Shoot:** when `AttackCooldown` has elapsed and line of sight to the player's
  torso is clear, fire a projectile. The projectile travels at `ProjectileSpeed`
  with a slight gravity arc.
- **Retreat:** if the player closes to `MinimumRange` (3 m), the Skeleton backs
  away at `MovementSpeed` until range is re-established, firing while retreating.
- **Loss of target:** if the player leaves `DetectionRange` for > 5 s, return to
  Wander. (Longer than the Zombie's 3 s — Skeletons are more patient.)
- **Sunlight burn:** same as Zombie — 1 HP/s in open sky during daytime. Skeletons
  do not seek shade actively; they continue to shoot while burning.

### 3.4 Drops and XP

Per `design/gdd/items.md` §3.7:

- **Guaranteed:** `Item.Bone × 1–2` (100 %).
- **Rolled:** `Item.String` at 25 % chance (1 item).
- **Experience:** 5 XP.

### 3.5 Weather effects

| Weather | Spawn rate mult. | Aggression mult. | Effect on Skeleton |
|---|---|---|---|
| Clear | 1.0 | 1.0 | Baseline; long sight lines favor Skeletons |
| Overcast | 1.0 | 1.0 | Baseline |
| Rain | 0.7 | 0.8 | Fewer spawns; projectiles slightly less accurate (wind) |
| Fog | 1.3 | 1.2 | More spawns; detection range reduced to 20 m (fog); still dangerous at close range |
| Ash Storm | 1.5 | 1.5 | Most dangerous; detection range reduced to 15 m but spawn surge |

**Fog interaction:** in Fog weather, the Skeleton's effective detection range is
halved (`40 m → 20 m`) because visibility is reduced. This is a partial
counterbalance to the spawn surge.

## 4. Formulas

### 4.1 Time to kill (player, ranged exchange)

Skeleton projectile damage = 4 HP. Player with a sword deals `SwordDamage` per hit
at melee range. The Skeleton will not let the player close easily, so the
effective exchange depends on terrain.

```
SkeletonTTK_player = SkeletonHealth / PlayerSwordDamage   (if player closes to melee)
PlayerTTK_skeleton = PlayerHealth / (4 * ShotsLanded)     (ranged, player cannot easily retaliate)
```

### 4.2 Projectile travel time

```
TravelTime = Distance / ProjectileSpeed = d / 2500  (seconds, d in cm)
```

At max attack range (1500 cm): `TravelTime = 1500 / 2500 = 0.6 s`.

### 4.3 Lead / prediction (simple)

The Skeleton aims at the player's current position plus a lead based on player
velocity:

```
AimPoint = PlayerPos + PlayerVelocity * (Distance / ProjectileSpeed)
```

No advanced prediction; this keeps the Skeleton beatable by changing direction.

### 4.4 Detection

```
Detected = (Distance(Player, Skeleton) <= EffectiveDetectionRange)
         AND (LineOfSight(Player, Skeleton) == true)
         AND (LightLevelAt(Skeleton) < 7 OR IsNight)
```

where `EffectiveDetectionRange = DetectionRange * WeatherVisibilityFactor`
(Clear/Overcast = 1.0, Rain = 0.9, Fog = 0.5, Ash Storm = 0.375).

### 4.5 Sunlight burn time-to-death

```
BurnTTD = 20 / 1 = 20 s   (same as Zombie)
```

### 4.6 Spawn eligibility

Same as Zombie (§4.4 of mob-zombie.md), with the same weather multipliers.

### 4.7 XP award

```
XP = 5
```

## 5. Edge Cases

- **Skeleton on high ground:** no special spawn bias, but a naturally spawned
  Skeleton on a hill is a severe threat. The player must use cover or close the
  distance. This is intended.
- **Skeleton with no line of sight:** does not fire; moves to re-establish LOS.
  Will not fire through solid blocks.
- **Skeleton at minimum range:** retreats while firing. A player who sprints
  directly at a Skeleton will take 1–2 shots before closing to melee.
- **Skeleton in a 1-block-wide corridor:** cannot strafe; fires in a straight
  line. The player can use the corridor as a kill funnel.
- **Skeleton and Zombie in the same fight:** Skeleton pins the player at range
  while Zombie closes. This is the intended combined threat.
- **Skeleton dies from sunlight burn:** still drops loot and XP.
- **Skeleton shot at a player behind a torch:** torch is non-solid, so the shot
  passes through. The player is not protected by a torch.
- **Skeleton in fog:** detection range halved; fires only when it can see the
  player. Spawn surge compensates.
- **Skeleton shot lands on a block:** projectile is destroyed on impact with
  solid terrain; no block damage.
- **Multiple Skeletons focusing one player:** each fires independently on its own
  cooldown. No coordination in v1.
- **Weather transition during strafe:** aggression multiplier lerps; strafe speed
  and fire rate adjust gradually.

## 6. Dependencies

| System | Why | Status |
|---|---|---|
| `UMobSpawnerSystem` | Spawn cycle, light level, despawn | Implemented |
| `AMobCharacter` | Health, damage, sunlight burn, drops, XP | Implemented |
| `UDayNightCycleSystem` | Day/night, weather multipliers | Implemented |
| `UPlayerVitalSystem` | Damage application, death | Implemented |
| `UBaublesSystem` | Sword damage (melee counter) | Implemented |
| Projectile system | Arrow/bolt actor | **Not implemented** (placeholder melee in current code) |
| Item layer (`design/gdd/items.md`) | Bone, String drops | Not implemented (block proxies today) |
| `AXPOrbPickup` | XP drop | Implemented |
| `NavigationSystem` | Pathing, cover-seeking | Implemented (basic) |

## 7. Tuning Knobs

| Knob | Default | Range | Effect |
|---|---|---|---|
| `Health` | 20 | 10–40 | Survivability |
| `AttackDamage` | 4 | 2–8 | Per-shot threat |
| `MovementSpeed` | 250 cm/s | 150–400 | Chase/strafe speed |
| `DetectionRange` | 4000 cm | 2000–6000 | How early the player is spotted |
| `AttackRange` | 1500 cm | 800–2500 | Ranged engagement distance |
| `MinimumRange` | 300 cm | 100–600 | Retreat trigger distance |
| `AttackCooldown` | 2.0 s | 1.0–4.0 | Fire rate |
| `ProjectileSpeed` | 2500 cm/s | 1500–4000 | Projectile travel speed |
| `ProjectileGravity` | 0.5× | 0–1.0 | Arc steepness |
| `BurnRate` | 1 HP/s | 0.5–2 | Sunlight lethality |
| `LightLevelThreshold` | 7 | 0–15 | Darkness required to spawn |
| `DetectionLossTimeout` | 5.0 s | 2–10 | Patience before giving up |
| `BoneDropMin/Max` | 1 / 2 | 0–4 | Loot quantity |
| `StringDropChance` | 0.25 | 0–0.5 | Rare loot rate |
| `XPDropped` | 5 | 1–15 | Progression reward |
| `FogDetectionFactor` | 0.5 | 0.25–0.75 | Detection range in fog |

## 8. Acceptance Criteria

1. A Skeleton spawns only where light level < 7, between 24 m and 128 m from the
   player, on a solid surface, while total mob count < 20.
2. A Skeleton detects the player within 40 m with line of sight.
3. A Skeleton fires a projectile dealing 4 HP every 2.0 s within 15 m when line of
   sight is clear.
4. A Skeleton retreats when the player is within 3 m, firing while retreating.
5. A Skeleton strafes to maintain ~10–14 m distance from the player.
6. A Skeleton under open sky during daytime ignites and loses 1 HP/s.
7. A Skeleton drops 1–2 Bone (100 %) and String at 25 % chance, plus 5 XP, on death.
8. A Skeleton does not fire through solid blocks.
9. In Fog weather, a Skeleton's detection range is halved (40 m → 20 m).
10. Loot rolls are deterministic given a fixed seed.
11. A Skeleton despawns when further than 128 m from the player.