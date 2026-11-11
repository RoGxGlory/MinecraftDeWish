# GDD — Lighting & Power (Light Sources, Light Level, Sodium)

| Field | Value |
|---|---|
| **Status** | Draft (2026-11-11) |
| **System owner** | game-designer / systems-designer / gameplay-programmer / technical-artist |
| **Implementation** | `UMobSpawnerSystem::CalculateLightLevel`, `UDayNightCycleSystem`, `AWorldGenerator::PlaceBlock` (torch resting), `AChunkActor` (torch mesh + non-solid mesh) |
| **Related** | `design/gdd/day-night-cycle.md`, `design/gdd/weather-system.md`, `design/gdd/mob-spawning.md`, `design/gdd/light-and-power.md` (phase-3 power grid), `design/gdd/angels.md`, `design/art/art-direction.md` §3, `Docs/SYSTEMS_REFERENCE.md` §9–10, `Docs/architecture/adr-0001-lumen-mandatory.md`, `Docs/architecture/adr-0004-render-policy.md` |

---

## 1. Overview

Lighting in MinecraftDeWish is **both a gameplay system and a rendering system**.
Gameplay-wise, every voxel has a light level in `[0, 15]` that gates mob
spawning (spawn only where `LightLevel < 7`) and Angel movement (`IsDark`
requires `LightLevel < AngelLightThreshold = 7`). Rendering-wise, light comes
from a small, prioritised pool of dynamic point lights (max 16, shadowless)
fed by torches, lamps and furnaces, plus the global Lumen indirect pass and
the day/night sky.

The **single source of truth for light is the light level**, not the rendered
pixels. A torch raises the light level of the voxels around it whether or not
its point light is currently in the 16-light render pool. This separation is
what lets a base with 40 torches stay safe even though only 16 of them glow
on screen.

The visual identity (art-direction §3) imposes one non-negotiable rule:
**sodium (`#E8A33D`) is the only warm light in the game**, and **the torch is
the brightest object on screen, always**. Every other source (sky, moonlight,
ambient, fog) is cold, desaturated and ashen. The player reads warmth as
safety.

This GDD covers: light sources and their levels, the light-level calculation,
placement rules, the point-light pool, the sodium visual hierarchy, fuel
policy, and integration with day/night, weather, mob spawning and Angels.
The **power grid** (generators, batteries, circuits, shedding) is covered by
`design/gdd/light-and-power.md` and is out of scope here; this GDD only
defines the *consumers* (lamps) and their light output.

---

## 2. Player Fantasy

> *"The torch is not decoration. It is a border."*

The fantasy is **the border**. Darkness is the world's weapon; light is the
player's only answer, and it is always insufficient. The player learns, by
dying once, that a dark cave mouth is not an invitation but a trap, and that
placing a torch is the primary defensive act of the early game.

Three beats the lighting system must deliver:

- **The first torch.** The player places a torch in a cave. The darkness
  retreats 5 blocks in every direction. They feel the border form. They
  understand, instantly, that the torch is a tool, not decoration.
- **The brightest thing.** The player steps out into the night. The world is
  cold, grey, ashen. Their torch is the only warm thing on screen. They feel
  small, and safe, and watched.
- **The power lesson.** The player builds a base, places 20 torches, and
  realises the render pool only shows 16 of them glowing. They learn that light
  level (gameplay) and light rendering (visual) are different budgets, and that
  electric lamps (phase 3) are the upgrade path.

The lighting must never feel arbitrary. A dark corner that spawns a mob must
be explainable by *"no torch within 5 blocks, no sky"*. A lit room that stays
safe must be explainable by *"light level ≥ 7"*. The player reads darkness as
a resource the world spends against them, and light as the currency they
spend back.

---

## 3. Detailed Rules

### 3.1 Light level (the core quantity)

Every voxel position `p` has an integer light level `L(p) ∈ [0, 15]`.

```
L(p) = clamp( max( SkyLight(p), TorchLight(p), LampLight(p), OtherLight(p) ), 0, 15 )
```

- `0` = pitch black (unlit cave, no sky, no torch).
- `15` = full daylight / fully lit.
- Threshold for safety: `L(p) ≥ 7` prevents mob spawns and Angel movement.
- Threshold for darkness: `L(p) < 7` allows mob spawns and Angel movement.

Light level is a **gameplay quantity**. It is computed on demand by
`UMobSpawnerSystem::CalculateLightLevel` (and, in the target architecture, a
general `AWorldGenerator::GetLightLevelAt` query — see `light-and-power.md`
§6). It is **not** derived from rendered pixels.

### 3.2 Sky light contribution

`SkyLight(p)` depends on whether `p` is exposed to open sky and on time/weather:

- **Sky exposure check:** `p` is exposed to sky iff every voxel from `p.Z+1`
  up to `ChunkHeight` is air or a non-solid block (torches, leaves). A single
  opaque block above `p` blocks sky exposure.
- **Day (exposed):** `SkyLight = 15`.
- **Night (exposed):** `SkyLight = 4` (moonlight; below the safety threshold,
  so open ground spawns mobs at night, but not pitch black).
- **Enclosed (no sky exposure):** `SkyLight = 0` (caves, interiors, cellars).
- **Weather modulation (design intent, not yet implemented):** multiply by
  `WeatherAmbientFactor` from `weather-system.md` §3.4 (Clear 1.0, Overcast
  0.7, Rain 0.6, Fog 0.4, Ash Storm 0.3), then round and clamp to `[0, 15]`.
  This is why Fog and Ash Storm feel more dangerous: they darken the world and
  widen spawn zones. Currently `CalculateLightLevel` only checks `IsDay()` and
  ignores weather; wiring weather into the light level is a tracked task.

Note on the directional light: the sun/moon intensity uses the continuous
`BaseSkyLight` curve from `day-night-cycle.md` §3.3 (range `[0.02, 1.0]`).
The voxel light level uses the discrete day/night values above. The two are
related but not identical: the continuous curve drives how bright the world
*looks*, the discrete values drive gameplay. This is deliberate — gameplay
needs a hard day/night flip for spawn gating, while visuals need a smooth
ramp for atmosphere.

### 3.3 Torch light contribution

A torch is a non-solid block (placed in the NonSolidMesh of `AChunkActor`)
that emits light. It is the early-game, fuel-free light source.

- `TorchBaseLight = 14` (one less than full daylight, so a torch never
  out-shines the sun).
- `TorchLightRadius = 5` blocks.
- Contribution at position `p` from a torch at `t`:
  `max(0, TorchBaseLight − dist(p, t))`, but only if `dist(p, t) ≤ TorchLightRadius`.
- `TorchLight(p) = max` over all torches `t` within radius. A torch sets
  `L(p) ≥ 9` (`14 − 5`) within 5 blocks, which is `≥ 7`, so **no mob spawns
  within 5 blocks of a torch**. This is the "torch avoidance" rule shared with
  `mob-spawning.md` §3.4.
- Beyond 5 blocks from any torch, the torch's contribution is treated as 0
  for spawn gating (deliberate simplification; raising `TorchLightRadius`
  widens protection).

**Fuel policy — DECISION: torches are infinite.** A torch, once placed, burns
forever. No fuel, no consumption, no degradation, no burnout timer.

Rationale:
1. Torches are the player's primary defensive tool and "border" (§2). Making
   them consumable would punish exploration and turn the early game into a
   fuel-logistics chore before the power system (phase 3) exists.
2. The existing implementation treats torches as permanent blocks
   (`EBlockType::Torch`), and `UMobSpawnerSystem` assumes they persist.
3. The consumable/fuelled alternative is already covered by the phase-3 power
   grid: electric lamps (`light-and-power.md` §3.1) cost EU and are the upgrade
   path. Torches = free but dim (level 14, radius 5); lamps = brighter
   (level 15, radius 8) but powered. This creates a clean progression.

Open question (deferred): a future "torch burns out after N hours" variant
could add tension, but it conflicts with the "border" fantasy and is not
planned. If added, it must be a separate block ID (e.g. `TorchLit` /
`TorchSpent`) to preserve save compatibility.

### 3.4 Lamps (electric, phase 3)

Electric lamps are the powered upgrade to torches. They are defined in
`light-and-power.md` §3.1 and consume EU from a circuit.

- `LampBaseLight = 15` (full daylight equivalent; brighter than a torch).
- `LampRadius = 8` blocks (from `light-and-power.md` §3.1).
- `LampDraw = 2 EU/s` per lamp.
- Contribution: `max(0, 15 − dist(p, lamp))` within 8 blocks.
- A lamp requires a live circuit (power) to emit light. An unpowered lamp
  contributes 0 to the light level (it is a dark block). This is the
  brownout failure mode: when power dies, lamps go dark and light levels drop,
  which is exactly when mobs and Angels become dangerous indoors.
- `NightOnly` scheduling (`light-and-power.md` §3.4) turns a lamp off during
  the day when the sun does the same job for free. A `NightOnly` lamp
  contributes 0 during the day and its full level at night.

### 3.5 Other light sources

| Source | Light level | Radius | Powered | Notes |
|---|---|---|---|---|
| Daylight (open sky) | 15 | Global | No | Free; only above ground, only by day (via SkyLight) |
| Torch | 14 | 5 blocks | No | Fuel-free; the early-game border (§3.3) |
| Campfire | 15 | 8 blocks | No | Loud (`Loudness 6`); consumes wood; `light-and-power.md` §3.1 |
| Electric lamp | 15 | 8 blocks | Yes (2 EU/s) | Phase-3 workhorse; `light-and-power.md` §3.1 |
| Floodlight | 15 | 20 blocks (cone) | Yes (6 EU/s) | Directional; `light-and-power.md` §3.1 |
| Furnace (active) | 13 | 4 blocks | No (fuelled by smelting) | Sodium glow while smelting; 0 when idle |
| Monitor glow | 4 | 0 | Yes | A monitor room is dark by design (`light-and-power.md` §3.1) |
| Moonlight (night, open sky) | 4 | Global | No | Via SkyLight; below safety threshold |

A position's light level is the **max** of all applicable contributions, so a
torch next to a furnace takes the torch's 14, not the furnace's 13.

### 3.6 Placement rules

Torches (and lamps) are placed via `AWorldGenerator::PlaceBlock`, which
validates "torch resting constraints":

- **Attachment:** a torch must be placed against a solid, non-air block. It
  cannot float in air. The face clicked determines orientation:
  - Click a top face → torch stands on the floor (vertical, flame up).
  - Click a side face → torch mounts on the wall (angled, flame up).
  - Click a bottom face → torch hangs from the ceiling (inverted).
- **No stacking:** a torch cannot be placed on another torch or on a non-solid
  block (leaves, glass, other torches). It requires a solid support.
- **Voxel occupancy:** a torch occupies a non-solid voxel (`NonSolidMesh`,
  `QueryOnly` collision, `Overlap` for Pawns). It does not block player
  movement or mob movement, and mobs can pass through it.
- **Spacing:** no minimum spacing enforced. Torches may be placed adjacent.
  (Render pool prioritisation handles the visual cost — §3.8.)
- **Lamps** follow the same attachment rules but occupy a full solid voxel
  (they are blocks, not non-solid attachments) and emit from their centre.

### 3.7 Day/night and weather integration

- **Day/night:** `SkyLight(p)` flips between 15 (day, exposed) and 4 (night,
  exposed). This is the primary reason the surface is safe by day and
  dangerous by night. See `day-night-cycle.md` §3.6.
- **Weather:** weather modulates ambient light (`CurrentAmbientLightFactor`:
  Clear 1.0, Overcast 0.7, Rain 0.6, Fog 0.4, Ash Storm 0.3) and, via the
  design-intent weather modulation in §3.2, darkens the voxel light level.
  Indoor weather effects (rain, ash particles) are suppressed by sky-exposure
  checks.
- **Mobs:** `UMobSpawnerSystem::ProcessSpawnCycle` spawns only where
  `L(p) < 7`. Weather multipliers (Rain 0.7×, Fog 1.3×, Ash Storm 1.5×)
  scale attempt counts on top. See `mob-spawning.md` §3.2–3.3.
- **Angels:** `IsDark` (angel movement gate) is `L(p) < AngelLightThreshold`
  (7). A torch, lamp or sunlit voxel blocks Angel movement. See
  `angels.md` §3.1–3.2 and `light-and-power.md` §3.1.

### 3.8 Rendered light (point-light pool)

Gameplay light level uses **all** torches and lamps. Rendered light uses a
**pool of at most 16 concurrent dynamic point lights**, because the engine
budget and the horror identity ("the torch is the brightest object on screen")
require prioritisation, not abundance.

- **Pool size:** `MaxDynamicPointLights = 16`.
- **Shadowless:** all pool lights cast **no dynamic shadows** (ADR-0004:
  "Torches, debris, pickups, XP orbs and the crack overlay cast no dynamic
  shadow. Only the sun, the player and mobs cast shadows."). This is a hard
  constraint: shadowed point lights are too expensive on the 1060 floor and
  would break the frame budget.
- **Color:** sodium `#E8A33D` for every warm source (torch, lamp, campfire,
  furnace glow, generator glow). No other warm colour is used. Cold sources
  (sky, moonlight) use the grade's cold palette. This is the "one warm source
  only" rule from `art-direction.md` §2.
- **Intensity hierarchy (visual):** torch > lamp > furnace glow > ambient.
  The torch is the brightest object on screen, always (`art-direction.md` §3).
  Achieved by high point-light intensity + emissive material (the torch's
  `M_Global` alpha channel drives emissive) + small attenuation radius.
- **Attenuation radius:** torch ≈ 600 cm (6 blocks, gameplay radius 5 + 1
  margin); lamp ≈ 900 cm (9 blocks, radius 8 + 1).
- **Selection algorithm (design intent):**
  1. Gather all active light-emitting blocks (torches + powered lamps +
     active furnaces) within a candidate radius of the camera (e.g. 64 blocks).
  2. Sort by distance to camera (nearest first).
  3. Take the first 16. Update the pool on a throttle (≈ 0.25 s) to avoid
     per-frame churn and popping.
  4. Lights not in the pool are not rendered as point lights but still
     contribute to gameplay light level (§3.1) and Lumen indirect via the
     coarse GI proxy where applicable.
- **Lumen integration:** Lumen is mandatory on every preset (ADR-0001). The
  terrain feeds Lumen through a throttled coarse GI proxy (ADR-0001,
  `voxel-performance-plan.md`); dynamic point lights contribute to Lumen's
  screen-probe indirect lighting. Torches are therefore part of the indirect
  bounce, which is what makes a lit corridor feel warm even outside the direct
  cone.
- **Emissive materials:** torch and lamp meshes use `M_Global` with the alpha
  channel feeding emissive, so the block itself glows even when its point
  light is culled from the pool. This guarantees the "brightest object"
  invariant survives pool overflow.

---

## 4. Formulas

| Formula | Expression | Range / Units | Notes |
|---|---|---|---|
| `L(p)` (light level) | `clamp(max(SkyLight, TorchLight, LampLight, OtherLight), 0, 15)` | `[0, 15]` (int) | Gameplay quantity; single source of truth |
| `SkyLight(p)` (day, exposed) | `15` | `[0, 15]` | Full daylight |
| `SkyLight(p)` (night, exposed) | `4` | `[0, 15]` | Moonlight; below safety threshold |
| `SkyLight(p)` (enclosed) | `0` | `[0, 15]` | Caves, interiors, cellars |
| `SkyLight(p)` (weather-modulated) | `round(SkyLight_base × WeatherAmbientFactor)` | `[0, 15]` | Design intent; not yet implemented |
| `TorchLight(p)` | `max over torches t of max(0, 14 − dist(p,t))` for `dist ≤ 5` | `[0, 14]` | 0 if no torch within 5 blocks |
| `LampLight(p)` | `max over lamps l of max(0, 15 − dist(p,l))` for `dist ≤ 8` (and powered) | `[0, 15]` | 0 if unpowered or out of range |
| `SafetyThreshold` | `7` | light levels | `L(p) ≥ 7` → no mob spawn, Angel cannot move |
| `TorchBaseLight` | `14` | light levels | One less than full daylight |
| `TorchLightRadius` | `5` | blocks | Torch protection radius |
| `LampBaseLight` | `15` | light levels | Full daylight equivalent |
| `LampRadius` | `8` | blocks | Lamp protection radius |
| `PoolSize` | `16` | point lights | Concurrent rendered dynamic lights |
| `WeatherAmbientFactor` | Clear 1.0, Overcast 0.7, Rain 0.6, Fog 0.4, Ash Storm 0.3 | `[0.3, 1.0]` | From `weather-system.md` §3.4 |
| `BaseSkyLight` (visual) | piecewise linear (see `day-night-cycle.md` §3.3) | `[0.02, 1.0]` | Drives directional light intensity, not voxel level |
| `DirectionalLightIntensity` | `SkyLightIntensity × CurrentAmbientLightFactor × 10.0` | `[0.0, 10.0]` | Sun/moon `ULightComponent` intensity |

### Example calculations

**Torch in a cave (night, no sky):**
- `SkyLight = 0` (enclosed).
- Torch at distance 2: `TorchLight = 14 − 2 = 12`.
- `L = clamp(max(0, 12), 0, 15) = 12 ≥ 7` → safe, no spawn. ✓

**Torch at distance 5 (edge of radius):**
- `TorchLight = 14 − 5 = 9 ≥ 7` → still safe (border holds). ✓

**Torch at distance 6 (beyond radius):**
- `TorchLight = 0` (ignored beyond radius).
- `L = 0 < 7` → dark, spawn allowed. This is the "one block further in" lesson. ✓

**Open surface at noon:**
- `SkyLight = 15`. `L = 15 ≥ 7` → safe. ✓

**Open surface at midnight:**
- `SkyLight = 4`. `L = 4 < 7` → dark, spawn allowed (night surface spawns). ✓

**Furnace room (furnace active, no torch):**
- `SkyLight = 0` (enclosed). `FurnaceLight = 13` at centre, `13 − 4 = 9` at 4 blocks.
- At 4 blocks: `L = 9 ≥ 7` → safe. At 5 blocks: `L = 0 < 7` → dark. ✓

**Brownout (power dies, lamp goes dark):**
- Lamp was contributing 15 within 8 blocks. Now 0.
- If no torch/sky: `L` drops from 15 to 0 → `IsDark` true → Angel can move indoors. This is the failure the whole system prevents. ✓

---

## 5. Edge Cases

| Case | Resolution |
|---|---|
| **Pool overflow (more than 16 lights in view)** | Only the 16 nearest render as point lights. The rest still contribute to gameplay light level and glow via emissive material. No light-level change. |
| **Torch placed at pool boundary (17th torch)** | The 17th torch's point light is culled, but its emissive material still glows and its light level still protects. Player sees a slightly dimmer torch but the gameplay border holds. |
| **Torch placed on ceiling** | Orientation inverted; light level contribution identical (isotropic radius). Point-light attenuation sphere is symmetric, so orientation does not affect gameplay. |
| **Torch placed underwater** | Water is not simulated in v0.2 (art-direction §12.2). No special handling; torch behaves as in air. If water is added later, torches are extinguished (level 0) and must be relit — tracked as open question. |
| **Torch placed in a chunk that unloads** | Torch block persists in chunk data; light level query returns 0 for unloaded chunks (no spawn evaluation there). On reload, torch re-emits. |
| **Two torches adjacent** | Light levels do not stack beyond `max`; `L = max(14, 14) = 14`. No brighter-than-14 from torches alone. |
| **Torch + lamp overlap** | `L = max(14, 15) = 15` (lamp wins). Lamp's larger radius extends the safe zone. |
| **Torch placed on a non-solid block (leaves, glass)** | Placement rejected by `PlaceBlock` torch resting constraints. No floating torches. |
| **Lamp unpowered** | Contributes 0. Treated as a dark block. Light level may drop below threshold → spawn/Angel risk. |
| **Lamp on `NightOnly` during day** | Contributes 0 during day; full level at night. A cellar with only `NightOnly` lamps is dark at noon. |
| **Furnace stops smelting** | Light level drops from 13 to 0 at the furnace's position. If no other source, the area becomes dark. |
| **Weather darkens light level (design intent)** | Fog (0.4) and Ash Storm (0.3) lower `SkyLight`, widening spawn zones during those weathers. Not yet wired into `CalculateLightLevel`. |
| **Mob spawns at the edge of torch radius** | At distance 6, `L = 0 < 7` (if no sky). Spawn allowed one block beyond the border. This is the intended "border" lesson. |
| **Angel at the edge of a torch's light** | `IsDark` uses the same `L < 7`. An Angel standing at distance 6 from a torch is in darkness and can move. At distance 5, `L = 9 ≥ 7`, Angel freezes. |
| **Point-light pool update hitch** | Pool re-selection is throttled (≈ 0.25 s). No per-frame allocation. Lights fade in/out via intensity lerp to avoid popping. |
| **Lumen surface cache thrash from torches** | Torches are small dynamic lights, not geometry. They do not invalidate the coarse GI proxy. The proxy rebuilds on a 1–2 s throttle independent of torch placement. |
| **Save/load** | Torch/lamp blocks persist in chunk voxel data (block ID + metadata). Light level is recomputed on load, not stored. Pool state is transient (rebuilt). |

---

## 6. Dependencies

| System | Why | Status |
|---|---|---|
| `UMobSpawnerSystem` | `CalculateLightLevel` is the current light-level implementation; spawn gating uses `L < 7` | Implemented |
| `UDayNightCycleSystem` | Sky light flip (day 15 / night 4); `BaseSkyLight` curve; `IsDay()`/`IsNight()` | Implemented |
| Weather System (`weather-system.md`) | Ambient light factor; weather modulation of light level (design intent) | Implemented (modulation of light level not yet wired) |
| `AWorldGenerator` | `PlaceBlock` torch resting constraints; `GetVoxelAt` for sky-exposure and torch queries; chunk data | Implemented |
| `AChunkActor` | NonSolidMesh for torches; torch mesh generation (`AddTorchMesh`); emissive material | Implemented |
| Angels (`angels.md`) | `IsDark` gate uses `L < 7`; torches/lamps block movement | Partial (light query exists via spawner) |
| Power Grid (`light-and-power.md`) | Lamps consume EU; brownout drops light levels; `NightOnly` scheduling | Not implemented (phase 3) |
| Lumen (ADR-0001) | Mandatory GI; torches contribute to indirect bounce via screen probes | Implemented (engine-level) |
| ADR-0004 (render policy) | Torches cast no dynamic shadows; VSM shadow scope | Accepted |
| Art Direction (`art-direction.md` §2–3) | Sodium `#E8A33D` is the only warm light; torch is brightest on screen | Accepted |
| `M_Global` (PBR material) | Alpha channel → emissive; torch glow | Implemented |

Bidirectional note: `mob-spawning.md` §3.4 references this GDD as the canonical home
of the light-level formula; `day-night-cycle.md` §6 and `light-and-power.md` §6
reference this GDD for torch/lamp light output.

---

## 7. Tuning Knobs

### Implemented (data-driven via UPROPERTY)

| Knob | Default | Range | Effect |
|---|---|---|---|
| `TorchLightRadius` | 5 | 3–8 | Torch protection radius (blocks). Wider = safer, but more torches needed to cover a base. |
| `MaxMobCap` | 20 | 5–50 | Global mob ceiling (interacts with light: more dark area → more spawns up to cap). |
| `LightLevelThreshold` (mob spawn / Angel) | 7 | 4–10 | How much light protects. Lower = easier; mobs spawn in dimmer areas. |
| `CycleDurationSeconds` | 1200.0 | 600–2400 | Day/night cycle; affects SkyLight flip frequency. |
| `WeatherTransitionDuration` | 45.0 | 15–120 | Weather lerp; affects ambient light ramp. |

### Design intent (not yet data-driven)

| Knob | Default | Range | Effect | Note |
|---|---|---|---|---|
| `TorchBaseLight` | 14 | 10–15 | Torch brightness. 14 = one less than daylight (sun stays dominant). | Hardcoded in `CalculateLightLevel`. |
| `LampBaseLight` | 15 | 12–15 | Lamp brightness. | From `light-and-power.md`. |
| `LampRadius` | 8 | 5–12 | Lamp protection radius. | From `light-and-power.md`. |
| `MoonlightLevel` (night, exposed) | 4 | 0–6 | Night surface light level. Below threshold → spawns allowed. | Hardcoded. |
| `PoolSize` (max point lights) | 16 | 8–32 | Concurrent rendered dynamic lights. Raising costs frame budget; lowering dims the world. | Render budget; ADR-0004. |
| `PoolUpdateInterval` | 0.25 | 0.1–1.0 | Throttle for pool re-selection (s). | Render tuning. |
| `TorchPointLightIntensity` | (hero) | — | Point-light intensity; must exceed all other sources. | Art/tech tuning. |
| `TorchAttenuationRadius` | 600 cm | 400–800 | Point-light falloff (gameplay radius 5 + 1 margin). | Render tuning. |
| `LampAttenuationRadius` | 900 cm | 700–1100 | Lamp point-light falloff. | Render tuning. |
| `WeatherAmbientFactor` (per state) | Clear 1.0 … Ash 0.3 | 0.1–1.0 | Darkens sky light during weather. | From `weather-system.md`. |
| `AngelLightThreshold` | 7 | 4–10 | Must equal mob-spawn threshold for consistency. | `angels.md` §4. |
| `FurnaceActiveLight` | 13 | 10–14 | Furnace glow while smelting. | Design intent. |
| `CampfireBaseLight` | 15 | 12–15 | Campfire brightness. | `light-and-power.md`. |
| `MaxDynamicPointLights` | 16 | 8–32 | Render pool cap. | Hard constraint from ADR-0004 + perf budget. |
| `TorchFuelPolicy` | Infinite | Infinite / Consumable | Whether torches burn forever. Current decision: infinite. | See §3.3. |

---

## 8. Acceptance Criteria

- [ ] `CalculateLightLevel(p)` returns a value in `[0, 15]` for any voxel position.
- [ ] An open-sky voxel returns 15 during day and 4 during night (matching `IsDay()`).
- [ ] An enclosed voxel (solid block above) returns 0 when no torch/lamp is within range.
- [ ] A torch sets `L(p) ≥ 9` for all voxels within 5 blocks (Manhattan/chebyshev distance per implementation's squared-distance check).
- [ ] No mob spawns within 5 blocks of a torch (`L ≥ 7`).
- [ ] A torch at distance 6 contributes 0 (border holds at 5, dark at 6).
- [ ] Torches are infinite: a placed torch never degrades, extinguishes, or requires fuel over a 5-day play session.
- [ ] A lamp contributes 15 within 8 blocks only while its circuit is powered; an unpowered lamp contributes 0.
- [ ] A `NightOnly` lamp contributes 0 during the day and its full level at night.
- [ ] An Angel cannot move (`IsDark` false) on any voxel with `L ≥ 7`, including torch-lit and lamp-lit voxels.
- [ ] The point-light pool never exceeds 16 concurrent lights; overflow lights are culled but still glow via emissive material.
- [ ] All pool lights are shadowless (no dynamic shadows from torches/lamps).
- [ ] All warm light sources use sodium `#E8A33D`; no other warm colour appears in the scene.
- [ ] The torch is the brightest object on screen in a side-by-side capture (torch point-light intensity ≥ lamp and ≥ ambient).
- [ ] Weather modulation (design intent) lowers `SkyLight` during Fog (×0.4) and Ash Storm (×0.3), widening spawn zones — verified once wired.
- [ ] Light level is recomputed on load (not stored); a save/load round-trip preserves torch blocks and their protection.
- [ ] Performance: `CalculateLightLevel` costs < 0.05 ms per call; pool update costs < 0.5 ms per 0.25 s tick; no per-frame allocation in the pool path.
- [ ] A base with 40 torches keeps all 40 voxels at `L ≥ 7` even though only 16 torches render as point lights.