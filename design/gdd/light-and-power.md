# GDD — Light & Power

| Field | Value |
|---|---|
| Status | Draft (2026-11-09) — **0% implemented** |
| System owner | game-designer / systems-designer / gameplay-programmer |
| Related | `design/gdd/game-concept.md` §4.2, `design/gdd/angels.md` §3.1–3.2, `design/gdd/npc-colony.md` §3.4 |

---

## 1. Overview

Light is the only defence in the game that **scales**. A torch protects a radius; a lit base
protects a volume; a camera protects a line of sight. All of it costs energy, and energy
costs fuel, and fuel costs **noise** — which is the one thing the Angels can hear.

The power system is therefore the phase-3 economy: the player builds a machine that keeps
the dark away, and then has to feed it forever. Every lamp, camera and monitor is a load on
a generator that is also a beacon.

**The central invariant:** *the fortress must be cheaper to run than it is to abandon.* If
fuel logistics cost more player time than relocating, the phase-3 loop collapses into
chores.

---

## 2. Player Fantasy

> *"The generator is running. The lights are on. Nothing in this house can move."*

The fantasy is **the hum**. The player who spent phase 1 holding a torch now owns a building
that is lit by a machine, and the machine's sound is the sound of safety — except that the
same sound is what the Angels follow.

Three beats:

- **The first switch.** One lamp, one generator, one room that is safe without a torch. It
  is the first time the player has been safe without *looking*.
- **The first blackout.** The fuel runs out at night. The player learns that the fortress is
  a promise the generator can break.
- **The first staffed night.** A watcher on the monitors, the generator fuelled, the lights
  on a night schedule. The player can finally do something other than watch.

---

## 3. Detailed Rules

### 3.1 Light sources

Light is measured on the Minecraft-style `0–15` scale. `AngelLightThreshold` is `7`
(`design/gdd/angels.md` §4).

| Source | Light level | Powered | Safe radius (`level − 7`) | Notes |
|---|---|---|---|---|
| **Daylight (open sky)** | `15` | No | Entire surface | Free, but only above ground and only by day |
| **Torch** | `14` | No | `7` blocks | Craftable from phase 1; infinite burn in the current build |
| **Campfire** | `15` | No | `8` blocks | Loud (`Loudness 6`), consumes wood |
| **Electric lamp** | `15` | Yes (`2 EU/s`) | `8` blocks | The workhorse of phase 2–3 |
| **Floodlight** | `15` | Yes (`6 EU/s`) | `20` blocks in a cone | Directional; covers a wall or a yard, not a room |
| **Monitor glow** | `4` | Yes | `0` | A monitor room is **not** a lit room — it is dark by design |

**Falloff rule:** light drops by 1 per block of distance through air, and is blocked by
opaque blocks. A torch's safe radius is therefore literal, and a base that is 40 blocks wide
cannot be lit by torches alone — which is exactly when electricity becomes rational.

### 3.2 The power network

Three node types:

| Node | Function | Placement |
|---|---|---|
| **Generator** | Produces `EU/s` while fuelled. Loud. | Anywhere, but the player will learn to bury it |
| **Battery** | Stores `EU`, discharges when capacity < load | Near the generator or the critical zone |
| **Consumer** | Draws `EU/s` while enabled | Lamps, floodlights, cameras, monitors, machinery |

Circuits are connected by cable blocks. A circuit is a **connected graph**; capacity and load
are summed per circuit. There is no global grid — a fortress can have several circuits, which
is how the player isolates the critical zone from the workshop.

```
CircuitCapacity = Σ GeneratorOutput
CircuitLoad     = Σ ConsumerDraw
```

### 3.3 Brownout and shedding

When `CircuitLoad > CircuitCapacity` and the battery is empty, the circuit **sheds load in a
fixed priority order**:

| Priority | Shed order | Why |
|---|---|---|
| 1 | Industry / automation | Least dangerous to lose |
| 2 | Non-security lamps (decorative, workshops) | Annoying, not lethal |
| 3 | Monitor banks | Cameras go blind — dangerous, but lights still hold the rooms |
| 4 | Cameras | The blind angles open |
| 5 | **Security lamps (last)** | Lights are the final line: they keep the rooms and the colonists alive |

The shedding order is the design statement of the whole system: **the lights are the last
thing to go out.** Cameras are a luxury of a healthy grid.

### 3.4 Fuel

| Fuel | Energy | Notes |
|---|---|---|
| Planks | `10 000 EU` | Early, wasteful |
| Charcoal | `40 000 EU` | Renewable via forestry |
| Coal | `50 000 EU` | The phase-2 staple |
| Biofuel | `120 000 EU` | Phase-3; produced by a farm + refinery, staffed by colonists |

```
FuelPerDay = (CircuitLoad × DayLength) / EnergyPerFuel
```

At `DayLength = 1200 s`, a `100 EU/s` base consumes `120 000 EU` per day: about **2.4 coal**
or **1 biofuel** per day. That is a readable number the player can plan around.

**Night-only scheduling** halves consumption: a lamp set to `NightOnly` is off during the
day, when the sun does the same job for free. This is the first efficiency lesson and it is
deliberately discoverable.

### 3.5 Noise is the tax

Generators are the loudest thing the player can build.

| Source | `Loudness` (blocks) |
|---|---|
| Generator | `30` |
| Industrial generator | `60` |
| Soundproofing a generator room | `×0.4` (`30 → 12`) |

Noise feeds `design/gdd/angels.md` §3.4: an Angel that hears the generator investigates it,
and convergence toward the base (`angels.md` §3.8) is **faster near a loud base**.

**This is the intended trade:** power protects the interior and advertises the exterior. The
player who buries and soundproofs the generator pays in space and materials; the player who
does not pays in Angels.

### 3.6 Camera banks and monitor rooms

A **camera bank** is a physical monitor room containing a monitor block, wired to up to
**4 cameras**.

Each camera in a bank requires:

1. **Power** (the circuit is live),
2. **An operator assigned to the bank** (`design/gdd/npc-colony.md` §3.4),
3. A **clear line of sight** from the camera to the Angel.

Remove any one and the camera observes nothing. This is the rule that ties three systems
together: electricity, staff, and geometry.

### 3.7 Failure cascade

Failures are **staged and readable**, never instant:

| Stage | Trigger | Player experience |
|---|---|---|
| 1. Fuel empty | Generator stops | The hum stops. Nothing else, yet |
| 2. Battery | Load shifts to the battery | `BatteryAutonomy = Storage / Load` — one night at 100 EU/s |
| 3. Shedding | Battery empty | Industry dies, then monitors, then cameras, then lights |
| 4. Darkness | Security lamps out | Angels can move indoors. This is the failure the whole system exists to prevent |

**Battery proposal:** `60 000 EU`, which is exactly **600 s (one in-game night)** at a
`100 EU/s` load. The battery is a one-night promise, not a solution.

### 3.8 Late-game automation

Biofuel closes the loop: a farm produces crops, a refinery converts them, a colonist runs it,
and the generator stays fed without the player hauling coal. This is the point where the
fortress stops being a chore and becomes the creative sandbox the concept promises.

**Tuning target:** by the time the player can build a camera bank, the fuel loop should be
automatable within the same milestone. A fortress that still needs manual coal runs is a
fortress the player will abandon.

---

## 4. Formulas

| Formula | Value | Rationale |
|---|---|---|
| `DayLength` | `1200 s` | 20-minute cycle (`UDayNightCycleSystem`) |
| `NightLength` | `600 s` | Half the cycle |
| `LampDraw` | `2 EU/s` | 20 lamps = 40 EU/s |
| `CameraDraw` | `5 EU/s` | 8 cameras = 40 EU/s |
| `MonitorBankDraw` | `10 EU/s` | 2 banks = 20 EU/s |
| `FloodlightDraw` | `6 EU/s` | Directional, expensive per unit of coverage |
| `GeneratorOutput` | `150 EU/s` | One generator runs a small fortress |
| `IndustrialGeneratorOutput` | `600 EU/s` | Phase-3 industry |
| `BatteryStorage` | `60 000 EU` | One night at 100 EU/s |
| `CoalEnergy` | `50 000 EU` | ~2.4 coal/day at 100 EU/s |
| `BiofuelEnergy` | `120 000 EU` | ~1 per day at 100 EU/s |
| `FuelPerDay` | `Load × 1200 / EnergyPerFuel` | The planning number |
| `SafeRadius` | `LightLevel − AngelLightThreshold` | A torch protects 7 blocks |
| `GeneratorLoudness` | `30 blocks`, `×0.4` if soundproofed | The beacon |
| `BatteryAutonomy` | `Storage / Load` | One night at design load |

---

## 5. Edge cases

| Case | Resolution |
|---|---|
| Two circuits connected by a cable | They merge; capacity and load are summed |
| Circuit loop | Allowed; no penalty, no infinite energy |
| Generator destroyed | Load immediately shifts to battery, then shedding |
| Battery on a different circuit than the generator | It does not help. Circuits are physical |
| Camera has power but its bank has no operator | Observes nothing |
| Operator dies | The whole bank goes blind, not just one camera |
| All lights on `NightOnly` and the player enters a cellar at noon | The cellar is dark. Scheduling is a surface strategy only |
| Floodlight pointed at a wall | The cone is blocked; the wall's room is lit, the room behind is not |
| Lamp placed in a room with no ceiling | Sky light dominates by day; the lamp matters only at night |
| Soundproofing a generator | Cables must still exit; the room is not sealed, only muffled |
| Battery empties mid-shedding | Shedding order continues; nothing re-enables until capacity returns |
| Player builds a second generator | Capacity adds; noise adds too |
| Angel hears the generator through a wall | `HearingWallPenalty` applies (`angels.md` §4); a buried generator is quieter |

---

## 6. Dependencies

| System | Why | Status |
|---|---|---|
| Light query (`IsDark`) | The gate the lamps satisfy | Partial — `MobSpawnerSystem::CalculateLightLevel` exists, no general query |
| Lighting & Power (`design/gdd/lighting-power.md`) | Light levels, torches, lamps, sodium, point-light pool, placement, fuel policy | Draft (this GDD is the canonical owner) |
| Day/night cycle | `NightOnly` scheduling, night length | Implemented |
| Angel hearing | Generator noise | Not implemented |
| Camera observation | The thing power enables | Not implemented |
| NPC colony | Operators, farmers, refinery staff | Not implemented |
| Item system | Fuel items, cables, cameras, batteries | Not implemented |
| Voxel destruction | Angels breach cables and generators | Implemented (`BreakBlockAtVoxel`) |
| Save/load | Circuit state, fuel, assignments | Not documented |

---

## 7. Tuning knobs

| Knob | Default | Effect |
|---|---|---|
| `AngelLightThreshold` | 7 | How much light protects |
| `LampDraw` / `CameraDraw` / `MonitorBankDraw` | 2 / 5 / 10 EU/s | Base cost curve |
| `GeneratorOutput` | 150 EU/s | How much one machine covers |
| `BatteryStorage` | 60 000 EU | Size of the emergency window |
| `CoalEnergy` / `BiofuelEnergy` | 50 000 / 120 000 | Fuel efficiency |
| `GeneratorLoudness` / `SoundproofFactor` | 30 / 0.4 | The beacon trade |
| `SheddingOrder` | Industry → decor → monitors → cameras → security lights | Design statement; change with care |
| `NightOnlyMultiplier` | 0.5 | Value of scheduling |

---

## 8. Acceptance criteria

1. A room lit by an electric lamp does not let Angels move at night.
2. Removing the lamp lets them move, even with the player's back turned.
3. A camera with power and an assigned operator freezes Angels in its cone.
4. A camera with power and no operator freezes nothing.
5. A camera without power freezes nothing.
6. Empty fuel stops the generator, and the battery carries the circuit for exactly one night
   at design load.
7. When the battery empties, shedding follows the priority order, and security lights are the
   last to go.
8. `FuelPerDay` is displayed to the player and matches consumption.
9. A `NightOnly` lamp consumes nothing during the day.
10. A generator is audible to Angels at 30 blocks, and soundproofing reduces that to 12.
11. Two connected circuits report summed capacity and load.
12. A fortress with an automated biofuel loop can run for at least 5 in-game days without
    player intervention.

---

## 9. Build order (implementation plan)

1. **Light query** — a general `GetLightLevelAt()` usable by Angels and the spawner.
2. **Electric lamp + cable + generator + fuel item**, with a debug circuit panel.
3. **Load / capacity / shedding** with a visible HUD on the generator.
4. **Battery** and the staged failure cascade.
5. **Noise emission** from the generator into the Angel hearing system.
6. **Camera** (power only). Verify it still freezes nothing without an operator.
7. **Monitor bank + operator assignment** (requires `npc-colony.md` step 5).
8. **Floodlight**, `NightOnly` scheduling, **biofuel refinery**.
9. **Save/load** for circuit state.

---

## 10. Open questions

- Is the generator the only power source, or are there passive sources (wind, water) that
  cost no fuel but produce little?
- Does the Source's lair have power, or is the end-game deliberately a torch-only run?
- Can Angels breach a **cable** or a **generator** specifically, or only structural blocks?
- Should `NightOnly` be a per-lamp toggle or a global schedule with exceptions?
- How much fuel does the player carry? (Inventory stack size interacts with hauling.)
- Is there an "EMP" or blackout event that forces the player to defend the generator room?
