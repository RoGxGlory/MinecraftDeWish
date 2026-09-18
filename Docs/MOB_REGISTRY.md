# Hostile Mob Registry — MinecraftDeWish

This document defines hostile creature statistics, artificial intelligence state machines, special traits, and loot tables.

---

## Mob Roster

### 1. Zombie (`EMobType::Zombie`)
The classic undead melee aggressor.
- **Health**: 20.0 HP (10 hearts)
- **Attack Damage**: 3.0 HP (1.5 hearts)
- **Movement Speed**: 230.0 cm/s
- **Detection Range**: 16.0 meters (1600 cm)
- **Attack Range**: 1.5 meters (150 cm)
- **Attack Cooldown**: 1.0 second
- **Special Behaviors**:
  - `bBurnsInSunlight = true`: Ignites and loses 1 HP/s when under open sky during daytime.
- **Loot Table**:
  - `BlockID 1 (Dirt / Flesh)`: 1–2 items (100% chance)
  - `BlockID 7 (Iron Ore)`: 1 item (5% rare chance)
- **Experience Drop**: 5 XP

---

### 2. Skeleton (`EMobType::Skeleton`)
Ranged undead marksman.
- **Health**: 20.0 HP (10 hearts)
- **Attack Damage**: 4.0 HP (2.0 hearts)
- **Movement Speed**: 250.0 cm/s
- **Detection Range**: 40.0 meters (4000 cm)
- **Attack Range**: 15.0 meters (1500 cm)
- **Attack Cooldown**: 2.0 seconds
- **Special Behaviors**:
  - `bBurnsInSunlight = true`: Ignites in daylight when exposed to sky.
- **Loot Table**:
  - `BlockID 1 (Bone / Arrow drop)`: 1–2 items (100% chance)
- **Experience Drop**: 5 XP

---

### 3. Spider (`EMobType::Spider`)
Agile, low-profile arachnid with wall climbing capabilities.
- **Health**: 16.0 HP (8 hearts)
- **Attack Damage**: 2.0 HP (1.0 heart)
- **Movement Speed**: 300.0 cm/s (Fast)
- **Detection Range**: 16.0 meters (1600 cm)
- **Attack Range**: 2.0 meters (200 cm)
- **Attack Cooldown**: 0.8 seconds
- **Capsule Dimensions**: Half-Height 50 cm, Radius 60 cm (wide and low)
- **Special Behaviors**:
  - `bCanClimbWalls = true`: When forward motion is blocked by a voxel wall, immediately adds vertical climbing velocity ($0.7 \times \text{MoveSpeed} = 210$ cm/s) to scale over blocks and pursue the player.
  - `bBurnsInSunlight = false`: Does not burn during daytime.
- **Loot Table**:
  - `BlockID 17 (String / Web proxy)`: 1–2 items (100% chance)
- **Experience Drop**: 5 XP

---

### 4. Creeper (`EMobType::Creeper`)
Silent ambush stalker that self-detonates near players.
- **Health**: 20.0 HP (10 hearts)
- **Attack Damage**: Up to 43.0 HP (lethal at ground zero)
- **Movement Speed**: 220.0 cm/s
- **Detection Range**: 16.0 meters (1600 cm)
- **Attack Range**: 3.0 meters (300 cm)
- **Fuse Duration**: 1.5 seconds (`ExplosionFuseTimer`)
- **Special Behaviors**:
  - `bExplodes = true`: Within 3.0 meters of player, enters `Explode` state and begins hissing fuse.
  - If player flees beyond 6.0 meters, the fuse cancels and the Creeper returns to `Chase`.
  - When fuse expires:
    - Applies heavy radial damage to player ($43.0 \times \text{DamageScale}$).
    - Breaks all voxel blocks in a 3-block radius sphere ($D_x^2 + D_y^2 + D_z^2 \le 9$).
    - Self-destructs and drops 5 XP (no item loot drops).
- **Experience Drop**: 5 XP

---

## AI State Machine Flowchart

```
                 ┌──────────────┐
                 │     Idle     │
                 └──────┬───────┘
                        │
             ┌──────────┴──────────┐
             ▼                     ▼
      ┌─────────────┐       ┌─────────────┐
      │   Wander    │       │    Chase    │◄─────┐
      └──────┬──────┘       └──────┬──────┘      │
             │                     │             │
             └──────────┬──────────┘             │ (Target backs away)
                        │                        │
             ┌──────────┴──────────┐             │
             ▼                     ▼             │
      ┌─────────────┐       ┌─────────────┐      │
      │   Attack    │       │   Explode   ├──────┘
      │  (Melee)    │       │  (Creeper)  │
      └─────────────┘       └──────┬──────┘
                                   │
                                   ▼
                            ┌─────────────┐
                            │  Detonate!  │
                            └─────────────┘
```

---

## Spawning Rules & Conditions

1. **Light Level Threshold**: Mob can only spawn if local light level $< 7$.
2. **Torch Exclusion**: Mobs will never spawn within 5 blocks of an active torch.
3. **Player Proximity Rings**:
   - $\text{Distance} < 24$ meters: Safe zone (no spawns).
   - $24 \text{ meters} \le \text{Distance} \le 128 \text{ meters}$: Active spawn zone.
   - $\text{Distance} > 128$ meters: Immediate despawn radius.
4. **Surface Validation**: The surface block underneath must be solid (no spawning on air, water, or torches). Two blocks above must be air.
5. **Density Cap**: Maximum 20 hostile mobs active simultaneously within the spawner's operational volume.
