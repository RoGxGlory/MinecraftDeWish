# GDD — World Generation (Voxel)

| Field | Value |
|---|---|
| Status | Draft (2026-11-11) |
| System owner | systems-designer / gameplay-programmer / engine-programmer |
| Related | `Docs/SYSTEMS_REFERENCE.md` §1–2, `Docs/GAME_DESIGN.md` §2, `Docs/BLOCK_REGISTRY.md`, `design/gdd/poi-placement.md`, `design/gdd/mob-spawning.md`, `design/gdd/weather-system.md`, `design/gdd/save-load.md`, `design/gdd/game-concept.md` |
| Source of truth for code | `AWorldGenerator` (`WorldGenerator.h/.cpp`), `AChunkActor` (`ChunkActor.h/.cpp`), `FVoxelStructureGenerator`, `FVoxelNoise`, `VoxelDataTypes.h` |

---

## 1. Overview

`AWorldGenerator` is the single authority for the voxel world. It is an `AActor`
placed in the level and owns four concerns:

1. **Procedural terrain** — an infinite 2D column field (continentalness, rolling
   hills, detail) shaped by five biomes (Plains, Forest, Mountains, Desert,
   Horror Forest), with a hard bedrock floor at Z = 0.
2. **Sub-surface features** — 3D worm caves carved into stone, and stratified
   ore veins (Coal, Iron, Gold, Diamond, Emerald) placed by depth and biome.
3. **Surface features** — biome-specific trees (oak canopies) and, in Plains
   and Desert, the legacy single-house village stamp (to be replaced by the
   region-grid POI system in `design/gdd/poi-placement.md`).
4. **Infinite streaming** — chunks of `16 × 16 × ChunkHeight` blocks are
   spawned, generated and meshed on demand around the player, within a tunable
   render distance of 2–64 chunks (radius), and unloaded (with their deltas
   saved) when they leave the radius.

Generation is deterministic from `WorldSeed`: the same seed, in any streaming
order, on any machine, produces the same world. The world is **not** authored
by hand; the only authored content is the POI modules that the generator stamps
into it.

The chunk height is tunable (`ChunkHeight`, `[32, 256]`) with a shipped default
of **192** (raised from 64 in Lot 2 of the voxel performance plan). Older
documents and the legacy save format still quote 64; 192 is the current value
and a save written at a different height is a hard load failure.

---

## 2. Player Fantasy

> *"The world is not a level. It is a place that goes on forever, and it is
> trying to kill me."*

The world generation exists to make the player feel three things:

1. **Scale and indifference.** The terrain is vast, continuous and unauthored.
   There is no edge, no "level boundary" sign. The player is small inside a
   world that does not care about them, and that smallness is the source of the
   game's loneliness.
2. **Legibility through biomes.** Despite being infinite, the world is readable.
   A desert reads as flat and bright; mountains as jagged and tall; a Horror
   Forest as dark and wet. The player learns to read the silhouette of the land
   and to predict what is under it — caves, ores, trees — before they dig.
3. **Discovery through depth.** The surface is the safe, lit, familiar layer.
   Every block down is a step into darker, rarer, more dangerous material. The
   depth gradient (dirt → stone → ores → bedrock, caves at mid-depth) gives
   mining a sense of descent and reward.

The horror layer (Angels, POIs, light) sits on top of this terrain and depends
on it: an Angel needs a dark cave, a village needs a flat pad, a hospital needs
a valley. The terrain is the stage the horror plays on.

---

## 3. Detailed Rules

### 3.1 Coordinate conventions and voxel scaling

- **Voxel size:** 100 Unreal Units (UU) = 1.0 m = 1 block. All world-space
  positions are `block × BlockScale`.
- **Chunk dimensions:** `CHUNK_SIZE_X × CHUNK_SIZE_Y × ChunkHeight` =
  `16 × 16 × ChunkHeight` blocks. Chunk height tunable `[32, 256]`, default
  `192`.
- **Chunk indexing:** 2D grid coordinates `(ChunkX, ChunkY)`; Z ranges from
  `0` (bedrock floor) to `ChunkHeight - 1` (sky). There is **no** third chunk
  axis — a chunk is a full-height column.
- **World seed:** `WorldSeed` (int32, default `1337` in the editor; replaced by
  a random seed per world in the save system — see `design/gdd/save-load.md`).

### 3.2 Biome system

Five biomes, classified per column from three fractal noise fields:

| Field | Noise call | Frequency | Range | Meaning |
|---|---|---|---|---|
| `Continental` | `Fractal2D(..., 3, 0.005, 0.5, 2.0)` | 0.005 (200-block period) | [-1, 1] | Broad landmass / elevation bias |
| `Temp` | `Fractal2D_01(..., 3, 0.006)` | 0.006 | [0, 1] | Temperature |
| `Moisture` | `Fractal2D_01(... + 5000, 3, 0.006)` | 0.006 | [0, 1] | Humidity |

The fields are sampled through `SampleBiomeFields()`, which first warps the
input coordinates by `BiomeBlendWarp` (two octaves, amplitude 3 blocks) and then
adds `BiomeBlendNoise` field jitter (interlocking lobes). **Both the hard
biome lookup and the smooth cross-fade sample through this function**, so the
texture fade can never drift away from the contour where the surface block
flips.

Classification (`ClassifyBiomeFields`), in priority order:

```
if Continental > 0.35  → Mountains
elif Temp > 0.60 and Moisture < 0.40  → Desert
elif Temp < 0.40 and Moisture > 0.55  → HorrorForest
elif Moisture > 0.50  → Forest
else  → Plains
```

Biome cross-fade (vertex colour, in `M_Global`):

- Weight capped at `0.5` so a block's own texture stays dominant.
- Driven by `GetSurfaceMaterialWeightGrid()` (chunk-wide, shared field
  samples, one-cell apron), sampled per face corner in `AChunkActor`.
- Fade is gated to each column's top four blocks (`Z + 3 >= ColumnTopZ`), so
  cave walls, cliff faces and deep stone keep their own material.
- Width in blocks: `SmoothStep(0.75, 0.75 + BiomeBlendWidth, BlocksToBorder)`,
  where `BlocksToBorder` is the signed distance to the threshold divided by the
  local field gradient (central differences on the apron). `BiomeBlendWidth`
  default `4` blocks.
- Terrain height is ramped across the desert border by `GetDesertBlendAt()`
  (field units, `BiomeHeightRamp` default `0.06`) instead of switching on the
  discrete biome, so a desert no longer meets its neighbours with a cliff.

### 3.3 Terrain height

`GetTerrainHeight(WorldX, WorldY, Biome, DesertBlend)`:

```
Continental = Fractal2D(WX, WY, 3, 0.005, 0.5, 2.0)   // 200-block period
Hills       = Fractal2D(WX+1000, WY+1000, 3, 0.02, 0.5, 2.0)  // 50-block period
Detail      = Fractal2D(WX+2500, WY+2500, 2, 0.06, 0.5, 2.0)  // 17-block period

BaseHeight  = ChunkHeight * 0.4            // ≈ 76.8 at default 192
Height      = BaseHeight + Continental*8 + Hills*8 + Detail*3

if Continental > 0.20:
    MountainRise = ((Continental - 0.20) / 0.80)^2 * 20
    Height += MountainRise

DesertT = smooth desert factor in [0,1] (0.5 on contour)
if DesertT > 0:
    Height = lerp(Height, BaseHeight + Continental*4, 0.5 * DesertT)

return clamp(round(Height), 4, ChunkHeight - 8)
```

Surface block assignment per column:

- `Z == 0` → Bedrock.
- `Z == TerrainHeight` → Sand (Desert), Stone (Mountains above `0.65 × ChunkHeight`),
  else Grass.
- `TerrainHeight - 3 <= Z < TerrainHeight` → Sand (Desert), else Dirt.
- `4 <= Z < TerrainHeight - 3` → Stone.
- `Z > TerrainHeight` → Air.

### 3.4 Caves

Enabled by `bEnableCaves`. 3D worm caves: the intersection of two Perlin3D
fields forms tubular tunnels.

```
For each column (X, Y) in the chunk:
    MaxCaveZ = TerrainHeight - 6
    if MaxCaveZ > 6:
        for Z in [4 .. MaxCaveZ]:
            N1 = Perlin3D(WX*0.035, WY*0.035, WZ*0.035)
            N2 = Perlin3D((WX+317)*0.035, (WY+571)*0.035, (WZ+193)*0.035)
            if N1*N1 + N2*N2 < 0.0035:
                block = Air
```

Caves are strictly underground (`Z <= TerrainHeight - 6`) so the surface is
preserved. They only carve Stone/Air; they never remove bedrock or ores already
written (ores are written in a later pass that checks for Stone).

### 3.5 Ores

Enabled by `bEnableOres`. Stratified veins: for every Stone block between
`Z = 1` and `Z = TerrainHeight - 2`, a 3D fractal threshold test decides the
ore. Priority order (first match wins, `continue`):

| Ore | Depth gate | Noise test | Threshold | Notes |
|---|---|---|---|---|
| Diamond | `Z <= 16` | `Fractal3D(WX*2.5, WY*2.5, WZ*2.5, 2, 0.05)` | `> 0.74` | Deepest, rarest |
| Gold | `Z <= 32` | `Fractal3D(WX*2.0, WY*2.0, (WZ+100)*2.0, 2, 0.045)` | `> 0.68` | Medium-deep |
| Iron | `Z <= 55` | `Fractal3D(WX*1.5, WY*1.5, (WZ+200)*1.5, 2, 0.038)` | `> 0.60` | Common underground |
| Coal | `Z <= 60` | `Fractal3D(WX*1.2, WY*1.2, (WZ+300)*1.2, 2, 0.032)` | `> 0.54` | Abundant, all depths |
| Emerald | `Biome == Mountains and Z >= 20` | `Fractal3D(WX*3.0, WY*3.0, (WZ+400)*3.0, 2, 0.06)` | `> 0.78` | Rare, mountain-exclusive |

Ores only replace Stone, so a cave that has already carved a column to Air is
not re-filled. An ore vein is a contiguous region of the 3D noise above the
threshold; there is no explicit "vein size" parameter — the size emerges from
the noise frequency.

### 3.6 Trees

Enabled by `bEnableTrees`. A second pass runs after terrain/caves/ores so that
leaf blocks written above the surface are not overwritten. For each column
`(X, Y)` with `2 <= X <= 13` and `2 <= Y <= 13` (a 2-block apron from the
chunk edge, so a tree never straddles a chunk boundary without its trunk being
inside):

```
THash = hash(WorldX, WorldY, WorldSeed)   // splitmix-style, deterministic
TreeRoll = (THash & 0xFFFF) / 65535.0

spawn if:
   Forest      and TreeRoll < 0.040
   HorrorForest and TreeRoll < 0.030
   Plains      and TreeRoll < 0.008
   (Mountains, Desert: never)

and TerrainHeight + 8 < ChunkHeight   // headroom for the canopy
```

`FVoxelStructureGenerator::GenerateTree` writes an oak log trunk with a full
rounded 3D leaf canopy on all sides. The trunk height and canopy radius are
fixed by the generator (not tunable today). Trees are written with
`SetBlockGenerated` so they are not persisted as edits.

### 3.7 Structures (legacy village)

The current code stamps **one village house per chunk** when:

```
CenterBiome in {Plains, Desert}
and (VHash % 7) == 0,  VHash = hash(Coord.X, Coord.Y, WorldSeed)
and CenterSurfaceZ + 8 < ChunkHeight
```

at chunk centre `(8, 8)`. `FVoxelStructureGenerator::GenerateVillageHouse`
writes a cobblestone foundation and walkway, wood-log corners, wood-plank
walls, glass windows, a doorway, a cobblestone roof, and an interior crafting
table, furnace and ground torch.

This path is **explicitly replaced** (not extended) by the region-grid POI
system in `design/gdd/poi-placement.md`: one POI decision per 1024×1024-block
region, with authored modules (village, hospital, police station, relay,
chapel) and a hidden-POI second roll. Until that system ships, the legacy
house remains. The replacement removes the `VHash % 7` path in the same commit
that lands step 4 of the POI build order.

### 3.8 Streaming

`UpdateChunkStreaming()` runs every tick and maintains the set of loaded chunks:

1. Convert the player position to a chunk coordinate
   (`WorldLocationToChunkCoord`).
2. For every chunk coordinate within `RenderDistance` (Chebyshev radius) of the
   player chunk, if not loaded, enqueue it in `GenerationQueue`.
3. `ProcessGenerationQueue()` spawns an `AChunkActor` per queued chunk and
   calls `GenerateChunkData` (terrain, caves, ores, trees, structures), then
   `ApplyMeshData` (solid + non-solid procedural mesh sections). Generation is
   time-budgeted: `MaxGenerationTimeMs` (default 4.0 ms) per frame, replacing
   the legacy `MaxChunkGenerationsPerFrame` (default 2) cap.
4. Generation and meshing may run on background threads
   (`FPendingAsyncChunkGen`, `FPendingAsyncChunkMesh`); completion is drained on
   the game thread.
5. Chunks outside the radius are saved (`SaveChunkDelta`) and unloaded
   (`DestroyActor`), freeing their memory.
6. When a rendering setting changes face culling, affected chunks are queued in
   `RemeshQueue` and rebuilt at `MaxChunkRemeshesPerTick` (8) per tick.

`SetRenderDistance(NewRenderDistance)` clamps to `[2, 64]`, persists the value
to `GameUserSettings.ini` under `[MinecraftDeWish.GameSettings] RenderDistance`,
and re-runs streaming when a player chunk is known.

### 3.9 Meshing (per chunk)

`AChunkActor::GenerateMeshData` traverses the 16×16×ChunkHeight block array and
culls faces whose neighbour is opaque (`IsBlockTransparent` neighbour check).
Each face carries:

- `UV0.XY` — face UVs (0..1), rotated per block, phase-shifted on isotropic
  tops (`bStochasticTopUVs`).
- `UV1.X` — primary Texture2DArray slice index (0..36).
- `UV1.Y` — secondary (neighbouring biome) slice index.
- `VertexColor.R` — slice index (legacy mirror of `UV1.X`).
- `VertexColor.G` — tiling-breakup mask (1 = natural terrain, 0 = constructed).
- `VertexColor.B` — biome blend weight [0, 0.5].

Solid blocks go to section 0 with `QueryAndPhysics` collision; non-solid blocks
(Torch only, today) go to section 1 with `QueryOnly` collision (`Overlap` for
Pawns, `Block` for WorldStatic traces). `ApplyMeshData` uploads the buffers to
the procedural mesh sections.

### 3.10 Performance budgets

- Target 60 FPS → 16.6 ms frame budget; gameplay systems under 4 ms/frame.
- Generation budget `MaxGenerationTimeMs` = 4.0 ms/frame (range [0.5, 16.0]).
- Remesh budget 8 chunks/tick.
- Streaming cost is measured each tick into `FVoxelFrameStats` (streaming /
  generation / mesh / remesh ms + chunk counters) and sampled by
  `UVoxelBenchSubsystem` into `Saved/Profiling/VoxelBench-*.csv`.
- Torches use a global pool of 16 shadowless point lights (`MaxTorchLights`).

---

## 4. Formulas

### 4.1 Terrain height

See §3.3. Summary:

```
H = 0.4*ChunkHeight + 8*C + 8*Hills + 3*Detail
  + (C > 0.20 ? ((C-0.20)/0.80)^2 * 20 : 0)
  desert ramp: H = lerp(H, 0.4*ChunkHeight + 4*C, 0.5*DesertT)

C, Hills, Detail ∈ [-1, 1] fractal noise; clamped output ∈ [4, ChunkHeight-8].
```

Example at `ChunkHeight = 192`, flat plains (C≈0, Hills≈0, Detail≈0, DesertT≈0):
`H ≈ 76.8 → 77`. A mountain peak (C≈0.8): `H ≈ 76.8 + 6.4 + … + 18 ≈ 101`.

### 4.2 Biome classification

```
Continental = Fractal2D(warped, 3, 0.005, 0.5, 2.0)
Temp        = Fractal2D_01(warped, 3, 0.006)
Moisture    = Fractal2D_01(warped + (5000,5000), 3, 0.006)

warp: SX = X + Fractal2D(X,Y,2,0.018,0.5,2.0)*BiomeBlendWarp   (BiomeBlendWarp=3)
      SY = Y + Fractal2D(X+7919,Y+104729,2,0.018,0.5,2.0)*BiomeBlendWarp
jitter: Temp += Fractal2D(X,Y,2,0.055,0.5,2.0)*BiomeBlendNoise
        Moisture -= same jitter   (BiomeBlendNoise=0.010)

Classify:
  Continental > 0.35                            → Mountains
  Temp > 0.60 and Moisture < 0.40                → Desert
  Temp < 0.40 and Moisture > 0.55               → HorrorForest
  Moisture > 0.50                               → Forest
  else                                          → Plains
```

### 4.3 Biome cross-fade (vertex weight)

Per material membership from signed distance to the thresholds:

```
S = smooth membership for Sand  (Temp > 0.60, Moisture < 0.40)
R = smooth membership for Stone (Continental > 0.35)
Grass = (1-S)*(1-R),  Sand = S*(1-R),  Stone = R*(1-S)   (normalized)
```

Fade half-width in blocks: `SmoothStep(0.75, 0.75 + BiomeBlendWidth, BlocksToBorder)`
with `BlocksToBorder = signedDistance / localGradient` (central differences on the
apron). Weight capped at `0.5`. Plateau of 0.75 blocks guarantees the shared
corner between two differently-classified blocks lands on `0.5`.

### 4.4 Cave condition

```
Carve if N1^2 + N2^2 < 0.0035,  for Z in [4, TerrainHeight-6]
N1 = Perlin3D(WX*0.035, WY*0.035, WZ*0.035)
N2 = Perlin3D((WX+317)*0.035, (WY+571)*0.035, (WZ+193)*0.035)
```

### 4.5 Ore thresholds

See §3.5 table. Each ore is a contiguous region of a 3D fractal above its
threshold, gated by depth (and biome for Emerald). No explicit vein-size
parameter.

### 4.6 Tree placement probability

```
TreeRoll = hash(WorldX, WorldY, WorldSeed) & 0xFFFF  / 65535.0

P(spawn) =
  0.040  in Forest
  0.030  in HorrorForest
  0.008  in Plains
  0.000  in Mountains, Desert
gated by TerrainHeight + 8 < ChunkHeight and 2 <= X,Y <= 13 (chunk-local).
```

Expected trees per chunk ≈ 16×16 × P (only Forest/Plains/Horror columns count):
Forest ≈ 10.2 trees/chunk, Plains ≈ 2.0, HorrorForest ≈ 7.7.

### 4.7 Legacy village gate

```
spawn if CenterBiome ∈ {Plains, Desert}
  and (hash(Coord.X, Coord.Y, WorldSeed) % 7) == 0
  and CenterSurfaceZ + 8 < ChunkHeight
```

≈ 1 house per 7 chunks in Plains/Desert, at chunk centre.

### 4.8 Streaming radius

```
LoadedSet = { chunks C : max(|C.x - P.x|, |C.y - P.y|) <= RenderDistance }
RenderDistance ∈ [2, 64], default 8
Max loaded chunks = (2*RD + 1)^2  → 25 at RD=2, 169 at RD=8, 16641 at RD=64
```

### 4.9 Voxel ↔ world conversion

```
WorldUU = (BlockIndex + 0.5) * BlockScale     (BlockScale = 100 cm)
BlockIndex = floor(WorldUU / BlockScale)
ChunkCoord = floor(BlockIndex / 16)
```

---

## 5. Edge Cases

| Case | Resolution |
|---|---|
| `ChunkHeight` changed between builds | Save format rejects the chunk (`ChunkHeight` mismatch in header); chunk regenerates from seed. See `save-load.md` §3.3. |
| Terrain height at a column exceeds `ChunkHeight - 8` | Clamped to `ChunkHeight - 8`; the top 8 blocks stay Air. Prevents blocks above the world limit. |
| Terrain height below 4 | Clamped to 4; bedrock at 0, stone above. Prevents floating bedrock or empty columns. |
| Cave carving reaches the surface | Impossible: caves only carve `Z <= TerrainHeight - 6`, leaving a 6-block stone cap. |
| Cave carving removes an ore already written | Ores are written after caves, and only into Stone, so a carved column stays Air. No ore floats in a cave. |
| Ore threshold produces a vein crossing a chunk border | Each chunk evaluates noise independently from world coordinates, so the vein is continuous across borders (same seed, same function). |
| Tree at chunk edge | The flora pass restricts `X, Y ∈ [2, 13]`, so a trunk is always at least 2 blocks from the edge and its canopy stays inside the chunk. |
| Tree with insufficient headroom (`TerrainHeight + 8 >= ChunkHeight`) | Skipped. No partial tree, no canopy clipping the sky. |
| `bEnableCaves / bEnableOres / bEnableTrees = false` | The corresponding pass is skipped entirely; terrain and surface blocks are unaffected. |
| Render distance set below 2 or above 64 | Clamped to `[2, 64]` by `SetRenderDistance`. |
| Player at world origin with no chunk loaded | The first streaming pass spawns the player's chunk and its neighbours; generation is time-budgeted so the first frame may be a partial load. |
| Two chunks generate concurrently | Async generation uses per-chunk state; completion is drained on the game thread. No shared mutable state between chunks. |
| A chunk is unloaded and reloaded | It regenerates from seed, then `LoadChunkDelta` re-applies player edits. Generated blocks are not persisted (see `save-load.md` §3.4). |
| `BiomeBlendStrength = 0` | Cross-fade disabled; `VertexColor.B = 0` and `UV1.Y = UV1.X`. Material lerp is a no-op. |
| `BiomeBlendWidth <= 0` | Treated as disabled (`KINDA_SMALL_NUMBER` guard); no fade. |
| A column classified Mountains above `0.65 × ChunkHeight` | Surface block is Stone instead of Grass (snow-line stand-in). |
| Horror Forest classification | Requires both cold (`Temp < 0.40`) and wet (`Moisture > 0.55`); a cold dry column becomes Plains/Mountains, a warm wet one becomes Forest. |
| `MaxGenerationTimeMs` exceeded mid-chunk | The current chunk generation completes (atomic per chunk); further chunks defer to the next frame. No torn chunk. |
| `WorldSeed` identical across two worlds | Terrain is identical; `WorldId` keeps the saves separate. |

---

## 6. Dependencies

| System | Why | Status |
|---|---|---|
| `AWorldGenerator` | Subject of this document | Implemented |
| `AChunkActor` | Renders each `16×16×ChunkHeight` chunk; meshing, collision, block queries | Implemented |
| `FVoxelStructureGenerator` | Trees and the legacy village house | Implemented (2 functions) |
| `FVoxelNoise` | All fractal/Perlin noise (terrain, caves, ores, biomes) | Implemented |
| `VoxelDataTypes.h` | `CHUNK_SIZE_X/Y`, `DEFAULT_CHUNK_SIZE_Z (192)`, `DEFAULT_BLOCK_SCALE (100)`, `EBlockType`, `EBiomeType`, `FChunkCoord` | Implemented |
| `Block_DataTable` | Block definitions, durability, texture slice mapping | Implemented |
| `UMiningQueueSystem` | Breaks generated blocks; consumes durability | Implemented |
| `UDayNightCycleSystem` | Sky light for surface brightness (indirect) | Implemented |
| `UMobSpawnerSystem` | Spawns on generated surfaces in dark areas | Implemented |
| `design/gdd/poi-placement.md` | Replaces the legacy village stamp with region-grid POIs | Draft (not implemented) |
| `design/gdd/mob-spawning.md` | Depends on terrain surfaces, caves, light levels | Draft |
| `design/gdd/weather-system.md` | Modulates sky light; does not change terrain | Draft |
| `design/gdd/save-load.md` | Chunk format v2, `SetBlock`/`SetBlockGenerated` split, version gating | Draft |
| `design/gdd/game-concept.md` | Biomes (Plains, Forest, Mountains, Desert, Horror Forest), POIs | Draft |
| `design/gdd/light-and-power.md` | Light levels, torches (future electric lamps) | Draft (0% implemented) |
| `M_Global` material | Texture array sampling, biome cross-fade, tiling dithering | Implemented |

Bidirectional note: `mob-spawning.md`, `poi-placement.md`, `weather-system.md` and
`save-load.md` all reference this document as the source of terrain truth; any
change to a threshold, frequency or depth gate here must be propagated to those
documents and to `GeneratorVersion`.

---

## 7. Tuning Knobs

All knobs are `EditAnywhere` on `AWorldGenerator` (or in `Block_DataTable` for
block data) and must never be hardcoded in C++. Safe ranges are the
`ClampMin`/`ClampMax` declared in the header where present.

| Knob | Default | Range | Effect |
|---|---|---|---|
| `WorldSeed` | 1337 (editor) | int32 | Entire world identity. Changing it regenerates everything. |
| `ChunkHeight` | 192 | [32, 256] | Vertical world size. Changing it invalidates all saves. |
| `BlockScale` | 100 cm | — | Voxel size. Changing it breaks all coordinate math; do not tune at runtime. |
| `RenderDistance` | 8 | [2, 64] | Loaded radius in chunks. Memory and CPU scale as `RD²`. |
| `TerrainFrequency` | 0.0035 | — | Base continental noise frequency. Lower = broader landmasses. |
| `bEnableCaves` | true | bool | Toggles 3D worm caves. |
| `bEnableOres` | true | bool | Toggles all ore veins. |
| `bEnableTrees` | true | bool | Toggles tree generation. |
| `bEnableTopTextureBlending` | true | bool | Toggles biome cross-fade. |
| `BiomeBlendWidth` | 4.0 | [0, 32] | Fade half-width in blocks. |
| `BiomeBlendNoise` | 0.010 | [0, 0.25] | Border interlock jitter. |
| `BiomeBlendWarp` | 3.0 | [0, 32] | Large-scale border meander in blocks. |
| `BiomeBlendStrength` | 1.0 | [0, 1] | Overall fade strength (0 = disabled). |
| `BiomeHeightRamp` | 0.06 | [0, 0.30] | Desert height-ramp half-width in field units. |
| `MaxGenerationTimeMs` | 4.0 | [0.5, 16.0] | Generation budget per frame. |
| `MaxChunkGenerationsPerFrame` | 2 | — | Legacy chunk-count cap (kept for compat). |
| `MaxChunkRemeshesPerTick` | 8 | — | Remesh throughput after a setting change. |
| `bFastLeaves` | true (stored) | bool | **Currently has no visual effect** (see §3.9 note). Do not expose as working. |
| `WindStrength` | 1.0 | [0, 2] | Foliage wind multiplier (material-driven, no remesh). |

Biome classification thresholds (`Continental > 0.35`, `Temp > 0.60`,
`Moisture < 0.40`, etc.) are currently constants in `ClassifyBiomeFields` and
`DesertBlendFromFields`. They are tuning-relevant and should be promoted to
`EditAnywhere` properties or DataTable rows before shipping; until then, any
change requires a code edit and a `GeneratorVersion` bump.

---

## 8. Acceptance Criteria

1. The same `WorldSeed` produces a bit-identical world on two different machines,
   in any streaming order, including chunk border continuity for terrain, caves
   and ore veins.
2. A chunk's terrain column matches `GetTerrainHeight()` for every `(X, Y)` in
   `[0, 15]²`, with Bedrock at Z = 0 and Air above `TerrainHeight`.
3. Caves never breach the surface: every carved voxel satisfies `Z <=
   TerrainHeight - 6`.
4. Ores only replace Stone; no ore appears in Air, Dirt, Sand or Bedrock, and
   Emerald appears only in Mountains columns.
5. Trees spawn only in Forest, HorrorForest and Plains, only with
   `TerrainHeight + 8 < ChunkHeight`, and their trunk is at least 2 blocks from
   any chunk edge.
6. Biome classification is deterministic and matches the threshold table in §3.2
   for a set of reference `(Continental, Temp, Moisture)` triples.
7. The biome cross-fade weight is in `[0, 0.5]`, is `0` outside the top four
   blocks of a column, and the block ID flips exactly on the `GetBiomeAt()`
   contours (no hard seam).
8. A desert border ramps height via `GetDesertBlendAt()`; no cliff appears where
   a desert meets another biome.
9. Streaming loads every chunk within `RenderDistance` and unloads (with delta
   save) every chunk outside it; `GetActiveChunkCount()` matches
   `(2*RD+1)²` on flat ground.
10. `SetRenderDistance` clamps to `[2, 64]`, persists to `GameUserSettings.ini`,
    and re-runs streaming when a player chunk is known.
11. Generation never exceeds `MaxGenerationTimeMs` per frame (measured via
    `FVoxelFrameStats::GenerationMs`); a chunk is never left half-generated.
12. A modified block survives a reload via `LoadChunkDelta`; a generated block
    (tree, ore, cave) does not overwrite a player edit.
13. The legacy `VHash % 7` village house is removed in the same commit that
    lands POI step 4 (see `poi-placement.md` §9).
14. All tuning knobs in §7 are `EditAnywhere` with declared clamp ranges; no
    balance value is hardcoded in C++ for terrain, caves, ores, trees or
    streaming.