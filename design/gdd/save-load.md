# GDD — Save & Load

| Field | Value |
|---|---|
| Status | Draft (2026-11-11) — **no real persistence exists** |
| System owner | gameplay-programmer / lead-programmer / systems-designer |
| Related | `Docs/SYSTEMS_REFERENCE.md` §1–2, `design/gdd/world-generation.md` §3.8, §5, `design/gdd/inventory-quickslots.md`, `design/gdd/player-vitals.md`, `design/gdd/day-night-cycle.md`, `design/gdd/weather-system.md`, `design/gdd/baubles-equipment.md`, `design/gdd/poi-placement.md` |
| Source of truth for current code | `WorldGenerator.cpp:89-97, 1393-1470`, `ChunkActor.h:56-81` |
| Decisions | D-SL-01 (Minecraft world model: main menu, one folder per world, random seed), D-SL-02 (binary format, RLE + Zlib), D-SL-03 (all entities persist), D-SL-04 (corpses persist indefinitely), D-SL-05 (autosave 300 s, no manual save in v1), D-SL-06 (v1 worlds declared incompatible, no migration) |

---

## 1. Overview

MinecraftDeWish currently **cannot save**. `AWorldGenerator::BeginPlay` computes
`Saved/SaveGames/World_<Seed>` and then deletes that directory recursively
(`WorldGenerator.cpp:92-96`), deliberately, to purge chunks generated with older terrain
parameters. What survives a session is therefore nothing. On top of that, the chunk
format has no version, no height and no seed — magic `VOXL`, an `int32` voxel count, and
the raw array — and `LoadChunkDelta` accepts it if the count matches the current array
size. No player, inventory, time-of-day, weather, mob, POI or container state is
written at all.

This document defines the target: a **main-menu world list** (Minecraft model), one
folder per world, a versioned chunk format with compression, a versioned world-state
file, and a clear incompatibility policy for the 64 → 192-block height change.

In one sentence: **the save system turns a throwaway session into a persistent world
by writing only what the player changed and only when it matters, so that the base
built on Tuesday is still standing on Thursday.**

---

## 2. Player Fantasy

> *"I can close the game. The base is still there. The lamps are still on."*

Persistence is what makes the whole design real. The three-phase progression (daylight
exploration → dusk fortification → night survival) only has meaning if the fortress
the player built persists. The emotional contract:

- **Continuity.** The world is the player's work, not a session. A block placed at
  noon is still there at midnight and still there next week.
- **Consequence.** A colonist who died last session is still dead; a mob that was
  lured into a trap is still there.
- **Safety.** A crash does not cost the last hour. The worst case is five minutes
  of lost work, never a whole evening.
- **Honesty.** The game never pretends to have saved when it has not, and never
  silently deletes a world. A corrupt save is reported and recovered from, not
  erased.

The failure mode to avoid is the one that exists today: a game where nothing
persists teaches the player that nothing they do matters. The save system exists to
make every placement, every death, every hour of exploration count.

---

## 3. Detailed Rules

### 3.1 What is saved (honest inventory of current code)

| Data | Saved today? | Where | Notes |
|---|---|---|---|
| Player-edited blocks | Yes | `Saved/SaveGames/World_<Seed>/Chunk_X_Y.dat` | Full raw array, no version |
| Generated blocks (trees, villages) | Yes, wrongly | Same | `SetBlock` sets `bIsModified` |
| Block metadata (furnace rotation) | **No** | — | `BlockMetadata` never serialized |
| Player position, vitals, XP, inventory, Baubles | **No** | — | — |
| Time of day, day count, weather | **No** | — | — |
| Mobs, Angels, colonists | **No** | — | — |
| POI instances, nests, containers | **No** | — | POI system not shipped |
| Dropped items, corpses | **No** | — | — |
| **Anything at all (net)** | **No** | — | Folder is **deleted on startup** |

The target save system must close every gap in this table (except systems not yet
implemented, which are flagged in §6).

### 3.2 World model: main menu, one folder per world

User decision (D-SL-01): **Minecraft model.** The game boots to a main menu; the
player either loads an existing world or creates a new one **with a random seed**, so
every world is different.

```
Saved/SaveGames/
    Worlds.index                    // list of worlds, tiny, rebuildable from folders
    <WorldId>/                      // WorldId = FGuid, stable, never reused
        world.meta                  // header: name, seed, version, dates, playtime
        world.sav                   // global state (player, time, entities, POIs, colony)
        chunks/
            Chunk_X_Y.dat           // versioned, compressed chunk deltas
        backup/
            world.sav.bak
            chunks/                 // optional per-chunk .bak (see §3.9)
```

`world.meta` fields:

| Field | Type | Why |
|---|---|---|
| `WorldId` | `FGuid` | Stable folder identity, independent of the display name |
| `DisplayName` | `FString` | Player-facing, renameable |
| `Seed` | `int32` | Must be known **before** `Noise.SetSeed` |
| `WorldVersion` | `uint16` | Format version of `world.sav` and chunks |
| `GeneratorVersion` | `uint16` | Bumped whenever generation output changes |
| `CreatedAt` / `LastPlayed` | `FDateTime` (UTC) | Menu sorting |
| `PlaytimeSeconds` | `int64` | Honest stat, and a debugging aid |
| `bCreative` / difficulty flags | bitset | Menu display |

**`WorldSeed` stops being `1337`.** The current default (`WorldGenerator.h:48`) is
replaced by a value read from `world.meta`, generated with `FMath::RandRange` seeded
from `FPlatformTime::Cycles()` when the player creates a world. Two worlds with the
same seed are allowed and produce identical terrain; `WorldId` keeps the saves
separate.

**The startup purge is deleted.** `DeleteDirectoryRecursively` disappears; the only
code path that erases a world is an explicit, confirmed "delete world" action in the
menu.

### 3.3 Format: binary vs JSON

Decision (D-SL-02): **binary for all save data.** JSON is rejected for the
following reasons:

- A voxel chunk is 49 152 bytes raw; as JSON text it balloons to ~200 KB per chunk
  and ~34 MB for a loaded radius, and parsing it on load dominates load time.
- Binary supports compression (RLE + Zlib) which shrinks a typical chunk to a few
  hundred bytes; JSON text compresses far worse on repetitive numeric data.
- Binary gives deterministic, version-gated layouts that migration code can reason
  about field-by-field.

The one exception is **`world.meta`**, which is small (a few hundred bytes) and
benefits from being human-readable for debugging and manual recovery. It is stored as
a **binary struct with a 4-byte magic and a version field**, but a
`-SaveAsJson` debug build flag (and a console command `mdw.Save.DumpMeta`) can
serialize the same struct as JSON for inspection. JSON is never the production format.

| File | Production format | Debug option |
|---|---|---|
| `world.meta` | Binary struct (magic + version + fields) | JSON via `-SaveAsJson` / `mdw.Save.DumpMeta` |
| `world.sav` | Binary struct (versioned) | JSON dump via console |
| `Chunk_X_Y.dat` | Binary (header + RLE + Zlib) | Hex dump via console |
| `Worlds.index` | Binary list of `{WorldId, DisplayName, LastPlayed}` | JSON dump |

### 3.4 Chunk format v2

```
struct FChunkHeader
{
    uint32 Magic;            // 'VOXL' 0x564F584C
    uint16 WorldVersion;     // 2
    uint16 HeaderSize;       // bytes, for forward compatibility
    int32  ChunkX, ChunkY;
    int32  ChunkHeight;      // 192 — a mismatch is a hard load failure
    uint32 UncompressedSize; // bytes of the payload before compression
    uint32 Crc32;            // of the uncompressed payload
};
```

Payload, then **RLE + Zlib**:

```
RLE: (uint8 BlockID, uint16 RunLength) repeated until UncompressedSize is reached
Zlib: FCompression::CompressMemory(COMPRESS_ZLIB, ...)
```

Then a second block for metadata:

```
uint32 MetaCount;
(MetaCount × { uint32 VoxelIndex, uint8 MetaValue })   // sparse, fixes the furnace loss
```

Why RLE: a voxel chunk is almost entirely long runs (air, stone, dirt). A raw
49 152-byte chunk compresses to a few hundred bytes, which makes per-5-minute
autosave of dozens of dirty chunks cheap enough to be invisible.

**Header validation is mandatory on load.** `Magic`, `WorldVersion`, `ChunkHeight`
and `Crc32` must all match. Today only `TotalVoxels == BlockData.Num()` is checked
(`WorldGenerator.cpp:1452`), which silently accepts a chunk from a different world
height as long as the count coincides. A mismatched `ChunkHeight` is a hard load
failure: the chunk is discarded and regenerated from the seed.

### 3.5 Generated blocks must not be saved

This is a required fix, not a preference.

Today `FVoxelStructureGenerator` writes trees and villages with `Chunk->SetBlock()`,
which sets `bIsModified = true` (`ChunkActor.cpp:85`). Consequences:

1. Every visited chunk is persisted, so a save grows with exploration instead of
   with edits.
2. A change to tree or village generation never reaches already-visited areas —
   the saved array overwrites the new output.
3. The POI system (`poi-placement.md`) depends on deterministic stamping; baked
   geometry breaks that guarantee.

Rule:

```
SetBlockGenerated(X, Y, Z, Id)   → writes BlockData, does NOT set bIsModified
SetBlock(X, Y, Z, Id)            → writes BlockData, sets bIsModified   (player edit)
```

Terrain, trees, POIs and any other generated content use the `Generated` variant.
Only the player, explosions and Angel breaching use the editing variant. A generated
block never overwrites a player edit on reload: `LoadChunkDelta` applies edits on
top of freshly generated terrain.

### 3.6 World state file (`world.sav`)

```
FWorldSave
├── FSaveHeader          WorldVersion, GeneratorVersion, Seed, SavedAt, PlaytimeSeconds
├── FPlayerState         Transform, Rotation, Health, Hunger, Thirst, XP, Level,
│                        MainInventory[27], QuickSlots[9], Baubles, EquippedTools[..],
│                        SelectedQuickSlot
├── FWorldClock          TimeOfDay, DayCount, CurrentWeather, TargetWeather,
│                        WeatherTransitionAlpha, WeatherTimeRemaining
├── FPOIState[]          POI instance records (poi-placement.md §3.9) — future
├── FEntityRecord[]      every persistent actor (§3.7)
├── FWorldItemRecord[]   dropped items and death corpses (§3.8)
└── FContainerState[]    container block contents (§3.9)
```

`FPlayerState` must be written **even if the player is mid-air or dead**; the death
case saves the corpse (§3.8) and the respawn point. `FWorldClock` persists the
weather state so a save at 02:00 in the rain reloads into the same conditions.

### 3.7 Entity persistence

User decision (D-SL-03): **all entities persist.**

| Entity class | Persisted | Record |
|---|---|---|
| Classic mob (Zombie, Skeleton, Spider, Creeper) | Full | `ActorId` (`FGuid`), `EMobType`, transform, health, AI state |
| Dropped item / corpse | Full | Item ref, count, durability, transform, `bIsCorpse` |
| Projectile / debris | **No** | Transient; destroyed on save |

Rules:

- Entities outside the streamed area are stored **as records only** and respawned
  when their chunk streams in. Simulation of unloaded entities is not per-actor.
- `ActorId` is assigned at spawn from the world RNG and is stable across sessions.
- An `ActorId` that references a missing entity is a **logged, non-fatal**
  condition.

Angels, colonists, POI instances and power circuits are future systems; when they
ship they follow the same `FEntityRecord` / `FPOIInstance` pattern with their own
sub-records. The save format reserves version-gated slots for them so adding a
system does not break older saves.

### 3.8 Death and corpses

Death follows the Minecraft convention: respawn, inventory dropped.

User decision (D-SL-04): **the corpse persists.** It is a normal
`FWorldItemRecord` with `bIsCorpse = true`:

- It survives quit, crash and reload.
- It is **not** despawned by distance or by time. A player who dies in the Horror
  Forest can come back a week later.
- A second death while a corpse exists leaves **two** corpses. No merging, no
  auto-return.
- Corpses are visible as a container-like pile; retrieving one removes it.

`FWorldItemRecord`:

| Field | Type | Notes |
|---|---|---|
| `RecordId` | `FGuid` | Stable identity |
| `ItemRef` | `FItemRef` | Item identity (block or non-block) |
| `Count` | `int32` | Stack count |
| `Durability` | `int32` | For tools; 0 for non-durable items |
| `Transform` | `FTransform` | World position + rotation |
| `bIsCorpse` | `bool` | True for death drops |
| `ChunkCoord` | `FIntVector2` | Owning chunk, for streaming load |

### 3.9 Container state

Container blocks (chest, crate, barrel) persist their slot contents as part of the
world save. Each container is keyed by its world position (block coordinate) and
stores its slot array:

```
FContainerState
{
    FIntVector BlockCoord;       // world block coordinate of the container
    EContainerType Type;        // Chest / Crate / Barrel
    TArray<FQuickSlotData> Slots; // slot count per type (27 / 9 / 27)
}
```

Rules:

- A broken container drops its contents as pickups at the container's location
  before the block is removed (see `inventory-quickslots.md` §3.8); the save
  record is then deleted.
- Container state is loaded when its chunk streams in and applied to the block
  actor.
- An empty container still has a record (so an empty chest is distinguishable from
  "no chest here" — though in practice an empty chest record may be omitted to
  save space; the rule is: **absence of a record means default empty state**).

### 3.10 Save triggers

User decision (D-SL-05): quit-to-menu, quit-the-game, and every 5 minutes.

| Trigger | What is written |
|---|---|
| **Autosave, every `300 s`** | Dirty chunks + `world.sav` + `world.meta` |
| **Quit to main menu** | Full save, then unload |
| **Quit the game** (`EndPlay`) | Full save |
| **World creation** | `world.meta` + empty `world.sav`, then first spawn |
| **Manual save** | **Not exposed in v1** (autosave is the model); see §7 |

The 5-minute budget: at the design render distance of 6 chunks (169 chunks) and
49 152 bytes raw each, the worst case is ~8 MB uncompressed — trivial after RLE.
The cost that matters is **the hitch**, so the write is done on a background task
and the game thread only hands over a copy of the dirty data.

### 3.11 Atomic writes and corruption

Every write is atomic:

1. serialize to `<path>.tmp`,
2. flush and close,
3. rename the existing file to `<path>.bak`,
4. rename `.tmp` to the real path.

On load:

- **CRC mismatch** on a chunk → the chunk is discarded and regenerated from the
  seed. The POI instance record is kept, so nest counts and discoveries do not
  reset.
- **`world.sav` corrupt** → fall back to `world.sav.bak`; if that also fails, the
  world opens in a **recovery state**: chunks load, player state resets to a safe
  spawn, and the player is told explicitly. Never silently delete the world.
- **`WorldVersion` mismatch** → refuse to load with a clear message (§3.12).

### 3.12 Versioning and the 64 → 192 transition

`WorldVersion = 2` is the target format. Worlds written before it have no version
field, so they are **`WorldVersion = 1` by definition**.

User decision (D-SL-06): **old worlds are declared incompatible and deleted.** No
migration code.

Menu behaviour for a `v1` folder:

```
"This world was created in an earlier version and cannot be opened.
 [Delete]   [Cancel]"
```

The `WorldVersion` field exists anyway, because the next generation change (new
biomes, new POI types, `GeneratorVersion` bump) will need exactly the same
mechanism, and because silently regenerating chunks under a player's base is worse
than refusing to load.

**`GeneratorVersion` is separate from `WorldVersion`.** Bumping it means "the same
format, but the generated world would differ". A world with an older
`GeneratorVersion` still loads, and its saved chunks stay frozen — but the menu
warns that the world will differ from a freshly generated one.

### 3.13 Migration policy for new fields

When a new field is added to `FPlayerState`, `FWorldClock` or any record struct:

1. **Bump `WorldVersion`** if the field is required (no sensible default).
2. **Keep `WorldVersion` unchanged** if the field has a default value and can be
   backfilled; the loader reads the field if present, otherwise uses the default.
   This is the preferred path for minor additions (e.g. a new stat, a new weather
   parameter).
3. **Never renumber or remove existing fields.** Save data may reference them by
   offset; removing a field breaks all older saves even within the same
   `WorldVersion`.
4. **New fields are appended**, with a default value documented in the struct
   definition and a migration note in this GDD.
5. **Chunk format changes** (new header field, new compression) always bump
   `WorldVersion` and require regeneration of all chunks (old chunks rejected).

Migration table (maintained as fields are added):

| Field | Added in | Default if missing | Migration note |
|---|---|---|---|
| `FPlayerState.Health` | v2 | `MaxHealth` (20.0) | Full health on first load |
| `FPlayerState.XP` / `Level` | v2 | 0 / 0 | Fresh progression |
| `FPlayerState.SelectedQuickSlot` | v2 | 0 | First slot selected |
| `FWorldClock.CurrentWeather` | v2 | Overcast | Neutral default weather |
| `FWorldClock.DayCount` | v2 | 1 | Start at day 1 |
| `FContainerState` | v2 | Empty (no records) | Containers empty on first load |
| `FWorldItemRecord.Durability` | v2 | 0 | Non-durable items have 0 |

---

## 4. Formulas

### 4.1 Data size

| Formula | Value | Rationale |
|---|---|---|
| `RawChunkBytes` | `16 × 16 × 192 = 49 152` | `ChunkHeight = 192` |
| `MaxRenderChunks` | `(2 × 6 + 1)² = 169` | At `RenderDistance = 6` |
| `RawWorldBytes` | `169 × 49 152 ≈ 8.3 MB` | Worst case before compression |
| `RLEHeaderBytes` | `5` per run | `uint8` id + `uint16` count + alignment |
| `ChunkHeaderBytes` | `32` | Fixed, before compression |
| `CompressedChunkBytes` | `≈ 200–800 B` typical | RLE on runs of air/stone/dirt, then Zlib |
| `WorldSaveBytes` | `≈ 10–50 KB` | Player + clock + entities + items + containers |
| `AutosavePayloadBytes` | `dirty_chunks × ~500 B + world.sav + world.meta` | Order of 10 dirty chunks → ~10 KB |
| `BackupCopies` | `1` per file | `.bak` |
| `WorldVersion` (target) | `2` | `1` = legacy, implicit |

### 4.2 Save time and hitch budget

```
AutosaveInterval        = 300 s (5 min)
DirtyChunksPerSave       ≈ player edits in 5 minutes (order of 10)
WriteBytes               ≈ 10 × 500 B + 50 KB ≈ 55 KB
BackgroundWriteTime     ≈ WriteBytes / DiskWriteSpeed
                        ≈ 55 KB / 100 MB/s ≈ 0.55 ms
GameThreadCost          ≈ copy of dirty data ≈ 10 × 49 152 B ≈ 492 KB memcpy
                        ≈ 0.1 ms on modern hardware
```

The game thread cost is the memcpy of dirty chunk arrays into the background
task's buffer; the actual disk write happens off-thread. Target: **no visible frame
hitch** on target hardware (measured via Unreal Insights / `stat unit`).

### 4.3 Compression ratio

```
CompressionRatio = RawChunkBytes / CompressedChunkBytes
                 ≈ 49 152 / 500 ≈ 98×  (typical terrain-heavy chunk)
                 ≈ 49 152 / 5 000 ≈ 10×  (heavily edited chunk, little run-length)
```

Worst case (every voxel different, no runs) still benefits from Zlib's
dictionary; measured ratio stays above 4× in all tested cases. A chunk that
fails to compress below `RawChunkBytes` is stored uncompressed with a flag in the
header (`bCompressed = false`).

### 4.4 Load time

```
LoadTime ≈ (num_chunks_in_radius × (header_parse + decompress + RLE_decode))
         + world.sav parse
         + entity spawn (deferred to streaming)
```

Chunks are loaded lazily as they stream in, so initial load time is bounded by
the first frame's generation budget (`MaxGenerationTimeMs = 4.0 ms`), not by the
total world size. A cold start loads the player's chunk + neighbours on demand.

### 4.5 Player spawn on load

```
SpawnLocation = FPlayerState.Transform.Location
               clamped to a safe surface if inside a block (caller responsibility)
PlaytimeSeconds += (Now - LastPlayed) on load
```

If `FPlayerState.Transform` is invalid (NaN, inside bedrock, or missing), fall back
to the world spawn point (terrain surface at origin). The vitals system does not
validate spawn safety (see `player-vitals.md` §5).

---

## 5. Edge Cases

| Case | Resolution |
|---|---|
| Player quits during a chunk generation | The chunk is not dirty, so nothing is written; it regenerates on reload |
| Crash mid-autosave | `.tmp` is incomplete and ignored; `.bak` is intact |
| Two worlds with the same seed | Allowed and identical in terrain; `WorldId` keeps them separate |
| Player renames a world | `DisplayName` changes, `WorldId` and folder do not |
| Chunk saved with `ChunkHeight = 64`, world now 192 | `WorldVersion` mismatch → refuse (v1 world) |
| Save references an `ActorId` that no longer exists | Logged, non-fatal; the dependent role goes unfilled |
| Player dies twice before recovering the first corpse | Two corpses, both persistent |
| Player dies in an unloaded chunk | The corpse record is written anyway; the chunk regenerates and the corpse is placed on top |
| Autosave fires while the player is falling | Position is saved as-is; the player reloads mid-fall. Acceptable, and better than teleporting |
| Storage full | Write fails, previous save intact, player is warned once |
| Corrupt `world.sav` and `.bak` | Recovery state (§3.11), never a silent delete |
| `GeneratorVersion` older than the build | World loads, menu warns that the terrain differs from a fresh world |
| A POI was regenerated because its chunk was corrupt | Nest count is preserved from `FPOIInstance`; the player does not get a free cleared POI |
| Player deletes a world from the menu | Explicit confirmation, folder removed |
| Legacy save with no `FItemRef` | Migration: `BlockID` → `FItemRef{Kind=Block, BlockID=BlockID}`; `ItemCount` preserved |
| Main-inventory slot count mismatch on load | Slots beyond the saved count are treated as empty; logged |
| Selected slot index out of range (save data) | Clamped to `[0, 8]` on load; logged as a migration warning |
| Container mined (broken) | Drops its contents as pickups before removal; save record deleted |
| Autosave during a weather transition | `WeatherTransitionAlpha` and `WeatherTimeRemaining` saved mid-transition; resumes correctly |
| `TimeOfDay` saved at 0.999 | Wraps on next tick; `DayCount` increments; no special case needed |
| Empty world (no edits) | `chunks/` directory is empty; `world.sav` holds player + clock only; load is instant |
| Save file from a newer `WorldVersion` | Refused with "newer version" message; no partial load |

---

## 6. Dependencies

| System | Why | Status |
|---|---|---|
| `AWorldGenerator` | Owns chunk files and the seed; `SetBlock` / `SetBlockGenerated` split | Implemented, format v1 |
| `AChunkActor` | Block data, metadata, `bIsModified` flag | Implemented |
| `UQuickSlotsInventorySystem` / `AC_Inventory` | 36 slots to persist (9 quick + 27 main) | Implemented (quick); main via Blueprint |
| `UPlayerVitalSystem` | Health, XP, level, alive state | Implemented |
| `UBaublesSystem` | 5 tool slots + durability | Implemented |
| `UDayNightCycleSystem` | `TimeOfDay`, `DayCount`, weather state; `SaveWeatherState` / `LoadWeatherState` | Implemented (helpers exist) |
| `UMobSpawnerSystem` / `AMobCharacter` | Mob records (type, transform, health, AI state) | Implemented |
| `ABlockItemPickup` | Dropped items persisted as `FWorldItemRecord` | Implemented (blocks only) |
| Container blocks (chest/crate/barrel) | Slot contents persisted | Implemented (blocks) |
| Main menu / world list UI | Load or create a world | **Does not exist** |
| `FPOIInstance` | POI state — future | Not implemented |
| Colony / power / cameras | Phase-3 state — future | Not implemented |
| `FItemRef` / `DT_Items` | Item identity in records | Not implemented (planned) |

Bidirectional note: `world-generation.md` §5 references this document for chunk
format v2, `SetBlock`/`SetBlockGenerated` split, and version gating. `inventory-quickslots.md`,
`player-vitals.md`, `day-night-cycle.md` and `weather-system.md` all reference this
document as their persistence target. Any change to a saved field must be propagated
here and to the relevant struct.

---

## 7. Tuning Knobs

| Knob | Default | Safe range | Effect |
|---|---|---|---|
| `AutosaveInterval` | `300 s` | 60–600 s | Risk vs. hitch; lower = less lost work, more I/O |
| `Compression` | RLE + Zlib | RLE-only / Zlib-only / none | Load/save CPU vs. disk size |
| `BackupCopies` | `1` | 0–3 | Recovery depth vs. disk usage |
| `EntityPersistenceMode` | All | All / Nearby only | Cost of a save; "Nearby only" skips far entities |
| `CorpseLifetime` | `∞` | 0 s – ∞ | Never expires by default (D-SL-04) |
| `WorldVersion` | `2` | — | Format gate; bump on breaking change |
| `GeneratorVersion` | (build-defined) | — | Generation compatibility warning |
| `bManualSaveEnabled` | `false` | bool | v1 ships autosave-only; manual save is a future opt-in |
| `MaxCachedLayouts` | `256` regions | 64–1024 | POI layout cache (not saved) |
| `SaveOnMinimized` | `false` | bool | Whether autosave runs while the game is minimized |
| `RecoveryStateOnCorrupt` | `true` | bool | Fall back to recovery state instead of refusing to load |
| `ChunkCompressionThreshold` | `RawChunkBytes` | — | Store uncompressed if compressed size ≥ this |

All knobs are data-driven (`EditAnywhere` or config). No knob value may be hardcoded
in C++.

---

## 8. Acceptance Criteria

1. Create a world, quit the game, relaunch, load: the world is identical, including
   the player's position, inventory, vitals, XP, selected quick slot, Baubles, time
   of day, weather, and day count.
2. A player-placed block survives a reload; a generated tree does **not** overwrite
   a player-removed block.
3. A generated tree that changes in a build update appears in **unvisited** chunks
   only, and never rewrites a visited one (requires `SetBlock`/`SetBlockGenerated`
   split).
4. Furnace rotation (block metadata) survives a reload.
5. A chunk from a different `ChunkHeight` is rejected, not silently accepted.
6. A truncated `.dat` file is regenerated from the seed without crashing.
7. A corrupt `world.sav` falls back to `.bak`; if both are corrupt, the world opens
   in recovery state with a clear message.
8. Every persistent actor keeps its `ActorId` and its role across a reload.
9. A corpse is still at the death location after a full game restart.
10. An Angel nest stays partially cleared after a reload (once POI system ships).
11. A camera assignment survives a reload (once colony system ships).
12. Container contents survive a reload; a broken container drops its contents as
    pickups before removal.
13. The startup path **never** deletes a save directory.
14. An autosave every 5 minutes does not produce a visible frame hitch on target
    hardware (measured via Unreal Insights).
15. A `v1` world is refused with a clear message and can be deleted from the menu.
16. A world created with a random seed produces different terrain from another world
    with a different seed.
17. `WorldId` is stable across renames; renaming a world changes only `DisplayName`.
18. Autosave writes are atomic: a crash mid-write leaves the previous save intact.
19. An automation test covers: round-trip save/load, corruption fallback, version
    rejection, corpse persistence, and chunk height mismatch.

---

## 9. Build order (implementation plan)

Ordered so that each step is shippable and the risky ones come first.

1. **Split `SetBlock` / `SetBlockGenerated`.** Remove `bIsModified` from generation.
   This alone fixes the "trees baked into saves" bug and is a prerequisite for
   everything else.
2. **Chunk format v2** — header, RLE, Zlib, CRC, sparse metadata. Reject v1.
3. **Remove the startup purge**, add a `world.meta` and a per-world folder.
4. **Main menu**: world list, create (random seed), load, delete.
5. **`world.sav` v1** — header, player state, clock. Save on quit and every 5
   minutes.
6. **Entity records** — mobs first (they have `ActorId`), then dropped items.
7. **Corpses and dropped items.**
8. **Container state** (after container blocks are stable).
9. **POI instances and nest counts** (after `poi-placement.md` step 5).
10. **Colony and power state** (after their systems exist).
11. **Atomic writes and `.bak` recovery.**
12. **Automation tests**: round-trip, corruption, version rejection, corpse
    persistence.

Step 1 can ship immediately and is worth doing before any of the rest.

---

## 10. Open questions

- Does the main menu need a **world preview render** (Minecraft-style thumbnail)?
  It is a nice touch and a real implementation cost on a voxel world.
- Is there a "duplicate world" action? Cheap to implement, but it copies chunk
  files.
- Should autosave also run when the game is **minimized**, or only in focus?
  Minimized autosave can hitch an otherwise idle machine.
- How are **modded or hand-edited** `world.meta` files handled? Probably validate
  and refuse.
- Does the player ever get a manual save? Autosave-only is cleaner but some
  players will want the control (knob `bManualSaveEnabled` reserved).
- `Seed` is `int32`. Is that enough for the "every world is different" promise?
  Yes in practice (2³² seeds), but a `FString` seed phrase would be more
  player-legible.
- Should `world.meta` be JSON in production for debuggability? Current decision:
  binary with a debug JSON dump option. Revisit if tooling needs it.