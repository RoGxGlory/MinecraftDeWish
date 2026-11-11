# GDD — Day/Night Cycle

| Field | Value |
|---|---|
| **Status** | Draft (2026-11-11) |
| **System owner** | game-designer / systems-designer / gameplay-programmer |
| **Implementation** | `UDayNightCycleSystem` (cycle, sun pitch, sky light, time string, weather state machine) |
| **Related** | `design/gdd/weather-system.md`, `design/gdd/lighting-power.md`, `design/gdd/light-and-power.md`, `design/gdd/mob-spawning.md`, `Docs/SYSTEMS_REFERENCE.md` §9, `Docs/GAME_DESIGN.md` §2.2 |

---

## 1. Overview

The Day/Night Cycle is the heartbeat of MinecraftDeWish. It drives a 20-minute real-time loop
(10 minutes day, 10 minutes night) that rotates the sun, modulates sky light, changes mob
behaviour, and — through the Weather System — shifts the atmosphere. Hostile mobs burn in
sunlight and spawn in darkness, so the cycle directly dictates when the player is safe to
explore and when they must shelter. The cycle is the single source of truth for "what time is
it" and feeds every system that depends on light or time of day.

The cycle is implemented by `UDayNightCycleSystem`, a `UActorComponent` attached to the
`AWorldGenerator`. It also hosts the Weather state machine (see `design/gdd/weather-system.md`);
weather modulates fog, ambient light, particles, and mob behaviour on top of the base day/night
curve.

---

## 2. Player Fantasy

> *"The sun is setting. I have ten minutes to get home."*

The fantasy is **the countdown**. The player learns to read the sky the way a sailor reads
the sea — golden light means opportunity, long shadows mean urgency, darkness means danger.
Each phase of the cycle should feel distinct:

- **Dawn** — relief. The sky brightens, mobs begin to burn, and the world opens up again.
- **Midday** — freedom. Full visibility, maximum light, the safest window for surface
  exploration and building.
- **Dusk** — tension. Shadows lengthen, the first mobs stir, and the player races to
  shelter.
- **Midnight** — dread. The darkest point, the highest mob spawn pressure, and the moment
  when a lit base matters most.

The cycle must never feel like a timer the player stares at. It should be felt through
light, shadow, mob sounds, and the position of the sun — the clock string is a convenience,
not the primary feedback.

---

## 3. Detailed Rules

### 3.1 Cycle Structure

- **Total cycle duration:** 20 minutes real-time (1200 seconds), configurable via
  `CycleDurationSeconds` (clamped `[600, 2400]`).
- **Day:** `TimeOfDay ∈ [0.00, 0.50)` — 10 minutes. Sun traverses east → zenith → west.
  Hostile mobs exposed to open sky burn.
- **Night:** `TimeOfDay ∈ [0.50, 1.00)` — 10 minutes. Moonlight conditions; hostile mobs
  spawn in unlit areas.
- `TimeOfDay` is a normalised float in `[0.0, 1.0)` that advances continuously and wraps
  at 1.0. On wrap, `DayCount` increments by 1.
- Default initial `TimeOfDay` is `0.05` (shortly after sunrise), so a freshly created world
  starts in daylight rather than at the exact dawn boundary.
- The cycle can be paused via `bPauseCycle`; while paused, time does not advance, but weather
  transitions continue to run.

### 3.2 Sun Rotation

- The `SunLightActor` (a Directional Light) is rotated each tick so its pitch tracks the
  sun's position.
- **Sun pitch angle:** `SunPitch = -TimeOfDay × 360°`
  - `0°` at dawn (eastern horizon, `TimeOfDay = 0.0`)
  - `-90°` at noon (zenith, `TimeOfDay = 0.25`)
  - `-180°` at dusk (western horizon, `TimeOfDay = 0.5`)
  - `-180° … -360°` during night (sun below the horizon, `TimeOfDay = 0.5–1.0`)
- The sun's yaw (azimuth) is fixed at `-90°` (east–west axis); only pitch changes.
- Directional-light intensity is driven by `GetSkyLightIntensity() × CurrentAmbientLightFactor × 10.0`.

### 3.3 Sky Light Intensity

- `GetSkyLightIntensity()` returns a daylight factor in `[0.0, 1.0]`.
- The base (pre-weather) intensity follows a **piecewise linear** curve that peaks at midday
  and bottoms at midnight, with short ramps at dawn/dusk:

  ```
  Day   (0.00 ≤ t < 0.50):
    t ∈ [0.00, 0.05):  Base = lerp(0.1, 0.5, t / 0.05)          // dawn ramp up
    t ∈ [0.05, 0.45):  Base = 1.0                                 // midday plateau
    t ∈ [0.45, 0.50):  Base = lerp(1.0, 0.1, (t - 0.45) / 0.05)  // dusk ramp down
  Night (0.50 ≤ t < 1.00):
    t ∈ [0.50, 0.55):  Base = lerp(0.1, 0.02, (t - 0.50) / 0.05) // dusk→night ramp
    t ∈ [0.55, 0.95):  Base = 0.02                                 // night plateau
    t ∈ [0.95, 1.00):  Base = lerp(0.02, 0.1, (t - 0.95) / 0.05) // pre-dawn ramp
  ```

  where `t = TimeOfDay`.
- The Weather System further modulates this value via `CurrentAmbientLightFactor`
  (see `design/gdd/weather-system.md` §4).
- Final effective intensity: `SkyLightIntensity = Base × CurrentAmbientLightFactor`,
  clamped to `[0.0, 1.0]`.

  **Graph description (base, pre-weather):** rises from 0.1 at dawn to 1.0 by
  `TimeOfDay = 0.05`, holds flat at 1.0 until 0.45, falls to 0.1 by 0.50, dips to 0.02 by
  0.55, holds at 0.02 through the night, then ramps back to 0.1 by 1.0. The curve is
  asymmetric (long flat day plateau, long flat night plateau) rather than sinusoidal, giving a
  distinct "long safe midday" feel and a deep, flat midnight.

### 3.4 Day/Night Boundaries

| Boundary | TimeOfDay | In-game time | Event |
|---|---|---|---|
| Dawn | 0.00 | 06:00 | Day begins; mobs begin burning; spawn rate drops; weather re-roll |
| Noon | 0.25 | 12:00 | Maximum sky light; safest point |
| Dusk | 0.50 | 18:00 | Night begins; mobs stop burning; spawn rate rises; weather re-roll |
| Midnight | 0.75 | 00:00 | Minimum sky light; peak mob spawn pressure |

- `IsDay()` returns `true` when `TimeOfDay ∈ [0.0, 0.5)`.
- `IsNight()` returns `true` when `TimeOfDay ∈ [0.5, 1.0)` (i.e. `!IsDay()`).

### 3.5 Mob Sunlight Burning

- Mobs flagged with `bBurnsInSunlight = true` (Zombie, Skeleton) take **1 HP per timer fire**
  when all of the following are true:
  1. `IsDay()` is true.
  2. The mob is exposed to open sky (sky-exposure check via `AWorldGenerator` / `IsExposedToSky()`).
  3. The mob is not sheltered by a solid block above it.
  4. The weather is **not** Rain (rain protects mobs from sunlight burning).
- Burning is checked on a **1.0 s timer** (not per-frame) to limit performance cost
  (ADR-0009). A separate `ProcessSunlightBurning(DeltaTime)` path exists but is inactive
  because `bCanEverTick = false` on `AMobCharacter`.
- Burning mobs emit a visual flame effect and play a burning sound (cosmetic, not yet wired
  to Niagara in v0.2).
- Spiders and Creepers do **not** burn in sunlight (they are hostile regardless of time).

### 3.6 Mob Spawning and Time of Day

- `UMobSpawnerSystem::ProcessSpawnCycle()` (every 2.0 s) spawns hostile mobs in dark areas
  (light level < 7) on valid solid surfaces.
- Spawn attempts are scaled by the weather spawn-rate multiplier
  (`GetMobSpawnRateMultiplier()`), which is `1.0` for Overcast/Clear. The base day/night
  effect on spawning comes from the **light level**, not a separate multiplier:
  - **Day:** exposed-to-sky tiles have light level 15 → no surface spawns; caves (light < 7)
    still spawn.
  - **Night:** exposed-to-sky tiles have light level 4 → surface spawns allowed.
- Weather multipliers apply on top (Rain 0.7×, Fog 1.3×, Ash Storm 1.5× — see weather GDD).
- Spawn distance: 24–128 blocks from the player. Despawn: mobs further than 128 blocks.
- Mob cap: 20 active mobs.

### 3.7 Time Display

- `GetTimeString()` returns a formatted clock string: `"Day N, HH:MM"`.
- **Day number:** `DayCount`, an integer that starts at 1 and increments by 1 each time
  `TimeOfDay` wraps from ~1.0 to 0.0 (i.e. at each dawn). It is persisted via
  `SaveWeatherState` / `LoadWeatherState`.
- **In-game clock** (24-hour, dawn = 06:00):
  - `Hours24 = (TimeOfDay × 24 + 6) mod 24`
  - `HH = floor(Hours24)`
  - `MM = floor((Hours24 − HH) × 60)`
- The clock is displayed in the HUD (top-right corner) and updates every in-game minute.
- Day number increments at each dawn (`TimeOfDay` wraps from ~1.0 to 0.0).

### 3.8 Delegates and Events

- `OnTimeChanged(TimeOfDay, bIsDay)` — broadcast on **day/night boundary crossings only**
  (when `IsDay()` flips), not every tick. Consumers: HUD clock, mob spawner, weather
  system (triggers a weather re-roll), mob characters (burning check).
- `OnWeatherChanged(NewWeather, OldWeather)` — broadcast by the Weather System when the
  weather state changes (see `design/gdd/weather-system.md`).
- `OnWeatherTransitionStarted(FromWeather, ToWeather, Duration)` — broadcast when a weather
  transition begins.
- `OnFogDensityChanged(FogDensity)` — broadcast during weather transitions.

### 3.9 Weather Integration

The Day/Night Cycle and the Weather System share the same `UDayNightCycleSystem` component.
Weather extends the cycle with atmospheric states (Clear, Overcast, Rain, Fog, Ash Storm)
that modulate fog density, ambient light, particle effects, and mob behaviour. The weather
state machine selects a new weather at dawn/dusk boundaries or when `WeatherTimeRemaining`
reaches zero. See `design/gdd/weather-system.md` for the full state table and formulas.

---

## 4. Formulas

| Formula | Expression | Range / Units | Notes |
|---|---|---|---|
| `TimeOfDay` | `TimeOfDay + Δt / CycleDurationSeconds`, wrapped to `[0,1)` | `[0.0, 1.0)` | Incremental advance per tick; wraps at 1.0, incrementing `DayCount` |
| `SunPitch` | `-TimeOfDay × 360°` | `[-360°, 0°)` | Degrees; drives Directional Light pitch (yaw fixed at -90°) |
| `BaseSkyLight` | piecewise linear (see §3.3) | `[0.02, 1.0]` | Peaks 1.0 at midday plateau; bottoms 0.02 at night plateau |
| `SkyLightIntensity` | `clamp(BaseSkyLight × CurrentAmbientLightFactor, 0, 1)` | `[0.0, 1.0]` | Weather-modulated |
| `DirectionalLightIntensity` | `SkyLightIntensity × CurrentAmbientLightFactor × 10.0` | `[0.0, 10.0]` | Applied to the sun's `ULightComponent` |
| `DayCount` | increments by 1 on each `TimeOfDay` wrap | `[1, ∞)` | Persisted via save/load |
| `InGameHour` | `floor((TimeOfDay × 24 + 6) mod 24)` | `[0, 23]` | 24-hour clock, dawn = 06:00 |
| `InGameMinute` | `floor(((TimeOfDay × 24 + 6) mod 24 − InGameHour) × 60)` | `[0, 59]` | 60-minute clock |
| `MobBurnDamage` | `1.0 HP` per 1.0 s timer fire | HP/s | Applied to burning mobs; suppressed by rain |
| `LightLevel (day, exposed)` | `15` | `[0, 15]` | Full daylight → no surface spawns |
| `LightLevel (night, exposed)` | `4` | `[0, 15]` | Moonlight → surface spawns allowed |
| `LightLevel (torch)` | `max(current, 14)` | `[0, 15]` | Torch overrides within 5-block radius |

### Example calculations

**Noon (`TimeOfDay = 0.25`):**
- `SunPitch = -0.25 × 360° = -90°` (zenith)
- `BaseSkyLight`: `0.25 ∈ [0.05, 0.45)` → `1.0`
- `InGameHour = floor((0.25 × 24 + 6) mod 24) = floor(6 + 6) = 12` → 12:00 ✓
- `InGameMinute = floor((12.0 − 12) × 60) = 0` → 12:00 ✓

**Midnight (`TimeOfDay = 0.75`):**
- `SunPitch = -0.75 × 360° = -270°` (below horizon)
- `BaseSkyLight`: `0.75 ∈ [0.55, 0.95)` → `0.02`
- `InGameHour = floor((0.75 × 24 + 6) mod 24) = floor(18 + 6) mod 24 = floor(24) mod 24 = 0` → 00:00 ✓

**Dawn (`TimeOfDay = 0.0`):**
- `SunPitch = 0°` (eastern horizon)
- `BaseSkyLight`: `0.0 ∈ [0.00, 0.05)` → `lerp(0.1, 0.5, 0) = 0.1`
- `InGameHour = floor((0 + 6) mod 24) = 6` → 06:00 ✓

**Dusk (`TimeOfDay = 0.5`):**
- `SunPitch = -0.5 × 360° = -180°` (western horizon)
- `BaseSkyLight`: `0.5 ∈ [0.45, 0.50)` → `lerp(1.0, 0.1, (0.5 − 0.45)/0.05) = lerp(1.0, 0.1, 1.0) = 0.1`
- `InGameHour = floor((0.5 × 24 + 6) mod 24) = floor(12 + 6) = 18` → 18:00 ✓

---

## 5. Edge Cases

| Case | Resolution |
|---|---|
| Player mines through the night | Cycle continues; no pause. Player can play indefinitely in darkness with torches. |
| Player sleeps (if sleep is added) | Cycle advances to next dawn; time skipped. (Not implemented in v0.2; reserved for future.) |
| Save/load mid-cycle | `TimeOfDay`, `DayCount`, `CurrentWeather`, `TargetWeather`, `WeatherTransitionAlpha`, `WeatherTimeRemaining` are saved and restored via `SaveWeatherState` / `LoadWeatherState`. Cycle resumes from saved position. |
| First spawn | `TimeOfDay` starts at `0.05` (shortly after sunrise). Weather starts as Overcast (see weather GDD). |
| Cycle duration changed at runtime | `CycleDurationSeconds` is a tuning knob (clamped `[600, 2400]`); changing it mid-cycle scales the advance rate `Δt / CycleDurationSeconds` from the next tick. |
| Mob burning at dusk boundary | A mob burning at `TimeOfDay = 0.499` stops burning when `TimeOfDay` crosses 0.5 (next 1.0 s timer fire). The timer ensures the check happens on the next tick, not instantly. |
| Sky exposure during rain | Weather system handles rain particles; sky exposure for burning is based on open-sky check, not weather state. However, burning is explicitly suppressed during Rain (see §3.5). A mob in rain does not burn even if exposed to sky during day. |
| Player in a cave during day | Sky exposure = false; mobs do not burn. Spawning follows cave rules (light level < 7). |
| Very long session (days) | `DayCount` grows unbounded (int32); no overflow risk within realistic play sessions (< 1000 days). |
| `TimeOfDay` floating-point drift | `TimeOfDay` is incremented by `Δt / CycleDurationSeconds` each tick and wrapped; small drift is bounded by the wrap. (Design intent was to recompute from elapsed seconds, but implementation increments; acceptable at 10 Hz tick.) |
| Paused cycle | `bPauseCycle = true` stops time advancement; weather transitions still run. |
| Weather transition during boundary | Weather re-roll at dawn/dusk calls `SelectNewWeather`, which no-ops if a transition is already in progress. |
| Null `SunLightActor` | `UpdateSunRotation` early-returns; cycle still advances, but sun is not rotated and light intensity is not applied. |

---

## 6. Dependencies

| System | Why | Status |
|---|---|---|
| `UDayNightCycleSystem` | Core implementation (cycle, sun pitch, sky light, time string, weather) | Implemented |
| Weather System (`design/gdd/weather-system.md`) | Modulates sky light, fog, ambient, mob behaviour; shares the same component | Implemented (extending DayNightCycleSystem) |
| `UMobSpawnerSystem` | Spawn rate by time of day via light level; weather multipliers | Implemented |
| `AMobCharacter` | Sunlight burning (1 HP per 1.0 s timer; rain-protected) | Implemented |
| `AWorldGenerator` | Sky-exposure queries for burning and spawning; block light queries | Implemented |
| HUD / UI | Clock display, day number | Partial (clock string exists) |
| Save/Load System | Persist `TimeOfDay`, `DayCount`, weather state | Partial (save helpers exist) |
| Light & Power (`design/gdd/light-and-power.md`) | `NightOnly` lamp scheduling depends on `IsNight()` | Not implemented |

---

## 7. Tuning Knobs

### Implemented (data-driven via UPROPERTY, editable in editor)

| Knob | Default | Range (ClampMin/Max) | Effect |
|---|---|---|---|
| `CycleDurationSeconds` | 1200.0 | 600–2400 | Total cycle length (day + night). 600 = 10 min, 2400 = 40 min. |
| `WeatherTransitionDuration` | 45.0 | 15–120 | Weather change lerp time (s). |
| `FogDensityMax` | 0.8 | 0.3–1.0 | Maximum fog density (Fog state). |
| `AshStormProbability` | 0.05 | 0.0–0.2 | Chance of ash storm at night. |
| `ClearSkyProbability` | 0.10 | 0.0–0.3 | Chance of clear sky. |
| `RainParticleRate` | 500 | 100–2000 | Rain particle spawn rate. |
| `AshParticleRate` | 300 | 50–1000 | Ash particle spawn rate. |
| `MobAggressionFogMultiplier` | 1.2 | 1.0–2.0 | Mob aggression in fog. |
| `MinWeatherDurationSeconds` | 0.0 | 0–300 | Minimum seconds before weather can change again. |
| `bPauseCycle` | false | — | Pauses time advancement (debug/testing). |

### Design intent (not yet data-driven / hardcoded in v0.2)

| Knob | Default | Range | Effect | Note |
|---|---|---|---|---|
| Day fraction (dawn→dusk) | 0.5 | 0.3–0.7 | Fraction of cycle that is day | Hardcoded to 0.5 (`IsDay()` checks `< 0.5`). Changing requires code change. |
| Dusk boundary | 0.5 | 0.3–0.7 | Start of night (TimeOfDay value) | Must match day fraction. |
| Dawn time-of-day | 0.0 | Fixed | Start of day | Fixed at 0.0. |
| Moonlight floor (night sky light) | 0.02 | 0.0–0.2 | Minimum base sky light at night | Hardcoded in the piecewise curve (night plateau). |
| Midday plateau start | 0.05 | — | When sky light reaches 1.0 | Hardcoded. |
| Midday plateau end | 0.45 | — | When sky light begins falling | Hardcoded. |
| Dawn ramp duration | 0.05 | — | Length of dawn ramp | Hardcoded. |
| Dusk ramp duration | 0.05 | — | Length of dusk ramp | Hardcoded. |
| Mob burn damage | 1.0 | 0.5–3.0 | HP/s dealt to burning mobs | Hardcoded in `ProcessSunlightBurning`. |
| Mob burn check interval | 1.0 | 0.5–2.0 | Seconds between burning checks | Hardcoded timer in `BeginPlay`. |
| Day surface spawn multiplier | (via light level) | — | Surface spawns suppressed by light level 15 | Emergent from light-level system, not a separate knob. |
| Night surface spawn multiplier | (via light level) | — | Surface spawns allowed at light level 4 | Emergent from light-level system. |

---

## 8. Acceptance Criteria

- [ ] `TimeOfDay` advances continuously and wraps from ~1.0 to 0.0 at cycle end, incrementing `DayCount`.
- [ ] Full cycle completes in `CycleDurationSeconds` (default 1200 s).
- [ ] `SunPitch` equals `-TimeOfDay × 360°` at all points in the cycle.
- [ ] `IsDay()` returns true for `TimeOfDay ∈ [0.0, 0.5)` and false otherwise.
- [ ] `IsNight()` returns true for `TimeOfDay ∈ [0.5, 1.0)` and false otherwise.
- [ ] `GetSkyLightIntensity()` peaks at 1.0 during the midday plateau (`TimeOfDay ∈ [0.05, 0.45)`) and bottoms at 0.02 during the night plateau (`TimeOfDay ∈ [0.55, 0.95)`).
- [ ] Mobs with `bBurnsInSunlight` lose 1 HP per 1.0 s when exposed to open sky during day.
- [ ] Burning is suppressed during Rain weather.
- [ ] Burning stops (within one check interval) when a mob enters shade or night falls.
- [ ] `GetTimeString()` returns correct day number and in-game clock (dawn = 06:00, noon = 12:00, dusk = 18:00, midnight = 00:00).
- [ ] Day number increments by 1 at each dawn.
- [ ] `OnTimeChanged` fires on day/night boundary crossings (not every tick).
- [ ] Cycle state (`TimeOfDay`, `DayCount`, weather) survives save/load round-trip.
- [ ] Weather system correctly modulates sky light and ambient (see weather GDD acceptance criteria).
- [ ] Surface mob spawning is measurably lower during day than during night.
- [ ] Performance: cycle update costs < 0.1 ms per frame (tick interval 0.1 s, no per-frame heavy computation).
- [ ] Pausing the cycle (`bPauseCycle`) stops time advancement but keeps weather transitions running.