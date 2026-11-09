# GDD — Save & Load

| Field | Value |
|---|---|
| Status | Draft (2026-11-09) — **no real persistence exists** |
| System owner | gameplay-programmer / lead-programmer |
| Related | `design/gdd/poi-placement.md` §3.9, `design/gdd/npc-colony.md`, `design/gdd/items.md`, `design/gdd/light-and-power.md`, `design/gdd/angels.md` |
| Source of truth for current code | `WorldGenerator.cpp:89-97, 1393-1470`, `ChunkActor.h:56-81` |

---

## 1. Overview

**The game cannot save.** `AWorldGenerator::BeginPlay` computes
`Saved/SaveGames/World_<Seed>` and then **deletes that directory recursively**
(`WorldGenerator.cpp:92-96`), deliberately, to purge chunks generated with older terrain
parameters. What survives a session is therefore nothing.

On top of that, the chunk format has **no version, no height and no seed** — magic `VOXL`,
an `int32` voxel count, and the raw array — and `LoadChunkDelta` accepts it if the count
matches the current array size. No player, inventory, time-of-day, mob, POI or colony state is
written at all.

This document defines the target: a **main-menu world list** (Minecraft model), one folder per
world, a versioned chunk format with compression, a versioned world-state file, and a clear
incompatibility policy for the 64 → 192-block change.

---

## 2. Player Fantasy

> *"I can close the game. The base is still there. The lamps are still on."*

Persistence is what makes the whole design real. The three-phase progression only has meaning
if the fortress the player built on Tuesday is still standing on Thursday. The emotional
contract:

- **Continuity.** The world is the player's work, not a session.
- **Consequence.** A colonist who died last session is still dead.
- **Safety.** A crash does not cost the last hour.

The failure mode to avoid is the one that exists today: a game where nothing persists teaches
the player that nothing they do matters.

---

## 3. Detailed Rules

### 3.1 What is saved today (honest inventory)

| Data | Saved? | Where | Notes |
|---|---|---|---|
| Player-edited blocks | Yes | `Saved/SaveGames/World_<Seed>/Chunk_X_Y.dat` | Full raw array |
| Generated blocks (trees, villages) | Yes, wrongly | Same | `SetBlock` sets `bIsModified` |
| Block metadata (furnace rotation) | **No** | — | `BlockMetadata` is never serialized |
| Player position, vitals, XP, inventory, Baubles | **No** | — | — |
| Time of day | **No** | — | — |
| Mobs, Angels, colonists | **No** | — | — |
| POI instances, nests, containers | **No** | — | Does not exist |
| Circuits, fuel, camera assignments | **No** | — | Does not exist |
| **Anything at all** | **No** | — | The folder is **deleted on startup** (`WorldGenerator.cpp:92-96`) |

### 3.2 World model: main menu, one folder per world

User decision: **Minecraft model.** The game boots to a main menu; the player either loads an
existing world or creates a new one **with a random seed**, so every world is different.

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
```

`world.meta` fields:

| Field | Type | Why |
|---|---|---|
| `WorldId` | `FGuid` | Stable folder identity, independent of the display name |
| `DisplayName` | `FString` (localized input) | Player-facing, renameable |
| `Seed` | `int32` | Must be known **before** `Noise.SetSeed` |
| `WorldVersion` | `uint16` | Format version of `world.sav` and chunks |
| `GeneratorVersion` | `uint16` | Bumped whenever generation output changes |
| `CreatedAt` / `LastPlayed` | `FDateTime` (UTC) | Menu sorting |
| `PlaytimeSeconds` | `int64` | Honest stat, and a debugging aid |
| `bCreative` / difficulty flags | bitset | Menu display |

**`WorldSeed` stops being `1337`.** The current default (`WorldGenerator.h:48`) is replaced by
a value read from `world.meta`, generated with `FMath::RandRange` seeded from
`FPlatformTime::Cycles()` when the player creates a world.

**The startup purge is deleted.** `DeleteDirectoryRecursively` disappears; the only code path
that erases a world is an explicit, confirmed "delete world" action in the menu.

### 3.3 Chunk format v2

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

Why RLE: a voxel chunk is almost entirely long runs (air, stone, dirt). A raw 49 152-byte
chunk compresses to a few hundred bytes, which makes per-5-minute autosave of dozens of dirty
chunks cheap enough to be invisible.

**Header validation is mandatory on load.** `Magic`, `WorldVersion`, `ChunkHeight` and `Crc32`
must all match. Today only `TotalVoxels == BlockData.Num()` is checked
(`WorldGenerator.cpp:1452`), which silently accepts a chunk from a different world height as
long as the count coincides.

### 3.4 Generated blocks must not be saved

This is a required fix, not a preference.

Today `FVoxelStructureGenerator` writes trees and villages with `Chunk->SetBlock()`, which
sets `bIsModified = true` (`ChunkActor.cpp:85`). Consequences:

1. Every visited chunk is persisted, so a save grows with exploration instead of with edits.
2. **A change to tree or village generation never reaches already-visited areas** — the saved
   array overwrites the new output.
3. The POI system (`poi-placement.md`) depends on deterministic stamping; baked geometry breaks
   that guarantee.

Rule:

```
SetBlockGenerated(X, Y, Z, Id)   → writes BlockData, does NOT set bIsModified
SetBlock(X, Y, Z, Id)            → writes BlockData, sets bIsModified   (player edit)
```

Terrain, trees, POIs and any other generated content use the `Generated` variant. Only the
player, explosions and Angel breaching use the editing variant.

### 3.5 World state file (`world.sav`)

```
FWorldSave
├── FSaveHeader          WorldVersion, GeneratorVersion, Seed, SavedAt, PlaytimeSeconds
├── FPlayerState         Transform, Rotation, Health, Hunger, Thirst, XP, Level,
│                        MainInventory[27], QuickSlots[9], Baubles, EquippedTools[..]
├── FWorldClock          TimeOfDay, DayNumber, bSourceKilled
├── FPOIState[]          POI instance records (poi-placement.md §3.9)
├── FEntityRecord[]      every persistent actor (§3.6)
├── FWorldItemRecord[]   dropped items and death corpses (§3.7)
├── FColonyState         colonists, roles, needs, camera assignments, stockpiles
└── FPowerState          circuits, generators, fuel, batteries, lamp schedules
```

`FPlayerState` must be written **even if the player is mid-air or dead**; the death case saves
the corpse (§3.7) and the respawn point.

### 3.6 Entity persistence

User decision: **all entities persist.**

| Entity class | Persisted | Record |
|---|---|---|
| **Colonist** | Full | `ActorId` (`FGuid`), transform, health, hunger, thirst, nerve, background, role, camera bank, shift |
| **Angel** | Full | `ActorId`, transform, state, `ObservationSeconds`, nest region, `bFromNest` |
| **Classic mob** | Full | `ActorId`, `EMobType`, transform, health, AI state |
| **Dropped item / corpse** | Full | Item ref, count, durability, transform, `bIsCorpse` |
| **Projectile / debris** | **No** | Transient; destroyed on save |

Rules:

- Entities outside the streamed area are stored **as records only** and respawned when their
  chunk streams in. Simulation of unloaded entities is a *colony tick* abstraction
  (`npc-colony.md` §10), not per-actor simulation.
- `ActorId` is assigned at spawn from the world RNG and is stable across sessions. It is what
  makes camera assignments and colonist roles resolvable after a reload.
- An `ActorId` that references a missing entity is a **logged, non-fatal** condition: the
  camera bank goes unstaffed rather than crashing.

### 3.7 Death and corpses

Death follows the Minecraft convention (`game-concept.md` §5.4): respawn, inventory dropped.

User decision: **the corpse persists.** It is a normal `FWorldItemRecord` with
`bIsCorpse = true`:

- It survives quit, crash and reload.
- It is **not** despawned by distance or by time. A player who dies in the Horror Forest can
  come back a week later.
- A second death while a corpse exists leaves **two** corpses. No merging, no auto-return.
- Corpses are visible as a container-like pile; retrieving one removes it.

### 3.8 Save triggers

User decision: quit-to-menu, quit-the-game, and every 5 minutes.

| Trigger | What is written |
|---|---|
| **Autosave, every `300 s`** | Dirty chunks + `world.sav` + `world.meta` |
| **Quit to main menu** | Full save, then unload |
| **Quit the game** (`EndPlay`) | Full save |
| **World creation** | `world.meta` + empty `world.sav`, then first spawn |
| **Manual save** | Not exposed in v1 (autosave is the model) |

The 5-minute budget: at the design render distance of 6 chunks (169 chunks) and 49 152 bytes
raw each, the worst case is ~8 MB uncompressed — trivial after RLE. The cost that matters is
**the hitch**, so the write is done on a background task and the game thread only hands over a
copy of the dirty data.

### 3.9 Atomic writes and corruption

Every write is atomic:

1. serialize to `<path>.tmp`,
2. flush and close,
3. rename the existing file to `<path>.bak`,
4. rename `.tmp` to the real path.

On load:

- **CRC mismatch** → the chunk is discarded and regenerated from the seed. The POI instance
  record is kept, so nest counts and discoveries do not reset.
- **`world.sav` corrupt** → fall back to `world.sav.bak`; if that also fails, the world opens
  in a **recovery state**: chunks load, player state resets to a safe spawn, and the player is
  told explicitly. Never silently delete the world.
- **`WorldVersion` mismatch** → refuse to load with a clear message (§3.10).

### 3.10 Versioning and the 64 → 192 transition

`WorldVersion = 2` is the target format. Worlds written before it have no version field, so
they are **`WorldVersion = 1` by definition**.

User decision: **old worlds are declared incompatible and deleted.** No migration code.

Menu behaviour for a `v1` folder:

```
"This world was created in an earlier version and cannot be opened.
 [Delete]   [Cancel]"
```

The `WorldVersion` field exists anyway, because the next generation change (new biomes, new POI
types, `GeneratorVersion` bump) will need exactly the same mechanism, and because silently
regenerating chunks under a player's base is worse than refusing to load.

**`GeneratorVersion` is separate from `WorldVersion`.** Bumping it means "the same format, but
the generated world would differ". A world with an older `GeneratorVersion` still loads, and
its saved chunks stay frozen — but the menu warns that the world will differ from a freshly
generated one.

---

## 4. Formulas

| Formula | Value | Rationale |
|---|---|---|
| `AutosaveInterval` | `300 s` | At most 5 minutes lost |
| `MaxRenderChunks` | `(2 × 6 + 1)² = 169` | At `RenderDistance = 6` |
| `RawChunkBytes` | `16 × 16 × 192 = 49 152` | `ChunkHeight = 192` |
| `RawWorldBytes` | `169 × 49 152 ≈ 8.3 MB` | Worst case before compression |
| `RLEHeaderBytes` | `5` per run | `uint8` id + `uint16` count + alignment |
| `BackupCopies` | `1` per file | `.bak` |
| `ChunkHeaderBytes` | `32` | Fixed, before compression |
| `WorldVersion` (target) | `2` | `1` = legacy, implicit |

Sanity check on the autosave hitch:

```
DirtyChunksPerSave   ≈ player edits in 5 minutes (order of 10)
WriteBytes           ≈ 10 × (few hundred B) + world.sav (few dozen KB)
```

That is small enough that the background write is effectively invisible.

---

## 5. Edge Cases

| Case | Resolution |
|---|---|
| Player quits during a chunk generation | The chunk is not dirty, so nothing is written; it regenerates on reload |
| Crash mid-autosave | `.tmp` is incomplete and ignored; `.bak` is intact |
| Two worlds with the same seed | Allowed and identical in terrain; `WorldId` keeps them separate |
| Player renames a world | `DisplayName` changes, `WorldId` and folder do not |
| Chunk saved with `ChunkHeight = 64`, world now 192 | `WorldVersion` mismatch → refuse |
| Save references an `ActorId` that no longer exists | Logged, non-fatal; the dependent role goes unfilled |
| Player dies twice before recovering the first corpse | Two corpses, both persistent |
| Player dies in an unloaded chunk | The corpse record is written anyway; the chunk regenerates and the corpse is placed on top |
| Autosave fires while the player is falling | Position is saved as-is; the player reloads mid-fall. Acceptable, and better than teleporting |
| Storage full | Write fails, previous save intact, player is warned once |
| Corrupt `world.sav` and `.bak` | Recovery state (§3.9), never a silent delete |
| `GeneratorVersion` older than the build | World loads, menu warns that the terrain differs from a fresh world |
| A POI was regenerated because its chunk was corrupt | Nest count is preserved from `FPOIInstance`; the player does not get a free cleared POI |
| Player deletes a world from the menu | Explicit confirmation, folder removed, no recycle bin interaction |

---

## 6. Dependencies

| System | Why | Status |
|---|---|---|
| `AWorldGenerator` | Owns chunk files and the seed | Implemented, format v1 |
| `AChunkActor::SetBlock` | Must be split into generated/edit variants | Implemented, flags everything |
| `BlockMetadata` | Furnace rotation | Implemented, never saved |
| Main menu / world list UI | Load or create a world | **Does not exist** |
| `FPOIInstance` | POI state | Not implemented |
| Player state (inventory, vitals, Baubles, tools) | The player's progress | Partially implemented, never saved |
| Colony, power, cameras | Phase-3 state | Not implemented |
| `FLootTable` / `FItemRef` | Item records in the save | Not implemented |
| Day/night clock | `TimeOfDay`, `DayNumber` | Implemented, never saved |

---

## 7. Tuning Knobs

| Knob | Default | Effect |
|---|---|---|
| `AutosaveInterval` | `300 s` | Risk vs. hitch |
| `Compression` | RLE + Zlib | Load/save CPU vs. disk |
| `BackupCopies` | `1` | Recovery depth |
| `MaxCachedLayouts` | `256` regions | POI layout cache (not saved) |
| `EntityPersistenceMode` | All | Cost of a save |
| `CorpseLifetime` | `∞` | Never expires |
| `WorldVersion` | `2` | Format gate |

---

## 8. Acceptance criteria

1. Create a world, quit the game, relaunch, load: the world is identical, including the
   player's position, inventory, vitals, XP and the time of day.
2. A player-placed block survives a reload; a generated tree does **not** overwrite a
   player-removed block.
3. A generated tree that changes in a build update appears in **unvisited** chunks only, and
   never rewrites a visited one.
4. Furnace rotation survives a reload.
5. A chunk from a different `ChunkHeight` is rejected, not silently accepted.
6. A truncated `.dat` file is regenerated from the seed without crashing.
7. A corrupt `world.sav` falls back to `.bak`.
8. Every persistent actor keeps its `ActorId` and its role across a reload.
9. A corpse is still at the death location after a full game restart.
10. An Angel nest stays partially cleared after a reload.
11. A camera assignment survives a reload.
12. The startup path **never** deletes a save directory.
13. An autosave every 5 minutes does not produce a visible frame hitch on the target hardware.
14. A `v1` world is refused with a clear message and can be deleted from the menu.

---

## 9. Build order (implementation plan)

Ordered so that each step is shippable and the risky ones come first.

1. **Split `SetBlock` / `SetBlockGenerated`.** Remove `bIsModified` from generation. This alone
   fixes the "trees baked into saves" bug and is a prerequisite for everything else.
2. **Chunk format v2** — header, RLE, Zlib, CRC, sparse metadata. Reject v1.
3. **Remove the startup purge**, add a `world.meta` and a per-world folder.
4. **Main menu**: world list, create (random seed), load, delete.
5. **`world.sav` v1** — header, player state, clock. Save on quit and every 5 minutes.
6. **Entity records** — colonists and Angels first (they have `ActorId`), then classic mobs.
7. **Corpses and dropped items.**
8. **POI instances and nest counts** (after `poi-placement.md` step 5).
9. **Colony and power state** (after their systems exist).
10. **Atomic writes and `.bak` recovery.**
11. **Automation tests**: round-trip, corruption, version rejection, corpse persistence.

Step 1 can ship immediately and is worth doing before any of the rest.

---

## 10. Open questions

- Does the main menu need a **world preview render** (Minecraft-style thumbnail)? It is a nice
  touch and a real implementation cost on a voxel world.
- Is there a "duplicate world" action? Cheap to implement, but it copies chunk files.
- Should autosave also run when the game is **minimized**, or only in focus? Minimized
  autosave can hitch an otherwise idle machine.
- How are **modded or hand-edited** `world.meta` files handled? Probably validate and refuse.
- Does the player ever get a manual save? Autosave-only is cleaner but some players will want
  the control.
- `Seed` is `int32`. Is that enough for the "every world is different" promise? Yes in
  practice (2³² seeds), but a `FString` seed phrase would be more player-legible.
