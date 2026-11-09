# GDD — Items, Inventory & Durability

| Field | Value |
|---|---|
| Status | Draft (2026-11-09) — **not implemented** |
| System owner | game-designer / gameplay-programmer |
| Related | `design/gdd/light-and-power.md`, `design/gdd/npc-colony.md`, `Docs/TOOL_REGISTRY.md`, `Docs/BLOCK_REGISTRY.md` |
| Source of truth for current code | `QuickSlotsInventorySystem.h`, `ToolTypes.h`, `CraftingSmeltingSystem.h`, `BlockItemPickup.h` |

---

## 1. Overview

The game has **no item system**. Every pickup is a `uint8` block ID, so a mob drops Dirt when
it should drop a bone, a spider drops Leaves when it should drop string, and NPC trade has
nothing to trade. The inventory is a 9-slot quickslot model (`FQuickSlotData`, `BlockID`,
`ItemCount`, `MaxStackSize = 64`), and tool crafting is a **tier gate with no material
cost** (`CanCraftTool` only compares tiers).

This document defines the smallest item layer that lets loot, trade, medicine, fuel and
crafting work, **without rewriting the block pipeline**.

**Chosen model (user decision):** blocks stay blocks; non-placeable items get their own
registry. Durability and repair are real. The inventory is the classic 36 slots. Trade is
pure barter — there is no currency.

---

## 2. Player Fantasy

> *"Everything I carry is either a tool, a material, or a reason to go back out there."*

The item layer is where the game stops being a tech demo and starts having an **economy of
scarcity**. Every object must answer one of three questions:

- *Does it keep the dark away?* (fuel, batteries, torches)
- *Does it keep me alive?* (food, water, medicine, bandages)
- *Does it make the next trip possible?* (tools, materials, trade goods)

Anything that answers none of them is cut. The inventory is small, so a full pack is a
decision, not a dump.

---

## 3. Detailed Rules

### 3.1 Two item families

| Family | Identity | Where it lives | Examples |
|---|---|---|---|
| **Block item** | `uint8 BlockID` (existing `EBlockType`) | `FQuickSlotData.BlockID` | Dirt, Stone, Iron Ore, Torch |
| **Non-block item** | Stable `FName ItemId` | New `FItemStack` field | Bone, String, Medicine, Water, Battery |

A unified reference is required so loot tables, recipes and barter can name either:

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

**Why hybrid:** the block pipeline (`BlockItemPickup`, `ChunkActor`, `M_Global`, the
pickup mesh, the drop physics) is already built around `BlockID`. Replacing it with a unified
item registry would rewrite four working systems for no gameplay gain. The hybrid costs one
`Kind` discriminator and one `if` in the pickup path.

**Non-goal:** blocks never become "items with a quality". A block is a block.

### 3.2 Item IDs

- Format: `Item.<PascalCaseName>` — `Item.Bone`, `Item.String`, `Item.Medicine`.
- Stored as `FName`, **never renumbered**, never renamed after shipping: save data and trade
  offers reference them (`ue-data.md`).
- Every item is a `UItemDefinition` DataAsset (or a `DT_Items` row) with: `ItemId`,
  `DisplayName` (`FText`), `Description` (`FText`), `Icon` (`TSoftObjectPtr<UTexture2D>`),
  `MaxStackSize`, `Category`, `Value`, `UseEffect` (optional).

### 3.3 Item catalogue v1

Kept deliberately small. Categories in bold.

| Item ID | Category | Source | Use |
|---|---|---|---|
| `Item.Bone` | **Material** | Skeleton | Trade, bone tools (later) |
| `Item.String` | **Material** | Spider | Crafting, traps |
| `Item.SpiderEye` | **Material** | Spider (33 %) | Antidote ingredient |
| `Item.RottenFlesh` | **Material** | Zombie | Bait, compost; edible only when cooked |
| `Item.Gunpowder` | **Material** | Creeper | Explosives, ammunition |
| `Item.ScrapMetal` | **Material** | Police station, wrecks | Repairs, crafting |
| `Item.Electronics` | **Material** | Hospital, relay | Cameras, monitors, generators |
| `Item.Battery` | **Component** | Police station, relay | Portable power, camera crafting |
| `Item.CleanWater` | **Consumable** | Well, bottles | Thirst |
| `Item.CannedFood` | **Consumable** | POI loot | Hunger |
| `Item.CookedMeat` | **Consumable** | Campfire | Hunger |
| `Item.Bandage` | **Consumable** | Crafting (string + cloth) | Heal 10 HP over 5 s |
| `Item.Medicine` | **Consumable** | Hospital | Heal 40 HP, cure poison |
| `Item.Antidote` | **Consumable** | Spider eye + water | Cure poison |
| `Item.FuelPellet` | **Fuel** | Refinery (biofuel) | Generator fuel (`light-and-power.md`) |
| `Item.Blueprint` | **Lore** | POI offices | Unlocks a recipe; readable |

**Cut on purpose:** no currency, no keys (denial is geometric, not keyed), no ammo types
until a ranged weapon exists, no crafting-tier tokens.

### 3.4 Inventory

| Property | Value | Notes |
|---|---|---|
| Quickslots | `9` | Existing `NUM_QUICK_SLOTS` |
| Main slots | `27` | New; UI-side `AC_Inventory` must expose them |
| **Total** | **36** | Minecraft-classic (user decision) |
| Stack size | `64` default, per-item override | Tools: `1` |
| Weight | **None** | User decision |
| Drop | `A` (`IA_DropItem`) | Existing, drops hovered or selected stack |
| Pickup order | matching quickslot stack → matching main stack → empty quickslot → empty main | Predictable |

`FQuickSlotData` gains a `FItemRef Ref` alongside the legacy `BlockID`. `BlockID` remains
populated for block items so the existing widget and icon path keep working during migration.

### 3.5 Durability

| Rule | Value |
|---|---|
| Mining a block | `−1` durability |
| Hitting a mob | `−1` durability |
| Broken state | Durability `0`, tool unusable, **not destroyed** |
| Repair | Crafting table: tool + material unit |
| Repair amount | `+25 %` of `MaxDurability` per material unit |
| Material per tier | Wood → planks, Stone → cobblestone, Iron → iron ingot, Diamond → diamond |
| Repair cap | `MaxDurability` |

```
UnitsNeeded = ceil((MaxDurability - CurrentDurability) / (MaxDurability × 0.25))
```

**Why repair instead of destruction:** the durability is already in `FToolInstance`
(`ConsumeDurability`, `MaxDurability` 59 / 131 / 250 / 1561 / 2031). Destruction would waste
that data and add a re-grind the horror pacing cannot afford. Repair keeps the mining loop
meaningful (you need the ore) without making a bad night cost you a diamond pickaxe.

### 3.6 Crafting

The current `CanCraftTool(CurrentTier, DesiredTier)` is a **tier gate with no ingredients**.
That is the root cause of the missing economy: crafting costs nothing.

New schema:

```cpp
USTRUCT(BlueprintType)
struct FCraftRecipe
{
    FName RecipeId;                    // stable
    TArray<FItemStack> Ingredients;
    TArray<FItemStack> Output;
    float CraftTime = 1.0f;
    ECraftStation Station = ECraftStation::Hand; // Hand, Table, Furnace, Workbench
};
```

- Recipes live in `DT_Recipes` and are validated by an automation test.
- Tool recipes must cost real materials (e.g. Iron Pickaxe = 3 iron ingots + 2 planks).
- Smelting keeps `FSmeltRecipe` but `InputBlockID`/`OutputBlockID` become `FItemRef`, so a
  furnace can smelt an item into an item (ore → ingot stays a block-to-block case).

### 3.7 Loot tables

Replaces the block-proxy hack (`Docs/MOB_REGISTRY.md`).

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
    int32 Rolls = 1;
};
```

| Mob | Guaranteed | Rolled |
|---|---|---|
| Zombie | `RottenFlesh ×1–2` | `ScrapMetal` 5 % |
| Skeleton | `Bone ×1–2` | `String` 25 % |
| Spider | `String ×1–2` | `SpiderEye` 33 % |
| Creeper | `Gunpowder ×1` | — |
| **Angel** | **Nothing** | — (not killable until the Source dies; see `angels.md` §3.10) |

**Determinism:** loot rolls take an explicit seed so automation tests can assert exact drops.

### 3.8 Barter (no currency)

User decision: **pure barter**. No caps, no tokens.

```cpp
USTRUCT(BlueprintType)
struct FBarterOffer
{
    TArray<FItemStack> Wants;
    TArray<FItemStack> Gives;
    FName NpcId;
    int32 RefreshDay = 0;
};
```

- Each recruitable or trading NPC holds **one active offer**, refreshed once per in-game day.
- The player must have every `Wants` stack in the inventory; the exchange is atomic.
- `Value` on `UItemDefinition` exists **only to keep offers fair** during authoring:
  `0.75 ≤ Σ Wants.Value / Σ Gives.Value ≤ 1.25`. It is never shown as a price and never
  spendable.
- Recruitment (`npc-colony.md` §3.3) uses the same structure for its "trade" route.

### 3.9 What the player never sees

- No currency, no price tag in a currency.
- No item durability as a number in the HUD (a bar only, on the equipped tool).
- No weight or encumbrance.
- No item quality tiers or random affixes.

---

## 4. Formulas

| Formula | Value | Rationale |
|---|---|---|
| `MaxStackSize` | `64` default | Minecraft convention |
| `RepairFractionPerUnit` | `0.25` | Four units to fully repair |
| `UnitsNeeded` | `ceil(ΔDurability / (Max × 0.25))` | Readable cost |
| `DurabilityPerBlock` / `PerHit` | `1` / `1` | Simple, predictable |
| `BarterValueTolerance` | `0.75–1.25` | Offers stay fair without a currency |
| `OfferRefresh` | `1 per in-game day` | Ties trade to the day/night loop |
| `LootRolls` | per table | Deterministic with an explicit seed |

---

## 5. Edge cases

| Case | Resolution |
|---|---|
| Inventory full on pickup | Item stays on the ground; the pickup is not consumed |
| Partial stack fit | What fits is taken; the remainder stays as a pickup |
| Two damaged tools of the same type | Both remain separate stacks; no auto-merge |
| Repairing a broken tool | Allowed; `UnitsNeeded` for a fully broken tool = 4 units |
| Smelting a non-block item | Supported via `FItemRef`; the furnace UI filters valid inputs |
| Barter with an incomplete `Wants` set | Offer is rejected, nothing is consumed |
| NPC offer references a removed item | Validation test fails at cook; the offer is skipped at runtime |
| Loot table chances > 1 summed | Allowed — each entry rolls independently, 100 % entries go in `Guaranteed` |
| Block item with `Kind == Item` mismatch | Invalid; `FItemRef` validation rejects it in editor and in tests |
| Save game from before the item system | Version field + migration table: legacy `BlockID` → `FItemRef{Kind=Block}` |
| Player drops a stack into the world | Existing `BlockItemPickup` handles blocks; non-block items need a pickup mesh or an icon billboard |

---

## 6. Dependencies

| System | Why | Status |
|---|---|---|
| `BlockItemPickup` | Pickup path, physics launch, cooldown | Implemented for blocks only |
| `UQuickSlotsInventorySystem` | Storage and widget refresh | Implemented, 9 slots, block-only |
| `AC_Inventory` (Blueprint) | Main inventory | Exists, undocumented in C++ |
| `FToolInstance` / `UBaublesSystem` | Durability already modelled | Implemented |
| `UCraftingSmeltingSystem` | Recipes, smelting | Implemented, block-only, no ingredients |
| `UMobSpawnerSystem` / `AMobCharacter` | Loot drops | Implemented with block proxies |
| NPC colony | Barter, consumables, recruitment | Not implemented |
| Light & power | Fuel items, batteries | Not implemented |
| Save/load | Item stacks, durability, offers | Not documented |

---

## 7. Tuning knobs

| Knob | Default | Effect |
|---|---|---|
| `MaxStackSize` | 64 | Inventory pressure |
| `RepairFractionPerUnit` | 0.25 | Repair economy |
| `DurabilityPerBlock` / `PerHit` | 1 / 1 | Tool lifespan |
| `LootChance` per entry | per table | Economy faucets |
| `BarterValueTolerance` | ±25 % | Trade generosity |
| `OfferRefreshDays` | 1 | Trade frequency |
| Item `Value` | per item | Authoring fairness only |

---

## 8. Acceptance criteria

1. A Skeleton drops `Item.Bone`, not Dirt.
2. A Spider drops `Item.String` and can drop `Item.SpiderEye`.
3. An Angel drops nothing, ever, until the Source dies.
4. A block pickup still places a block when selected; a non-block item cannot be placed.
5. Inventory holds 27 main + 9 quickslots = 36 slots, stacks of 64.
6. A tool loses 1 durability per block broken and per hit.
7. A broken tool is unusable and repairable at a crafting table for 1–4 material units.
8. Crafting an Iron Pickaxe consumes its ingredients.
9. Barter exchanges exactly the `Wants` for the `Gives`, atomically, with no currency.
10. Loot rolls are deterministic with a fixed seed.
11. Every `FItemRef` in every DataTable resolves at cook time; a dangling ID fails the build.
12. A save from the legacy format loads with every block pickup mapped to `Kind = Block`.

---

## 9. Build order (implementation plan)

1. **`FItemRef`, `FItemStack`, `EItemKind`** + validation helpers + tests.
2. **`UItemDefinition`** + `DT_Items` with the v1 catalogue.
3. **Inventory migration** — `FQuickSlotData` gains `Ref`; legacy `BlockID` stays populated.
4. **Pickup path** — non-block items spawn a pickup; blocks unchanged.
5. **Loot tables** replacing the proxies in `AMobCharacter`.
6. **Durability** already exists; add repair recipe + UI.
7. **`DT_Recipes`** and real tool costs.
8. **Barter** — depends on the NPC system.
9. **Save/load** for stacks and offers.

Steps 1–5 are independent of the Angel system and can ship first.

---

## 10. Open questions

- Does the 27-slot main inventory live in C++ (like the quickslots) or stay in `AC_Inventory`?
  The current split is undocumented and will bite during migration.
- Are tools carried as items before being equipped, or crafted directly into a Baubles slot?
- Does `Item.Blueprint` unlock recipes, or is it pure lore? (Unlocking adds a progression
  gate the design has not asked for.)
- How are non-block pickups rendered in the world — a mesh, a billboard, or a shared
  generic pickup icon?
- Does food spoil? Spoilage adds pressure to exploration but is another timer to explain.
