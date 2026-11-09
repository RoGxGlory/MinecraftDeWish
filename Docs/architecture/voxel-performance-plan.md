# Voxel World Performance Plan

Status: **active**
Owner: `technical-director`
Last updated: 2026-11-09

Companion to `adr-0001-lumen-mandatory.md` and `adr-0002-generated-vs-player-voxel-writes.md`.
All numbers below are **engineering estimates derived from the code**, not measurements.
Lot 0 exists to replace them with real ones.

## 1. Diagnosis

The renderer is not the bottleneck. The streaming loop is.

| Confirmed fact | Evidence |
| --- | --- |
| One `AChunkActor` + 2 `UProceduralMeshComponent` per chunk | `ChunkActor.cpp:13,24` |
| Collision is a Chaos trimesh cooked **synchronously** per chunk | `bUseAsyncCooking=false`, `bUseComplexAsSimpleCollision=true` — `ChunkActor.cpp:15-16,26-27` |
| Generation, meshing, collision cook and upload all run on the game thread | `ProcessGenerationQueue()` |
| Two chunks per 50 ms tick | `MaxChunkGenerationsPerFrame = 2`, `TickInterval = 0.05` |
| Each generated chunk triggers a **full** remesh of up to 4 neighbours | `UpdateNeighborChunkMeshes()` |
| Leaves, glass, fences and doors generate every internal face (`WorldGenerator.cpp:250-257`) | unchanged; leaf culling reverted (see §4), glass/fences/doors still unoptimised |
| Every chunk used to be written to disk in full on unload | fixed by ADR-0002 |
| The save directory was deleted at startup | fixed by ADR-0002 |

Estimated cost per chunk: **8–25 ms** (noise ~45 %, meshing ~18 %, neighbour remesh ~20 %,
collision ~12 %, spawn + upload ~5 %). At render distance 8 the world fills over ~5 s with
40–50 frames at 25–45 ms.

Additional structural costs, not yet measured:
- `M_Global` appears to be `BLEND_Masked` for all terrain, which kills early-Z.
- `r.RayTracing=True` on geometry that re-meshes continuously.
- One `UPointLightComponent` per torch, re-scanned on every remesh.

## 2. Target architecture

1. **Data** — paletted 16³ sections (`uint16` palette + bit-packed data), per-chunk
   `uint8 HeightMap[256]` and `uint64 OccupancyMask[12]`. Empty sections cost nothing; this is
   what makes a 192-block world affordable.
2. **Generation / meshing** — pure functions over plain data, on `UE::Tasks` workers, cancellable,
   prioritised by player velocity. Game thread only applies finished results.
3. **Rendering** — no `UProceduralMeshComponent` for terrain. Custom `FPrimitiveSceneProxy` with
   persistent RHI buffers per region (4×4 chunks), range updates instead of buffer recreation,
   packed vertices (8–12 bytes vs ~64 today), greedy meshing, baked AO, no tangents.
4. **LOD** — 2×/4×/8× rings with skirts and dithered fade-in. Pop-in is the main perceived
   "not smooth", before frame rate.
5. **Collision** — greedy boxes for a 5×5-chunk window around the player + DDA voxel raycasts.
   No trimesh per chunk.
6. **Lighting** — Lumen mandatory (ADR-0001) fed by a throttled coarse GI proxy; terrain split
   into Opaque / Cutout / Translucent material paths; torch light pool or baked block light.

## 3. Lots

| Lot | Content | Expected gain | Cost | Risk |
| --- | --- | --- | --- | --- |
| **0** | Profiling scopes, write-policy fix, save purge removal, Lumen scalability override | −60 to −80 % of streaming hitches | 3–5 d | Low |
| **1** | Off-thread generation | GT cost per chunk 3–8 ms → ~0.1 ms; streaming throughput ×5–10 | 5–8 d | Low |
| **2** | Off-thread meshing, data-oriented store, upload budget, LRU cold cache | spikes < 12 ms; memory −30/40 %; revisiting is free | 8–12 d | Medium |
| **3** | Custom scene proxy, packed vertices, AO, greedy meshing, material split, GI proxy | memory −50 %; base pass −10/25 %; triangles −50/80 % | 10–15 d | High |
| **4** | LOD, box collision, DDA raycast, light pooling | unlocks render distance > 8 **and** 192 blocks | 8–12 d | Medium |
| **5** | Scalability presets, dynamic resolution, upscaler, PSO precache | ×2–4 fps on low-end | 5–8 d | Low |

**Hard prerequisite for 192 blocks: lots 0, 1 and 2.** Raising the height first triples
generation, mesh memory and frame spikes on an already saturated loop.

## 4. Measurement protocol

Lot 0 must ship a reproducible benchmark before any optimisation is claimed:

- dedicated benchmark map, fixed seed, five scripted camera paths (spawn, 60 s straight walk,
  180° turn, fall into a cave, 50-block demolition);
- Test/Shipping build, documented reference machine;
- Unreal Insights capture with `-trace=cpu,gpu,counters,memory,bookmark`;
- `stat unit`, `stat game`, `stat scenerendering`, `stat rhi`, `stat streaming`,
  `stat physics`, `stat gc`, `stat lumen`, `stat virtualshadowmaps`.

Trace scopes already added in `WorldGenerator.cpp`:
`AWorldGenerator_ProcessGenerationQueue`, `_GenerateChunkData`, `_ChunkUpdateMesh`,
`_UpdateNeighborChunkMeshes`, `_UpdateChunkStreaming`.

The scripted part of the protocol is automated by `UVoxelBenchSubsystem`
(`Source/MinecraftDeWish/{Public,Private}/VoxelBenchSubsystem.*`). Run it with
`-VoxelBench` on the command line or `mdw.Bench.Start <label>` in the console; it writes
`Saved/Profiling/VoxelBench-<label>-<timestamp>.csv` with per-frame frame time, queue depths
and the four stage timings from `FVoxelFrameStats`, then logs the p50/p95/p99/max summary.
`mdw.Bench.Marker <text>` stamps a phase change (spawn, turn, teleport) into the CSV so a
capture can be sliced. Do not claim a lot's gain without two CSVs (before / after) taken on
the same machine with the same label.

### Leaf rendering — reverted to full leaves, opaque leaves deferred to lot 3

Every leaf face is generated again. The earlier `IsFaceVisible()` rule culled only the
leaf-to-leaf contact, but `M_Global` is a **Masked** (alpha-tested), **single-sided** material
(verified through EngineWire: `blendMode: Masked`, `twoSided: false`). Culling the contact
between two leaves therefore meshed the canopy as a hollow shell: the interior vanished, and
thin sections let the player see straight through the tree. That regression was shipped, seen in
PIE and reverted — the function is gone and face culling is back to
`IsBlockTransparent(NeighborBlockID)`.

`bFastLeaves` is kept as a persisted player preference but no longer affects the mesh, and
`SetFastLeaves()` no longer queues a remesh. Do not re-enable leaf-to-leaf culling until leaves
can be rendered genuinely opaque.

Open item (lot 3): truly opaque fast leaves. Correction to an earlier plan — **`VertexColor.A`
is not free**: it already drives the `Alpha` input of two `LinearInterpolate` nodes (base colour
and normal) in `M_Global`. The free channels are **R and G**. The route is therefore:
leaf faces write `0` into `G` (R/G are unconnected today; G currently carries an unused
`BlendMask`), the Opacity Mask chain becomes `max(existing, 1 - G)`, and the culling can then go
back to "leaves are opaque for everyone" — which is the larger triangle win, since it also culls
the terrain behind a canopy. This is a change to the single terrain material, so it needs its
own approval and a visual pass over every block type.

### Budgets

| Metric | Baseline | After lots 0–2 | After lots 3–4 |
| --- | --- | --- | --- |
| Game thread, streaming | to measure | < 6 ms | < 5 ms |
| Game thread, worst frame | to measure | < 12 ms | < 10 ms |
| Upload per frame | not measured | < 2 MB | < 2 MB |
| Visible draw calls | ~1 200–1 600 | same | < 800 |
| Chunk store RAM, RD 8 / 64 h | ~200 MB | < 90 MB | < 60 MB |
| Chunk store RAM, RD 8 / 192 h | ~600 MB | ~200 MB | < 140 MB |
| Fill time, RD 8 | ~5 s, 40+ hitches | < 3 s, no hitch > 16 ms | < 2 s |

## 5. Remaining ADRs to write

1. Paletted 16³ section representation and serialisation.
2. Data-oriented chunk store without per-chunk actors.
3. Asynchronous generation and meshing pipeline (pools, cancellation, determinism).
4. Custom scene proxy with pooled RHI buffers.
5. Greedy meshing with baked ambient occlusion.
6. LOD rings and crack handling.
7. Greedy box collision and DDA raycasts.
8. Streaming budgets and chunk cache policy.
9. Terrain material split (opaque / cutout / translucent).
10. Scalability presets and low-end policy.
11. Chunk persistence format v2 (real delta, versioned).
12. Dynamic lighting in the voxel world (torch pool vs baked block light).
13. World height migration to 192 blocks.

## 6. Resolved and open decisions

| Decision | Status |
| --- | --- |
| Lumen policy | **Resolved — mandatory everywhere** (ADR-0001) |
| Voxel write policy | **Resolved** (ADR-0002) |
| 192-block timing | Recommended: after lots 1–3. Awaiting confirmation. |
| Collision strategy | Recommended: greedy boxes + DDA. Awaiting confirmation. |
| Render distance ceiling | Currently 64 in code; recommended ceiling 32. Awaiting confirmation. |
| Foliage representation | **Resolved — voxel leaves, no foliage cards.** `bFastLeaves` is stored but inert; opaque fast leaves deferred to lot 3. |
| Upscaler plugin (FSR/DLSS/XeSS) | Requires a plugin ADR. Awaiting confirmation. |
| `r.RayTracing` default | Recommended off; independent of Lumen. Awaiting confirmation. |
