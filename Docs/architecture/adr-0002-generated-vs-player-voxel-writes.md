# ADR-0002: Generated Voxel Writes Are Not Player Edits

## Status
Accepted

## Context

### Problem Statement
Every voxel write went through `AChunkActor::SetBlock()`, which sets `bIsModified = true`.
World generation writes every voxel of every chunk through that same function, so **every
chunk ever visited is flagged dirty** and written to disk in full (16 KB at
`CHUNK_HEIGHT = 64`) when it unloads. A second, worse consequence sat in `BeginPlay()`:
the whole save directory was deleted recursively on startup, so player edits never survived
a restart.

The "delta" file is not a delta either: it serialises the entire block array and drops
`BlockMetadata`, so furnace rotations are lost on load.

### Constraints
- Generation must stay deterministic from the world seed.
- Player edits must persist across sessions.
- The existing file format (magic `VOXL`, full block array) is already on disk in test worlds.
- The chunk store is about to be rewritten (palette sections, async generation); the write
  policy must be correct before that rewrite, not after.

### Requirements
- Distinguish a reproducible write (generation) from a durable write (player edit).
- Only durable writes may mark a chunk for persistence.
- No data may be deleted at startup.
- No behavioural change to terrain generation.

## Decision

Split the write API into two paths:

| API | Used by | Sets `bIsModified` |
| --- | --- | --- |
| `SetBlock` | player break/place, any runtime edit | yes |
| `SetBlockGenerated` | terrain, caves, ores, trees, villages | **no** |
| `SetBlockWithMeta` | player-placed rotated blocks | yes |
| `SetBlockWithMetaGenerated` | generator-placed furnaces/doors | **no** |

`AWorldGenerator::BeginPlay()` no longer deletes `WorldSavePath`; it only creates the
directory. A terrain-parameter change must be handled by a format version inside the chunk
file, never by wiping data. The startup purge was removed in the same change.

### Key Interfaces
```cpp
// ChunkActor.h
void SetBlock(int32 X, int32 Y, int32 Z, uint8 BlockID);                 // durable
void SetBlockGenerated(int32 X, int32 Y, int32 Z, uint8 BlockID);        // reproducible
void SetBlockWithMeta(int32 X, int32 Y, int32 Z, uint8 BlockID, uint8 MetaRotation);
void SetBlockWithMetaGenerated(int32 X, int32 Y, int32 Z, uint8 BlockID, uint8 MetaRotation);
```

## Alternatives Considered

### Alternative 1: Keep one setter and reset the flag after generation
- **Description**: Call `SetBlock` everywhere, then `bIsModified = false` at the end of
  `GenerateChunkData()`.
- **Pros**: One-line change.
- **Cons**: Any generator path that forgets to reset (or that runs after the reset, such as
  structures) silently reintroduces the bug; the intent is invisible at the call site.
- **Rejection Reason**: Too easy to regress, and it does not fix `SetBlockWithMeta`.

### Alternative 2: Track a per-voxel dirty bit
- **Description**: Bitset of modified voxels; save only those.
- **Pros**: True delta; smallest files.
- **Cons**: Doubles the write path complexity before the chunk store rewrite, which will
  introduce palettes anyway.
- **Rejection Reason**: Deferred to the chunk format v2 ADR; this ADR only fixes the policy.

## Consequences

### Positive
- A chunk is written only if the player actually changed it.
- Player edits survive a restart (the purge is gone).
- Disk I/O on unload drops to zero for untouched terrain.
- Structures (trees, villages) no longer bake themselves into save files, so changing the
  generator now affects visited-but-unedited chunks on the next load.

### Negative
- Chunk files written by the previous build are **full copies** and still override generation;
  they will keep doing so until the v2 format reinterprets them as deltas.
- In the current format, a terrain-generator change produces no automatic invalidation: an
  old saved chunk and a new generated one can look different at a seam.

### Risks
- **Seams between old full-chunk saves and new terrain.** Mitigation: chunk format v2 adds a
  version + generator-parameter hash; mismatching chunks are regenerated and the player's
  edits are re-applied from a real delta.
- **A future generator path uses `SetBlock` by mistake.** Mitigation: automation test that
  generates a fresh chunk and asserts `bIsModified == false`, plus a test that a player edit
  sets it to true.

## Performance Implications
- **CPU**: removes a full-chunk serialise + file write per unloaded chunk; no measurable
  cost added.
- **Memory**: none.
- **Load Time**: unchanged for fresh worlds; faster for unedited areas, which no longer read
  a redundant file.
- **Network**: not applicable.

## Migration Plan
1. Ship the four setters and route all generator calls to the `*Generated` variants (done).
2. Remove the `BeginPlay` purge (done).
3. Next: chunk format v2 (version, `ChunkHeight`, CRC, RLE, sparse `BlockMetadata`), which
   turns the file into a real delta and makes old full copies migratable.

## Validation Criteria
- `MinecraftDeWish.Voxel.GeneratedChunkIsNotDirty`: after `GenerateChunkData`, `bIsModified`
  is false for a chunk that has no save file.
- `MinecraftDeWish.Voxel.PlayerEditIsDirty`: after `SetBlock`, `bIsModified` is true.
- Manual: launch twice, break a block, relaunch — the block stays broken.

## Related Decisions
- `Docs/architecture/adr-0001-lumen-mandatory.md`
- `Docs/architecture/voxel-performance-plan.md`
- `Docs/SYSTEMS_REFERENCE.md` §1
