# Weather & Day/Night System — GDD

**Status:** Draft
**Date:** 2026-11-11
**Author:** Systems Designer (delegated from Technical Director)
**References:** Docs/SYSTEMS_REFERENCE.md (§9 UDayNightCycleSystem), design/art/art-direction.md (§9)

## 1. Overview

A dynamic atmosphere system that modulates the world's mood through day/night
cycle and weather, while keeping the base grade (LUT) constant. The player
feels the weather without the screen changing colour. The system extends the
existing `UDayNightCycleSystem` with weather states, fog, rain, ash storms and
clear sky, each affecting light, particles, audio and mob behavior.

## 2. Player Fantasy

The world breathes. Most of the time it is oppressive — cold, foggy, sodium-lit.
But the weather shifts: a rare clear sky feels almost more unsettling than the
fog, because you can see further. Rain washes the world and makes surfaces gleam.
Ash storms choke the air and turn the world grey. The player reads the weather
as a threat indicator, not as a decoration.

## 3. Detailed Rules

### 3.1 Day/Night Cycle (existing, 20 min)

- **Day (0.00–0.50, 10 min):** sun traverses east→zenith→west. Hostile mobs
  burn in sunlight. Visibility high.
- **Night (0.50–1.00, 10 min):** moonlight. Hostile mobs spawn in unlit areas.
  Visibility low.

### 3.2 Weather States

Five weather states, each with a duration and transition:

| State | Duration | Transition | Effect |
|---|---|---|---|
| **Clear** | 2-4 min | from any (rare, 10% chance) | Full visibility, no particles, calm audio |
| **Overcast** | 5-8 min | default state | Moderate fog, muted light, baseline ambiance |
| **Rain** | 3-5 min | from Overcast (30% chance) | Wet surfaces, reflections, audio wash, reduced visibility |
| **Fog** | 4-6 min | from Overcast (20% chance) or Rain | Thick fog, 30-50 block visibility, high tension |
| **Ash Storm** | 2-3 min | from any (5% chance, night only) | Grey particles, wind, noise, reduced visibility, mobs more aggressive |

### 3.3 Transitions

- Transitions take 30-60 seconds (smooth lerp of fog density, light intensity,
  particle spawn rate).
- Weather changes only at dawn/dusk boundaries or after a minimum duration.
- No instant weather changes — always a gradual shift.

### 3.4 Effects per state

| Effect | Clear | Overcast | Rain | Fog | Ash Storm |
|---|---|---|---|---|---|
| Fog density | 0.0 | 0.3 | 0.2 | 0.8 | 0.6 |
| Visibility (blocks) | 120 | 80 | 60 | 30 | 40 |
| Ambient light | 1.0 | 0.7 | 0.6 | 0.4 | 0.3 |
| Surface wetness | 0.0 | 0.0 | 0.8 | 0.2 | 0.0 |
| Particle spawn (dust) | 0.5 | 1.0 | 0.3 | 0.1 | 2.0 |
| Rain particles | 0 | 0 | 1 | 0 | 0 |
| Ash particles | 0 | 0 | 0 | 0 | 1 |
| Mob aggression | 1.0 | 1.0 | 0.8 | 1.2 | 1.5 |
| Mob spawn rate | 1.0 | 1.0 | 0.7 | 1.3 | 1.5 |

### 3.5 Weather and Angel behavior

- **Clear sky:** Angel is more visible at distance. Player can spot it earlier.
  Angel is more cautious (slower approach).
- **Fog/Ash Storm:** Angel is harder to spot. Player has less reaction time.
  Angel is more aggressive (faster approach when not observed).
- **Rain:** no direct effect on Angel. Wet surfaces may affect player movement.

## 4. Formulas

### Weather selection (per day/night boundary)

```
P(weather) = base_weight[weather] * time_of_day_modifier * previous_weather_modifier
```

- base_weight: Clear=0.10, Overcast=0.40, Rain=0.20, Fog=0.20, AshStorm=0.05 (night only)
- time_of_day_modifier: day → Clear×2, AshStorm×0; night → Fog×1.5, AshStorm×3
- previous_weather_modifier: no same weather twice in a row; Rain → Fog×2; Fog → Clear×2

### Fog density lerp

```
FogDensity(t) = Lerp(CurrentFog, TargetFog, t / TransitionDuration)
```

where t is time since weather change, TransitionDuration = 30-60s.

### Ambient light modifier

```
AmbientLight = BaseAmbient * TimeOfDayFactor * WeatherFactor
```

- TimeOfDayFactor: day=1.0, night=0.3
- WeatherFactor: from table above

## 5. Edge Cases

- **Weather change during mining:** mining continues, crack overlay unaffected. Weather transition is visual only.
- **Weather change during combat:** combat unaffected. Weather does not pause gameplay.
- **Save/load:** weather state and time-of-day are saved and restored.
- **First spawn:** weather starts as Overcast (neutral default).
- **Indoor areas:** weather effects (rain, ash) are suppressed indoors (detected by sky exposure).
- **Performance:** weather particles use Niagara, capped at 2000 particles total. Fog is a post-process, not volumetric.

## 6. Dependencies

- `UDayNightCycleSystem` (existing) — extended with weather
- `AWorldGenerator` — sky exposure queries for indoor detection
- Niagara particle systems (new: NS_Rain, NS_AshStorm, NS_DustMote existing)
- Post-process material for fog (new: M_PP_Fog)
- Audio system (new: rain ambience, wind, thunder)
- `UMobSpawnerSystem` — weather affects spawn rate and mob aggression

## 7. Tuning Knobs

| Knob | Default | Range | Effect |
|---|---|---|---|
| `CycleDurationSeconds` | 1200.0 | 600-2400 | Day/night cycle length |
| `WeatherTransitionDuration` | 45.0 | 15-120 | Weather change lerp time |
| `FogDensityMax` | 0.8 | 0.3-1.0 | Maximum fog density |
| `AshStormProbability` | 0.05 | 0.0-0.2 | Chance of ash storm at night |
| `ClearSkyProbability` | 0.10 | 0.0-0.3 | Chance of clear sky |
| `RainParticleRate` | 500 | 100-2000 | Rain particle spawn rate |
| `AshParticleRate` | 300 | 50-1000 | Ash particle spawn rate |
| `MobAggressionFogMultiplier` | 1.2 | 1.0-2.0 | Mob aggression in fog |

## 8. Acceptance Criteria

- [ ] Weather changes are smooth (no instant pops)
- [ ] Fog reduces visibility to 30 blocks in Fog state
- [ ] Rain produces wet surface reflections
- [ ] Ash storm produces grey particles and wind
- [ ] Clear sky is rare and feels unsettling
- [ ] Weather state is saved and restored
- [ ] Weather affects mob spawn rate and aggression
- [ ] Performance: weather particles < 2000 total, fog is post-process only
- [ ] Weather transition doesn't cause frame hitches