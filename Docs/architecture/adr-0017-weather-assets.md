# ADR-0017: Weather Niagara and Post-Process Material Assets

## Status
Accepted

## Date
2026-11-11

## Deciders
Technical Director, with Technical Artist

## References
- ADR-0016 (Weather & Day/Night system architecture)
- `design/gdd/weather-system.md`
- `Docs/art/weather-asset-creation-guide.md`
- `Docs/art/weather-asset-parameters.md`
- `Docs/art/material-parameters.md`

## Context

### Problem Statement
ADR-0016 fixed the weather system architecture: `UDayNightCycleSystem` drives fog, rain
and ash effects at runtime from soft references to content assets. The C++ side is
implemented and compiled, but the content assets it references do not exist yet. The
system requires three (optionally four) authored assets:

- `NS_Rain` — Niagara system for rain particles.
- `NS_AshStorm` — Niagara system for ash motes.
- `M_PP_Fog` — post-process material for distance fog.
- `MI_PP_Fog` (optional) — material instance for designer tuning.

Without these assets the soft references stay null and the weather logic runs with no
visual output (no fog, no rain, no ash). This ADR records the contract those assets
must satisfy so content authoring stays aligned with the C++ side and with the
performance budget.

### Constraints
- PC Windows only, single player, Unreal Engine 5.8.
- Particle budget: **2000 total** across all weather particles (GDD §5, edge case
  "Performance").
- Fog is **post-process only** — no volumetric fog, no VDB, no ray-marched volume.
- No third-party rendering plugin without an ADR (ADR-0016 constraint).
- The C++ side must not change to accommodate an asset: assets conform to the code
  contract, not the reverse. Any rename of the `SpawnRate` parameter requires both a
  C++ change and an ADR update.
- Lumen mandatory (ADR-0001); the fog material must not disable or bypass Lumen.

### Requirements
- Rain particles scale with the C++-driven spawn rate via a user float parameter
  named exactly `SpawnRate`.
- Ash particles scale the same way, via an identically named `SpawnRate` user float.
- Fog is driven entirely by the component's `BlendWeight` (no code-set material
  parameters); all look tuning lives in the material / material instance.
- Indoor suppression (sky exposure) activates/deactivates the Niagara components;
  assets must tolerate rapid activate/deactivate without hitches.
- Assets load synchronously on first transition in `CreateEffectComponents`; they must
  be small enough and pre-loadable to avoid frame hitches.
- Fixed bounds on both Niagara systems so they are not incorrectly culled.

## Decision

Author four content assets under `Content/Weather/`, each conforming to the
contracts below. Full step-by-step authoring instructions live in
`Docs/art/weather-asset-creation-guide.md`; full parameter tables in
`Docs/art/weather-asset-parameters.md`. This ADR records the binding decisions, not
the editor steps.

### 1. NS_Rain (Niagara System)
- Path: `Content/Weather/NS_Rain`.
- One emitter `RainDrops`, looping, active.
- **Must expose a User float parameter named exactly `SpawnRate`** (default 500.0,
  range 100–2000), bound to the emitter's Spawn Rate module. This is the contract
  with `RainNiagaraComponent->SetFloatParameter(FName("SpawnRate"), ...)`.
- Max particle count: **1500** (stays within the 2000 global budget, leaving room
  for dust/ash).
- Spawn volume box `X=2000, Y=2000, Z=100` cm, offset `Z=+800` cm above the
  component. Downward velocity `1500–2500` cm/s, small horizontal drift `±100`.
- Render: Sprite Renderer, translucent, unlit or default lit, **no shadows**, fixed
  bounds. Stretched streak sprite. Alpha `0.4–0.6`.
- No collision with world geometry required; kill after lifetime (1.0–1.5 s) or
  when below the spawn plane.

### 2. NS_AshStorm (Niagara System)
- Path: `Content/Weather/NS_AshStorm`.
- One emitter `AshMotes`, looping, active.
- **Must expose a User float parameter named exactly `SpawnRate`** (default 300.0,
  range 50–1000), bound to the emitter's Spawn Rate module. Contract with
  `AshNiagaraComponent->SetFloatParameter(FName("SpawnRate"), ...)`.
- Max particle count: **1000**.
- Spawn volume box `X=3000, Y=3000, Z=1500` cm, offset `Z=+500` cm. Slow downward
  velocity `50–150` cm/s, constant horizontal wind `+300` cm/s, curl-noise
  turbulence.
- Render: Sprite Renderer, translucent, unlit, **depth fade enabled**, no shadows,
  fixed bounds. Grey soft particle, alpha `0.3–0.5`, size `2.0–6.0` cm.

### 3. M_PP_Fog (Material, Post Process domain)
- Path: `Content/Weather/M_PP_Fog`.
- Material Domain = **Post Process**. Blendable Location = **Before Tonemapping**,
  Blendable Priority = 0. Does not bypass Lumen.
- Distance-based fog: `FogFactor = saturate((PixelDepth - FogStart) / (FogEnd - FogStart)) * FogDensity`,
  then lerp scene color toward `FogColor` by `FogFactor`, plus a contrast loss
  (`* (1 - FogFactor * ContrastLoss)`).
- Exposed scalar/vector parameters (defaults): `FogColor` (0.35, 0.37, 0.40),
  `FogDensity` (0.3), `FogStart` (500 cm), `FogEnd` (4000 cm), `ContrastLoss` (0.3).
- **No dynamic material parameters are set from code.** The component sets
  `UPostProcessComponent::BlendWeight = CurrentFogDensity` and adds the material as
  a `WeightedBlendable` (weight 1.0). All look tuning lives in the material /
  material instance.

### 4. MI_PP_Fog (Material Instance, optional)
- Path: `Content/Weather/MI_PP_Fog`, instance of `M_PP_Fog`.
- Purpose: designer-facing tuning without touching the master material.
- All five parameters from §3 exposed and overridable.

### 5. Assignment
- Soft references on the `AWorldGenerator`'s `DayNightCycleSystem` component
  (category `Weather|Content`):
  - `FogPostProcessMaterial` → `M_PP_Fog` (or `MI_PP_Fog`).
  - `RainNiagaraSystem` → `NS_Rain`.
  - `AshNiagaraSystem` → `NS_AshStorm`.

### 6. Budget and rendering policy
- Combined Niagara max particle count across rain + ash ≤ **2000** (rain 1500,
  ash 1000, but the two never peak simultaneously in practice; the GDD cap is the
  hard ceiling).
- Both Niagara systems: no shadows, fixed bounds, translucency.
- Fog is a single post-process blendable, not volumetric. No VDB, no ray-marched
  volume, no volumetric fog component.
- Assets must be pre-loadable / small to avoid first-transition hitches from
  `LoadSynchronous` in `CreateEffectComponents`.

## Consequences

### Positive
- Content conforms to a fixed, documented contract (`SpawnRate` name, post-process
  blendable, `BlendWeight`-driven fog), so the C++ side stays asset-agnostic.
- Particle budget enforced at the content side (max particle counts in Niagara),
  matching the GDD performance edge case.
- Fog stays cheap: one post-process pass, no volumetric cost, no Lumen interaction.
- Designer tuning is isolated in `MI_PP_Fog` and the Niagara user parameters,
  without code changes.
- Indoor suppression and activate/deactivate are handled by toggling components;
  assets are authored to tolerate it.

### Negative
- Four assets must be authored and assigned manually in the editor; missing
  assignment means weather logic runs with no visual output (mitigation: default
  asset reference in the Blueprint / level, plus the verification checklist).
- `SpawnRate` is a magic string shared between C++ and content. A mismatch
  silently fails to drive spawn rate (mitigation: document in guide and parameters
  doc, verify in editor).
- `LoadSynchronous` on first transition can hitch; assets must be preloaded or kept
  small (mitigation: preload in `BeginPlay` or level streaming).
- Two Niagara systems plus a post-process pass add a small, constant overhead even
  when inactive (mitigation: components are created only if soft refs are assigned,
  and deactivated when not in the relevant weather state).

### Risks
- **Parameter name drift:** renaming `SpawnRate` in either C++ or Niagara breaks
  the contract. Mitigation: this ADR + parameters doc are the single source of
  truth; any change requires an ADR update.
- **Budget overflow:** if a designer raises max particle counts above the caps,
  the 2000 total budget is exceeded. Mitigation: caps documented in parameters
  doc; automation test could assert configured max counts.
- **Fog too heavy:** a high `FogDensity`/`ContrastLoss` can obscure gameplay and
  hurt readability. Mitigation: safe ranges in parameters doc; playtest verification.
- **Indoor suppression false positives/negatives:** vertical voxel scan may
  misclassify partially enclosed spaces. Mitigation: test in caves and under
  overhangs (verification checklist).

## Performance Implications
- Niagara: ≤ 2000 particles total, no shadows, fixed bounds, translucency.
- Fog: single post-process blendable before tonemapping; no volumetric pass.
- No per-frame material parameter updates from code (`BlendWeight` only, set on
  transition/tick at 10 Hz).
- Asset load cost on first transition; mitigate via preload.

## Migration Plan
1. Author `NS_Rain`, `NS_AshStorm`, `M_PP_Fog` (and optionally `MI_PP_Fog`) per
   `Docs/art/weather-asset-creation-guide.md`.
2. Assign soft references on the `DayNightCycleSystem` component in the level.
3. Run the verification checklist in the guide (PIE weather states, `SpawnRate`
   scaling, indoor suppression, `stat unit` / Unreal Insights particle count).
4. Preload assets in `BeginPlay` or via level streaming if first-transition hitches
   are observed.

## Validation Criteria
- PIE: weather starts as Overcast; fog visible at density 0.3.
- `mdw.Weather.Set 3` (Fog) ramps fog density to `FogDensityMax` over ~45 s.
- Rain activates only in Rain state and deactivates indoors.
- Ash activates only in AshStorm state (night) and deactivates indoors.
- `SpawnRate` parameter visibly scales particle count in both Niagara systems.
- No frame hitches on first weather transition.
- `stat unit` / Unreal Insights: particle count ≤ 2000.

## Alternatives Considered

- **Volumetric fog (exponential height fog / VDB):** rejected. The GDD and
  ADR-0016 explicitly require fog as a post-process, not volumetric, to keep the
  cost predictable on the target hardware and to avoid Lumen/volumetric interactions.
  A post-process distance fog gives the desired "grey veil" at a fraction of the
  cost.
- **Single combined Niagara system for rain + ash:** rejected. Rain and ash have
  different spawn volumes, velocities, lifetimes and appearances, and only one is
  active at a time per weather state. Separate systems keep each simple, independently
  tunable, and independently activatable by the C++ side.
- **Code-set material parameters (dynamic `SetScalarParameterValue`):** rejected.
  The component already drives intensity via `BlendWeight`; pushing extra parameters
  from code would couple look tuning to code and require C++ changes for every tweak.
  All tuning stays in the material / material instance.
- **Hardcoded asset references in code (not soft refs):** rejected. Soft refs keep
  the C++ asset-agnostic, allow swapping assets per level, and avoid forcing the
  assets to load at startup. Already decided in ADR-0016.
- **No material instance (designers edit the master):** rejected. A material
  instance (`MI_PP_Fog`) gives a safe, non-destructive tuning surface and matches
  the project's data-driven tuning philosophy (Blueprint/C++ boundary rules).

## Related Decisions
- ADR-0016 (Weather & Day/Night system architecture)
- ADR-0001 (Lumen mandatory)
- `design/gdd/weather-system.md`
- `Docs/art/weather-asset-creation-guide.md`
- `Docs/art/weather-asset-parameters.md`
- `Docs/SYSTEMS_REFERENCE.md` §9