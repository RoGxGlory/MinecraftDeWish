# ADR-0012: Off-thread meshing pipeline

## Status
Accepted

## Date
2026-11-11

## Context

Lot 1 moved chunk generation off the game thread. Lot 2 (this ADR) moves meshing
(`GenerateMeshData`) off the game thread as well, leaving only buffer upload
(`ApplyMeshData`) on the game thread with a time budget.

### Problem Statement

`GenerateMeshData` + `ApplyMeshData` both run on the game thread today, via
`AChunkActor::UpdateMesh()`. Even though generation is async, the mesh build
blocks the GT. At 0.5 ms per chunk this is acceptable for single chunks but
spikes to 20+ ms during neighbour remeshes.

### Decision

1. **Temp buffer for data race**: `AChunkActor::SetBlockGenerated` now writes to
   `PendingBlockData` (a separate `TArray<uint8>`). The game thread copies
   `PendingBlockData → BlockData` via `ApplyPendingData()` when generation
   completes. This prevents the worker (writing) from racing the GT (reading
   `BlockData` for meshing/queries).

2. **Off-thread meshing**: When async generation completes:
   - `ApplyPendingData()` copies data to `BlockData` on GT.
   - `GenerateMeshData` is launched on a `UE::Tasks` worker, writing to
     `PendingSolidMeshData` / `PendingNonSolidMeshData`.
   - `bHasPendingMesh` is set true when the worker finishes.

3. **Budget-bounded upload**: `ProcessCompletedAsyncMeshes()` drains completed
   mesh futures on the GT, calling `ApplyPendingMesh()` (which calls
   `ApplyMeshData`) with a time budget (`MaxGenerationTimeMs`). Neighbour remeshes
   are queued after the primary mesh upload.

4. **TRACE scopes**: `AWorldGenerator_ProcessCompletedAsyncMeshes`,
   `AWorldGenerator_ChunkApplyMesh`, `AChunkActor_ApplyPendingData`,
   `AChunkActor_ApplyPendingMesh`, `AWorldGenerator_GenerateChunkMesh_Async`.

### Consequences

- GT mesh time drops from ~0.5 ms/chunk to ~0.1 ms (upload only).
- Memory: temporary mesh buffers add ~2x peak during transition; freed after apply.
- Risk: cross-chunk reads in `ShouldRenderFace` may race if neighbour data changes
  mid-mesh. Mitigation: meshing reads a snapshot of `BlockData` (copied at launch).
  Full fix deferred to Lot 3 (custom scene proxy with region-based data).

### Validation Criteria

- Before/after VoxelBench CSVs on same machine.
- No regression in block break/place, raycasts, torch lights, crack overlay.
- No data races under ThreadSanitizer (if available).