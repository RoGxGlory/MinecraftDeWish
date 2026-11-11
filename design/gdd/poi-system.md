# GDD — POI System (Points of Interest)

| Field | Value |
|---|---|
| Status | Draft (2026-11-11) — **not implemented** (only a per-chunk legacy house exists) |
| System owner | game-designer / level-designer / systems-designer / gameplay-programmer |
| Related | `design/gdd/poi-placement.md` (placement algorithm — canonical for placement), `design/gdd/world-generation.md` §3.7, `design/gdd/mob-spawning.md`, `design/gdd/weeping-angels.md` §3.9, `design/gdd/items-loot.md`, `design/gdd/lighting-power.md`, `design/gdd/light-and-power.md`, `design/gdd/npc-colony.md`, `design/gdd/save-load.md`, `Docs/BLOCK_REGISTRY.md`, `Docs/MOB_REGISTRY.md`, `Docs/SYSTEMS_REFERENCE.md` §1–2 |
| Source of truth for code | `AWorldGenerator::GenerateChunkContent`, `FVoxelStructureGenerator`, `FPOIInstance`, `FPOILayout`, `EPOIType`, `FContainerState` (all to be implemented) |

---

## 1. Overview

A **Point of Interest (POI)** is an authored, multi-chunk structure stamped into the
procedural voxel world by the generator. POIs are the game's **narrative and economic
anchors** (game-concept.md §5.5, pillar P4): every POI exists because it has a reason to
enter (loot, safety, story, NPCs) and a reason to leave (danger, Angel density, fuel
cost). The world is procedural, but the *places that matter* are hand-authored and
selected deterministically from the seed.

The POI system is the umbrella over four concerns:

1. **Types** — the catalogue of POIs (Village, Hospital, Police Station, Chapel, Relay,
   Bunker, and hidden POIs), each with its own module set, loot profile, Angel nest and
   lighting contract.
2. **Placement** — where a POI lands in the world. This is specified canonically in
   `design/gdd/poi-placement.md` (region grid, jitter, biome tables, constraints). This
   document summarises the contract and focuses on the *system* built on top of placement.
3. **Structures** — the voxel modules (walls, doors, floors, roofs, stairs, windows,
   lamp posts, landmarks) that make up each POI, stamped per intersecting chunk.
4. **Encounters & rewards** — containers and their loot tables, Angel nest anchors, NPC
   anchors, and the lighting that defines whether a POI is safe.

POIs are **stateless geometry + stateful instance records**. The blocks are regenerated
from the seed; only `FPOIInstance` (nest counts, discovered flag, container state,
village-lit cache) is saved. Player edits always win over stamped blocks, because
`LoadChunkDelta` runs after stamping.

The system replaces — and does not extend — the legacy single-house-per-chunk stamp
(`VHash % 7` in Plains/Desert). That path is removed in the same commit that lands the
first real POI module (poi-placement.md §9, step 4).

---

## 2. Player Fantasy

> *"Something is out there, and I can see it from here."*

The POI system exists to make the world **readable at a distance** and **worth crossing**.
The player should:

- **Navigate by silhouette.** A tower, an antenna, a bell spire — the world is a set of
  landmarks, not a noise field. Every landmark POI is visible from 150+ blocks on flat
  ground, so the player always has a heading.
- **Decide before arriving.** The landmark announces the risk. A hospital spire over a
  dead forest is a promise of loot and a warning. A chapel spire in a Horror Forest is a
  promise of lore and a single, terrible encounter.
- **Feel the gap.** Between POIs there is wilderness, and the wilderness is where the
  night catches you. Sparse placement (≈ 1024 blocks average, 640 minimum) is what makes
  a POI feel like an event rather than a room in a corridor.
- **Read the ruin.** A hospital is evidence of what happened: wards, gurneys, medicine
  cabinets, a triage desk. The POI teaches the story of the collapse without a line of
  text (pillar P4).
- **Earn the safety.** A village is safe *only while its lamps hold*. The player who
  steals a lamp to light their own base has just broken the village's safety — and can
  watch the consequence arrive.

The opposite fantasy — "there is always something useful nearby" — is explicitly
rejected. It converts exploration into shopping.

---

## 3. Detailed Rules

### 3.1 POI types (catalogue)

Each POI is an `EPOIType` value with its own module set, loot profile, Angel nest size,
lighting contract and landmark. All POIs fit inside a **64 × 64 block footprint** and a
**POIHeight** that fits under `ChunkHeight` (poi-placement.md §3.4, C4).

| Type | Biome affinity | Footprint | Height | Landmark | Angel nest | Discovery class |
|---|---|---|---|---|---|---|
| **Village** | Plains, Forest, Foothills | 48×48 | 12 | Watchtower, lit top | 0 | Landmark (visible 150+) |
| **Hospital** | Forest, Blight, Horror Forest | 56×40 | 24 | Broken lift shaft / antenna | 6–12 | Landmark (visible 180+) |
| **Police Station** | Plains, Foothills, Desert, Scrubland | 40×32 | 16 | Radio mast | 3–6 | Landmark (visible 160+) |
| **Chapel** | Blight, Horror Forest, Marsh | 32×24 | 22 | Bell spire | 1–3 | Landmark (visible 150+) |
| **Relay / Observatory** | Mountains, Foothills | 32×32 | 48 | Dish mast | 2–4 | Landmark (visible 250+) |
| **Bunker** | Desert, Scrubland, Mountains | 32×32 | 12 (underground) | Vent stack / antenna stub | 2–4 | Landmark (vent stack) or hidden |
| **Hidden POI** | Any (second roll) | 16×16 | 8 | None | 0–2 | Hidden (tell within 24 blocks) |

Hidden POI sub-types: cellar hatch, cave mouth, buried research site, collapsed shelter.
They have no landmark and are placed by a second roll inside a non-empty region
(poi-placement.md §3.6).

**Discovery classes:**
- **Class A — landmark POIs** carry an authored vertical element visible from 150–250
  blocks on flat ground. The landmark must be lightable or lit so the approach is never
  a blind walk.
- **Class B — hidden POIs** have no landmark and emit a *tell* (draft of air, sound cue,
  discoloured blocks, torch stub, cable) within `DiscoveryRadius = 24` blocks and none
  beyond. They reward walking slowly, not flying over the map.

### 3.2 Placement (contract summary)

Placement is fully specified in `design/gdd/poi-placement.md`. This section states the
contract the rest of the system relies on; it does not re-derive the algorithm.

- **Region grid:** one POI decision per `RegionSize = 1024` block region, with
  `Jitter = 192` blocks. Average spacing ≈ 1024, minimum spacing `640` blocks
  (`RegionSize − 2×Jitter`), guaranteed without any inter-region query.
- **Selection:** the POI type is picked from a biome-weighted table (poi-placement.md §3.3).
  If the chosen type fails its placement constraints (C1–C4), the region is **empty** —
  no re-roll, to preserve determinism.
- **Constraints:** terrain height above sea level, slope ≤ 3 blocks across the 64×64
  footprint, pad depth above bedrock, pad height below `ChunkHeight`, and player-edit
  precedence (C6).
- **Terrain reconciliation:** the pad is flattened to `PadZ = median(terrain height at
  the 4 corners)` with a 2-block margin and a 3-block feathered blend into the surrounding
  terrain. Fill uses the biome's subsoil block; the POI's own foundation blocks are
  written after reconciliation and always win.
- **Determinism:** `RegionHash = FNV1a(WorldSeed ^ (RegionX × 0x9E3779B1) ^ (RegionY × 0x85EBCA77))`.
  Same seed, same world, any streaming order, any machine.
- **State:** no saved placement state. Placement is fully derived from the seed. Only
  `FPOIInstance` (nest counts, discovered flag, container state) is saved.

**Placement probability (per region, by biome):** see poi-placement.md §3.3 table. The
`None` weight is the main "sparse" dial: Desert is empty 65% of the time, Horror Forest
40%, Plains 30%.

### 3.3 Structures (modules)

A POI is assembled from **modules** — authored voxel templates stamped by
`FVoxelStructureGenerator`. Modules are rectangular, axis-aligned, and never exceed the
64×64 footprint. A POI is a layout (a list of module placements) computed once per region
and cached in `TMap<FIntPoint, FPOILayout>` with an LRU cap.

**Module vocabulary:**

| Module | Dimensions (blocks) | Materials | Notes |
|---|---|---|---|
| **Wall (straight)** | 1 thick, variable length, 3 high | Cobblestone, wood planks, brick, concrete | Opaque; blocks light and Angels (until breached) |
| **Wall (corner)** | 1×1, 3 high | Same as straight | Connects two straight walls |
| **Door frame** | 1 wide, 2 high opening in a wall | Cobblestone/wood | No door block in v1 — open gap; a door block is a future item |
| **Floor slab** | 1 thick, variable | Cobblestone, wood planks, tile | Solid; provides a walkable surface and a spawn surface |
| **Roof slab** | 1 thick, variable | Cobblestone, wood planks, metal sheet | Opaque; blocks sky light and rain |
| **Stairs** | 1 wide, 1 rise per block | Cobblestone, wood | Provides vertical traversal inside multi-floor POIs |
| **Window** | 1×1 opening in a wall | Glass (opaque cube in current build) | Blocks observation (glass is opaque today) |
| **Window bars** | 1×1 overlay | Iron bars | Blocks player passage, allows light and observation |
| **Fence** | 1 thick, 1.5 high | Wood, iron | Blocks player and mob movement, allows light |
| **Lamp post** | 1×1, 4 high, torch/lamp on top | Wood, stone | The village's safety condition (§3.6) |
| **Well** | 3×3 | Cobblestone + water (air in v1) | Central square node; NPC anchor |
| **Watchtower** | 3×3 base, 20 high | Cobblestone + wood | Landmark; lit top |
| **Radio mast** | 1×1 base, 24 high | Metal (scrap) | Landmark; police station |
| **Bell spire** | 2×2 base, 22 high | Stone + wood | Landmark; chapel |
| **Dish mast** | 2×2 base, 40–48 high | Metal | Landmark; relay |
| **Vent stack** | 1×1 base, 6 high | Concrete | Landmark; bunker |
| **Foundation** | Full footprint, 1 thick at PadZ | Cobblestone/stone | Written after terrain reconciliation; always wins |
| **Crate / barrel / chest** | 1×1, 1 high | Wood, metal | Container (§3.5) |

**Construction rules:**
- Walls are **3 blocks high** (floor to ceiling) in single-floor POIs; multi-floor POIs
  (hospital) use 3 blocks per floor with a 1-block floor slab between.
- Doorways are **2 blocks high, 1 block wide**, placed in walls. No door block in v1 —
  the gap is open. A door block (open/closed, breachable) is a future item.
- Windows are placed at **2 blocks above floor** (eye level) and are 1×1.
- Roofs are flat slabs 1 block thick, overhanging the walls by 1 block.
- Stairs rise 1 block per step, 1 block wide, with a 2-block headroom minimum.
- Lamp posts are placed every `VillageLampSpacing = 12` blocks around the village
  perimeter, with a light radius of 8 blocks, giving a 4-block overlap margin.
- **No POI module may contain a block that is not in `Docs/BLOCK_REGISTRY.md`.** New
  blocks require a registry entry and a `GeneratorVersion` bump.

**POI layout authoring:** layouts are data (a `FPOILayout` struct containing a list of
`FModulePlacement` with module ID, origin offset, and rotation). They live in DataAssets
or DataTables, not in code (ue-data.md). Designers author them in the editor; the
generator stamps them.

### 3.4 Containers (loot sources)

POIs reward exploration through **containers**: chests, crates, barrels, lockers, medicine
cabinets, ammo boxes and safes. Containers are placed inside POI modules at authored
anchor points (§3.7). They are voxel blocks (a container is a solid block with an
interaction) — opening one fires a delegate and displays the container's inventory panel.

**Container types:**

| Container | Block ID (future) | Capacity (slots) | Found in | Loot profile |
|---|---|---|---|---|
| **Chest** (wooden) | `Block.Chest` | 27 | Village, chapel, hidden | General loot, food, tools |
| **Crate** (wooden) | `Block.Crate` | 9 | Warehouse, police station | Bulk materials, scrap |
| **Barrel** | `Block.Barrel` | 9 | Cellar, hospital, bunker | Food, water, fuel |
| **Locker** (metal) | `Block.Locker` | 9 | Police station, bunker | Weapons, armour, ammo |
| **Medicine cabinet** | `Block.MedCabinet` | 9 | Hospital | Medicine, bandages, antidote |
| **Ammo box** | `Block.AmmoBox` | 9 | Police station, bunker | Gunpowder, scrap, (future ammo) |
| **Safe** | `Block.Safe` | 9 | Police station, relay, bunker | High-value: blueprints, electronics, batteries |

**Container rules:**
- A container is a solid block with `QueryAndPhysics` collision. Right-clicking it
  within 4 blocks fires `OnContainerOpened` and displays its inventory panel.
- Container contents are rolled **once per POI instance**, on first discovery, using a
  seeded RNG keyed on `(WorldSeed, RegionCoord, ContainerIndex)`. The rolled contents
  are stored in `FPOIInstance.Containers` and persist across reloads.
- A container that has been opened and emptied stays empty (saved state).
- A container that has been destroyed (mined) drops its remaining contents as pickups
  and is removed from `FPOIInstance.Containers`.
- **Container loot is independent of Angel nests.** Looting a container does not reduce
  the nest count; clearing the nest does not restock the container.

**Loot tables (per POI type):** see §4.2.

### 3.5 Angel nests (encounter anchoring)

Angel nests are the **encounter layer** of a POI. Each POI type has a fixed nest size
(`AngelNestRemaining`), and nests are anchored at authored points inside the module —
never in the first module the player enters (poi-placement.md §3.10).

**Nest counts:**

| POI | `AngelNestRemaining` | Anchor rules |
|---|---|---|
| Village | 0 | Angels never nest in a lit place |
| Hospital | 6–12 | One anchor per dark ward; **never** in the entrance module |
| Police station | 3–6 | Anchored behind the armoury denial |
| Chapel | 1–3 | Anchored under the floor or behind the altar |
| Relay / observatory | 2–4 | Anchored in the equipment room |
| Bunker | 2–4 | Anchored in the deepest room / generator room |
| Hidden POI | 0–2 | Anchored in the deepest module |

**Nest rules:**
- An anchor is a spawn point, not a guaranteed spawn. The Angel (or Angels) at an
  anchor are spawned when the POI is first loaded and the player is within
  `ActivationRadius = 80` blocks, subject to the dark/light gate (`L < 7` at the anchor).
  An anchor in a lit voxel spawns nothing until the light is removed.
- `AngelNestRemaining` decreases by 1 each time an Angel anchored to this POI is
  destroyed. It never goes below 0. When it reaches 0, the nest is empty and the POI
  is clearable.
- A cleared nest stays clear (saved state in `FPOIInstance`).
- Angels spawned from a nest do **not** count against the normal mob cap
  (`MaxMobCap = 20`). They have their own population rules (weeping-angels.md §3.9).
- **The first module of every POI is Angel-free.** The first room must be readable and
  safe, or the POI teaches nothing and kills the player in the doorway. This is a hard
  authoring rule, verified by an automation test (§8).

### 3.6 Lighting contract (per POI type)

Lighting is the **safety contract** of a POI. Each POI type defines which spaces are
lit (safe) and which are dark (dangerous). The contract uses the light level
`L(p) ∈ [0, 15]` from `lighting-power.md` §3.1, with the safety threshold
`L(p) ≥ 7`.

**Village — the lit perimeter (the canonical case):**
- Every tile inside the village footprint must have `L(p) ≥ 7`. This is the
  `VillageLit` condition, cached in `FPOIInstance.bVillageLit` and recomputed on
  block change.
- Lamp posts every 12 blocks with radius 8 give a 4-block overlap margin. Removing one
  lamp leaves an unlit ring of ~4 blocks — small, but enough for an Angel to work with.
- The watchtower's top is lit (torch/lamp), making the village visible at night from
  150+ blocks.
- **A village is safe only while its light holds.** Stealing a lamp breaks the
  perimeter; the player can watch the gap form and the consequence arrive.

**Hospital — dark wards:**
- The entrance module (triage, reception) is lit (`L ≥ 7`) — the first room is safe.
- Wards, corridors and the basement are dark (`L < 7`) by design. Angels nest there.
- Emergency lighting (a few powered lamps on `NightOnly`-off circuits, or battery-backed
  lamps) may exist in some wards, creating islands of safety the player must move between.
- The lift shaft / antenna landmark is unlit (broken), so it reads as a silhouette.

**Police station — emergency power:**
- The entrance and front desk are lit.
- The armoury (behind the denial) is dark — Angels nest behind it.
- A backup generator (fuelled by coal/scrap) may power a few lamps in the cells and
  evidence room. If the generator is out of fuel, the station goes dark and becomes
  lethal. This teaches the power system early.

**Chapel — candlelight:**
- The nave is lit by torches/candles (`L ≥ 7`) — a small safe zone.
- The crypt (under the floor) and the space behind the altar are dark. 1–3 Angels nest
  there.
- The bell spire is unlit (silhouette landmark).

**Relay / observatory — the dark tower:**
- The ground floor (equipment room entrance) is lit.
- The upper floors and the dish platform are dark. 2–4 Angels nest in the equipment room.
- The dish mast is a 250-block-visible landmark; it is unlit (silhouette).

**Bunker — sealed and dark:**
- The entrance (vent stack, hatch) is lit.
- The interior is entirely dark by design (`L < 7` everywhere inside). The bunker is a
  nest, not a shelter — 2–4 Angels inside.
- A bunker may contain a fuelled generator and a few lamps, but they are off by default
  (the previous occupants are gone). The player who restarts the generator lights the
  bunker and flushes the Angels — or attracts them.

**Hidden POI — the tell:**
- Hidden POIs are dark by default. The *tell* (draft, sound, discolouration, torch stub)
  is the only signal within 24 blocks. No lighting contract beyond "dark until the player
  brings light."

**Lighting sources used in POIs** (from `lighting-power.md` §3.5):
- Torch: level 14, radius 5, infinite fuel, sodium `#E8A33D`.
- Campfire: level 15, radius 8, loud (`Loudness 6`), consumes wood.
- Electric lamp: level 15, radius 8, 2 EU/s, powered.
- Furnace (active): level 13, radius 4.
- Monitor glow: level 4, radius 0 (a monitor room is dark by design).

POI lighting is placed by the module author (DataAsset). The generator stamps the torch
blocks (block ID + position) as part of the module; the light level is recomputed on load
(not stored).

### 3.7 Encounter anchoring (NPCs and Angels)

Angel nests are specified in §3.5. NPC anchors (for `npc-colony.md`) are also authored
points inside modules:

| POI | NPC anchors | Notes |
|---|---|---|
| Village | 2–5 | Spawn points for recruitable colonists; trade point + watch |
| Hospital | 0 | No NPCs (abandoned) |
| Police station | 0–1 | A survivor or a corpse with a note (lore) |
| Chapel | 0–1 | A lone survivor or a priest (lore) |
| Relay | 0 | Automated; no staff |
| Bunker | 0–1 | A survivor with a story |
| Hidden POI | 0 | Alone |

NPC anchors are voxel positions (world coordinates) stored in `FPOIInstance` (or a
separate `FNPCAnchor` record). They are not spawned by the POI system; the NPC colony
system reads them when the POI is discovered.

### 3.8 Chunk and streaming integration

POIs are stamped into chunks during generation, identically to terrain and trees. The
integration contract (poi-placement.md §3.7):

1. **`GetOrBuildLayout(Region)`** computes the candidate, constraints, pad and module
   list once per region and caches it. The layout is pure: same seed → same result, any
   order, any machine.
2. **`WriteIntersection(Layout, Chunk)`** writes only the voxels inside the chunk's own
   `16 × 16 × ChunkHeight` bounds. A POI that spans 4 chunks is stamped 4 times, once
   per chunk, each writing its own slice.
3. **`LoadChunkDelta` runs after stamping** (`WorldGenerator.cpp` ordering). Player
   edits always overwrite POI blocks. This ordering must be preserved — it is the
   mechanism by which a mined wall stays mined.
4. **Encounters (Angel nests, NPC anchors, container contents) are not written as
   blocks.** They are recorded in `FPOIInstance`, keyed by `RegionCoord`.
5. **A chunk that intersects a POI is generated at the same generation budget**
   (`MaxGenerationTimeMs = 4.0 ms/frame`). Stamping is part of chunk generation, not a
   separate pass. A POI spanning multiple chunks does not stall streaming.
6. **When a chunk is unloaded**, its voxel data (including POI blocks) is saved as a
   delta (`SaveChunkDelta`) and the actor is destroyed. On reload, the chunk regenerates
   from seed (re-stamping the POI) and `LoadChunkDelta` re-applies player edits.
7. **The LRU layout cache** has a cap (proposed 256 entries). A region's layout is
   rebuilt on cache eviction; because it is pure, the rebuild is identical. No state is
   lost.
8. **POI instance records are saved separately** from chunk data, keyed by
   `RegionCoord`. They are loaded at world load and updated on discovery, nest clearing
   and container interaction.

**`FPOIInstance` struct:**

```cpp
USTRUCT()
struct FPOIInstance
{
    FIntPoint RegionCoord;        // stable key (region grid coordinate)
    EPOIType  Type;
    FIntVector PadOrigin;         // world voxel of the pad corner
    int32     AngelNestRemaining; // finite nest (weeping-angels.md §3.9)
    int32     NpcCount;
    bool      bDiscovered;
    bool      bVillageLit;        // cached, recomputed on block change
    TArray<FContainerState> Containers;
    TArray<FVector> AngelAnchors;  // world positions of nest anchors
    TArray<FVector> NpcAnchors;   // world positions of NPC anchors
};
```

**`FContainerState` struct:**

```cpp
USTRUCT()
struct FContainerState
{
    FIntVector VoxelPos;          // world voxel of the container block
    EContainerType Type;
    bool bOpened;
    bool bDestroyed;
    TArray<FItemStack> Contents;  // rolled on first discovery, persisted
};
```

### 3.9 Discovery and the "first room" rule

- A POI is **discovered** when the player enters its footprint (or, for hidden POIs,
  when the player is within `DiscoveryRadius = 24` blocks and the tell is triggered).
- Discovery sets `FPOIInstance.bDiscovered = true` and persists.
- A discovered POI may emit a one-time notification (sound cue, journal entry) —
  deferred to the UI spec.
- **The first module of every POI is Angel-free.** This is a hard authoring rule: the
  entrance module must have `L ≥ 7` (lit) or be small enough that the player can read
  the space before an Angel moves. Verified by an automation test that checks no Angel
  anchor lies in the entrance module's bounds.
- A POI that the player has not discovered is still generated (blocks exist) but its
  `FPOIInstance` is not created until discovery. Nest counts are initialised on
  creation, not on generation, so an undiscovered POI's Angels are not yet spawned.

---

## 4. Formulas

### 4.1 Placement (summary; canonical in poi-placement.md §4)

```
RegionSize   = 1024 blocks
Jitter       = 192 blocks
MinSpacing   = RegionSize - 2 * Jitter = 640 blocks
MaxFootprint = 64 × 64 blocks
PadZ         = median(TerrainHeight at 4 pad corners)
FeatherWidth = 3 blocks
PadMargin    = 2 blocks
MaxPadSlope  = 3 blocks across the footprint

RegionHash   = FNV1a(WorldSeed ^ (RegionX * 0x9E3779B1) ^ (RegionY * 0x85EBCA77))
JitterX      = (RegionHash & 0xFF) / 255.0 * 2 * Jitter - Jitter
JitterY      = ((RegionHash >> 8) & 0xFF) / 255.0 * 2 * Jitter - Jitter
WeightRoll   = ((RegionHash >> 16) & 0xFFFF) / 65535.0
```

### 4.2 Loot tables (per POI type)

Loot uses the `FLootTable` / `FLootEntry` system from `items-loot.md` §3.9. Each
container type in a POI rolls its own table, seeded on `(WorldSeed, RegionCoord,
ContainerIndex)`. Counts are `RandRange(MinCount, MaxCount)`; rolled entries succeed
with probability `Chance`.

**Village (chest, crate, barrel):**

| Container | Guaranteed | Rolled |
|---|---|---|
| Chest (central) | `Item.CannedFood ×2–4`, `Item.CleanWater ×1–2` | `Item.ScrapMetal` 40%, `Item.Blueprint` 15% |
| Crate (workshop) | `Item.ScrapMetal ×3–6` | `Item.Electronics` 20%, `Item.Battery` 10% |
| Barrel (cellar) | `Item.CannedFood ×1–3` | `Item.FuelPellet` 15% |

**Hospital (medicine cabinet, chest, barrel):**

| Container | Guaranteed | Rolled |
|---|---|---|
| Medicine cabinet | `Item.Medicine ×2–4`, `Item.Bandage ×3–6` | `Item.Antidote` 25% |
| Chest (triage) | `Item.Bandage ×2–4` | `Item.Medicine` 30%, `Item.CleanWater` 50% |
| Barrel (basement) | `Item.CleanWater ×2–4` | `Item.FuelPellet` 10% |
| Safe (records) | `Item.Blueprint ×1–2` | `Item.Electronics` 40% |

**Police station (locker, ammo box, safe, crate):**

| Container | Guaranteed | Rolled |
|---|---|---|
| Locker | `Item.ScrapMetal ×2–4` | `Item.Battery` 25% |
| Ammo box | `Item.Gunpowder ×1–3` | `Item.ScrapMetal` 30% |
| Safe (armoury) | `Item.ScrapMetal ×4–8` | `Item.Electronics` 30%, `Item.Blueprint` 20% |
| Crate (storage) | `Item.ScrapMetal ×3–6` | `Item.Battery` 15% |

**Chapel (chest):**

| Container | Guaranteed | Rolled |
|---|---|---|
| Chest (altar) | `Item.CannedFood ×1–2` | `Item.Medicine` 15%, `Item.Blueprint` 10% |

**Relay / observatory (safe, crate):**

| Container | Guaranteed | Rolled |
|---|---|---|
| Safe (records) | `Item.Electronics ×2–4`, `Item.Blueprint ×1–2` | `Item.Battery` 40% |
| Crate (equipment) | `Item.ScrapMetal ×2–4` | `Item.Electronics` 30%, `Item.Battery` 20% |

**Bunker (locker, ammo box, safe, barrel):**

| Container | Guaranteed | Rolled |
|---|---|---|
| Locker | `Item.ScrapMetal ×3–6` | `Item.Battery` 30% |
| Ammo box | `Item.Gunpowder ×2–4` | `Item.ScrapMetal` 40% |
| Safe (command) | `Item.Electronics ×2–3`, `Item.Blueprint ×1–2` | `Item.Battery` 50% |
| Barrel (stores) | `Item.CannedFood ×2–4`, `Item.CleanWater ×2–4` | `Item.FuelPellet` 25% |

**Hidden POI (chest or barrel):**

| Container | Guaranteed | Rolled |
|---|---|---|
| Chest/barrel | `Item.CannedFood ×1–2` | `Item.ScrapMetal` 30%, `Item.Blueprint` 20%, `Item.Medicine` 10% |

**Loot roll rules** (from `items-loot.md` §3.9):
- `Guaranteed` entries always spawn (count rolled uniformly in `[MinCount, MaxCount]`).
- Each `Rolled` entry is evaluated independently with its `Chance`; on success a count is
  rolled in `[MinCount, MaxCount]`.
- Rolls use `FRandomStream LootRNG(Seed)` with seed `(WorldSeed, RegionCoord, ContainerIndex)`.
- Advance order: `Guaranteed[]` then `Rolled[]`, entry by entry, so a fixed seed yields a
  reproducible drop set.

### 4.3 Angel nest counts and anchoring

```
NestSize[POI]:
    Village            = 0
    Hospital           = RandRange(6, 12)
    PoliceStation      = RandRange(3, 6)
    Chapel             = RandRange(1, 3)
    Relay              = RandRange(2, 4)
    Bunker             = RandRange(2, 4)
    HiddenPOI          = RandRange(0, 2)

NestSize is rolled once per POI instance, on discovery, seeded on (WorldSeed, RegionCoord).
```

Angel anchors are authored positions inside the module (world voxel coordinates). An anchor
is never in the first module (entrance). Nest count is stored in `FPOIInstance.AngelNestRemaining`
and decremented on Angel destruction.

### 4.4 Light levels (POI-relevant)

```
SafetyThreshold        = 7   (L(p) >= 7 → no mob spawn, Angel cannot move)
TorchBaseLight         = 14  (radius 5 blocks)
LampBaseLight          = 15  (radius 8 blocks)
CampfireBaseLight      = 15  (radius 8 blocks)
FurnaceActiveLight     = 13  (radius 4 blocks)
MoonlightLevel (night, exposed) = 4
Daylight (day, exposed)         = 15
Enclosed (no sky)               = 0
```

Village lit condition:
```
VillageLit = for every tile t inside the village footprint: L(t) >= 7
```

Lamp spacing for full coverage:
```
LampSpacing = 12 blocks, LampRadius = 8 blocks → overlap = 2 * 8 - 12 = 4 blocks
```

### 4.5 Distance and visibility

```
LandmarkVisibility:
    Village       = 150 blocks (watchtower, lit top)
    Hospital      = 180 blocks (lift shaft / antenna)
    Police        = 160 blocks (radio mast)
    Chapel        = 150 blocks (bell spire)
    Relay         = 250 blocks (dish mast)
    Bunker        = 80 blocks  (vent stack) — or hidden (no landmark)
    Hidden POI    = 0 (no landmark; tell within 24 blocks)

DiscoveryRadius (hidden) = 24 blocks
ActivationRadius (Angel nest) = 80 blocks
```

### 4.6 Container interaction

```
ContainerOpenRange = 4 blocks (right-click within 4 blocks)
ContainerCapacity:
    Chest       = 27 slots
    Crate       = 9 slots
    Barrel      = 9 slots
    Locker      = 9 slots
    MedCabinet  = 9 slots
    AmmoBox     = 9 slots
    Safe        = 9 slots
```

---

## 5. Edge Cases

| Case | Resolution |
|---|---|
| Candidate fails a placement constraint | Region is empty. No re-roll (poi-placement.md §3.3). Determinism preserved. |
| Two POIs in adjacent regions | Impossible by `MinSpacing = 640` blocks. |
| POI crosses a region border | Impossible by `Jitter + footprint/2 < RegionSize/2`. |
| POI spans multiple chunks | Supported: stamped per intersecting chunk via `WriteIntersection`. |
| Player edits blocks inside a POI | `LoadChunkDelta` runs after stamping, so edits win. A mined wall stays mined. |
| Terrain parameters change between sessions | Every chunk is stale; `save-load.md` requires a version field. POI layouts regenerate identically from seed. |
| A village loses a lamp to the player | `VillageLit` becomes false; the village stops being safe. Working as designed. |
| A hidden POI sits under a landmark POI | Forbidden: one hidden slot per region, only if `POIType != None`. |
| A chunk streams in from a neighbouring region | Cache key is the region, not the chunk; both neighbours resolve to the same layout. |
| `AngelNestRemaining` reaches 0 | Nest empty; POI is clearable and stays clear (saved state). |
| POI discovered state after reload | Saved in `FPOIInstance.bDiscovered`; the island does not re-announce itself. |
| A new POI type added mid-development | Old worlds change. Save version bump + explicit "regenerate world" prompt. |
| Container mined (destroyed) | Drops remaining contents as pickups; removed from `FPOIInstance.Containers`. |
| Container opened but not emptied | State saved; contents persist across reloads. |
| Angel anchor in a lit voxel | No spawn until the light is removed. The anchor waits. |
| Angel anchor in the entrance module | Forbidden by authoring rule; verified by automation test. |
| POI generated but not discovered | No `FPOIInstance` created; nest not initialised; Angels not spawned. |
| POI spans a chunk that fails to generate | The chunk is retried next frame; stamping is atomic per chunk. No torn POI. |
| LRU cache evicts a region layout | Layout is rebuilt identically from seed. No state lost. |
| Two containers with the same seed roll | Seeds are `(WorldSeed, RegionCoord, ContainerIndex)` — unique per container. |
| Container loot roll with seed 0 | Valid seed; `FRandomStream(0)` is legal. |
| POI footprint exceeds 64×64 | Rejected at authoring time (data validation). |
| POI height exceeds `ChunkHeight` | Fails constraint C4; region is empty. |
| Bedrock under a POI pad | Constraint C3 (`PadZ − 8 ≥ 4`) prevents foundations on bedrock. |
| POI on a slope > 3 blocks | Fails constraint C2; region is empty. No cliff-cutting. |
| Player builds inside a POI footprint | Player blocks are saved as chunk deltas and win on reload. |
| POI landmark taller than render distance | At `RenderDistance = 6` (96 blocks), a 150-block landmark is not visible. This is a real conflict with the current default; must be resolved before phase 1 ships (poi-placement.md §10). |
| Bunker interior entirely dark | By design. The player must bring light or accept Angel risk. |
| Hospital entrance lit but wards dark | By design. The first room is safe; the rest is not. |
| Chapel crypt under the floor | Angels nest there; the floor blocks observation (solid block). |
| Relay dish mast at 48 blocks height | Must fit under `ChunkHeight = 192` with 8-block margin: `PadZ + 48 + 8 ≤ 192` → `PadZ ≤ 136`. Enforced by C4. |

---

## 6. Dependencies

| System | Why | Status |
|---|---|---|
| `AWorldGenerator::GenerateChunkContent` | Stamping hook; `SetBlockGenerated` / `SetBlockWithMetaGenerated` | Implemented |
| `AChunkActor` | Renders stamped blocks; meshing, collision, block queries | Implemented |
| `FVoxelStructureGenerator` | Module writer (tree, house, POI modules) | Implemented (2 functions; POI modules to add) |
| `FVoxelNoise` / `SampleBiomeFields` | Terrain height, biome classification for placement | Implemented |
| `VoxelDataTypes.h` | Chunk dimensions, `EBlockType`, `EBiomeType`, `FChunkCoord`, `FIntVector` | Implemented |
| `Block_DataTable` | Block definitions for POI materials (cobblestone, planks, glass, etc.) | Implemented |
| `GetTerrainHeight` / `GetBiomeAt` | Candidate validation, pad height, biome tables | Implemented |
| `LoadChunkDelta` ordering | Player edits must win over stamped blocks | Implemented (`WorldGenerator.cpp`) |
| `UMobSpawnerSystem::CalculateLightLevel` | Light level query for `VillageLit`, Angel gating, spawn gating | Implemented (partial — no general `GetLightLevelAt`) |
| `UDayNightCycleSystem` | Sky light, day/night for POI lighting contract | Implemented |
| `UMobSpawnerSystem` | Classic mobs spawn in dark POI areas | Implemented |
| Weeping Angels (`weeping-angels.md`) | Nest anchors, `AngelNestRemaining`, light gate | Not implemented (0%) |
| Item / Loot system (`items-loot.md`) | Container contents, `FItemRef`, `FLootTable` | Not implemented |
| NPC colony (`npc-colony.md`) | NPC anchors, recruitment, staffing | Not implemented |
| Light & Power (`light-and-power.md`) | Electric lamps, generators, fuel for POI lighting | Not implemented (phase 3) |
| Lighting & Power (`lighting-power.md`) | Torch/lamp light levels, point-light pool, sodium | Draft |
| Save/Load (`save-load.md`) | `FPOIInstance`, `FContainerState`, chunk deltas, version gating | Draft |
| `Docs/BLOCK_REGISTRY.md` | Block IDs used in POI modules | Implemented |
| `Docs/MOB_REGISTRY.md` | Mob types for nests (Angel entry to add) | Implemented (Angel missing) |
| `design/levels/LEVEL_DESIGN_FRAMEWORK.md` | Lynch landmarks, Totten level design lens | Draft |

Bidirectional note: `world-generation.md` §3.7 and `poi-placement.md` §3.7 reference this
document for the POI system built on top of placement. `weeping-angels.md` §3.9 references
this document for nest counts and anchoring. `items-loot.md` references this document for
container loot tables. `lighting-power.md` references this document for POI lighting
contracts. Any change to a nest count, loot table, light contract or module here must be
propagated to those documents.

---

## 7. Tuning Knobs

All knobs are `EditAnywhere` on the POI data asset / `AWorldGenerator` and must never be
hardcoded in C++. Safe ranges are the `ClampMin`/`ClampMax` declared in the header.

### Placement (canonical in poi-placement.md §7)

| Knob | Default | Range | Effect |
|---|---|---|---|
| `RegionSize` | 1024 | ≥ 4×(Jitter+32) | Overall POI density. Must stay ≥ 4×(Jitter+32). |
| `Jitter` | 192 | 0–384 | Organic feel vs. minimum spacing. |
| Biome POI weight tables | §3.3 (poi-placement) | sum = 1.0 | Which POI types appear where. |
| `None` weight per biome | 30–85 % | 0–1 | Emptiness; the main "sparse" dial. |
| `MaxPadSlope` | 3 | 1–8 | How mountainous a POI site may be. |
| `FeatherWidth` | 3 | 1–8 | How much the pad scars the terrain. |
| `HiddenPOIRate` | 50 % | 0–100 % | Amount of concealed content. |
| `DiscoveryRadius` (hidden) | 24 | 8–64 | How close the player must be to find a hidden POI. |

### Structures

| Knob | Default | Range | Effect |
|---|---|---|---|
| `WallHeight` | 3 | 2–5 | Blocks per floor. |
| `DoorWidth` / `DoorHeight` | 1 / 2 | fixed | Doorway dimensions. |
| `WindowHeight` | 2 | 1–3 | Blocks above floor for windows. |
| `RoofOverhang` | 1 | 0–2 | Blocks of roof overhang. |
| `VillageLampSpacing` | 12 | 8–16 | Lamp post spacing (blocks). |
| `LampRadius` | 8 | 5–12 | Lamp light radius (blocks). |
| `MaxPOIFootprint` | 64×64 | fixed | Hard constraint of the region system. |

### Loot

| Knob | Default | Range | Effect |
|---|---|---|---|
| `LootChance` per entry | per table | 0–1 | Economy faucets per container. |
| `ContainerCapacity` (chest) | 27 | 9–54 | Inventory pressure per POI. |
| `ContainerCapacity` (crate/barrel/etc.) | 9 | 9–27 | Small container capacity. |
| `LootSeedSalt` | 0 | int32 | Salt added to the loot seed for a "fresh" world variant. |

### Angel nests

| Knob | Default | Range | Effect |
|---|---|---|---|
| `NestSize[POI]` | per §3.5 | 0–20 | Difficulty of clearing a POI. |
| `ActivationRadius` | 80 | 40–150 | How early a Dormant Angel wakes near a POI. |
| `AngelLightThreshold` | 7 | 4–10 | Must equal mob-spawn threshold for consistency. |

### Lighting

| Knob | Default | Range | Effect |
|---|---|---|---|
| `TorchLightRadius` | 5 | 3–8 | Torch protection radius (blocks). |
| `TorchBaseLight` | 14 | 10–15 | Torch brightness. |
| `LampBaseLight` | 15 | 12–15 | Lamp brightness. |
| `SafetyThreshold` | 7 | 4–10 | How much light protects. |
| `PoolSize` (point lights) | 16 | 8–32 | Concurrent rendered dynamic lights. |

### Streaming

| Knob | Default | Range | Effect |
|---|---|---|---|
| `RenderDistance` | 8 | 2–64 | Loaded radius in chunks. POI landmarks need ≥ 150 blocks visibility → RD ≥ 10 (160 blocks) at minimum for landmark visibility. |
| `MaxGenerationTimeMs` | 4.0 | 0.5–16.0 | Generation budget per frame (includes POI stamping). |
| `LRUCacheCap` (layouts) | 256 | 64–1024 | Cached region layouts. |

---

## 8. Acceptance Criteria

1. **A POI is placed deterministically.** Same seed, same world, any machine, any streaming
   order, produces the same POI at the same position with the same type.
2. **Two POIs are never closer than 640 blocks.** Verified by a scan over a 4096×4096
   region.
3. **No POI overlaps a region border.** Verified by checking `PadOrigin` against region
   bounds.
4. **A chunk that intersects a POI writes only its own voxels**, and the assembled result
   is visually identical to the authored layout. Verified by a reference-layout comparison
   test.
5. **Player modifications inside a POI survive a reload.** Verified by mining a wall block,
   reloading, and confirming it is still air.
6. **Every landmark POI is visible from at least 150 blocks on flat ground.** Verified by
   a line-of-sight test from 150 blocks in each cardinal direction.
7. **Every hidden POI emits a tell within 24 blocks and none beyond.** Verified by a
   proximity test.
8. **A village generates with a lit perimeter**, and stealing one lamp creates an unlit gap
   that lets an Angel approach. Verified by a light-level scan and an Angel movement test.
9. **The first module of every POI is Angel-free.** Verified by an automation test that
   checks no Angel anchor lies in the entrance module's bounds.
10. **`AngelNestRemaining` decreases when an Angel is destroyed** and stays at zero after.
    Verified by a kill-and-reload test.
11. **Container contents are deterministic** with a fixed seed (identity + count
    reproducible). Verified by a loot-roll test.
12. **A container that has been opened and emptied stays empty** across reloads. Verified
    by an open-empty-reload test.
13. **A destroyed container drops its remaining contents** and is removed from
    `FPOIInstance.Containers`. Verified by a mine-container test.
14. **The old per-chunk `VHash % 7` house no longer exists.** Verified by a grep for the
    pattern in `WorldGenerator.cpp`.
15. **Placement needs zero extra save data beyond `FPOIInstance`.** Verified by a save-size
    audit.
16. **Every POI type has at least one loot container** with a non-empty loot table. Verified
    by a data validation test.
17. **Every POI type has a documented lighting contract** (which spaces are lit/dark).
    Verified by a doc review.
18. **POI stamping never exceeds `MaxGenerationTimeMs` per frame.** Verified by a
    performance test with a 4-chunk POI crossing the player's position.
19. **A POI's Angel anchors are never in the entrance module.** Verified by an automation
    test over all POI layouts.
20. **All tuning knobs in §7 are `EditAnywhere` with declared clamp ranges.** Verified by a
    code review.

---

## 9. Build order (implementation plan)

1. **`EPOIType`, `FPOIInstance`, `FContainerState`, `FPOILayout`, `FModulePlacement`**
   structs + validation helpers + tests.
2. **Region hash + candidate + `None` table** (poi-placement.md step 1). Debug command
   that prints the POI of the region under the player. No geometry.
3. **Constraints C1–C4** and an empty-pad stamp (flat stone platform). Verify spacing.
4. **`GetOrBuildLayout` cache** and per-chunk intersection stamping.
5. **One real module: the village** (square, well, houses, lamps, watchtower). Remove the
   `VHash % 7` path in the same commit.
6. **`FPOIInstance`** and village lamp coverage (`VillageLit`).
7. **Container blocks** (chest, crate, barrel) + interaction delegate + inventory panel.
8. **Loot tables** per POI type + seeded roll + persistence.
9. **Hidden POIs** with the tell.
10. **Hospital / police / chapel / relay / bunker** modules.
11. **Angel nest anchors** (requires `weeping-angels.md` steps 1–5).
12. **NPC anchors** (requires `npc-colony.md`).
13. **Save/load** of `FPOIInstance` and `FContainerState`.
14. **Road stubs**, then a decision on a real road network.
15. **Landmark visibility verification** and render-distance conflict resolution.

Steps 1–5 remove the current contradictory behaviour and produce a playable sparse world.

---

## 10. Open questions

- Road network between POIs: road stubs only, or real generated paths? A real network is
  the single biggest cost in this document and is not required by the concept.
- Are hospitals multi-floor? With `ChunkHeight = 192` a three-floor hospital is possible,
  but it multiplies the Angel nest and the blind-angle count.
- Should a region be allowed **two** landmark POIs at a larger `RegionSize` (e.g. 1536)?
  That would push average spacing to ~1.5 km, which may be too empty.
- Do villages get a **wall**, or is the lamp perimeter the only boundary? A wall implies a
  gate, which implies a guard, which implies colony scope.
- What is the minimum render distance that still lets a player see a landmark at 150 blocks?
  At `RenderDistance = 6` chunks (96 blocks) the answer today is **no** — this is a real
  conflict with the current default that must be resolved before phase 1 ships.
- Does a door block exist in v1, or are doorways open gaps? Open gaps are simpler but
  reduce the "seal the angle" gameplay. Deferred to `items.md`.
- Can Angels breach a **container** or a **lamp**, or only structural blocks? Breaching
  lamps would make the village's safety fragile in a new way. Deferred.
- Is the Bunker a landmark (vent stack) or always hidden? Both options are viable; the
  biome table decides. Deferred.
- Should POI discovery trigger a journal entry / map marker, or stay purely diegetic?
  Deferred to the UI spec.