# GDD — POI Placement

| Field | Value |
|---|---|
| Status | Draft (2026-11-09) — **not implemented** (only a per-chunk house exists) |
| System owner | game-designer / level-designer / gameplay-programmer |
| Related | `design/levels/LEVEL_DESIGN_FRAMEWORK.md` §6–7, `design/levels/biome-grammar.md` §6, `design/gdd/angels.md` §3.8, `design/gdd/items.md`, `design/gdd/save-load.md` |

---

## 1. Overview

The world is procedural, but the **places that matter are authored**. This document defines
how an authored POI (village, hospital, police station, relay, chapel) is chosen, placed,
reconciled with the terrain, and carved into a chunk that is generated independently of its
neighbours.

The current code does the opposite of this document: `AWorldGenerator::GenerateChunkContent`
places **one village house per chunk** whenever `VHash % 7 == 0` in Plains or Desert
(`WorldGenerator.cpp:1055-1071`). That is roughly one house every 112 blocks, confined to a
single 16×16 chunk, with no square, no road, no inhabitants and no relation to any landmark.
**It is replaced, not extended** (user decision).

The replacement is a **region grid with jitter**: one POI decision per 1024×1024-block
region, deterministic from the world seed, with no state to save. The algorithm never
invents geometry — it selects an authored layout and stamps the part of it that intersects
the chunk being generated.

---

## 2. Player Fantasy

> *"Something is out there, and I can see it from here."*

The placement system exists to make the world **readable at a distance** and **worth
crossing**. The player should:

- **Navigate by silhouette.** A tower, an antenna, a bell spire — the world is a set of
  landmarks, not a noise field.
- **Decide before arriving.** The landmark announces the risk. A hospital spire over a dead
  forest is a promise of loot and a warning.
- **Feel the gap.** Between POIs there is wilderness, and the wilderness is where the night
  catches you. Sparse placement is what makes a POI feel like an event rather than a room
  in a corridor.

The opposite fantasy — "there is always something useful nearby" — is explicitly rejected.
It converts exploration into shopping.

---

## 3. Detailed Rules

### 3.1 Why placement is a grid, not a search

Terrain-node placement (find a valley floor, find a confluence) is more organic, but it has
three fatal properties for an infinite world:

1. it costs a **neighbourhood query** over an unbounded area to prove a POI is valid,
2. it produces **clusters and deserts** that cannot be tuned,
3. it requires **state** to remember decisions across sessions.

A region grid with jitter is O(1) per chunk, deterministic, and needs no saved state. The
organic feel comes from the **jitter and the biome weights**, not from the search.

### 3.2 The region grid

```
RegionSize   = 1024 blocks
Jitter       = 192 blocks

RegionCoord  = (floor(X / RegionSize), floor(Y / RegionSize))
RegionHash   = Hash(WorldSeed, RegionCoord.X, RegionCoord.Y)
Candidate    = RegionCenter + JitterVector(RegionHash)      // each axis in [-192, +192]
```

Properties:

| Property | Value | Why |
|---|---|---|
| Average POI spacing | `≈ 1024` blocks | Matches the "sparse, landmark-driven" decision |
| Minimum POI spacing | `RegionSize − 2 × Jitter = 640` blocks | Guarantees no two POIs touch, without any inter-region query |
| Max POI footprint | `64 × 64` blocks | A chunk belongs to **exactly one** region, so generation never queries neighbours |
| State needed | **None** | Fully derived from the seed |
| Cost per chunk | One hash, one biome sample, one table lookup | Negligible next to terrain noise |

**The 64-block footprint cap is a hard constraint of the whole system.** It is what makes
stateless stitching possible. A POI that needs more space must be split into several
regions, or the region rule must change deliberately — not accidentally.

### 3.3 POI selection: the biome table

The candidate is not guaranteed to produce a POI. Each biome owns a **weighted table**, and
one entry is `None`:

```
POIType = WeightedPick(POITable[BiomeAt(Candidate)])
if POIType == None: this region has no POI
```

Proposed tables:

| Biome | Village | Hospital | Police station | Relay / observatory | Chapel | `None` |
|---|---|---|---|---|---|---|
| **Plains** | 40 % | 10 % | 15 % | 0 % | 5 % | 30 % |
| **Foothills** | 25 % | 5 % | 15 % | 10 % | 0 % | 45 % |
| **Forest** | 15 % | 20 % | 5 % | 0 % | 10 % | 50 % |
| **Blight** | 0 % | 25 % | 5 % | 0 % | 25 % | 45 % |
| **Horror Forest** | 0 % | 30 % | 0 % | 0 % | 30 % | 40 % |
| **Desert** | 5 % | 0 % | 25 % | 5 % | 0 % | 65 % |
| **Scrubland** | 10 % | 0 % | 20 % | 0 % | 0 % | 70 % |
| **Mountains** | 0 % | 0 % | 5 % | 25 % | 0 % | 70 % |
| **Marsh** | 5 % | 0 % | 0 % | 0 % | 10 % | 85 % |

This is how density varies by biome (user decision) without touching `RegionSize`: a Desert
region is *empty seven times out of ten*, a Plains region rarely is. The player learns the
terrain → expectation mapping implicitly.

**Validation rule:** if the chosen POI's own placement constraints fail (§3.4), the region
falls back to `None`. It does **not** re-roll, because a re-roll makes the result depend on
the order of constraint checks and breaks determinism across code changes.

### 3.4 Placement constraints

A candidate must pass **all** of these, or the region is empty:

| # | Constraint | Value | Failure reason |
|---|---|---|---|
| C1 | Terrain height at candidate | `> SeaLevel + 1` | No underwater POIs (there is no fluid simulation) |
| C2 | Surface slope over the 64×64 footprint | `≤ 3` blocks of height difference between corners | A pad cannot be flattened without a cliff |
| C3 | Pad depth below the surface | `PadZ − 8 ≥ 4` | Room for foundations above bedrock |
| C4 | Pad height below the ceiling | `PadZ + POIHeight + 8 ≤ ChunkHeight` | The landmark must fit under the world limit |
| C5 | Distance to the nearest other region's candidate | `≥ 640` (guaranteed by §3.2) | Automatic |
| C6 | Chunk not already saved by the player | — | Player edits always win (§3.7) |

`POIHeight` is the POI's own authored height, including its landmark. At `ChunkHeight = 192`
with `SeaLevel ≈ 77`, a mountain relay at pad 140 with a 48-block mast would fail C4 — that is
correct and intentional: **the tallest POIs are valley POIs**.

### 3.5 Terrain reconciliation

The pad is **flat**, with **softened edges** (user decision):

```
PadZ        = median(GetTerrainHeight at the 4 pad corners)   // robust against one spike
PadMargin   = 2 blocks                                        // wall thickness outside the layout
FeatherWidth= 3 blocks                                        // blend band outside the margin
```

For every column inside `FeatherWidth` of the pad:

```
t          = distanceIntoFeather / FeatherWidth             // 0 at the pad edge, 1 outside
ColumnZ    = round(lerp(PadZ, TerrainZ, smoothstep(t)))
```

Fill rules:

- **Cut:** blocks above `ColumnZ` are removed.
- **Fill:** blocks between `ColumnZ` and `TerrainZ` use the biome's **subsoil** block (Dirt in
  Forest/Plains, Sand in Desert, Stone in Mountains), with the biome's surface block on top.
- **Never** leave a floating island, a one-block column, or an exposed bedrock edge.
- The POI's own foundation blocks are written **after** reconciliation, so they always win.

The pad must not exceed `64 × 64`; the jitter guarantees the pad is never bisected by a
region border, because `Jitter + 32 < RegionSize / 2`.

### 3.6 Landmark and hidden POIs (mixed discovery)

Two discovery classes (user decision):

**Class A — landmark POIs.** Village, hospital, police station, relay, chapel. Each carries
an authored vertical element:

| POI | Landmark | Height above pad | Visibility target |
|---|---|---|---|
| Village | Watchtower with a lit top | `20` blocks | 150+ blocks |
| Hospital | Broken lift shaft / antenna | `28` | 180+ |
| Police station | Radio mast | `24` | 160+ |
| Relay / observatory | Dish mast | `40–48` | 250+ |
| Chapel | Bell spire | `22` | 150+ |

The landmark must be **lightable or lit**, so the approach is never a blind walk
(`LEVEL_DESIGN_FRAMEWORK.md` §3.1).

**Class B — hidden POIs.** Cellar hatches, cave mouths, buried research sites, collapsed
shelters. They have **no landmark** and are placed by a second roll inside the same region:

```
if POIType != None and RegionHash bit 0 == 1:
    HiddenPOI = WeightedPick(HiddenTable[Biome])
```

A hidden POI **must have a tell** — otherwise it is a coin flip, not exploration
(`LEVEL_DESIGN_FRAMEWORK.md` §2.1):

- a draft of air (dust particles rising from the ground),
- a faint sound cue inside `DiscoveryRadius = 24` blocks,
- a discoloured cluster of blocks (moss, rust, cracked stone),
- a torch stub or a cable running into the ground.

The tell is emitted at `24` blocks and not beyond. Hidden POIs are a reward for walking
slowly, not for flying over the map.

### 3.7 Stitching a POI into a chunk

Chunk generation is independent per chunk, so a POI is stamped, never "built":

```
OnGenerateChunk(Coord):
    Region   = RegionOf(Coord)
    Layout   = GetOrBuildLayout(Region)          // cached, deterministic from the seed
    if Layout.Type != None:
        WriteIntersection(Layout, Chunk)         // only the blocks inside this chunk
```

Rules:

1. `GetOrBuildLayout` computes the candidate, the constraints, the pad and the module list
   **once per region** and caches it in a `TMap<FIntPoint, FPOILayout>` with an LRU cap.
2. The layout is **pure** — same seed, same result, in any order, on any machine.
3. The chunk writes only the voxels inside its own `16 × 16 × ChunkHeight` bounds.
4. `LoadChunkDelta` runs **after** stamping (`WorldGenerator.cpp:1074`), so **player edits
   always overwrite POI blocks**. That ordering must be preserved.
5. Encounters (Angel nests, NPC spawns, container contents) are **not** written as blocks;
   they are recorded in a separate POI instance record (§3.9).

### 3.8 Village layout

The village replaces the current single house and is assembled from modules:

| Module | Count | Notes |
|---|---|---|
| Central square + well | 1 | The Lynch **node** where roads and doors face |
| Houses | `3–7` | 5×5 to 9×7 footprints, arranged facing the square |
| Lamp posts | `ceil(perimeter / 12)` | **This is the village's safety condition** |
| Watchtower (landmark) | 1 | Lit top, the only thing visible from 150 blocks |
| Crafting table + furnace | 1 each | Existing `GenerateVillageHouse` content, now in a workshop module |
| Road stubs | 2–4 | 20–40 blocks outward, fading into terrain. No global road network pre-v1 |
| NPC anchors | 2–5 | Spawn points for recruitable colonists (`npc-colony.md`) |

**The lamp posts are the mechanic.** A village is safe **only while its light holds**
(`biome-grammar.md` §6). The generator writes the village *with its lamps intact*; every lamp
is a real block with a real light level. If the player steals a lamp, the coverage hole is
real and immediate — the village stops being safe, and the player can watch it happen.

Coverage rule:

```
VillageLit = every tile inside the village footprint has
             LocalLightLevel >= AngelLightThreshold
```

Lamp spacing `12` blocks with a light radius of `8` (`light-and-power.md` §3.1) gives a
`4`-block overlap margin. Removing one lamp leaves an unlit ring of roughly `4` blocks —
small, but enough for an Angel to work with.

### 3.9 POI instance records

The blocks are stateless. The **state** lives in a small per-POI record:

```cpp
USTRUCT()
struct FPOIInstance
{
    FIntPoint RegionCoord;        // stable key
    EPOIType  Type;
    FIntVector PadOrigin;         // world voxel of the pad corner
    int32     AngelNestRemaining; // finite nest (angels.md §3.8)
    int32     NpcCount;
    bool      bDiscovered;
    bool      bVillageLit;        // cached, recomputed on block change
    TArray<FContainerState> Containers;
};
```

These records **must be saved**; the geometry must not (`save-load.md`).

### 3.10 Nest and encounter anchoring

Angel nests and NPC anchors are authored **anchor points inside the module**, not random
positions:

| POI | `AngelNestRemaining` | Anchor rules |
|---|---|---|
| Village | `0` | Angels never nest in a lit place |
| Hospital | `6–12` | One anchor per dark ward; **never** in the entrance module |
| Police station | `3–6` | Anchored behind the armoury denial |
| Chapel | `1–3` | Anchored under the floor or behind the altar |
| Relay | `2–4` | Anchored in the equipment room |
| Hidden POI | `0–2` | Anchored in the deepest module |

**Rule:** an anchor is never in the module the player enters first. The first room must be
readable and Angel-free, or the POI teaches nothing and kills the player in the doorway.

---

## 4. Formulas

| Formula | Value | Rationale |
|---|---|---|
| `RegionSize` | `1024` blocks | Sparse spacing without state |
| `Jitter` | `192` blocks | Organic offsets, still guarantees `640` minimum spacing |
| `MinPOISpacing` | `RegionSize − 2 × Jitter = 640` | User decision: `600–1200` |
| `MaxPOIFootprint` | `64 × 64` blocks | One chunk ↔ one region |
| `PadMargin` | `2` blocks | Foundation thickness |
| `FeatherWidth` | `3` blocks | Terrain blend |
| `MaxPadSlope` | `3` blocks across the footprint | Flattenable without a cliff |
| `DiscoveryRadius` (hidden) | `24` blocks | Reward for walking, not flying |
| `LandmarkVisibility` | `150–250` blocks | Lynch landmark targets |
| `VillageLampSpacing` | `12` blocks | Radius 8, overlap 4 |
| `VillageHouseCount` | `3–7` | Reads as a place, not a hamlet |
| `VillageNpcCount` | `2–5` | Enough to staff a trade point and a watch |
| `AngelNestSize[POI]` | per `angels.md` §3.8 | Difficulty per POI type |

Deterministic helpers:

```
RegionHash  = FNV1a(WorldSeed ^ (RegionX * 0x9E3779B1) ^ (RegionY * 0x85EBCA77))
JitterX     = (RegionHash & 0xFF) / 255.0 * 2 * Jitter - Jitter
JitterY     = ((RegionHash >> 8) & 0xFF) / 255.0 * 2 * Jitter - Jitter
WeightRoll  = ((RegionHash >> 16) & 0xFFFF) / 65535.0
```

---

## 5. Edge Cases

| Case | Resolution |
|---|---|
| Candidate lands in a biome that does not allow the rolled POI | Impossible: the POI is picked **from** the biome's table |
| Candidate fails a constraint | Region is empty. No re-roll (§3.3) |
| Candidate lands in the sea | Fails C1. No POI |
| Pad would cut into a mountain | Fails C2. The POI moves nowhere — the region is empty |
| Two POIs in adjacent regions | Impossible by `MinPOISpacing = 640` |
| A POI crosses a region border | Impossible by `Jitter + footprint/2 < RegionSize/2` |
| A POI would span more than one chunk | Supported: the layout is stamped per intersecting chunk |
| Player edits blocks inside a POI | Saved chunk delta is loaded after stamping, so edits win |
| Terrain parameters change between sessions | Every chunk is stale. This is why `save-load.md` requires a version field |
| A village loses a lamp to the player | `VillageLit` becomes false and the village stops being safe. Working as designed |
| A hidden POI sits under a landmark POI | Forbidden: one hidden slot per region, and only if `POIType != None` |
| A chunk streams in from a different region than the player expects | The cache key is the region, not the chunk; both neighbours resolve to the same layout |
| `AngelNestRemaining` reaches 0 | The nest is empty; the POI is clearable and stays clear (saved state) |
| POI discovered state after reload | Saved in `FPOIInstance.bDiscovered`; the island does not re-announce itself |
| A new POI type is added mid-development | Old worlds change. Save version bump + explicit "regenerate world" prompt |

---

## 6. Dependencies

| System | Why | Status |
|---|---|---|
| `AWorldGenerator::GenerateChunkContent` | Stamping hook | Implemented |
| `FVoxelStructureGenerator` | Module writer (tree, house) | Implemented, 2 functions |
| `LoadChunkDelta` ordering | Player edits must win | Implemented (`WorldGenerator.cpp:1074`) |
| `GetTerrainHeight` / `GetBiomeAt` | Candidate validation, pad height | Implemented |
| Light query for `VillageLit` | Village safety | Partial (`MobSpawnerSystem::CalculateLightLevel`) |
| Angel nests | Encounter anchoring | Not implemented (`angels.md`) |
| NPC anchors | Colony recruitment | Not implemented (`npc-colony.md`) |
| Containers / loot | POI reward | Not implemented (`items.md`) |
| Save of `FPOIInstance` | Nest counts, discovered state | Not documented (`save-load.md`) |
| `ChunkHeight = 192` | C4 landmark headroom | Not implemented |

---

## 7. Tuning Knobs

| Knob | Default | Effect |
|---|---|---|
| `RegionSize` | `1024` | Overall POI density. Must stay `≥ 4 × (Jitter + 32)` |
| `Jitter` | `192` | Organic feel vs. minimum spacing |
| Biome POI weight tables | §3.3 | Which POI types appear where; sum must be 1.0 |
| `None` weight per biome | §3.3 | Emptiness; the main "sparse" dial |
| `MaxPadSlope` | `3` | How mountainous a POI site may be |
| `FeatherWidth` | `3` | How much the pad scars the terrain |
| `HiddenPOIRate` | `50 %` of non-empty regions | Amount of concealed content |
| `DiscoveryRadius` | `24` | How close the player must be to find a hidden POI |
| Landmark heights | §3.6 | Visibility distance |
| `VillageLampSpacing` | `12` | How fragile village safety is |
| `AngelNestSize[POI]` | `angels.md` §3.8 | Difficulty of clearing a POI |

---

## 8. Acceptance Criteria

1. A POI is placed deterministically: same seed, same world, on any machine, in any
   streaming order.
2. Two POIs are never closer than `640` blocks.
3. No POI overlaps a region border.
4. A chunk that intersects a POI writes only its own voxels, and the assembled result is
   visually identical to the authored layout.
5. Player modifications inside a POI survive a reload.
6. Every landmark POI is visible from at least `150` blocks on flat ground.
7. Every hidden POI emits a tell within `24` blocks and none beyond.
8. A village generates with a lit perimeter, and stealing one lamp creates an unlit gap that
   lets an Angel approach.
9. The first module of every POI is Angel-free.
10. `AngelNestRemaining` decreases when an Angel is destroyed, and stays at zero after.
11. The old per-chunk `VHash % 7` house no longer exists.
12. Placement needs **zero** extra save data beyond `FPOIInstance`.

---

## 9. Build order (implementation plan)

1. **Region hash + candidate + `None` table.** A debug command that prints the POI of the
   region under the player. No geometry.
2. **Constraints C1–C4** and an empty-pad stamp (a flat stone platform). Verify spacing.
3. **`GetOrBuildLayout` cache** and per-chunk intersection stamping.
4. **One real module**: the village (square, well, houses, lamps, watchtower). Remove the
   `VHash % 7` path in the same commit.
5. **`FPOIInstance`** and village lamp coverage.
6. **Hidden POIs** with the tell.
7. **Hospital / police / chapel / relay** modules.
8. **Angel nest anchors** (requires `angels.md` steps 1–5).
9. **Save/load** of `FPOIInstance`.
10. **Road stubs**, then a decision on a real road network.

Steps 1–4 remove the current contradictory behaviour and produce a playable sparse world.

---

## 10. Open questions

- Road network between POIs: road stubs only, or real generated paths? A real network is the
  single biggest cost in this document and it is not required by the concept.
- Are hospitals multi-floor? With `ChunkHeight = 192` a three-floor hospital is possible,
  but it multiplies the Angel nest and the blind-angle count.
- Should a region be allowed **two** landmark POIs at a larger `RegionSize` (e.g. `1536`)?
  That would push average spacing to ~1.5 km, which may be too empty.
- Do villages get a **wall**, or is the lamp perimeter the only boundary? A wall implies a
  gate, which implies a guard, which implies colony scope.
- What is the minimum render distance that still lets a player see a landmark at 150 blocks?
  At `RenderDistance = 6` chunks (`96` blocks) the answer today is **no** — this is a real
  conflict with the current default that must be resolved before phase 1 ships.
