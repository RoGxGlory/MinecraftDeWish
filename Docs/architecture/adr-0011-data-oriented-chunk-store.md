# ADR-0011: Data-oriented chunk store (no per-chunk actor)

## Status
Proposed

## Date
2026-11-10

## Context

### Problem Statement
`AChunkActor` (one Actor + 2 `UProceduralMeshComponent` per chunk) is the single
largest structural cost in the voxel world: ~200 MB of chunk-store RAM at render
distance 8 / 64 height, and a GC-visible UObject per chunk. The baseline report
shows mesh generation dominating frame time (20–45 ms per chunk remesh) and the
streaming loop saturating the game thread. Lot 1 of
`voxel-performance-plan.md` moves generation off-thread; lot 2-3 move meshing
and rendering to a custom scene proxy. Both require the chunk data to live in a
plain, GC-free, cache-friendly structure instead of inside an Actor.

### Constraints
- UE 5.8, C++ primary.
- Must keep existing gameplay (block break/place, raycasts, torch lights, crack
  overlay, save/load) working during the migration — no big-bang rewrite.
- Replication not required for terrain (single-player world today).
- Save format v1 (ADR-0002) must keep loading during migration.

### Requirements
- Chunk identity = `FChunkCoord` (no UObject required to address a chunk).
- Block storage = flat `TArray<uint8>` (or paletted sections in a later ADR).
- Generation and meshing operate on plain data, runnable on `UE::Tasks` workers.
- Game thread only applies finished results (mesh upload, actor/component sync).
- Existing `AChunkActor` becomes a thin render/collision view over `FChunkData`,
  removed entirely in lot 3.

## Decision

Introduce `FChunkData` as the canonical, GC-free chunk representation:

```cpp
struct FChunkData
{
    FChunkCoord Coord;
    int32 Height = DEFAULT_CHUNK_SIZE_Z;
    float BlockScale = DEFAULT_BLOCK_SCALE;
    TArray<uint8> Blocks;        // flat, Size X*Y*Height
    TMap<int32, uint8> Metadata;  // sparse rotation/state
    bool bIsModified = false;

    FORCEINLINE uint8 GetBlock(int32 X, int32 Y, int32 Z) const { ... }
    void SetBlock(int32 X, int32 Y, int32 Z, uint8 ID, bool bMarkModified);
    void SetBlockGenerated(...); // leaves bIsModified untouched
};
```

`AWorldGenerator` owns `TMap<FChunkCoord, FChunkData> ChunkStore` (the source of
truth) and a parallel `TMap<FChunkCoord, TWeakObjectPtr<AChunkActor>> ChunkViews`
for rendering/collision only. Generation and meshing run on workers over
`FChunkData`; the game thread syncs the view (spawn/update/destroy actor) and
uploads mesh buffers.

Migration is incremental:
1. Lot 1 (this ADR, phase A): add `FChunkData` + `ChunkStore`; keep
   `AChunkActor::BlockData` as a mirror filled from `FChunkData` at apply time.
   Generation runs async over a captured `FChunkData*`.
2. Lot 2 (phase B): meshing runs async over `FChunkData`; actor becomes a
   passive buffer holder.
3. Lot 3 (phase C): remove `AChunkActor` entirely; render via custom scene proxy;
   collision via greedy boxes + DDA.

## Alternatives Considered

### Alternative 1: Keep one Actor per chunk, move only generation off-thread
- **Pros**: Smallest change; no new data structure.
- **Cons**: GC cost and RAM ceiling remain; blocks the 192-block height target
  and render-distance > 8.
- **Rejection Reason**: Does not address the structural RAM/GC cost; lots 2-4
  need plain data.

### Alternative 2: Full rewrite to a single region actor with sub-chunks
- **Pros**: Best cache locality, fewest UObjects.
- **Cons**: Breaks every existing system at once; high risk; weeks of work
  with no intermediate shippable state.
- **Rejection Reason**: Too big a leap; incremental migration preferred.

## Consequences

### Positive
- Chunks addressable without UObjects → enables worker-thread generation/meshing.
- Foundation for paletted sections, LRU cold cache, custom scene proxy.
- GC pressure drops as views are removed.

### Negative
- Two representations (FChunkData + AChunkActor mirror) during migration → sync
  cost and bug surface.
- Save/load must be updated to read/write FChunkData.

### Risks
- Race conditions between worker writes and game-thread reads of FChunkData.
  Mitigation: workers only touch chunks not yet published; publish via a
  single-threaded apply step.
- AChunkActor mirror drift. Mitigation: remove mirror in phase C; until then,
  assert equality in debug builds.

## Performance Implications
- **CPU**: generation/meshing off GT; GT only applies results (target < 5 ms).
- **Memory**: −30/40 % chunk-store RAM once views are removed (lot 2-3).
- **Load Time**: unchanged initially; improves with cold cache (lot 2).

## Migration Plan
Phase A (lot 1): add FChunkData + ChunkStore, async generation over FChunkData,
mirror into AChunkActor at apply. (This commit.)
Phase B (lot 2): async meshing, LRU cold cache, remove mirror reads.
Phase C (lot 3): custom scene proxy, remove AChunkActor.

## Validation Criteria
- Before/after CSVs (VoxelBench) on the same machine/label.
- No regression in block break/place, raycasts, torch lights, crack overlay,
  save/load.
- Unit tests for FChunkData GetBlock/SetBlock boundary and modified-flag rules.

## Related Decisions
- ADR-0002 (generated vs player voxel writes)
- voxel-performance-plan.md lots 1-4
- ADR-0004 (render policy)
