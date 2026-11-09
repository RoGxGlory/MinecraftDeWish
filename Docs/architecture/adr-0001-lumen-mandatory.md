# ADR-0001: Lumen Is Mandatory on Every Quality Level

## Status
Accepted

## Date
2026-11-09

## Amended
2026-11-09 — hardware floor raised from "very low-end / integrated GPU" to a dedicated GPU.
See "Hardware floor" under Decision.

## Context

### Problem Statement
The project targets "very low-end PCs" while keeping an atmospheric horror identity that
depends on indirect lighting: dark interiors, soft bounce light, readable silhouettes in
near-black scenes. The default Unreal scalability ladder disables Lumen at the two lowest
quality levels (`GlobalIlluminationQuality@0` and `ReflectionQuality@0`), which would make
the game look and read completely differently on weak hardware.

The Technical Director's initial recommendation was to keep Lumen on High only and ship a
"skylight + vertex AO" fallback. That recommendation was **overruled by the project owner**:
Lumen is a pillar of the visual identity and must never be switched off.

The corollary was accepted on the same date: rather than maintain two shading paths, the
**hardware floor moves up to a dedicated GPU**. See "Hardware floor" under Decision.

### Constraints
- PC Windows only, single player, Unreal Engine 5.8.
- Target: stable frame times during continuous chunk streaming; no hitch above the frame budget.
- The terrain is procedurally generated and re-meshed continuously. Lumen's surface cache
  assumes stable geometry.
- Ambient occlusion will eventually be baked into terrain vertices, which reduces (but does
  not remove) the need for Lumen.
- No third-party rendering plugin without an ADR.

### Requirements
- Lumen global illumination and Lumen reflections must remain enabled at every scalability
  level, including the lowest "Potato" preset.
- Low-end hardware must absorb the cost through quality knobs (probe resolution, downsampling,
  render distance, LOD, resolution scaling), never by disabling the lighting method.
- Lumen must not be invalidated by terrain re-meshing every frame.

## Decision

Lumen stays enabled everywhere. `Config/DefaultScalability.ini` overrides the engine defaults
so that `r.Lumen.DiffuseIndirect.Allow=1` and `r.Lumen.Reflections.Allow=1` on every
`GlobalIlluminationQuality` and `ReflectionQuality` level, including `@0`. The quality ladder
degrades resolution and probe density instead:

| Level | GI downsample | Radiance cache probe | Reflection downsample |
| --- | --- | --- | --- |
| `@0` (Potato) | 32 | 8 | 2 |
| `@1` (Low) | 24 | 8 | 2 |
| `@2` (Medium) | 16 | 16 | 1 |
| `@3` (High) | 16 | 16 | 1 |
| `@Cine` | 8 | 16 | 1 |

Because Lumen cannot consume geometry that is rebuilt ten times per second, this decision
**makes a coarse GI proxy mandatory** rather than optional: a low-frequency terrain mesh
(1/4 or 1/8 resolution) rebuilt on a throttle (1–2 s, or only when the surface topology
changes) becomes the only Lumen/ray-tracing input. The high-resolution terrain mesh is
excluded from the surface cache and from ray-tracing proxies.

Hardware ray tracing (`r.RayTracing`) is a **separate** decision and is not enabled by this
ADR.

### Hardware floor

Lumen on every preset makes a dedicated GPU a hard requirement, so the project **raises its
stated hardware floor** instead of shipping a non-Lumen fallback. Decided by the project owner
on 2026-11-09.

| Tier | Hardware | Target |
| --- | --- | --- |
| Minimum supported | GTX 1060 6 GB / RX 580 8 GB, 4c/8t, 8 GB RAM, SSD | 1080p, 60 fps, preset `@1` |
| Absolute floor | GTX 1050 Ti / RX 570 | 1080p, 30 fps, preset `@0`, 50–60 % resolution scale, render distance 4 |
| Out of scope | Integrated GPUs (Intel UHD, AMD Vega iGPU, Apple silicon) | Not supported |

An Intel N100 + UHD Graphics class machine is no longer the reference target. It is only the
limit at which the game is expected to launch and be playable at a reduced resolution and
render distance, not the machine the presets are tuned against. This closes the earlier
open question (preset tuning vs. N100) in favour of preset fidelity.

### Architecture Diagram

```
High-res terrain mesh  ──►  visible rendering only (no GI, no RT)
        │
        └─(throttled decimation)──►  coarse GI proxy  ──►  Lumen surface cache
                                                              │
                                                              ▼
                                                    diffuse GI + reflections
```

### Key Interfaces
- `Config/DefaultScalability.ini` — project override of the engine quality ladder.
- `UVoxelTerrainRegionComponent` (future) — owns the high-res mesh **and** the GI proxy.
- `FVoxelGIProxyBuilder` (future) — `Build(const FVoxelChunkData&, int32 Decimation)`.

## Alternatives Considered

### Alternative 1: Lumen on High only, fallback elsewhere (TD recommendation)
- **Description**: Keep Lumen for `@2`+; on `@0`/`@1` fall back to skylight + baked vertex AO.
- **Pros**: Keeps a real hardware floor of roughly GTX 1050 Ti class; simpler; no dual shading path.
- **Cons**: The game would not look like itself on the machines the project claims to support.
- **Rejection Reason**: Overruled by the project owner. Visual identity is a hard requirement.

### Alternative 2: No Lumen at all, approximate GI everywhere
- **Description**: Skylight + vertex AO + a few dynamic lights on every preset.
- **Pros**: Best performance, one shading path, lowest memory.
- **Cons**: Loses bounce light and indirect contrast, which is the core of the horror direction.
- **Rejection Reason**: Changes the art direction; out of scope for a technical decision.

### Alternative 3: Lumen with hardware ray tracing everywhere
- **Description**: Full `r.Lumen.HardwareRayTracing` on all presets.
- **Pros**: Best quality.
- **Cons**: Unusable below an RTX-class GPU; BLAS rebuilds on a re-meshing world.
- **Rejection Reason**: Contradicts the low-end goal. Kept as an optional High-end toggle only.

## Consequences

### Positive
- One lighting model across all presets: what the designers author is what every player sees.
- The horror contrast survives on weak hardware.
- Forces the team to solve the real problem (geometry churn invalidating the surface cache)
  instead of hiding it behind a fallback.

### Negative
- **The addressable hardware base narrows.** With the floor raised (see "Hardware floor"),
  integrated-GPU machines are excluded. The game will not run acceptably on the cheapest
  laptops, and the store page must say so.
- Lumen keeps a fixed per-frame cost that cannot be lowered by render distance.
- The coarse GI proxy is now a required subsystem, not an optimisation.

### Risks
- **Surface cache thrash from re-meshing.** Mitigation: the GI proxy is rebuilt on a throttle
  and only its chunks are fed to Lumen.
- **Low preset still too slow on the floor hardware.** Mitigation: resolution scaling and
  render distance are the pressure valves; measure on the reference machine before promising
  the preset.
- **cvar drift between UE 5.7 (repo reference) and 5.8.** Mitigation: the names above are
  verified against 5.8 before the first scalability pass ships; `stat lumen` must show Lumen
  active at `@0`.

## Performance Implications
- **CPU**: negligible directly; the GI proxy build adds a throttled background cost.
- **Memory**: Lumen scene + surface cache grow with world complexity; the proxy adds one
  decimated mesh per region (~5–15 % of the high-res mesh).
- **Load time**: first Lumen capture after a region becomes visible; mitigated by proxies.
- **Network**: not applicable (single player).

## Migration Plan
1. Ship `Config/DefaultScalability.ini` (done with this ADR).
2. Verify on UE 5.8 that `@0` really keeps Lumen on and measure the cost on the reference GPU.
3. Implement the coarse GI proxy with the terrain rewrite (performance plan, lot 3).
4. Remove the terrain from `bAffectDistanceFieldLighting`/RT once the proxy exists.

## Validation Criteria
- On `@0`, a capture with `stat lumen` shows the surface cache and screen probe gather active.
- `r.Lumen.DiffuseIndirect.Allow` and `r.Lumen.Reflections.Allow` both read `1` at every level.
- Frame time on the reference low-end machine stays within the budget defined in
  `Docs/architecture/voxel-performance-plan.md`.

## Related Decisions
- `Docs/architecture/adr-0002-generated-vs-player-voxel-writes.md`
- `Docs/architecture/voxel-performance-plan.md`
- `Docs/SYSTEMS_REFERENCE.md` §1
