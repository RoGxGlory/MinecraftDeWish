# Game Concept — MinecraftDeWish

| Field | Value |
|---|---|
| Status | **Draft for review** (2026-11-09) |
| Owner | creative-director / game-designer |
| Supersedes | `Docs/GAME_DESIGN.md` (now marked obsolete) |
| Implementation status | Sandbox foundation only — **the Angels are 0% implemented** |
| Related | `Docs/SYSTEMS_REFERENCE.md`, `Docs/BLOCK_REGISTRY.md`, `Docs/MOB_REGISTRY.md`, `Docs/TOOL_REGISTRY.md`, `production/audits/2026-11-09-docs-audit.md` |

---

## 1. Overview

**MinecraftDeWish** is a first-person voxel survival game: a Minecraft-like sandbox set in
a post-apocalyptic world where the **Weeping Angels** are the dominant predator. The player
mines, crafts, builds and automates exactly as they would in a block game — but every dark
place is inhabited by creatures that can only move when nobody is looking at them. The game
is therefore not about fighting a stronger enemy. It is about **managing attention**: the
player's gaze, then the light they carry, then the light they install, then the machines
that watch in their place. Progression is measured in how much of the world no longer needs
to be looked at by a human being.

---

## 2. Player Fantasy

> *"I do not get stronger. I get less afraid, because I stop having to look."*

The fantasy is **the builder who tames fear**. The player starts as prey in the dark and
ends as the architect of a place where the dark no longer matters — not because they can
kill the monsters, but because they have built a world that watches for them.

Three emotional beats carry the fantasy:

1. **Vulnerability.** The first night, one torch, a hole in a hillside. Every sound is a
   threat; every shadow must be checked. The player learns that *looking* is a survival
   action with a cost: you cannot look everywhere at once.
2. **Competence.** The player learns to plan with light — corridors, chokepoints, a lit
   perimeter — and starts to feel the world become predictable. Fear turns into logistics.
3. **Mastery.** Cameras, generators, fuel lines and automated walls replace the player's
   eyes. The Angels are still there. They just no longer reach the player. The final
   emotion is not triumph over an enemy but the *quiet* of a machine that never blinks.

The horror is therefore a **curve that descends**, not a pressure that escalates. The game
gets calmer as it gets bigger. This is the opposite of most survival horror and it is the
game's identity.

---

## 3. Design Pillars

| # | Pillar | What it forbids |
|---|---|---|
| **P1** | **The gaze is the mechanic.** Motion is gated by observation, not by health or damage. | No enemy whose counterplay is "deal more damage". No stealth that ignores line of sight. |
| **P2** | **Light is infrastructure.** A torch is a survival system with a cost, a radius and a failure mode. | No cosmetic-only lighting. No free, permanent light. |
| **P3** | **Progress = delegation of attention.** Each phase moves the burden of watching from the player's eyes to something — or someone — else. | No power curve that solves the threat by raw stats. No automation that removes the need for people. |
| **P4** | **The world is a ruin to read.** Villages, hospitals, police stations are evidence of what happened, and they are where the useful things are. | No empty procedural filler. No POI without a reason to enter and a reason to leave. |
| **P5** | **Fear recedes into mastery.** The end-game is a creative sandbox, not a harder fight. | No endless escalation. No final difficulty spike that invalidates the base. |

---

## 4. Core Loop and the Three Phases

### 4.1 The minute-to-minute loop

```
Observe the surroundings
        │
        ├──► is anything moving?  ── no ──►  mine / loot / build
        │                                      │
        │                                      └──► light budget check
        │
        └──► yes ──► hold it in view  ──►  retreat / reposition / seal the angle
```

Mining, crafting, smelting, inventory and vitals are the familiar Minecraft loop and are
already implemented — see `Docs/SYSTEMS_REFERENCE.md`. The Angel layer sits **on top of**
that loop and taxes it with attention.

### 4.2 The three phases

| Phase | Player loop | Binding constraint | Exit condition |
|---|---|---|---|
| **1 — Nomadic** | Explore, gather, craft, find POIs (NPC outposts, abandoned hospitals, police stations). Carry light; never sleep; never settle. | Night and interiors are lethal. No bed exists. The base is wherever the player stops for ten minutes. | Player has enough materials and confidence to build a permanent structure. |
| **2 — Precarious base** | Build walls, install fixed lighting, stockpile, upgrade tools and equipment. | Angels converge on the base. Light must be maintained and expanded; blind spots appear as the base grows. Relocation or deliberate evasion is a real option. | Player can generate power on site and cover blind spots without standing in them. |
| **3 — Fortress** | Cameras, electricity, generators, fuel, industrial automation at AE2 / Draconic scale — and the **colonists** who operate the camera banks, farm the food and guard the walls. | The fortress is only as safe as its power **and its staff**. Every camera is a load; every watcher is a mouth; every outage or death is a blind angle. | Player chooses to pursue the optional terminal objective — or not. |

### 4.3 The optional terminal objective

The Angels are **invulnerable** until the player kills the **Source**: an end-game boss that
is the origin of their invulnerability. Killing it is optional, is the only "ending" the
game offers, and permanently changes the world: the Angels become mortal.

After the Source dies the game does not end. It becomes a **post-horror sandbox**: the
threat is gone, the machines remain, and the player is free to build. This is deliberate —
the reward for finishing the horror is the creative game underneath it.

---

## 5. Detailed Rules

### 5.1 The Angel movement rule (canonical)

An Angel may move **if and only if both conditions hold**:

```
CanMove = (IsDark OR IsNight) AND NOT IsObserved
```

- `IsObserved` is true when **all** of the following hold:
  1. the observer has the Angel inside its field of view,
  2. there is no opaque geometry between the observer and the Angel,
  3. the Angel is within the observer's observation range.
- **Observers** are: the player, and (in phase 3) any **powered camera with an operator at
  its monitor**. A camera that nobody is watching observes nothing — hardware is not
  coverage, staffing is. See `design/gdd/angels.md` §3.2 and `design/gdd/npc-colony.md`.
- **`IsDark`** is a per-Angel test of the local light level, not a global day/night flag.
  A lit room at midnight is safe; an unlit cellar at noon is not.
- Daylight under open sky therefore always freezes them. This is the game's one guaranteed
  safe state, and the reason surface exploration stays playable.

### 5.2 Angel behaviours

| State | Trigger | Behaviour |
|---|---|---|
| **Statue** | `NOT CanMove` | No movement, no pathing. Visual tell: the pose is wrong, the head is turned, the hands are raised. |
| **Stalk** | `CanMove` and target not adjacent | Moves toward the player by the shortest available path. Fast, silent, no telegraph beyond displacement. |
| **Investigate** | Heard a noise | Moves to the noise origin, then searches for a few seconds before going dormant. |
| **Close** | Within strike range | Contact **kills instantly**. Angels do not "duel". |
| **Breach** | Player unreachable, a block is in the way | Slowly breaks the blocking block — **one block per 5 s per Angel**, regardless of material. Visible and audible for the full 5 s. |
| **Reposition** | Player looks away mid-approach | Keeps closing; may reroute to break line of sight before the player looks back. |
| **Dormant** | Far from the player, no light, not night | Idle in place, unrendered or minimally simulated, to keep the world cheap. |

Full rules, formulas and edge cases: `design/gdd/angels.md`.

### 5.3 Observation and the cost of looking

Observation is not free: the player cannot see behind themselves, cannot see through walls,
and cannot watch two corridors at once. Torches let the player **create** observation
(they raise the local light level, which is the other half of the gate), but they consume
fuel and time. This is the economic core of the game: **light buys attention**.

### 5.4 No sleep, death, and time

- **No bed. No sleep. No time skip.** Night is content that must be survived awake.
- **Death** follows the Minecraft convention: respawn at the world spawn point, inventory
  dropped where the player died. Recovering the corpse is a deliberate, tense traversal
  back into the dark.
- **Contact with an Angel is instant death.** No damage number, no armour check, no
  knockback save. The horror stays binary so the tuning space stays on attention.
- **Time** advances only while the player is playing. There is no way to fast-forward a
  night; the only ways to survive it are light, distance, or architecture.

### 5.5 The world

- Post-apocalyptic surface: biomes (Plains, Forest, Mountains, Desert, **Horror Forest**)
  with ruins seeded into them.
- **POIs** are the narrative and economic anchors: NPC villages/outposts (trade, safety,
  light), abandoned hospitals (medical loot, dark interiors, high Angel density), abandoned
  police stations (weapons, armour, evidence of the collapse).
- The world is procedurally generated, but **POIs are authored** and placed by the
  generator. This is the "blending handmade design with procedural generation" approach —
  see `design/levels/LEVEL_DESIGN_FRAMEWORK.md`.

### 5.6 Bestiary

- **Angels** are the primary threat and the only threat that scales with the three phases.
- **Classic mobs** (Zombie, Skeleton, Spider, Creeper) remain as secondary fauna. They give
  the surface a cost, they make noise and light discipline matter, and they fill the gap
  before Angels are implemented. They are never the reason the player is afraid at night —
  the Angels are.

### 5.7 Non-goals

- **Not a Minecraft clone.** Parity with Minecraft is not a goal; the Baubles equipment
  model and the horror layer are intentional divergences.
- **No co-op in v1.** Multiplayer is a post-v1 objective, explicitly out of scope here.
  (It is thematically excellent — two players relaying the gaze — which is exactly why it
  must be designed deliberately rather than bolted on.)
- **No realistic simulation.** No fluid dynamics, no structural physics, no survival
  minutiae (thirst, temperature) beyond what the Angel layer needs.
- **No combat system as the answer to Angels.** Shooting them is not the fantasy.
- **No mandatory ending.** The Source is optional.

---

## 6. Formulas

All values below are **proposed** and must move into DataTables/DataAssets before
implementation. They are stated here so they can be argued with.

### 6.1 Observation test

```
IsObserved = InFOV AND HasLineOfSight AND (Distance <= ObserveRange)

InFOV = dot(normalize(AngelPos - ObserverPos), ObserverForward) > cos(FOVHalfAngle)
```

| Variable | Meaning | Proposed default |
|---|---|---|
| `FOVHalfAngle` | Half-angle of the observation cone | `55°` (≈110° total, so peripheral vision still counts) |
| `ObserveRange` | Maximum observation distance | `6000 cm` (60 m) — beyond this the Angel is a silhouette, not a witness |
| `HasLineOfSight` | Unobstructed trace to the Angel's **head and torso**, not its feet | — |
| Camera variant | Cameras use a narrower cone and a longer range, and **only observe when an operator is watching their feed** | `FOVHalfAngle 25°`, `ObserveRange 12000 cm` |

Rationale: the cone must match what the *player* actually sees on screen, not a realistic
field of view, or the rule feels arbitrary. Cameras are deliberately worse at wide coverage
and better at distance — that is why a fortress needs **many** of them.

### 6.2 Light gate

```
IsDark = LocalLightLevel < AngelLightThreshold
```

| Variable | Proposed default | Rationale |
|---|---|---|
| `AngelLightThreshold` | `7` (Minecraft 0–15 scale) | Reuses the existing spawner convention (`MobSpawnerSystem` already treats `< 7` as dark), so the player learns one number |
| Torch light level | `14` at the source, falling off with distance | Matches the existing `HasNearbyTorch` implementation |

### 6.3 Angel speed by phase

```
AngelSpeed = BaseSpeed * PhaseMultiplier[Phase]
```

| Phase | `PhaseMultiplier` | Rationale |
|---|---|---|
| 1 — Nomadic | `1.0` | Fast enough that being caught in the open at night is fatal |
| 2 — Precarious base | `1.15` | Slightly faster, so a static base eventually loses the race |
| 3 — Fortress | `1.35` | Fast enough that an unpowered blind spot is immediately punished |

The multiplier is a *pressure* knob, not a difficulty slider: it forces the phase-2 base to
be either expanded or abandoned.

### 6.4 Light economy

```
TorchBurnTime = BaseBurnTime / BurnRateMultiplier
FuelCostPerCameraSecond = CameraDrawWatts / FuelEnergyDensity
```

Not yet tuned — the phase-3 economy needs its own GDD (`design/gdd/light-and-power.md`).
The invariant to preserve: **coverage must always cost something**, or phase 3 stops being
a game.

---

## 7. Dependencies

| System | Status | Document |
|---|---|---|
| Voxel world generation, chunk streaming, meshing | Implemented | `Docs/SYSTEMS_REFERENCE.md` §1–2 |
| Mining queue, tool tiers, Baubles | Implemented | `Docs/SYSTEMS_REFERENCE.md` §6–7, `Docs/TOOL_REGISTRY.md` |
| Crafting & smelting | Implemented | `Docs/SYSTEMS_REFERENCE.md` |
| Player vitals, XP | Implemented | `Docs/SYSTEMS_REFERENCE.md` §8, §12 |
| Day/night cycle | Implemented | `Docs/SYSTEMS_REFERENCE.md` §9 |
| Classic mob spawning and AI | Implemented | `Docs/SYSTEMS_REFERENCE.md` §10–11, `Docs/MOB_REGISTRY.md` |
| **Angel AI (observation, light gate, state machine)** | **Not implemented** | `design/gdd/angels.md` (to write) |
| **Light & power economy (phase 3)** | Not implemented | `design/gdd/light-and-power.md` (to write) |
| **NPC colony (recruitment, roles, hunger/thirst)** | **Not implemented** | `design/gdd/npc-colony.md` (to write) |
| **Item registry (replaces block proxies)** | Not implemented | `design/gdd/items.md` (to write) |
| **POI / structure placement** | Partial (`VoxelStructureGenerator` villages) | `design/gdd/poi-placement.md` (to write) |
| Level design framework | Not written | `design/levels/LEVEL_DESIGN_FRAMEWORK.md` |
| Save format | Not documented | `design/gdd/save-load.md` (to write) |

---

## 8. Tuning Knobs

| Knob | Default | Range | Effect |
|---|---|---|---|
| `AngelLightThreshold` | 7 | 0–15 | Lower = Angels move in brighter places = harder |
| `ObserveRange` | 6000 cm | 2000–15000 | Longer = safer open ground, weaker darkness |
| `FOVHalfAngle` | 55° | 35–75 | Wider = forgiving, but peripheral-freezing looks odd |
| `AngelSpeedMultiplier[Phase]` | 1.0 / 1.15 / 1.35 | 0.6–2.0 | The phase-pressure curve |
| `AngelSpawnDensity` | TBD | — | Population pressure per biome |
| `TorchBurnTime` | TBD | — | How often the player must return to light |
| `CameraObservationRange` | 12000 cm | 6000–20000 | Coverage per camera, and therefore power load |
| `SourceBossInvulnerabilityWindow` | TBD | — | How long the end-game fight can last |

---

## 9. Acceptance Criteria

The concept is correctly implemented when:

1. **Observation freezes.** An Angel in the player's FOV, unobstructed, within range, does
   not move — even at night in the dark.
2. **The gaze is insufficient alone.** An Angel outside the FOV at night moves toward the
   player, regardless of how well lit the player thinks the area is.
3. **Light freezes.** An Angel in a fully lit room does not move, even with the player's back
   turned, at night.
4. **Daylight freezes.** No Angel moves under open sky during the day.
5. **Statues read.** A frozen Angel is visually distinguishable from a moving one without
   a HUD marker.
6. **Night cannot be skipped.** There is no bed, no sleep, no time-skip action in the build.
7. **Death is Minecraft-shaped.** Player respawns at world spawn and the inventory is
   recoverable at the death location. Angel contact is instant death.
8. **The Source ends invulnerability.** Before it dies, no attack can kill an Angel; after,
   Angels take damage and die.
9. **The world stays open after the Source.** The player can keep playing and building.
10. **Phase 3 is a power game.** A camera with no power observes nothing.
11. **Walls buy time, not safety.** An Angel blocked by a wall breaches one block every 5 s,
   visibly and audibly, and stops the instant it is observed.
12. **A camera needs an operator.** A powered, correctly aimed camera with nobody at its
   monitor freezes nothing; the same camera with an assigned operator freezes Angels in its
   cone.

---

## 10. Level Design Lens (Totten)

The concept is read through *An Architectural Approach to Level Design* (2nd ed.):

- **Ch. 6 — Dangerous Architecture (prospect & refuge):** the entire night loop. Surfaces
  are prospect; interiors are refuge, except that refuge is where the Angels are densest.
- **Ch. 10 — Possibility Spaces:** the three phases are a **degenerative design** in reverse —
  the world starts maximally hostile and is progressively made intelligible and safe by the
  player's interventions.
- **Ch. 11 — Procedurally Generated Levels:** the biome pattern language plus authored POIs
  seeded into it. This is the project's core level-design method.
- **Ch. 4 — Basic Gamespaces:** Lynch's landmarks/paths/nodes/edges/districts are the only
  way to navigate an infinite voxel world without a minimap.
- **Ch. 8 — Level 1–1:** the first ten minutes must teach "look at the statue" without a
  single line of text.

See `design/levels/LEVEL_DESIGN_FRAMEWORK.md` for the applied framework.
