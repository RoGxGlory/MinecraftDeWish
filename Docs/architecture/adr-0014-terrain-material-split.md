# ADR-0014: Terrain material split (Opaque / Cutout / Translucent)

## Status
Proposed

## Date
2026-11-11

## Context

`M_Global` is a single `BLEND_Masked` (alpha-tested) material for all terrain.
This kills early-Z because every pixel runs the opacity mask test. The plan
calls for splitting into Opaque / Cutout / Translucent material paths.

### Problem Statement

- `M_Global` blend mode: Masked (alpha-tested) for all blocks including stone,
  dirt, etc. that are fully opaque.
- Early-Z is disabled for masked materials → GPU wastes fill on occluded pixels.
- Leaves, glass, fences, doors need alpha testing; stone/dirt/grass do not.

### Decision

1. **Opaque path**: Stone, dirt, grass, sand, cobblestone, ores, planks, etc.
   use a new `M_Global_Opaque` material (Blend Mode: Opaque). Early-Z enabled.

2. **Cutout path**: Leaves, fences, doors, torches use `M_Global_Cutout`
   (Blend Mode: Masked). Alpha test only where needed.

3. **Translucent path**: Glass, water use `M_Global_Translucent`
   (Blend Mode: Translucent). Separate render pass.

4. **Vertex color contract**: Unchanged. R=slice, G=wind, B=biome blend, A=AO.
   The cutout path adds opacity mask from the alpha-tested texture channels.

5. **Fast leaves (lot 3)**: Leaf faces write 0 into VertexColor.G (currently
   carries wind flag — needs reallocation). Opacity mask chain becomes
   `max(existing, 1 - G)`. Then leaf-to-leaf culling can be re-enabled.

### Consequences

- Base pass: −10 to −25% (fewer masked pixels, early-Z on opaque).
- Material instances: 3 instead of 1; slightly more PSO count.
- Risk: Medium. Requires visual pass over every block type to ensure no
  rendering regression (especially biome blending on opaque materials).

### Validation Criteria

- Visual diff < 1% vs single-material baseline (screenshot comparison).
- Early-Z pass visible in GPU profiler (r.EarlyZPass=1).
- No flickering or z-fighting on block boundaries.