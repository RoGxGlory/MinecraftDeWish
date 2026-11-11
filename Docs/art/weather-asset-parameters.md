# Weather Asset Parameters — MinecraftDeWish

**Last updated:** 2026-11-11
**Owner:** Technical Artist
**Rule:** every exposed parameter of a weather asset (Niagara system or post-process
material) must be listed here. A parameter not listed here must not be exposed.
Companion to `Docs/art/material-parameters.md`.

---

## Niagara systems

### NS_Rain

| Parameter | Type | Range | Default | Exposed as | Effect |
|---|---|---|---|---|---|
| `SpawnRate` | float (User) | 100–2000 | 500.0 | User parameter | Particles/sec; driven by C++ `SetFloatParameter("SpawnRate", ...)`. **Name must match exactly.** |
| `LifetimeMin` | float | 0.5–2.0 | 1.0 | Emitter module | Min particle lifetime (s) |
| `LifetimeMax` | float | 1.0–3.0 | 1.5 | Emitter module | Max particle lifetime (s) |
| `SpawnBoxSize` | Vector3 | — | (2000,2000,100) | Emitter module | Spawn volume above player (cm) |
| `SpawnHeightOffset` | float | 200–1500 | 800 | Emitter module | Vertical offset above component (cm) |
| `FallSpeedMin` | float | 1000–3000 | 1500 | Initialize Particle | Min downward velocity (cm/s) |
| `FallSpeedMax` | float | 1500–3500 | 2500 | Initialize Particle | Max downward velocity (cm/s) |
| `WindDrift` | float | 0–300 | 100 | Initialize Particle | Horizontal drift (cm/s) |
| `ParticleSizeMin` | float | 0.5–3.0 | 1.0 | Initialize Particle | Min sprite/mesh size (cm) |
| `ParticleSizeMax` | float | 1.0–4.0 | 2.0 | Initialize Particle | Max sprite/mesh size (cm) |
| `ParticleAlpha` | float | 0.1–0.9 | 0.5 | Render | Streak opacity |
| `MaxParticleCount` | int | 500–2000 | 1500 | Emitter | Hard cap (global budget 2000) |

**Render settings:** Translucent, Unlit or Default Lit, no shadows, fixed bounds.

### NS_AshStorm

| Parameter | Type | Range | Default | Exposed as | Effect |
|---|---|---|---|---|---|
| `SpawnRate` | float (User) | 50–1000 | 300.0 | User parameter | Particles/sec; driven by C++ `SetFloatParameter("SpawnRate", ...)`. **Name must match exactly.** |
| `LifetimeMin` | float | 2.0–5.0 | 3.0 | Emitter module | Min particle lifetime (s) |
| `LifetimeMax` | float | 4.0–8.0 | 6.0 | Emitter module | Max particle lifetime (s) |
| `SpawnBoxSize` | Vector3 | — | (3000,3000,1500) | Emitter module | Spawn volume around player (cm) |
| `SpawnHeightOffset` | float | 200–1000 | 500 | Emitter module | Vertical offset above component (cm) |
| `FallSpeedMin` | float | 20–100 | 50 | Initialize Particle | Min downward velocity (cm/s) |
| `FallSpeedMax` | float | 80–200 | 150 | Initialize Particle | Max downward velocity (cm/s) |
| `WindSpeed` | float | 100–600 | 300 | Initialize Particle | Constant horizontal wind (cm/s) |
| `Turbulence` | float | 0.0–1.0 | 0.5 | Update | Curl noise strength |
| `ParticleSizeMin` | float | 1.0–4.0 | 2.0 | Initialize Particle | Min mote size (cm) |
| `ParticleSizeMax` | float | 3.0–8.0 | 6.0 | Initialize Particle | Max mote size (cm) |
| `ParticleColor` | LinearColor | — | (0.4,0.4,0.4) | Initialize Particle | Ash tint |
| `ParticleAlpha` | float | 0.1–0.7 | 0.4 | Render | Mote opacity |
| `MaxParticleCount` | int | 300–1000 | 1000 | Emitter | Hard cap |

**Render settings:** Translucent, Unlit, depth fade enabled, no shadows, fixed bounds.

---

## Post-process material

### M_PP_Fog (Material Domain: Post Process)

| Parameter | Type | Range | Default | Effect |
|---|---|---|---|---|
| `FogColor` | LinearColor | — | (0.35, 0.37, 0.40) | Fog tint (grey-blue) |
| `FogDensity` | float | 0.0–1.0 | 0.3 | Overall fog strength (scaled by component BlendWeight) |
| `FogStart` | float | 100–2000 | 500 | Distance where fog begins (cm) |
| `FogEnd` | float | 1000–8000 | 4000 | Distance where fog is fully opaque (cm) |
| `ContrastLoss` | float | 0.0–0.5 | 0.3 | Darkening/contrast reduction factor |

**Blend settings:** Blendable Location = Before Tonemapping, Blendable Priority = 0.
The component sets `UPostProcessComponent::BlendWeight = CurrentFogDensity` and adds the
material as a `WeightedBlendable` (weight 1.0). No dynamic material parameters are set
from code.

---

## C++ contract summary

| Code call | Asset requirement |
|---|---|
| `RainNiagaraComponent->SetFloatParameter(FName("SpawnRate"), ...)` | NS_Rain user float param `SpawnRate` |
| `AshNiagaraComponent->SetFloatParameter(FName("SpawnRate"), ...)` | NS_AshStorm user float param `SpawnRate` |
| `FogPostProcessComponent->BlendWeight = CurrentFogDensity` | M_PP_Fog is a blendable post-process material |
| `FogPostProcessComponent->Settings.WeightedBlendables.Array.Add(Blendable)` with `FogMat` | M_PP_Fog assigned to `FogPostProcessMaterial` soft ref |

**Any rename of the `SpawnRate` parameter requires a C++ change and an ADR update.**

---

## Cross-references

- `Docs/art/material-parameters.md` — terrain/mob/UI materials.
- `Docs/art/weather-asset-creation-guide.md` — step-by-step editor authoring.
- `Docs/architecture/adr-0016-weather-system.md` — architecture and contracts.