# GDD — Items & Loot

| Field | Value |
|---|---|
| Status | Draft (2026-11-11) |
| System owner | game-designer / gameplay-programmer / economy-designer |
| Related | `design/gdd/items.md`, `design/gdd/weapons-tools.md`, `design/gdd/mob-zombie.md`, `design/gdd/mob-skeleton.md`, `design/gdd/mob-spider.md`, `design/gdd/mob-creeper.md`, `design/gdd/angels.md`, `design/gdd/save-load.md`, `Docs/BLOCK_REGISTRY.md`, `Docs/MOB_REGISTRY.md`, `Docs/SYSTEMS_REFERENCE.md` §3–4 |
| Source of truth for current code | `BlockItemPickup.h/.cpp`, `QuickSlotsInventorySystem.h/.cpp`, `ToolTypes.h`, `WorldGenerator.cpp` (`BreakBlockAtVoxel`), `MobCharacter.cpp` (`FMobDropEntry`) |

---

## 1. Overview

MinecraftDeWish has **no item registry yet**: every pickup in the world is a `uint8`
block ID. A Zombie drops Dirt, a Spider drops Leaves, and the Spider Eye has no block at
all. This document defines the **items & loot layer** that replaces those block-proxies
with real, stable item identities, without rewriting the block pipeline.

The chosen model (user decision, see `design/gdd/items.md` §1) is **hybrid**:

- **Block items** keep their existing `uint8 BlockID` identity and reuse the working
  `ABlockItemPickup` path (mesh, physics, magnet, merge).
- **Non-block items** (mob loot, consumables, materials, fuel) get a stable `FName ItemId`
  resolved through a `UItemDefinition` DataAsset / `DT_Items` row.
- **Tools** are a third family (`EItemKind::Tool`) whose runtime state lives in
  `FToolInstance` / `UBaublesSystem` and which are **never stacked**.

A unified `FItemRef` discriminator (`Kind`, `BlockID`, `ItemId`, `ToolType`/`ToolTier`)
lets loot tables, recipes and barter name either family with one struct. The pickup
path branches once on `Kind`.

This document is the design source of truth for **item taxonomy, loot drops, stacking,
pickup physics, merge, inventory layout and item entities**. Durability, repair,
crafting and barter are specified in `design/gdd/items.md` and `weapons-tools.md`; this
doc cross-references them rather than duplicating them.

---

## 2. Player Fantasy

> *"Every object I pick up is either a reason to keep going or a reason to go back."*

The loot layer is where the world stops being geometry and starts being **scarcity**.
The player fantasy:

- **A drop tells a story.** A bone on the ground means a Skeleton was here. Gunpowder
  means a Creeper detonated nearby. Loot is a breadcrumb, not a reward screen.
- **Inventory is a decision, not a dump.** 36 slots, stacks of 64, no weight — but a
  full pack means choosing what to leave behind. The horror pacing depends on the player
  never feeling safe enough to hoard everything.
- **Pickup feels physical.** Items arc out of a broken block or a dying mob, tumble,
  settle, then pull toward the player with a satisfying magnet once the cooldown
  expires. The world has weight.
- **Nothing is wasted.** A broken tool is retained and repairable; a partial stack
  merges; a dropped stack can be recovered. Loss comes from death (corpse), not from
  dropped items vanishing.

---

## 3. Detailed Rules

### 3.1 Item taxonomy

Three families, one discriminator:

| Family | Identity | Where it lives | Stackable | Examples |
|---|---|---|---|---|
| **Block item** | `uint8 BlockID` (`EBlockType`) | `FQuickSlotData.BlockID` / `FItemStack.Ref` | Yes (64) | Dirt, Stone, Iron Ore, Torch, Cobblestone |
| **Non-block item** | Stable `FName ItemId` | `FItemStack.Ref` | Yes (per-item max) | Bone, String, Rotten Flesh, Gunpowder, Spider Eye, Medicine, Battery |
| **Tool** | `ToolType` + `ToolTier` (`ToolId`) | `FToolInstance` in `UBaublesSystem` | **No (1)** | Wooden Pickaxe … Obsidian Sword |

```cpp
UENUM(BlueprintType)
enum class EItemKind : uint8
{
    None,
    Block,   // identity = BlockID
    Item,    // identity = ItemId
    Tool     // identity = ToolType + ToolTier (equipped in Baubles, never stacked)
};

USTRUCT(BlueprintType)
struct FItemRef
{
    EItemKind Kind = EItemKind::None;
    uint8     BlockID = 0;      // valid when Kind == Block
    FName     ItemId;           // valid when Kind == Item
    EToolType ToolType = EToolType::None;
    EToolTier ToolTier = EToolTier::None;
};
```

**Why hybrid:** `BlockItemPickup`, `ChunkActor`, `M_Global` and the drop physics are
built around `BlockID`. Replacing them with a unified registry rewrites four working
systems for no gameplay gain. The hybrid costs one `Kind` discriminator and one branch
in the pickup path. Blocks never become "items with quality" — a block is a block.

### 3.2 Item IDs

- Format: `Item.<PascalCaseName>` — `Item.Bone`, `Item.String`, `Item.Medicine`.
- Stored as `FName`, **never renumbered, never renamed** after shipping (save data and
  trade offers reference them; see `ue-data.md`).
- Every item is a `UItemDefinition` DataAsset (or a `DT_Items` row) with: `ItemId`,
  `DisplayName` (`FText`), `Description` (`FText`), `Icon`
  (`TSoftObjectPtr<UTexture2D>`), `MaxStackSize`, `Category`, `Value`, `UseEffect`
  (optional), `PickupMesh` (optional soft ref for non-block world rendering).

### 3.3 Item catalogue v1

Kept deliberately small. Full durability/repair/crafting/barter rules live in
`design/gdd/items.md`.

| Item ID | Category | Source | Use |
|---|---|---|---|
| `Item.Bone` | Material | Skeleton | Trade, bone tools (later) |
| `Item.String` | Material | Spider | Crafting, traps |
| `Item.SpiderEye` | Material | Spider (33 %) | Antidote ingredient |
| `Item.RottenFlesh` | Material | Zombie | Bait, compost; edible only when cooked |
| `Item.Gunpowder` | Material | Creeper | Explosives, ammunition |
| `Item.ScrapMetal` | Material | Police station, wrecks | Repairs, crafting |
| `Item.Electronics` | Material | Hospital, relay | Cameras, monitors, generators |
| `Item.Battery` | Component | Police station, relay | Portable power, camera crafting |
| `Item.CleanWater` | Consumable | Well, bottles | Thirst |
| `Item.CannedFood` | Consumable | POI loot | Hunger |
| `Item.CookedMeat` | Consumable | Campfire | Hunger |
| `Item.Bandage` | Consumable | Crafting (string + cloth) | Heal 10 HP over 5 s |
| `Item.Medicine` | Consumable | Hospital | Heal 40 HP, cure poison |
| `Item.Antidote` | Consumable | Spider eye + water | Cure poison |
| `Item.FuelPellet` | Fuel | Refinery (biofuel) | Generator fuel (`light-and-power.md`) |
| `Item.Blueprint` | Lore | POI offices | Unlocks a recipe; readable |

**Cut on purpose:** no currency, no keys (denial is geometric, not keyed), no ammo
types until a ranged weapon exists, no crafting-tier tokens.

### 3.4 Stacking

- Default `MaxStackSize = 64` (Minecraft convention).
- Per-item override via `UItemDefinition.MaxStackSize`; tools use `1`.
- A stack is identified by `(FItemRef, durability state)`. Two stacks with the same
  `FItemRef` but different remaining durability **do not merge** (see edge cases).
- Block items and non-block items both obey the same stack rules; the only difference
  is identity resolution.
- Stacking is the responsibility of `TryAddItemToPlayerInventory` (§3.7) and of the
  merge pass in `ABlockItemPickup` (§3.6).

### 3.5 Pickup (magnet, cooldown, launch)

`ABlockItemPickup` is the single world representation of a dropped item for **both**
families. Dynamics (SYSTEMS_REFERENCE §4):

- **Visuals:** procedural mini-voxel cube (25 cm) for block items, with texture
  coordinates matching the dropped block's Texture2DArray slice. Non-block items use
  either a shared generic pickup mesh or a billboard icon (open question §10 of
  `items.md`); until resolved they fall back to a 25 cm cube with the item's icon
  sampled as a decal. See §7 tuning.
- **Launch:** `LaunchPickup(const FVector& InVelocity, float InCooldown = 1.2f)`
  launches the pickup with an initial 3D velocity, gravity `1200 cm/s²`, air drag, and
  floor sweep collision.
- **Cooldown:** `PickupCooldown` (default 1.2 s) prevents the player from instantly
  re-picking a dropped item while it is still in mid-air / on the floor next to them.
- **Magnet:** after the cooldown expires, pickups within `MagnetRadius = 250 cm` are
  pulled toward the player. `AXPOrbPickup` uses the same pattern at 3.5 m for XP orbs;
  items use the shorter 2.5 m radius to keep the "arc and settle" feel.
- **Merges:** a pickup merges with nearby pickups of the same identity (§3.6).

Pickup flow on player contact (after cooldown):

1. Resolve `FItemRef` from the pickup.
2. Call `TryAddItemToPlayerInventory(Player, Ref, Count, OutRemaining)`.
3. If `OutRemaining == 0`: destroy the pickup.
4. If `OutRemaining > 0`: reduce the pickup's count to `OutRemaining` (partial fit) and
   leave it on the ground.

### 3.6 Merge

- A pickup merges with another pickup of the **same identity** (`FItemRef` equality)
  when their centres are within `MergeRadius = 100 cm` and both are settled (not
  being launched).
- Merge is attempted on pickup spawn and on a 0.5 s timer while settled.
- Merged count is clamped to `MaxStackSize` per pickup; overflow spawns a new pickup.
- Durability state must match for tools — but tools are never in pickups as stacks (they
  are equipped or in inventory as single instances), so this only matters for block and
  non-block items, which carry no durability.
- Merge never crosses the cooldown boundary: a freshly launched pickup cannot merge
  until it has settled (`bIsLaunched == false`).

### 3.7 Inventory

| Property | Value | Notes |
|---|---|---|
| Quickslots | 9 | Existing `NUM_QUICK_SLOTS`, `UQuickSlotsInventorySystem` |
| Main slots | 27 | New; `AC_Inventory` must expose them |
| **Total** | **36** | Minecraft-classic (user decision) |
| Stack size | 64 default, per-item override | Tools: 1 |
| Weight | None | User decision |
| Drop | `A` (`IA_DropItem`) | Existing, drops hovered or selected stack |
| Pickup order | matching quickslot stack → matching main stack → empty quickslot → empty main | Predictable |

`FQuickSlotData` gains a `FItemRef Ref` alongside the legacy `BlockID`. `BlockID`
remains populated for block items so the existing widget and icon path keep working
during migration.

`TryAddItemToPlayerInventory(AActor* Player, FItemRef Ref, int32 Count,
int32& OutRemainingCount)`:

1. First pass: fill existing stacks of the same `Ref` (quickslots first, then main),
   up to `MaxStackSize`.
2. Second pass: place remainder into the first empty slot (quickslot first, then main).
3. If no slot can accept the remainder, set `OutRemainingCount` to the leftover and
   leave the item on the ground (pickup is **not** consumed).

### 3.8 Drop (`IA_DropItem`)

`DropItemFromInventory(AActor* Player, int32 SlotIndexOverride)` (SYSTEMS_REFERENCE §3):

- Contextual: in gameplay, drops 1 item from the selected quickslot
  (`SelectedID - 1`). In an inventory/container panel, drops from the mouse-hovered
  slot.
- Decrements stack count by 1 (clearing the slot when empty).
- Spawns and launches `ABlockItemPickup` 2.5 blocks forward with realistic 3D launch
  physics (`LaunchPickup` with the forward velocity vector).
- For non-block items, the pickup uses the item's `PickupMesh` (or the generic
  fallback) instead of the block voxel cube.

Dropping a whole stack (shift-drop) is a tuning/UX decision deferred to the
inventory UI spec; v1 drops one unit per press.

### 3.9 Drop tables per mob

Replaces the block-proxy hack recorded in `Docs/MOB_REGISTRY.md`. Loot is rolled on
mob death via `FMobDropEntry` / `FLootTable`.

```cpp
USTRUCT(BlueprintType)
struct FLootEntry
{
    FItemRef Item;
    int32 MinCount = 1;
    int32 MaxCount = 1;
    float Chance = 1.0f;      // 0..1
};

USTRUCT(BlueprintType)
struct FLootTable
{
    TArray<FLootEntry> Guaranteed;  // always dropped
    TArray<FLootEntry> Rolled;      // each rolled independently
    int32 Rolls = 1;               // number of independent rolls over Rolled[]
};
```

| Mob | Guaranteed | Rolled | XP |
|---|---|---|---|
| **Zombie** | `Item.RottenFlesh ×1–2` | `Item.ScrapMetal` 5 % | 5 |
| **Skeleton** | `Item.Bone ×1–2` | `Item.String` 25 % | 5 |
| **Spider** | `Item.String ×1–2` | `Item.SpiderEye` 33 % | 5 |
| **Creeper** | `Item.Gunpowder ×1` | — | 5 |
| **Angel** | **Nothing** | — (not killable until the Source dies; see `angels.md` §3.10) | 0 (until Source dies) |

Rules:

- `Guaranteed` entries are always spawned (count rolled uniformly in
  `[MinCount, MaxCount]`).
- Each entry in `Rolled` is evaluated independently with its `Chance`; on success a
  count is rolled in `[MinCount, MaxCount]`.
- `Rolls` repeats the whole `Rolled[]` pass that many times (default 1). Use `Rolls > 1`
  for bosses, not for classic mobs.
- Loot rolls take an **explicit seed** so automation tests can assert exact drops
  (`LootRNG = FRandomStream(Seed)`).
- Drops are spawned at the mob's death location as `ABlockItemPickup` instances with
  a small outward launch velocity so they do not stack perfectly on the corpse.
- Creeper self-detonation drops 5 XP and **no item loot** (the gunpowder is consumed
  by the explosion). A Creeper killed by the player (not self-detonated) drops
  `Item.Gunpowder ×1`.

**Determinism contract:** given the same seed, mob type and death position, the set of
spawned pickups (identity + count + launch velocity direction) is identical. Launch
*speed magnitude* may vary within a documented range; only identity/count/seeded
directions are asserted by tests.

### 3.10 Block drops on mining

- `AWorldGenerator::BreakBlockAtVoxel(VoxelX, VoxelY, VoxelZ, OutDroppedBlockID)`
  destroys the block, updates meshes, spawns Chaos debris, spawns a pickup, and
  consumes Baubles tool durability.
- The dropped item is **the block itself** (`FItemRef{Kind=Block, BlockID=brokenBlock}`),
  count 1, for every mineable block. The one exception is **ores**, which drop their
  raw material item instead of the ore block:

| Broken block | Dropped item |
|---|---|
| Stone / Cobblestone / Dirt / Grass / Sand / Wood Log / Planks / Glass / Fence / Door / Torch / Barrel / Crafting Table / Furnace | The block itself (`Kind=Block`) |
| Coal Ore | `Item.Coal`? — **not in v1 catalogue; coal ore drops itself as a block** (deferred until fuel item exists) |
| Iron Ore | Iron Ore block (smelts to iron ingot; ingot is a future item) |
| Gold Ore | Gold Ore block |
| Diamond Ore | Diamond Ore block |
| Emerald Ore | Emerald Ore block |
| Leaves | Leaves block (or `Item.String` at 5 %? deferred — see open questions) |
| Bedrock | Nothing (indestructible) |

- Mining XP is granted on successful break: `XPPerBlock = 1` base, `XPOreMultiplier = 2`
  for ore blocks (see `weapons-tools.md` §4.5).
- The pickup is spawned at the block centre with a small upward+outward launch so it
  arcs onto the floor rather than clipping into the voxel below.

### 3.11 Item entities (`ABlockItemPickup`)

The single actor class for all dropped items. Full spec in SYSTEMS_REFERENCE §4.

- **Components:** procedural mesh (25 cm cube), movement via manual integration (not
  Chaos), floor sweep collision.
- **Properties:**
  - `BlockID` (legacy, populated for block items)
  - `ItemRef` (`FItemRef`, new — populated for both families)
  - `Count` (int32)
  - `PickupCooldown` (float, default 1.2 s)
  - `bIsLaunched` (bool)
  - `LaunchVelocity` (FVector)
- **Lifecycle:** spawned → launched → tumbles with gravity/drag → settles → cooldown
  expires → magnet pull toward player → on contact, `TryAddItemToPlayerInventory` →
  merge attempt → destroy if fully picked up.
- **Replication (if multiplayer):** `Count` and `ItemRef` replicated; physics is
  client-predicted cosmetic, server-authoritative pickup.

### 3.12 Non-block pickup rendering

Until the open question in `items.md` §10 is resolved, non-block pickups use a **shared
generic pickup mesh** (a 25 cm cube with the item's icon applied as a material
parameter) so they are visually distinct from block voxels without requiring a unique
mesh per item. Block items keep the procedural voxel cube. This is a tuning/asset
decision, not a gameplay rule; either path satisfies the acceptance criteria.

---

## 4. Formulas

### 4.1 Drop counts

```
GuaranteedCount = RandRange(MinCount, MaxCount)   // uniform integer
RolledCount     = RandRange(MinCount, MaxCount) if Rand() < Chance else 0
TotalDropped    = Σ GuaranteedCount + Σ RolledCount   (per entry, per roll)
```

RNG: `FRandomStream LootRNG(Seed)`; advance order is Guaranteed[] then Rolled[],
entry by entry, so a fixed seed yields a reproducible drop set.

### 4.2 Stack size

```
MaxStackSize(Item) = UItemDefinition.MaxStackSize  (default 64)
                   = 1 for tools (EItemKind::Tool)
EffectiveFit(CurrentStack, Incoming) = min(Incoming, MaxStackSize - CurrentStack.Count)
```

### 4.3 Pickup physics

```
Gravity             = 1200 cm/s²   (downward, Z-)
LaunchSpeedForward  = 250 cm/s     (initial, along player look + slight up)
LaunchSpeedUp       = 150 cm/s     (initial vertical)
AirDrag             = 0.98 per 0.1 s  (multiplicative)
FloorRestitution    = 0.3           (bounce on first contact, then settle)
SettleThreshold     = 5 cm/s        (velocity below this → bIsLaunched = false)
```

Example: a dropped item launched forward 2.5 blocks (250 cm) at 250 cm/s forward +
150 cm/s up travels ~1.0 s before first floor contact, bounces once, and settles
within ~1.4 s — comfortably inside the 1.2 s cooldown window so it cannot be
re-picked mid-arc.

### 4.4 Magnet

```
MagnetRadius        = 250 cm
MagnetAccel         = 800 cm/s²    (toward player, capped at MagnetMaxSpeed)
MagnetMaxSpeed      = 600 cm/s
Magnet activates     = PickupCooldown expired AND bIsLaunched == false
```

### 4.5 Merge

```
MergeRadius         = 100 cm
MergeCheckInterval  = 0.5 s
Merge allowed iff   SameIdentity(A, B) AND !A.bIsLaunched AND !B.bIsLaunched
                    AND Distance(A, B) <= MergeRadius
MergedCount         = min(A.Count + B.Count, MaxStackSize)
Overflow            = (A.Count + B.Count) - MergedCount   → spawns new pickup
```

### 4.6 Inventory pickup order

```
Pass 1 (fill):  for each slot in [quickslots 0..8, main 0..26]:
                    if slot.Ref == Incoming.Ref and slot.Count < MaxStackSize:
                        take = min(Incoming, MaxStackSize - slot.Count)
                        slot.Count += take; Incoming -= take
Pass 2 (empty): for each slot in [quickslots 0..8, main 0..26]:
                    if slot is empty:
                        slot.Ref = Incoming.Ref; slot.Count = min(Incoming, MaxStackSize)
                        Incoming -= slot.Count
                        if Incoming == 0: break
OutRemainingCount = Incoming
```

### 4.7 XP values

```
XPPerMobKill        = 5   (Zombie, Skeleton, Spider, Creeper)
XPPerBlock          = 1   (base)
XPOreMultiplier     = 2   (ore blocks)
XPPerLevel          = 7 + (Level × 3)   (UPlayerVitalSystem)
```

### 4.8 Drop launch (mob death)

```
DeathDropSpeed      = 150 cm/s   (outward radial from mob centre, +50 cm/s up)
DeathDropSpread     = ±30°       (randomised azimuth per pickup)
```

---

## 5. Edge Cases

| Case | Resolution |
|---|---|
| Inventory full on pickup | Item stays on the ground; the pickup is **not** consumed; `OutRemainingCount > 0` |
| Partial stack fit | What fits is taken; the remainder stays as a pickup with reduced `Count` |
| Two damaged tools of the same type | Both remain separate stacks; no auto-merge (durability differs) |
| Dropping a tool | Tools are not in the general inventory as stacks; dropping a tool unequips it into the Baubles slot's "broken/unequipped" state, not into a pickup. **Open question** whether an unequipped tool can be dropped as a pickup — deferred to `weapons-tools.md` |
| Merge of a launched and a settled pickup | Not allowed; merge waits until both settle |
| Merge overflow | Overflow spawns a new pickup at the same location; counts never exceed `MaxStackSize` |
| Pickup cooldown vs magnet | Magnet never activates before `PickupCooldown` expires, even if settled earlier |
| Player dies near pickups | Pickups are unaffected; the corpse is a separate `FWorldItemRecord` |
| Two players (multiplayer) racing a pickup | Server-authoritative: first valid pickup RPC wins; the other gets `OutRemainingCount = Count` and the pickup stays |
| Loot roll with seed 0 | Valid seed; `FRandomStream(0)` is a legal stream, not "unseeded" |
| Chance > 1.0 in a `Rolled` entry | Clamped to 1.0 at load; validation test warns |
| Guaranteed entry with `MinCount > MaxCount` | Validation fails at cook; entry is skipped at runtime |
| Non-block item with no `PickupMesh` | Falls back to the generic 25 cm cube with icon decal (§3.12) |
| Block item pickup for an air block (ID 0) | Never spawned; `BreakBlockAtVoxel` on air is a no-op |
| Creeper self-detonation | Drops 5 XP, **no** item loot (gunpowder consumed) |
| Angel death | No loot until the Source dies; see `angels.md` |
| Save from before the item system | Version field + migration: legacy `BlockID` → `FItemRef{Kind=Block}` |

---

## 6. Dependencies

| System | Why | Status |
|---|---|---|
| `ABlockItemPickup` | Pickup path, physics, cooldown, magnet, merge | Implemented for blocks only |
| `UQuickSlotsInventorySystem` | Storage, stacking, widget refresh, drop | Implemented, 9 slots, block-only |
| `AC_Inventory` (Blueprint) | 27 main slots | Exists, undocumented in C++ |
| `FToolInstance` / `UBaublesSystem` | Tools are a non-stacked family | Implemented |
| `UCraftingSmeltingSystem` | Recipes consume items | Implemented, block-only, no ingredients |
| `AMobCharacter` / `UMobSpawnerSystem` | Loot drops on death | Implemented with block proxies |
| `AWorldGenerator::BreakBlockAtVoxel` | Block drops on mining | Implemented |
| `UPlayerVitalSystem` | XP deposit on pickup? (no — XP on kill/break) | Implemented |
| `AXPOrbPickup` | Reference pattern for magnet | Implemented |
| Save/load (`FWorldItemRecord`) | Dropped items and corpses persist | Not implemented |
| NPC colony / barter | Consumes items | Not implemented |
| Light & power | Fuel items, batteries | Not implemented |

---

## 7. Tuning Knobs

| Knob | Default | Safe range | Effect |
|---|---|---|---|
| `MaxStackSize` | 64 | 16–256 | Inventory pressure |
| `PickupCooldown` | 1.2 s | 0.5–2.0 s | Prevents instant re-pickup |
| `MagnetRadius` | 250 cm | 150–400 cm | Pickup reach |
| `MagnetAccel` / `MagnetMaxSpeed` | 800 / 600 cm/s² | ±30 % | Pull feel |
| `MergeRadius` | 100 cm | 50–200 cm | Stack consolidation |
| `MergeCheckInterval` | 0.5 s | 0.25–1.0 s | Merge CPU cost |
| `LaunchSpeedForward` / `LaunchSpeedUp` | 250 / 150 cm/s | ±25 % | Drop arc |
| `Gravity` | 1200 cm/s² | 800–1600 cm/s² | Drop weight |
| `AirDrag` | 0.98 / 0.1 s | 0.95–0.99 | Arc length |
| `FloorRestitution` | 0.3 | 0.1–0.5 | Bounce |
| `DeathDropSpeed` / `Spread` | 150 cm/s / ±30° | ±25 % | Loot scatter |
| `LootChance` per entry | per table | 0–1 | Economy faucets |
| `XPPerMobKill` / `XPPerBlock` / `XPOreMultiplier` | 5 / 1 / 2 | 0–10 | Progression faucet |
| `QuickSlots` / `MainSlots` | 9 / 27 | fixed | Inventory size |

---

## 8. Acceptance Criteria

1. A Skeleton drops `Item.Bone` (1–2), not Dirt.
2. A Spider drops `Item.String` (1–2) and can drop `Item.SpiderEye` (33 %).
3. A Zombie drops `Item.RottenFlesh` (1–2) and can drop `Item.ScrapMetal` (5 %).
4. A Creeper killed by the player drops `Item.Gunpowder ×1`; a self-detonated Creeper drops none.
5. An Angel drops nothing, ever, until the Source dies.
6. A block pickup still places a block when selected; a non-block item cannot be placed.
7. Inventory holds 9 quick + 27 main = 36 slots, stacks of 64.
8. A dropped item arcs, settles, and cannot be re-picked for 1.2 s.
9. After cooldown, a pickup within 250 cm is pulled toward the player and picked up on contact.
10. Two pickups of the same identity within 100 cm merge into one stack (capped at 64).
11. A full inventory leaves the item on the ground with a reduced count (partial fit).
12. Loot rolls are deterministic with a fixed seed (identity + count reproducible).
13. Every `FItemRef` in every DataTable resolves at cook time; a dangling ID fails the build.
14. A save from the legacy format loads with every block pickup mapped to `Kind = Block`.
15. `TryAddItemToPlayerInventory` follows quickslot-first then main-slot order.

---

## 9. Build order (implementation plan)

1. **`FItemRef`, `FItemStack`, `EItemKind`** + validation helpers + tests.
2. **`UItemDefinition`** + `DT_Items` with the v1 catalogue.
3. **Inventory migration** — `FQuickSlotData` gains `Ref`; legacy `BlockID` stays populated; add 27 main slots.
4. **Pickup path** — `ABlockItemPickup` gains `ItemRef` and `Count`; non-block items use generic mesh.
5. **Loot tables** replacing the proxies in `AMobCharacter` (`FMobDropEntry` → `FLootTable`).
6. **Block drops** — `BreakBlockAtVoxel` spawns the correct `FItemRef` (block or future item).
7. **Merge + magnet** tuning and tests.
8. **Save/load** — `FWorldItemRecord` for dropped items and corpses.
9. **Durability/repair/crafting/barter** — see `design/gdd/items.md` steps 6–9.

Steps 1–5 are independent of the Angel system and can ship first.

---

## 10. Open questions

- Does the 27-slot main inventory live in C++ (like the quickslots) or stay in `AC_Inventory`?
  The current split is undocumented and will bite during migration.
- Are tools carried as items before being equipped, or crafted directly into a Baubles slot?
- How are non-block pickups rendered in the world — a mesh, a billboard, or a shared
  generic pickup icon? (§3.12 proposes the generic cube.)
- Does `Leaves` drop `Item.String` at a low chance, or stay a block drop?
- Does coal/iron/gold/diamond ore drop the ore block or a raw material item? v1 keeps
  the ore block; raw ingots are deferred until smelting produces them.
- Does food spoil? Spoilage adds pressure to exploration but is another timer to explain.
- Should dropping a tool be possible (as a pickup) or is unequip-to-Baubles the only path?