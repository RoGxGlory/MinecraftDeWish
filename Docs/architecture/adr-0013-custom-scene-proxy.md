# ADR-0013: Custom scene proxy with pooled RHI buffers

## Status
Proposed

## Date
2026-11-11

## Context

Lot 3 requires replacing `UProceduralMeshComponent` with a custom `FPrimitiveSceneProxy`
that uses persistent, pooled RHI buffers per 4×4-chunk region. This is the largest
single change in the performance plan.

### Problem Statement

Each chunk currently owns 2 `UProceduralMeshComponent` (solid + non-solid). Each
PMC recreates its vertex/index buffers on every mesh update, and the collision
cook runs synchronously per chunk. At render distance 8, this is ~377 chunks × 2
PMCs = 754 components, each with its own buffer allocation.

### Decision

1. **Region-based ownership**: A new `AVoxelRegionActor` (or subsystem) owns a 4×4
   chunk area (16×16 chunks at RD 8 = ~4 regions). Each region has one
   `FPrimitiveSceneProxy` with pooled RHI vertex/index buffers.

2. **Persistent buffers**: Buffers are allocated once per region and updated via
   `RHIUpdateTexture2D` / buffer range updates. No buffer recreation on mesh
   changes.

3. **Packed vertices**: 8–12 bytes per vertex (position + normal + UV packed)
   vs ~64 bytes today (FVector + FVector + FVector2D + FLinearColor + tangent).

4. **No tangents**: Normal mapping uses screen-space derivatives or precomputed
   tangent frames stored in a separate buffer.

5. **Collision**: Greedy boxes for a 5×5-chunk window around the player + DDA
   voxel raycasts. No trimesh per chunk.

### Consequences

- Draw calls drop from ~1200–1600 to < 800 (fewer components, batched regions).
- Memory: −50% (pooled buffers, no per-chunk PMC overhead).
- Risk: High. Requires deep UE rendering knowledge. Custom proxy must handle
  Lumen, Nanite, VSM, and all material features.
- Implementation: 4–6 weeks. Should be prototyped in a separate plugin/module
  before migrating the main world.

### Validation Criteria

- Render correctness: visual diff < 1% vs PMC baseline.
- Performance: draw calls < 800, chunk-store RAM < 60 MB at RD 8 / 64h.
- No regression in lighting, shadows, or collision.