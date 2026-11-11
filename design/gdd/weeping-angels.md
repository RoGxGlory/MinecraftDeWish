# GDD — Weeping Angels (Behavior)

| Field | Value |
|---|---|
| Status | Draft (2026-11-11) — 0% implemented |
| System owner | game-designer / systems-designer / ai-programmer |
| Related | `design/gdd/angels.md` (legacy, retained as historical reference), `design/gdd/game-concept.md` §4–5, `design/gdd/mob-spawning.md`, `design/gdd/lighting-power.md`, `design/gdd/weather-system.md`, `design/gdd/day-night-cycle.md`, `design/gdd/npc-colony.md`, `design/gdd/light-and-power.md`, `design/art/art-direction.md` §6, `Docs/SYSTEMS_REFERENCE.md` §10–11, `Docs/MOB_REGISTRY.md` |
| Source of truth for the code | `UMobSpawnerSystem`, `AMobCharacter`, `UDayNightCycleSystem`, `AWorldGenerator::CalculateLightLevel` (via spawner), `AWorldGenerator::BreakBlockAtVoxel` |

---

## 1. Overview

The Weeping Angel is the game's primary predator and the only entity that observes the
player the way the player observes it. It is a statue while watched, and it is unbelievably
fast while not. It cannot be killed (until the Source dies), cannot be outrun in the open,
and cannot be walled out permanently. The only reliable answer to an Angel is **another
observer**.

The Angel system is therefore not an enemy AI. It is the **enforcement mechanism** for the
game's core rule: attention is a resource, and it must be spent, delegated, or paid for.
The Angel is also the visual identity of the game (art-direction.md §6): the only smooth,
organic, non-voxel form in a voxel world — sculpted stone/plaster, `#E3DCCB`, high
roughness, no metal, no emissive, no VFX ever. The contrast between the voxel world and
the smooth statue **is** the mechanic made visible: a player who sees a smooth shape in a
blocky world knows, without a HUD, that they are looking at an Angel.

The Angel is the only mob that is **never rendered as a voxel**. It uses one of 8 static
statue poses (§3.6) and no walk cycle. Its only animation is movement (teleport-like
snaps between poses) and the grating of stone. It has no eyes.

---

## 2. Player Fantasy

> *"It is not chasing me. It is waiting for me to look away."*

The fantasy is **the gaze as a shield that cannot be held indefinitely**. Every Angel
encounter is a negotiation with the player's own attention:

- The player feels **powerful** while looking: the monster is a statue, harmless.
- The player feels **trapped** while looking: they cannot mine, craft, fight, or flee
  effectively without breaking the gaze.
- The player feels **hunted** the instant they look away, because the statue is now closer.

The Angel makes the player's *body* the resource: you can only look in one direction, and
the game knows it. The fantasy is preserved by three signals that must read without a HUD
(art-direction.md §6 acceptance criterion):

1. **Smooth silhouette** — the Angel is the only non-voxel, organic form in the world.
2. **Material contrast** — bone-white plaster (`#E3DCCB`) against the desaturated,
   ashen voxel world.
3. **Pose that changes between two glances** — the player sees a statue pointing one
   way, looks away, looks back, and it is now mid-stride. The *change* is the proof
   that it moved while unobserved.

At 60 blocks in fog, the Angel must read as a clear, smooth, immobile shape, backlit.
Enough. No VFX, no glow, no aura — ever.

---

## 3. Detailed Rules

### 3.1 The movement gate (quantum lock, canonical)

An Angel may move **only** when all of the following are true:

```
CanMove = (IsDark OR IsNight) AND NOT IsObserved
```

Both halves are required. A lit interior at midnight freezes them; an unlit cellar at noon
does not. Daylight under open sky always freezes them. This is the quantum lock: the Angel
is a statue while observed (or lit), and moves only when unobserved **and** in darkness.

- `IsDark` is the light-level gate: `LightLevel(p) < AngelLightThreshold (7)`. Reuses
  the same threshold as mob spawning (`mob-spawning.md` §3.3). A torch, lamp, furnace
  glow, or sunlit voxel blocks Angel movement.
- `IsNight` is the day/night gate: `UDayNightCycleSystem::IsNight()`. Night exposes
  the surface to moonlight (level 4) but does not by itself allow movement — the voxel
  must still be dark (`L < 7`).
- `IsObserved` is the gaze gate (§3.2–3.3).

An Angel that cannot move becomes a **Statue** (§3.5). It does not animate, does not
path, does not simulate beyond a cheap per-frame observation check. The pose it is frozen
in is the tell (§3.6).

### 3.2 Observation (the quantum lock's other half)

An Angel is **observed** when at least one observer satisfies all of:

1. the Angel is inside the observer's field of view,
2. no opaque block is between the observer and the Angel's **head or torso** (a line to
   the feet alone is not enough — the statue must be recognisable),
3. the Angel is within the observer's observation range.

**Observers:**

| Observer | Cone (total) | Half-angle | Range | Extra condition |
|---|---|---|---|---|
| **Player** | `110°` | `55°` | `60 blocks` | None |
| **Camera** | `50°` | `25°` | `120 blocks` | An operator (NPC or player) must be watching that camera feed, **and** the camera must be powered |

A camera with no operator observes nothing. This is the single most important rule in
phase 3: **coverage is staffing, not hardware.** A fortress full of unstaffed cameras is
a fortress full of statues that move the moment the player blinks.

The line-of-sight test uses a raycast from the observer's eye to the Angel's head/torso
bounding box. A torch flame, a non-solid block, or a transparent voxel (leaves) does not
block the ray; a solid voxel block does. Glass is a full opaque cube in the current build
(art-direction §12.1), so it blocks observation.

### 3.3 The observation latch

Freezing must not be exploitable by flicking the mouse. Observation is **latched**:

```
ObservationSeconds += DeltaTime          when the Angel is observed
ObservationSeconds  = 0                  the instant it is not

IsObserved = ObservationSeconds >= ObservationLatchTime   (0.15 s)
```

- A continuous look freezes the Angel in `0.15 s`.
- Any interruption resets the latch to zero immediately.
- This makes rapid look-tapping useless without punishing a player who genuinely turns
  away: the fantasy is preserved, the exploit is not.

The latch is per-Angel and per-observer aggregated: if *any* observer sustains the latch,
the Angel is observed. When the observing observer stops, the latch resets only if no other
observer is sustaining it.

### 3.4 Perception: sight, hearing, investigation

Angels have **no vision of their own** while frozen, but they perceive the world while
moving. Perception is two-channel:

**Hearing.** Every noisy action emits a `NoiseEvent` with a loudness in blocks. Hearing is
omnidirectional and attenuated by solid geometry.

```
EffectiveNoiseRadius = Loudness - (SolidBlocksBetween * HearingWallPenalty)
Heard = Distance(Angel, NoiseOrigin) <= EffectiveNoiseRadius
```

**Investigation.** A heard noise sets the Angel's target to the noise origin. It moves
there by the shortest path, then lingers. If it finds nothing, it returns to Dormant.

```
OnHeard(Origin):
    TargetPoint = Origin
    State = Investigate
    LingerTimer = InvestigateLinger

OnArrive:
    State = Search
    until LingerTimer <= 0 → State = Dormant
```

**Design consequence:** mining is loud. A player who digs a tunnel to escape is
broadcasting their position. Running is loud. Fighting a creeper is very loud. The player
who wants to be invisible must stand still in the dark — which is exactly when the Angels
can move. Silence is the only stealth, and silence is also the Angel's window.

Noise loudness (proposed):

| Action | `Loudness` (blocks) |
|---|---|
| Standing still | `0` |
| Walking | `4` |
| Sprinting | `10` |
| Mining (per hit) | `8` |
| Breaking a block | `12` |
| Creeper explosion | `40` |
| Village bell | `60` |
| Torch place / pick up | `2` |

### 3.5 States

| State | Entry | Behaviour | Exit |
|---|---|---|---|
| **Dormant** | Default, or search failed | No movement, no simulation beyond a cheap check (activation radius test) | Player enters `ActivationRadius`, or a `NoiseEvent` |
| **Statue** | `NOT CanMove` (gate closed) | Frozen. No pathing, no animation beyond the static pose. Pose is the tell. | Gate opens (`CanMove` true) |
| **Stalk** | `CanMove`, player known | Paths to the player at `AngelSpeed` | Player observed continuously → Statue |
| **Investigate** | `NoiseEvent` heard | Paths to the noise origin | Arrives, or becomes observed |
| **Search** | Arrived, nothing found | Looks around for `InvestigateLinger` | Timer ends → Dormant |
| **Attack** | Player within `AttackRange` | Contact kill, then re-enters Stalk | Always (instant) |
| **Breach** | Player unreachable, structure in the way | Attacks the blocking block (§3.8) | Path opens, or becomes observed |

The Angel **never** chooses to stand still voluntarily. If it is not moving, it is because
it is being watched (or lit). This is the single most important behavioural rule: an
Angel that stands still on its own breaks the fantasy.

### 3.6 Poses (8 statue poses)

The Angel has **no walk cycle**. It uses 8 static statue poses that read as
"caught mid-gesture". A pose is selected when the Angel freezes (enters Statue) and
persists until the Angel moves again. The pose is the **tell** that the player uses to
detect that the statue has changed between two glances.

| # | Pose name | Description | Read at distance |
|---|---|---|---|
| 1 | `HandsOnFace` | Both hands covering the face, head tilted down. The classic "weeping" pose. | Silhouette reads as hunched, arms up. |
| 2 | `Pointing` | One arm extended forward, finger pointing. Body turned toward the target. | Long horizontal silhouette; reads as "aimed". |
| 3 | `HeadTurned` | Torso facing one way, head turned sharply the other way. | Asymmetry reads as "noticing". |
| 4 | `MidStride` | One leg forward, arms at sides, body leaning into motion. | Reads as "caught walking". |
| 5 | `Crouched` | Low crouch, hands on the ground, wings folded tight. | Low, wide silhouette; reads as "coiled". |
| 6 | `WingsHalfOpen` | Standing upright, wings spread to ~45°, arms at sides. | Widest silhouette; reads as "displaying". |
| 7 | `ArmsCrossed` | Arms folded across the chest, wings folded, head level. | Narrow, vertical; reads as "waiting". |
| 8 | `Reaching` | Both arms extended forward and slightly down, fingers spread. | Reads as "grasping". |

**Pose selection rules:**

- When an Angel enters Statue, it keeps its **current pose** if it was already frozen in
  one (no flicker between frames). The pose only changes when the Angel has moved (and
  therefore was unobserved) between two observations.
- When an Angel transitions from moving (Stalk/Investigate) to frozen (Statue), it
  snaps to a new pose chosen by a deterministic roll keyed on `(AngelID, Position,
  TimeOfDay)`. The pose is **not** animated in; it is an instant snap. This is the
  "grating of stone" moment — the player hears a stone-grind sound and the statue is
  now in a different pose.
- The first pose on spawn is `HandsOnFace` (the default "weeping" silhouette).
- Pose changes only happen while unobserved. An observed Angel never changes pose. This
  is what makes the pose-difference the proof of movement: "I looked away, it moved,
  and now it is mid-stride instead of weeping."

**Acceptance (art-direction §6):** a frozen statue is distinguishable without a HUD by
three signals: smooth silhouette, material contrast (`#E3DCCB`), and a pose that changes
between two glances. If the player describes the Angel as "a mob", the contrast has
failed.

### 3.7 Contact (kill)

Touching an Angel **kills the player instantly**. There is no damage number, no armour
check, no knockback save. This keeps the horror binary and keeps the tuning space on
*attention* rather than on stats.

Death follows the Minecraft convention (`game-concept.md` §5.4): respawn at world spawn,
inventory dropped at the death location. The Angel, having killed, re-enters Stalk and
continues hunting (it does not despawn or stop).

Contact is evaluated on overlap between the player's collision capsule and the Angel's
collision volume (a capsule approximating the statue's silhouette, ~60 cm radius, ~180 cm
half-height). The check runs every frame while the Angel is in Stalk or Investigate and
the player is within `AttackRange = 1.5 blocks`.

### 3.8 Breaching

An Angel that cannot reach the player because of geometry **breaks the blocking block**.

- **Any block except Bedrock.** Dirt, stone, planks, obsidian — identical. Material does
  not matter; the rule is deliberately flat so that the player cannot "solve" safety with
  a harder material.
- **One block per `BlockBreakInterval` per Angel.** Proposed `5.0 s`. Breaching is
  **visible and audible**: the block shakes and emits a grinding sound for the full 5
  seconds, so the player has time to react — if they can get there.
- Multiple Angels on the same wall stack: three Angels breach at `5/3 ≈ 1.67 s` per block.
- Breaching stops instantly when the Angel becomes observed (it freezes mid-breach; the
  block's shake stops; progress is preserved — when the Angel unfreezes, it resumes from
  the same progress).
- **A sealed bunker is not safe. A watched bunker is.**

This is the rule that makes the fortress necessary: walls buy time, they do not buy
safety. Breaching uses `AWorldGenerator::BreakBlockAtVoxel` and respects the same
voxel data (the block becomes air, drops its item, and the chunk remeshes).

### 3.9 Population and ecology

Three sources, deliberately combined:

| Source | Rule | Purpose |
|---|---|---|
| **Nests** | Each POI spawns a fixed number of Angels (per POI type). Nests are finite and can be emptied. | Makes exploration methodical and POIs clearable |
| **Night spawn** | A **reduced** global spawn at night near the player (much lower than the current mob spawner) | Prevents the world from feeling empty |
| **Convergence** | Unobserved Angels near the base path toward it over successive nights | Creates the phase-2 pressure that forces relocation |

Nest sizes (proposed):

| POI | Angels | Notes |
|---|---|---|
| Village | `0` | Safe while lit; Angels do not nest in lit places |
| Abandoned hospital | `6–12` | Dark wards, many blind angles |
| Abandoned police station | `3–6` | Small, but the armoury is behind them |
| Chapel / shrine | `1–3` | A single Angel is a whole encounter |
| Horror Forest (ambient) | `1 per ~2 chunks` | Dense by design |

Convergence (proposed):

```
NightsElapsed += 1 each night
ConvergenceStrength = 1 + floor(NightsElapsed / 3)          capped at 6
Each night, up to ConvergenceStrength Angels within
ConvergenceRadius (250 blocks) of the base path toward it.
```

Once convergence starts, the base has three answers: **light it more**, **staff the
cameras**, or **move**. That is the phase-2 decision.

Angels are **not** part of the normal mob cap (`MaxMobCap = 20` is for hostile mobs only).
Angels have their own population rules above. An Angel does not count against the mob
cap, and mobs do not count against Angel population.

### 3.10 Light sensitivity

The Angel is a creature of darkness. Its movement gate (§3.1) requires `IsDark`
(`L < 7`), but its **behaviour** is also shaped by light beyond the binary gate:

- **Preference for darkness.** When an Angel has a choice of paths (e.g., during
  Investigate or when the player is not directly visible), it prefers voxels with lower
  light level. Pathfinding cost is weighted by `LightLevel`: a voxel at `L = 0` costs
  `1.0`, a voxel at `L = 6` costs `4.0`, and a voxel at `L >= 7` is impassable (the
  Angel cannot step onto a lit voxel). This makes Angels flow through shadows and
  avoid lit corridors even when unobserved.
- **Light as a wall.** A lit voxel is not just "not dark" — it is a hard boundary the
  Angel cannot cross while unobserved. The Angel will path around a lit patch, or breach
  through it if no path exists (§3.8). This is why a line of torches across a corridor
  is a temporary barrier, not a solution: the Angel will eventually breach the wall
  beside the torches.
- **Sunlight.** Under open sky during the day, `LightLevel = 15`, so `IsDark` is false.
  The Angel freezes. It does not burn (unlike Zombies/Skeletons) — it simply cannot
  move. At night, open sky drops to `L = 4`, which is `< 7`, so surface movement is
  allowed at night on open ground. This makes Angels a **nighttime surface threat** in
  addition to their cave/interior presence.
- **Weather darkening.** Weather modulates ambient light (weather-system.md §3.4:
  Clear 1.0, Overcast 0.7, Rain 0.6, Fog 0.4, Ash Storm 0.3). When weather modulation
  is wired into `CalculateLightLevel` (design intent), Fog and Ash Storm darken the world
  and widen the Angel's movement zones. An Angel that was frozen under a torch at
  `L = 9` in clear weather is still frozen (`9 >= 7`), but an Angel at `L = 6` in
  clear weather becomes even darker in Fog, and the *surrounding* voxels that were at
  `L = 7` (borderline) may drop below 7, expanding the movement zone. Weather does not
  override the torch gate; it expands the shadow.

### 3.11 Weather integration

Weather affects Angels through three channels:

1. **Light level (§3.10).** Weather darkens `SkyLight` via `WeatherAmbientFactor`,
   widening the dark zones where Angels can move. Fog (0.4) and Ash Storm (0.3) are
   the most dangerous weathers for Angel encounters.
2. **Spawn rate.** Angel night spawns (§3.9) scale with weather: Fog 1.3×, Ash Storm
   1.5× (same multipliers as mob spawning, weather-system.md §3.10). Rain reduces
   Angel spawns (0.7×) because wet surfaces and reduced visibility make the Angel
   harder to spot — but the Angel itself is not slowed by rain.
3. **Aggression.** Weather multiplies Angel movement speed: Fog 1.2×, Ash Storm 1.5×
   (clamped to 1.5×). Rain does not slow the Angel. The Angel is not affected by
   surface wetness (it is stone). This makes Fog and Ash Storm the two weathers where
   an Angel encounter is most lethal.
4. **Sound.** Rain and Ash Storm add ambient noise that can mask the Angel's grating
   stone sound (§3.8). The player may not hear a breach starting during a storm. This
   is intentional tension: weather is not just visual, it is an acoustic veil.

Weather transitions are smooth (30–60 s lerp); the multipliers lerp with them. No
instant snap.

### 3.12 What does not work

| Player attempt | Result | Why |
|---|---|---|
| Shoot it | No damage, ever (until the Source dies) | Invulnerability is the premise |
| Wear better armour | No effect on contact | Death is binary |
| Build a thicker wall | Delays, never prevents | Breaching ignores material |
| Hide in a lit room with the door closed | **Safe only while lit** | Light gate, not geometry |
| Kill it with a trap | Angels are not pushed, crushed or drowned | Not a physics puzzle |
| Look at it through a camera with nobody watching | **Fails** | §3.2 operator requirement |
| Place a torch on the Angel | No effect — Angel is a non-solid? No: Angel is a solid actor. Torch placement requires a solid support block; the Angel's body is not a valid support. | Placement rejected |
| Mine through the floor under the Angel | The Angel falls? No — the Angel is not affected by gravity. It stays at its Y level and continues pathing. | Not a physics puzzle |
| Drown it | No effect — Angels do not drown | Not a physics puzzle |
| Burn it | No effect — Angels do not burn in sunlight; they freeze | Light gate, not damage |

### 3.13 Invulnerability and the Source

Angels are invulnerable until the **Source** boss is killed (`game-concept.md` §4.3).
After that they become mortal: they take damage, they can die, and contact still kills
the player. The Source is optional, and killing it is the only thing in the game that
changes a law of the world. Before the Source dies, no weapon, trap, or environmental
hazard affects an Angel in any way.

---

## 4. Formulas

### 4.1 Observation

```
PlayerFOVHalfAngle   = 55°
PlayerObserveRange   = 60 blocks
CameraFOVHalfAngle   = 25°
CameraObserveRange   = 120 blocks

ObservedByPlayer(Angel, Player) =
    Distance(Player, Angel) <= PlayerObserveRange
    AND AngleBetween(Player.Forward, Direction(Player, Angel.HeadOrTorso)) <= PlayerFOVHalfAngle
    AND LineOfSight(Player.Eye, Angel.HeadOrTorso) == clear   // no solid voxel block in between

ObservedByCamera(Angel, Camera) =
    Camera.IsPowered
    AND Camera.HasOperator (NPC or player watching the feed)
    AND Distance(Camera, Angel) <= CameraObserveRange
    AND AngleBetween(Camera.Forward, Direction(Camera, Angel.HeadOrTorso)) <= CameraFOVHalfAngle
    AND LineOfSight(Camera.Lens, Angel.HeadOrTorso) == clear

IsObserved(Angel) = ObservedByPlayer OR ObservedByCamera (any observer)

ObservationLatch:
    ObservationSeconds += DeltaTime   if IsObservedRaw (any observer satisfies the above)
    ObservationSeconds  = 0           otherwise
    IsObserved = ObservationSeconds >= ObservationLatchTime (0.15 s)
```

### 4.2 Movement

```
CanMove(Angel, p) =
    LightLevel(p) < AngelLightThreshold (7)
    AND (IsNight() OR LightLevel(p) < AngelLightThreshold)   // IsDark is the binding constraint
    AND NOT IsObserved(Angel)

// Simplified: CanMove = (L(p) < 7) AND NOT IsObserved
// IsNight() is implied: at night, open sky L = 4 < 7, so surface movement is allowed.
// By day, open sky L = 15 >= 7, so surface movement is blocked. Caves (L = 0) allow
// movement day or night.

AngelSpeed (effective) = BaseSpeed × PhaseMultiplier × WeatherAggressionMultiplier
    PhaseMultiplier: Phase 1 = 1.0, Phase 2 = 1.15, Phase 3 = 1.35
    WeatherAggressionMultiplier: Clear 1.0, Overcast 1.0, Rain 1.0, Fog 1.2, Ash Storm 1.5
    clamped to 1.5×

BaseSpeed = TBD — must be faster than the player's sprint speed.
    An Angel in the open is a death sentence.
```

### 4.3 Detection (light)

```
IsDark(p) = LightLevel(p) < AngelLightThreshold (7)

LightLevel(p) = clamp(max(SkyLight, TorchLight, LampLight, OtherLight), 0, 15)
    (see lighting-power.md §4)

AngelLightThreshold = 7   (must equal the mob-spawn threshold for consistency)

Pathfinding cost (Angel prefers darkness):
    Cost(p) = 1.0 + (LightLevel(p) / AngelLightThreshold) × 3.0   for L < 7
    Cost(p) = INF (impassable)                                      for L >= 7
    // L = 0 → cost 1.0; L = 6 → cost 1.0 + (6/7)×3 ≈ 3.57
```

### 4.4 Kill condition

```
OnOverlap(PlayerCapsule, AngelCapsule):
    if Distance(Player, Angel) <= AttackRange (1.5 blocks):
        PlayerVitalSystem.ApplyDamage(INSTANT_KILL)   // no damage number, no armour
        // Death follows Minecraft convention: respawn at world spawn, inventory dropped
        Angel.State = Stalk   // Angel continues hunting
```

`INSTANT_KILL` is a sentinel that sets `CurrentHealth = 0` regardless of armour or
resistances. No armour check, no knockback save, no invulnerability frames.

### 4.5 Poses

```
PoseCount = 8
PoseIndex on spawn = 0 (HandsOnFace)

OnFreeze (enter Statue):
    if Angel was moving (Stalk/Investigate) since last freeze:
        newPose = DeterministicRoll(AngelID, Position, TimeOfDay) mod PoseCount
        Angel.Pose = Poses[newPose]
        PlayStoneGrindSound()
    else:
        Angel.Pose = unchanged   // no flicker

OnUnfreeze (gate opens):
    Angel.Pose = unchanged   // pose persists until next freeze after movement
```

The pose change is the proof of movement. An Angel that never moved keeps its pose;
an Angel that moved snaps to a new pose on freeze. This is what the player reads
between two glances.

### 4.6 Hearing / noise

```
EffectiveNoiseRadius = Loudness - (SolidBlocksBetween * HearingWallPenalty)
Heard = Distance(Angel, NoiseOrigin) <= EffectiveNoiseRadius

OnHeard(Origin):
    TargetPoint = Origin
    State = Investigate
    LingerTimer = InvestigateLinger (8 s)

OnArrive:
    State = Search
    LingerTimer counts down → on 0, State = Dormant
```

`HearingWallPenalty = 4 blocks` per solid block between the Angel and the noise origin.
A noise behind 2 solid blocks has its radius reduced by 8 blocks.

### 4.7 Breaching

```
BlockBreakInterval = 5.0 s per Angel per block
BreachingRate (N Angels on same wall) = N / BlockBreakInterval  (blocks per second)

OnBreachStart:
    Block.Shake = true
    PlayGrindSound(Block)
    BreachProgress = 0

OnBreachTick (DeltaTime):
    BreachProgress += DeltaTime / (BlockBreakInterval / NumAngelsOnBlock)
    if BreachProgress >= 1.0:
        AWorldGenerator::BreakBlockAtVoxel(BlockPos)   // block becomes air, drops item
        Angel.State = Stalk   // re-path
        Block.Shake = false

OnObserved (during breach):
    Block.Shake = false
    BreachProgress = preserved   // resumes on unfreeze
```

Bedrock is the only block that cannot be breached (`BreakBlockAtVoxel` returns false
for Bedrock). All other blocks (Dirt, Stone, Obsidian, etc.) breach at the same rate.

### 4.8 Population / convergence

```
NestSize[POI]:
    Village = 0
    AbandonedHospital = RandRange(6, 12)
    AbandonedPoliceStation = RandRange(3, 6)
    Chapel = RandRange(1, 3)
    HorrorForest = 1 per ~2 chunks

NightSpawn (reduced global, near player):
    AttemptsPerNight = 2   (vs. 4–6 for normal mobs)
    SpawnRange = [24, 80] blocks   (closer than normal mobs)
    Only at night, only in dark voxels, only if no Angel within 32 blocks

Convergence:
    NightsElapsed += 1 each night
    ConvergenceStrength = min(6, 1 + floor(NightsElapsed / 3))
    Each night: up to ConvergenceStrength Angels within ConvergenceRadius (250 blocks)
    of the base path toward it (shortest path on the navmesh).
```

---

## 5. Edge Cases

| Case | Resolution |
|---|---|
| Two Angels in a line | Only the front one is observed; the back one moves. Partial occlusion counts — the raycast to the back Angel's head/torso is blocked by the front Angel's body (which is a solid actor). |
| Angel observed by one observer but not another | Observed wins. The player's gaze is authoritative; a camera operator's gaze also counts. |
| Player looks at an Angel through a torch flame | Observed (torch is non-solid, does not block the ray). |
| Player looks through a glass block | **Not** observed — glass is a full opaque cube in the current build. Revisit if glass becomes transparent. |
| Player looks through a leaf block | Observed (leaves are non-solid for raycast purposes, matching torch). |
| Camera operator leaves the monitor | Coverage lapses immediately; every camera that operator staffed stops observing. |
| Camera has power but no operator | Observes nothing. |
| Operator is asleep / dead / panicking | No observation. NPC state must be surfaced to the player (UI indicator on the camera feed). |
| Angel in an unloaded chunk | Dormant; simulated on the next load with a "moved while you were away" offset, capped by `ActivationRadius`. The Angel may have moved up to `ActivationRadius` blocks toward the player while unloaded, but never into a lit voxel. |
| Player and Angel share a cell | Instant death, no exceptions. |
| Angel tries to breach Bedrock | Cannot. Bedrock is the only absolute wall. The Angel stays in Breach state, shaking the Bedrock block, forever (or until observed). |
| Player dies while an Angel is observed | The Angel stays frozen until the respawn, then resumes. No exploit (the latch resets on respawn, but the Angel was frozen by the death itself — the player was looking at it). |
| Angel on the far side of a chasm | Paths around it; may breach if no path exists. |
| The Source is killed | All Angels become mortal; contact still kills. |
| Angel spawns in a lit voxel | Cannot — spawn requires `L < 7` (same gate as mob spawning). Angels only spawn in dark voxels. |
| Angel is mid-breach when observed | Freezes; block stops shaking; progress preserved. On unfreeze, resumes from saved progress. |
| Angel is mid-breach when it rains | Rain does not slow the Angel. The breach continues at full rate. |
| Two Angels observe each other | Irrelevant — Angels do not observe each other. Only players and cameras observe. |
| Angel and player in the same voxel after a teleport | Instant death. No invulnerability frames. |
| Angel in fog at 60 blocks | Reads as a clear, smooth, immobile shape, backlit. Enough (art-direction §6). |
| Angel at exactly 60 blocks (player observe range boundary) | Observed (inclusive bound). At 61 blocks, not observed by player (but may be by a camera). |
| Angel at exactly 120 blocks (camera observe range boundary) | Observed by camera (inclusive bound). |
| Angel at `L = 7` exactly | Not dark (`L < 7` is false). Angel freezes. Boundary is strict. |
| Angel at `L = 6` exactly | Dark. Angel can move (if unobserved). |
| Weather transition mid-breach | Multipliers lerp; breach rate lerps. No instant snap. |
| Player places a torch while an Angel is approaching | If the Angel's current voxel becomes `L >= 7`, it freezes immediately (next frame). If the Angel is mid-stride, it snaps to a pose. |

---

## 6. Dependencies

| System | Why | Status |
|---|---|---|
| Day/night cycle | `IsNight` half of the gate | Implemented (`UDayNightCycleSystem`) |
| Local light query | `IsDark` half of the gate | Partial — spawner has `CalculateLightLevel`, no general `GetLightLevelAt` query (design intent: `light-and-power.md` §6) |
| Voxel raycast | Observation line of sight, breach target | Implemented (`AWorldGenerator` helpers) |
| Navigation / pathing | Stalk, investigate, convergence | Partial — current mobs use `AIModule`/`NavigationSystem` |
| Player camera | FOV, forward vector, eye position | Implemented |
| **NPC operator** | Camera observation | **Not implemented** (`design/gdd/npc-colony.md`) |
| **Power / electricity** | Camera power | **Not implemented** (`design/gdd/light-and-power.md`) |
| Voxel destruction | Breaching | Implemented (`BreakBlockAtVoxel`) |
| `EMobType` | Angel needs an entry | **Not present** — current enum is Zombie/Skeleton/Spider/Creeper; `EMobType::Angel` must be added |
| `UPlayerVitalSystem` | Instant kill on contact | Implemented |
| Weather system | Weather modulation of light, spawn rate, aggression | Implemented (modulation of light level not yet wired) |
| Art pipeline | 8 statue meshes, material `#E3DCCB`, stone-grind SFX | Not started (art-direction §6, §11 priority 5) |
| Source boss | Invulnerability gate | Not designed (`game-concept.md` §4.3) |

Bidirectional note: `mob-spawning.md` §3.3 references this GDD for the shared light
threshold (7). `lighting-power.md` §3.7 and §4 reference this GDD for `IsDark` gate.
`art-direction.md` §6 references this GDD (as `angels.md`) for the 8-pose acceptance
criterion.

---

## 7. Tuning Knobs

| Knob | Default | Range | Effect |
|---|---|---|---|
| `ObservationLatchTime` | 0.15 s | 0.05–0.5 | Anti-exploit strength; shorter = easier to flick-freeze |
| `PlayerFOVHalfAngle` | 55° | 40–70 | How wide the player's gaze is |
| `PlayerObserveRange` | 60 blocks | 30–90 | How far the player can freeze an Angel |
| `CameraFOVHalfAngle` | 25° | 15–40 | How narrow a camera's cone is (narrower = more cameras needed) |
| `CameraObserveRange` | 120 blocks | 60–180 | How far a camera sees |
| `AngelLightThreshold` | 7 | 4–10 | How much light protects; must equal mob-spawn threshold |
| `BaseSpeed` | TBD (>> sprint) | — | How lethal the open is |
| `PhaseMultiplier` | 1.0 / 1.15 / 1.35 | — | Speed scaling per game phase |
| `BlockBreakInterval` | 5.0 s | 2–15 | How long a wall buys |
| `ActivationRadius` | 80 blocks | 40–150 | How early a Dormant Angel wakes |
| `HearingWallPenalty` | 4 blocks | 0–8 | How much walls muffle sound |
| `InvestigateLinger` | 8 s | 3–20 | Threat duration of a noise |
| `AttackRange` | 1.5 blocks | 1.0–2.5 | Contact distance |
| `ConvergenceStrength` | 1 + nights/3, cap 6 | — | Phase-2 pressure curve |
| `ConvergenceRadius` | 250 blocks | 100–400 | How far the base reaches |
| `ConvergenceNightSpawnAttempts` | 2 | 1–4 | Reduced spawn rate vs. normal mobs |
| `NestSize[POI]` | per table | — | Difficulty per POI |
| `WeatherSpawnRateMultiplier` (Angel) | Rain 0.7, Fog 1.3, Ash 1.5 | — | Angel night spawn scaling |
| `WeatherAggressionMultiplier` (Angel) | Fog 1.2, Ash 1.5 | — | Angel speed scaling in weather |
| `PoseCount` | 8 | 4–12 | Number of statue poses |
| `AngelPathfindingLightCost` | 3.0 | 0–5 | How strongly Angels avoid lit voxels when pathing |

All knobs are `EditAnywhere` on the Angel actor / a data asset and should be exposed to
designers via the details panel. No knob value may be hardcoded in C++.

---

## 8. Acceptance Criteria

1. An Angel in the player's FOV, unobstructed, in range, does not move — day or night.
2. An Angel outside the FOV at night moves toward the player at `AngelSpeed`.
3. An Angel in a lit room does not move at night, even unobserved.
4. An Angel under open sky in daytime never moves.
5. Looking away for one frame does not let an Angel jump: the latch holds (0.15 s).
6. Touching an Angel kills the player instantly and the death is unambiguous
   (respawn at world spawn, inventory dropped).
7. An Angel that cannot path to the player breaches the blocking block in 5 s per Angel;
   3 Angels breach in ~1.67 s per block.
8. A camera with power but no operator does not freeze anything.
9. A camera with an operator at the monitor freezes Angels inside its cone.
10. A hospital nest contains a finite number of Angels and can be cleared.
11. After `NightsElapsed >= 3`, Angels converge on the base.
12. Killing the Source makes Angels damageable; contact still kills.
13. No Angel is ever damageable before the Source dies.
14. An Angel freezes instantly when its voxel becomes `L >= 7` (torch placed nearby).
15. An Angel prefers dark voxels when pathing: it will not step onto a lit voxel
    (`L >= 7`).
16. An Angel's pose changes only between two observations where it moved while
    unobserved; an observed Angel never changes pose.
17. An Angel in fog or ash storm moves 1.2× / 1.5× faster and spawns 1.3× / 1.5×
    more at night.
18. No VFX, glow, aura, or emissive appears on the Angel at any time. Only movement
    and stone-grind sound.
19. The Angel is the only smooth, non-voxel form in the world; it reads as a statue
    at 60 blocks in fog without a HUD.
20. Breaching stops instantly when the Angel becomes observed; progress is preserved.
21. Bedrock cannot be breached; the Angel stays in Breach state indefinitely.
22. An Angel does not count against the normal mob cap (`MaxMobCap = 20`).
