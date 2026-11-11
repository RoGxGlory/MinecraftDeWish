# ADR-0015: Greedy meshing with baked ambient occlusion

## Status
Proposed

## Date
2026-11-11

## Context

Lot 3 requires greedy meshing (merging adjacent faces of the same type into larger
quads) and baked ambient occlusion (per-vertex AO from neighbour solids).

### Problem Statement

Current meshing generates 6 faces per block, each as a 4-vertex quad (2 triangles).
A 16×16×64 chunk with 50% solid blocks = ~8192 faces = ~16384 triangles.
Greedy meshing can reduce this by 50–80% by merging coplanar faces of the same
block type.

### Decision

1. **Greedy meshing algorithm**: For each axis direction, sweep through the chunk
   and merge maximal rectangles of identical block faces. Standard voxel greedy
   meshing (Mikola Lysenko approach).

2. **Baked AO**: Per-vertex AO computed from the 3 blocks adjacent to each vertex.
   Stored in VertexColor.A (currently unused). Range: 0.25 (3 solid neighbours) to
   1.0 (0 solid neighbours).

3. **No tangents**: Tangent computed in shader from screen-space derivatives or
   precomputed in a separate buffer. Saves 12 bytes/vertex.

4. **AO in material**: M_Global samples VertexColor.A as a multiplier on the
   final base color (darkening). Requires material update.

### Implementation Notes

- `AChunkActor::GenerateMeshData` will be rewritten to use greedy quads.
- AO calculation is already partially implemented in `AddFace` (stores in
  VertexColor.A).
- Greedy meshing requires grouping faces by (block type, texture slice, face
  direction) and merging along 2 axes.

### Consequences

- Triangles: −50 to −80%.
- Mesh build time: +20–50% (greedy is more expensive per chunk but fewer
  triangles to upload).
- Risk: Medium. Greedy meshing changes UV mapping (larger quads need tiled UVs).

### Validation Criteria

- Triangle count reduction measured via `stat scenerendering`.
- Visual correctness: no missing faces, no z-fighting, UVs tile correctly.
- AO visible in renders (corner darkening).