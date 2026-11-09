# GDD — Weeping Angels

| Field | Value |
|---|---|
| Status | Draft (2026-11-09) — **0% implemented** |
| System owner | game-designer / ai-programmer |
| Related | `design/gdd/game-concept.md` §5, `design/gdd/npc-colony.md`, `design/levels/LEVEL_DESIGN_FRAMEWORK.md` §4 |
| Source of truth for the code | `Docs/SYSTEMS_REFERENCE.md` §10–11 (current mob AI), `Docs/MOB_REGISTRY.md` |

---

## 1. Overview

The Weeping Angel is the game's primary predator and the only entity that observes the
player the way the player observes it. It is a statue while watched, and it is unbelievably
fast while not. It cannot be killed, cannot be outrun in the open, and cannot be walled out
permanently. The only reliable answer to an Angel is **another observer**.

The Angel system is therefore not an enemy AI. It is the **enforcement mechanism** for the
game's core rule: attention is a resource, and it must be spent, delegated, or paid for.

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
the game knows it.

---

## 3. Detailed Rules

### 3.1 The movement gate (canonical)

```
CanMove = (IsDark OR IsNight) AND NOT IsObserved
```

Both halves are required. A lit interior at midnight freezes them; an unlit cellar at noon
does not. Daylight under open sky always freezes them.

### 3.2 Observation

An Angel is **observed** when at least one observer satisfies all of:

1. the Angel is inside the observer's field of view,
2. no opaque block is between the observer and the Angel's **head or torso**
   (a line to the feet alone is not enough — the statue must be recognisable),
3. the Angel is within the observer's observation range.

**Observers:**

| Observer | Cone | Range | Extra condition |
|---|---|---|---|
| **Player** | `110°` total (`FOVHalfAngle 55°`) | `60 blocks` | None |
| **Camera** | `50°` total (`FOVHalfAngle 25°`) | `120 blocks` | **An operator (NPC or player) must be watching that camera feed**, and the camera must be powered |

A camera with no operator observes nothing. This is the single most important rule in
phase 3: **coverage is staffing, not hardware.**

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

**Design consequence:** mining is loud. A player who digs a tunnel to escape is broadcasting
their position. Running is loud. Fighting a creeper is very loud. The player who wants to be
invisible must stand still in the dark — which is exactly when the Angels can move.

### 3.5 States

| State | Entry | Behaviour | Exit |
|---|---|---|---|
| **Dormant** | Default, or search failed | No movement, no simulation beyond a cheap check | Player enters `ActivationRadius`, or a `NoiseEvent` |
| **Statue** | `NOT CanMove` | Frozen. No pathing, no animation. Pose is the tell. | Gate opens |
| **Stalk** | `CanMove`, player known | Paths to the player at `AngelSpeed` | Player observed continuously → Statue |
| **Investigate** | `NoiseEvent` heard | Paths to the noise origin | Arrives, or becomes observed |
| **Search** | Arrived, nothing found | Looks around for `InvestigateLinger` | Timer ends → Dormant |
| **Attack** | Player within `AttackRange` | Contact kill, then re-enters Stalk | Always |
| **Breach** | Player unreachable, structure in the way | Attacks the blocking block (§3.6) | Path opens, or becomes observed |

The Angel **never** chooses to stand still voluntarily. If it is not moving, it is because it
is being watched.

### 3.6 Contact

Touching an Angel **kills the player instantly**. There is no damage number, no armour
check, no knockback save. This keeps the horror binary and keeps the tuning space on
*attention* rather than on stats.

Death follows the Minecraft convention (`design/gdd/game-concept.md` §5.4): respawn at world
spawn, inventory dropped at the death location.

### 3.7 Breaching

An Angel that cannot reach the player because of geometry **breaks the blocking block**.

- **Any block except Bedrock.** Dirt, stone, planks, obsidian — identical.
- **One block per `BlockBreakInterval` per Angel.** Proposed `5.0 s`. Material does not
  matter; the rule is deliberately flat so that the player cannot "solve" safety with a
  harder material.
- Breaching is **visible and audible**: the block shakes and emits a grinding sound for the
  full 5 seconds, so the player has time to react — if they can get there.
- Multiple Angels on the same wall stack: three Angels breach at `5/3 ≈ 1.67 s` per block.
- Breaching stops instantly when the Angel becomes observed.
- **A sealed bunker is not safe. A watched bunker is.**

This is the rule that makes the fortress necessary: walls buy time, they do not buy safety.

### 3.8 Population and ecology

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

Once convergence starts, the base has three answers: **light it more**, **staff the cameras**,
or **move**. That is the phase-2 decision.

### 3.9 What does not work

| Player attempt | Result | Why |
|---|---|---|
| Shoot it | No damage, ever (until the Source dies) | Invulnerability is the premise |
| Wear better armour | No effect on contact | Death is binary |
| Build a thicker wall | Delays, never prevents | Breaching ignores material |
| Hide in a lit room with the door closed | **Safe only while lit** | Light gate, not geometry |
| Kill it with a trap | Angels are not pushed, crushed or drowned | Not a physics puzzle |
| Look at it through a camera with nobody watching | **Fails** | §3.2 operator requirement |

### 3.10 Invulnerability and the Source

Angels are invulnerable until the **Source** boss is killed (`design/gdd/game-concept.md`
§4.3). After that they become mortal: they take damage, they can die, and contact still
kills the player. The Source is optional, and killing it is the only thing in the game that
changes a law of the world.

---

## 4. Formulas

| Formula | Value | Rationale |
|---|---|---|
| `ObservationLatchTime` | `0.15 s` | Long enough to defeat look-tapping, short enough to feel instant |
| `PlayerFOVHalfAngle` | `55°` | Matches what is on screen, not real vision |
| `PlayerObserveRange` | `60 blocks` | Below the 96-block render distance, so observation never exceeds visibility |
| `CameraFOVHalfAngle` | `25°` | Cameras are narrow — that is why a fortress needs many |
| `CameraObserveRange` | `120 blocks` | Cameras see further than a human |
| `AngelLightThreshold` | `7` (0–15) | Reuses the existing spawner convention |
| `ActivationRadius` | `80 blocks` | Dormant Angels wake when the player is close |
| `AngelSpeed` | `BaseSpeed × PhaseMultiplier` | `1.0 / 1.15 / 1.35` |
| `AttackRange` | `1.5 blocks` | Contact |
| `BlockBreakInterval` | `5.0 s` per Angel per block | Flat, material-independent |
| `InvestigateLinger` | `8 s` | A search must be long enough to be threatening |
| `HearingWallPenalty` | `4 blocks` per solid block | `Loudness - 4 × walls` |
| `ConvergenceRadius` | `250 blocks` | The base's "reach" into the world |
| `BaseSpeed` | TBD — faster than the player's sprint | An Angel in the open is a death sentence |

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

---

## 5. Edge cases

| Case | Resolution |
|---|---|
| Two Angels in a line | Only the front one is observed; the back one moves. Partial occlusion counts. |
| Angel observed by one observer but not another | Observed wins. The player's gaze is authoritative. |
| Player looks at an Angel through a torch flame | Observed (torch is non-solid). |
| Player looks through a glass block | **Not** observed — glass is a full opaque cube in the current build. Revisit if glass becomes transparent. |
| Camera operator leaves the monitor | Coverage lapses immediately; every camera that operator staffed stops observing. |
| Camera has power but no operator | Observes nothing. |
| Operator is asleep / dead / panicking | No observation. NPC state must be surfaced to the player. |
| Angel in an unloaded chunk | Dormant; simulated on the next load with a "moved while you were away" offset, capped by `ActivationRadius`. |
| Player and Angel share a cell | Instant death, no exceptions. |
| Angel tries to breach Bedrock | Cannot. Bedrock is the only absolute wall. |
| Player dies while an Angel is observed | The Angel stays frozen until the respawn, then resumes. No exploit. |
| Angel on the far side of a chasm | Paths around it; may breach if no path exists. |
| The Source is killed | All Angels become mortal; contact still kills. |

---

## 6. Dependencies

| System | Why | Status |
|---|---|---|
| Day/night cycle | `IsNight` half of the gate | Implemented (`UDayNightCycleSystem`) |
| Local light query | `IsDark` half of the gate | Partial — spawner has `CalculateLightLevel`, no general query |
| Voxel raycast | Observation line of sight, breach target | Implemented (`AWorldGenerator` helpers) |
| Navigation / pathing | Stalk, investigate | Partial — current mobs use `AIModule`/`NavigationSystem` |
| Player camera | FOV, forward vector | Implemented |
| **NPC operator** | Camera observation | **Not implemented** (`design/gdd/npc-colony.md`) |
| **Power / electricity** | Camera power | **Not implemented** (`design/gdd/light-and-power.md`) |
| Voxel destruction | Breaching | Implemented (`BreakBlockAtVoxel`) |
| `EMobType` | Angel needs an entry | **Not present** — current enum is Zombie/Skeleton/Spider/Creeper |

---

## 7. Tuning knobs

| Knob | Default | Range | Effect |
|---|---|---|---|
| `ObservationLatchTime` | 0.15 s | 0.05–0.5 | Anti-exploit strength |
| `AngelLightThreshold` | 7 | 0–15 | How easily light protects |
| `AngelSpeed` | TBD | — | How lethal the open is |
| `BlockBreakInterval` | 5.0 s | 2–15 | How long a wall buys |
| `ActivationRadius` | 80 blocks | 40–150 | How early the player is hunted |
| `HearingWallPenalty` | 4 blocks | 0–8 | How much walls muffle sound |
| `InvestigateLinger` | 8 s | 3–20 | Threat duration of a noise |
| `ConvergenceStrength` | 1 + nights/3 | — | Phase-2 pressure curve |
| `ConvergenceRadius` | 250 blocks | 100–400 | How far the base reaches |
| `NestSize[POI]` | per table | — | Difficulty per POI |

---

## 8. Acceptance criteria

1. An Angel in the player's FOV, unobstructed, in range, does not move — day or night.
2. An Angel outside the FOV at night moves toward the player at `AngelSpeed`.
3. An Angel in a lit room does not move at night, even unobserved.
4. An Angel under open sky in daytime never moves.
5. Looking away for one frame does not let an Angel jump: the latch holds.
6. Touching an Angel kills the player instantly and the death is unambiguous.
7. An Angel that cannot path to the player breaches the blocking block in 5 s per Angel.
8. A camera with power but no operator does not freeze anything.
9. A camera with an operator at the monitor freezes Angels inside its cone.
10. A hospital nest contains a finite number of Angels and can be cleared.
11. After `NightsElapsed >= 3`, Angels converge on the base.
12. Killing the Source makes Angels damageable; contact still kills.
13. No Angel is ever damageable before the Source dies.

---

## 9. Build order (implementation plan)

The system is 0% implemented. The order matters: each step must be playable.

1. **`EMobType::Angel` + a placeholder mesh + statue pose.** No behaviour. Verify it reads.
2. **Observation query** — FOV + line trace + range, exposed as a debug HUD number.
3. **Freeze / unfreeze** using the observation query and the latch. No movement yet.
4. **Light gate** + day/night gate on top of the freeze. Verify all four combinations.
5. **Stalk pathing** and contact death.
6. **Hearing + investigation** with a debug noise overlay.
7. **Breaching** with a visible/audible tell.
8. **Nests** placed by the POI generator (after `poi-placement.md`).
9. **Convergence** tied to nights elapsed.
10. **Camera observation** — requires the NPC operator system first.

Steps 1–7 are testable without any other new system. Every step needs an automation test
(`MinecraftDeWish.Angels.*`) per `.opencode/rules/test-standards.md`.

---

## 10. Open questions

- Does hearing reach through a closed door? (Currently: yes, attenuated by the block.)
- Can an Angel breach the block the player is standing on? (Currently: yes — it is a
  legitimate horror moment, but it may be unreadable.)
- Do camera operators panic and stop watching when an Angel gets close? (Proposed: yes, and
  it should be visible.)
- Does the Source have Angels guarding it, or is the approach the puzzle?
- What is the visual tell for "the statue is mid-step", and can it be read at 60 blocks?
