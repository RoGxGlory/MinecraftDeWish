# ADR-0016: Weather & Day/Night System Architecture

## Status
Accepted

## Date
2026-11-11

## Amended
2026-11-11 — implementation details added (C++ class, delegates, soft refs, asset contracts, per-state table).

## Context

### Problem Statement
The project needs a dynamic atmosphere system (day/night + weather) that modulates mood
without changing the base grade (LUT). See `design/gdd/weather-system.md`. The existing
`UDayNightCycleSystem` only handled the 20-minute sun cycle. Weather states, fog, rain,
ash storms, wet surfaces and mob integration are new.

### Constraints
- PC Windows only, single player, Unreal Engine 5.8.
- Single owning component for atmosphere state; one tick, one save/restore entry point.
- Effects are authored as content (Niagara + post-process material), not code.
- Particle budget: 2000 total. Fog is post-process only (no volumetric).
- Mob integration reads multipliers; does not mutate attributes directly.
- No third-party rendering plugin without an ADR.

### Requirements
- Five weather states: Clear, Overcast, Rain, Fog, AshStorm.
- Weighted transitions driven by time-of-day and previous weather.
- Smooth lerp of fog density, ambient light, surface wetness, particle spawn rates and mob
  multipliers over `WeatherTransitionDuration` (30-60s).
- Indoor suppression of rain/ash particles (sky exposure).
- Save/restore of time-of-day, day count, current/target weather, transition alpha, time remaining.
- Delegates for UI and dependent systems.

## Decision

Extend `UDayNightCycleSystem` (rather than create a separate subsystem) so time-of-day
and weather share one tick and one save/restore entry point. Weather is a state machine
with five states and weighted transitions. Transitions lerp fog density, ambient light,
surface wetness, particle spawn rates and mob multipliers over `WeatherTransitionDuration`
(30-60s). Effects are created at runtime from soft references to Niagara systems and a
post-process material, so content can be authored in the editor without code changes.
Mob integration: `UMobSpawnerSystem` reads the spawn-rate multiplier; `AMobCharacter`
reads the aggression multiplier and applies it to movement speed.

### Architecture Diagram

```
AWorldGenerator (owner)
  └── UDayNightCycleSystem (UActorComponent, Tick 10 Hz)
        ├── Time-of-day: TimeOfDay, DayCount, SunLightActor rotation
        ├── Weather state machine: CurrentWeather, TargetWeather, bWeatherTransitioning,
        │   WeatherTransitionAlpha, WeatherTimeRemaining
        ├── Interpolated: CurrentFogDensity, CurrentAmbientLightFactor, CurrentSurfaceWetness,
        │   CurrentMobSpawnRateMultiplier, CurrentMobAggressionMultiplier
        ├── Soft refs (content): FogPostProcessMaterial, RainNiagaraSystem, AshNiagaraSystem
        └── Runtime effects (created in BeginPlay):
              ├── UPostProcessComponent (bUnbound, BlendWeight = CurrentFogDensity)
              ├── UNiagaraComponent (rain, SpawnRate param)
              └── UNiagaraComponent (ash, SpawnRate param)
```

### Key Interfaces

**Enum / Struct**
- `EWeatherState { Clear=0, Overcast=1, Rain=2, Fog=3, AshStorm=4 }` (BlueprintType)
- `FWeatherStateParams` (USTRUCT, BlueprintType) — per-state target values: FogDensity,
  VisibilityBlocks, AmbientLightFactor, SurfaceWetness, ParticleSpawnDust, RainParticleRate,
  AshParticleRate, MobAggression, MobSpawnRate, DurationMinSeconds, DurationMaxSeconds.

**Tuning knobs (UPROPERTY EditAnywhere, Category Weather|Settings)**
- `WeatherTransitionDuration` (ClampMin 15, ClampMax 120, default 45)
- `FogDensityMax` (0.3-1.0, default 0.8)
- `AshStormProbability` (0.0-0.2, default 0.05)
- `ClearSkyProbability` (0.0-0.3, default 0.10)
- `RainParticleRate` (100-2000, default 500)
- `AshParticleRate` (50-1000, default 300)
- `MobAggressionFogMultiplier` (1.0-2.0, default 1.2)
- `MinWeatherDurationSeconds` (0-300, default 0)

**Runtime state (UPROPERTY VisibleAnywhere, Category Weather|State)**
- `CurrentWeather`, `TargetWeather`, `bWeatherTransitioning`, `WeatherTransitionAlpha`,
  `WeatherTimeRemaining`, and interpolated `CurrentFogDensity`, `CurrentAmbientLightFactor`,
  `CurrentSurfaceWetness`, `CurrentMobSpawnRateMultiplier`, `CurrentMobAggressionMultiplier`.

**Content soft refs (UPROPERTY EditAnywhere, Category Weather|Content)**
- `TSoftObjectPtr<UMaterialInterface> FogPostProcessMaterial` (M_PP_Fog)
- `TSoftObjectPtr<UNiagaraSystem> RainNiagaraSystem` (NS_Rain)
- `TSoftObjectPtr<UNiagaraSystem> AshNiagaraSystem` (NS_AshStorm)

**Delegates**
- `FOnTimeChanged(TimeOfDay, bIsDay)` (DayNight|Events)
- `FOnWeatherChanged(NewWeather, OldWeather)` int32 (Weather|Events)
- `FOnWeatherTransitionStarted(FromWeather, ToWeather, Duration)` int32, int32, float
- `FOnFogDensityChanged(FogDensity)` float

**Blueprint API (Weather category)**
- `GetWeatherState()`, `GetTargetWeatherState()`, `GetFogDensity()`,
  `GetAmbientLightFactor()`, `GetSurfaceWetness()`, `GetMobSpawnRateMultiplier()`,
  `GetMobAggressionMultiplier()`, `GetVisibilityBlocks()`, `SetWeather(EWeatherState, bool bImmediate)`,
  `GetWeatherParams(EWeatherState)`
- Save/restore: `SaveWeatherState(...)` / `LoadWeatherState(...)` (Weather|Save)

**Per-state parameter table (from GDD §3.4)**

| State | Fog | Visibility | Ambient | Wetness | Dust | Rain | Ash | Aggro | SpawnRate | Dur (s) |
|---|---|---|---|---|---|---|---|---|---|---|
| Clear | 0.0 | 120 | 1.0 | 0.0 | 0.5 | 0 | 0 | 1.0 | 1.0 | 120-240 |
| Overcast | 0.3 | 80 | 0.7 | 0.0 | 1.0 | 0 | 0 | 1.0 | 1.0 | 300-480 |
| Rain | 0.2 | 60 | 0.6 | 0.8 | 0.3 | 1 | 0 | 0.8 | 0.7 | 180-300 |
| Fog | FogDensityMax | 30 | 0.4 | 0.2 | 0.1 | 0 | 0 | MobAggressionFogMultiplier | 1.3 | 240-360 |
| AshStorm | 0.6 | 40 | 0.3 | 0.0 | 2.0 | 0 | 1 | 1.5 | 1.5 | 120-180 |

**Weather selection algorithm** (PickWeightedWeather)
- Base weights: Clear=ClearSkyProbability, Overcast=0.40, Rain=0.20, Fog=0.20, AshStorm=AshStormProbability.
- Time-of-day: day → Clear×2, AshStorm=0; night → Fog×1.5, AshStorm×3.
- Previous weather: no same twice; Rain→Fog×2; Fog→Clear×2.
- AshStorm is night-only (weight forced to 0 during day).

**Transition flow**
- Timer expiry or day/night boundary → `SelectNewWeather()` → `PickWeightedWeather()` →
  `StartTransition(ToState)` (broadcasts `OnWeatherTransitionStarted`).
- `UpdateWeather` lerps interpolated values via `ApplyWeatherTransition(Alpha)`, broadcasts `OnFogDensityChanged`.
- On completion: `CurrentWeather = TargetWeather`, broadcasts `OnWeatherChanged`, resets timer.

**Indoor suppression** (UpdateEffects)
- Sky exposure check from player location upward through voxels; if a solid non-transparent
  block is found, `bPlayerExposedToSky = false`.
- Rain and ash Niagara components activate/deactivate based on exposure and weather factor.

**Niagara parameter contract**
- Rain system (NS_Rain) and ash system (NS_AshStorm) must expose a user float parameter
  named `SpawnRate`. The component drives it via `SetFloatParameter(FName("SpawnRate"), ...)`.

**Post-process contract**
- M_PP_Fog is a blendable post-process material. The component sets
  `UPostProcessComponent::BlendWeight = CurrentFogDensity` and adds the material as a
  `WeightedBlendable`. No dynamic material parameters are set from code; all look tuning
  lives in the material.

## Consequences

### Positive
- Single owner for atmosphere state; simpler save/restore.
- Weather logic lives in the component, not in Blueprint; designers tune via UPROPERTY.
- Content assets authored separately; code is asset-agnostic via soft refs.
- Niagara/post-process components created only if their soft refs are assigned.

### Negative
- Content assets (NS_Rain, NS_AshStorm, M_PP_Fog) must be authored separately and assigned.
- Indoor detection is a vertical scan through voxels; invoked every 0.1s.
- `LoadSynchronous` in UpdateEffects/CreateEffectComponents can hitch on first load; assets
  should be preloaded.

### Risks
- Niagara parameter name mismatch (`SpawnRate`) silently fails to drive spawn rate.
  Mitigation: document in asset guide and verify in editor.
- Post-process material not assigned → no fog visual, but logic still runs.
  Mitigation: default asset reference in Blueprint.
- Mob integration depends on reading multipliers; if a mob ignores them, weather has no effect.
  Mitigation: test coverage.

## Performance Implications
- Tick at 10 Hz (0.1s) for time + weather.
- Particle cap 2000 total (content-side, configured in Niagara).
- Fog is a post-process blendable, not volumetric.
- Indoor sky-exposure check scans voxels upward per tick (bounded by ChunkHeight).

## Migration Plan
1. Extend `UDayNightCycleSystem` with weather state machine (done, compiled).
2. Author content assets: NS_Rain, NS_AshStorm, M_PP_Fog
   (see `Docs/art/weather-asset-creation-guide.md`).
3. Assign soft refs on the `AWorldGenerator`'s `DayNightCycleSystem` component.
4. Wire `UMobSpawnerSystem` to `GetMobSpawnRateMultiplier()` and `AMobCharacter` to
   `GetMobAggressionMultiplier()`.
5. Add save/load integration using `SaveWeatherState`/`LoadWeatherState`.

## Validation Criteria
- Smooth transitions, no pops; fog density reaches 0.8 in Fog state.
- Weather state persists across save/load.
- Mob spawn rate and aggression reflect weather multipliers.
- Niagara SpawnRate parameter drives particle count.
- Indoor suppression deactivates rain/ash inside enclosed spaces.

## Related Decisions
- `design/gdd/weather-system.md`
- `Docs/SYSTEMS_REFERENCE.md` §9
- `Docs/art/weather-asset-creation-guide.md`
- `Docs/art/weather-asset-parameters.md`
- `Docs/architecture/adr-0009-mob-behavior-architecture.md`