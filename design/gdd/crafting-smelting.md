# GDD — Crafting & Smelting

| Field | Value |
|---|---|
| Status | Draft (2026-11-11) |
| System owner | game-designer / systems-designer / gameplay-programmer |
| Related | `Docs/BLOCK_REGISTRY.md`, `Docs/TOOL_REGISTRY.md`, `Docs/SYSTEMS_REFERENCE.md` §1–5, `design/gdd/items.md`, `design/gdd/weapons-tools.md` |
| Source of truth for current code | `CraftingSmeltingSystem.h/.cpp`, `BlockRegistry`, `ToolTypes.h`, `QuickSlotsInventorySystem.h` |
| Decisions | ADR-0007 (DT_Tools data model), ADR-0002 (generated vs edited blocks) |

---

## 1. Overview

MinecraftDeWish provides two crafting stations plus hand crafting, following the familiar
Minecraft loop:

- **Hand crafting (2×2 grid)** — available in the player inventory at all times. Produces
  basic building blocks (planks, sticks, torches) and the Crafting Table itself.
- **Crafting Table (3×3 grid)** — Block ID 15. Unlocks tools, blocks, containers, and
  repair. Right-clicking a Crafting Table within 4 blocks fires `OnCraftingTableOpened`.
- **Furnace (smelting)** — Block ID 16. Converts raw materials into refined products
  (ores → ingots, logs → charcoal, sand → glass, cobblestone → stone). Right-clicking a
  Furnace within 4 blocks fires `OnFurnaceOpened`.

The system is implemented by `UCraftingSmeltingSystem`, which validates recipes, manages
furnace fuel burn timers, and drives smelting progress. Crafting is **instant** (no
craft-time delay in v1); smelting is **time-gated** by per-item cook time and fuel burn
duration.

Recipes are **data-driven** (target: `DT_Recipes`, see `design/gdd/items.md` §3.6) and
validated at cook time. The current C++ implementation is block-ID-only; the target
schema uses `FItemRef` so furnaces can smelt items into items and crafting can consume
non-block materials (bones, string, etc.).

---

## 2. Player Fantasy

> *"I turn what I dug up into what I need."*

Crafting is the **transformation fantasy**: the player takes raw, useless-in-the-moment
materials (a log, a lump of ore, a handful of sand) and, through a deliberate
arrangement, makes them into something that advances the loop — a tool that mines
faster, a block that lights the dark, a container that holds the loot of the next run.

The fantasy has three beats:

- **Discovery through arrangement.** The player discovers recipes by trying shapes.
  A 3×3 grid with three iron ingots across the top and two sticks below is a pickaxe;
  the *shape* is the knowledge, not a menu. Recipes are legible once seen.
- **Tier as commitment.** Crafting a diamond pickaxe costs three diamonds and two sticks —
  a real investment. The player feels the weight of the decision because the materials
  are scarce and the tool's durability is finite.
- **The furnace as patience.** Smelting is the only system that asks the player to
  wait. Fuel burns, progress ticks, the result appears. It is the game's one
  "leave it and come back" mechanic, and it rewards planning: load the furnace, go
  mine, return to a full output slot.

The system must never feel like a menu tax. Every recipe should be expressible as a
shape the player can picture, and every smelt should have a visible, predictable
progress bar.

---

## 3. Detailed Rules

### 3.1 Crafting grids

| Station | Grid size | Access | Recipes available |
|---|---|---|---|
| Hand (inventory) | 2×2 | Always (press `I` / inventory key) | Planks, sticks, torches, crafting table, basic repairs |
| Crafting Table (Block 15) | 3×3 | Right-click within 4 blocks | All hand recipes + tools, blocks, containers, fences, doors, full repair |
| Furnace (Block 16) | 1 input + 1 fuel + 1 output | Right-click within 4 blocks | Smelting only |

- The 2×2 grid is a sub-region of the player inventory panel. The 3×3 grid is the
  Crafting Table panel. Both use the same recipe-matching engine; the only difference
  is which recipes are *eligible* given the station's `ECraftStation` tag.
- A recipe tagged `Hand` can be crafted in either grid. A recipe tagged `Table` requires
  the Crafting Table. A recipe tagged `Furnace` is a smelt recipe, not a craft recipe.

### 3.2 Recipe matching

Two matching modes:

**Shape-based (ordered):** the ingredient pattern must appear in the grid at a
consistent offset, with empty cells allowed around it. The shape is defined as a
fixed-size mask (e.g. 2×3 for a pickaxe) and matched by sliding the mask over the
grid. Rotations and mirrors are **not** allowed unless the recipe explicitly opts in
(v1: no rotations, no mirrors — Minecraft parity).

**Shapeless (unordered):** the multiset of non-empty grid slots must exactly equal the
recipe's ingredient multiset. Position does not matter. Used for planks-from-log,
sticks-from-planks, and any "combine these N things" recipe.

**Matching priority:** shape-based recipes are tested first (sorted by ingredient count
descending, then by recipe ID ascending); shapeless recipes are tested only if no
shape-based recipe matched. This prevents a shapeless "3 planks" recipe from shadowing
a shaped "3 planks in a row" recipe.

**Output quantity:** a recipe produces a fixed output stack (`OutputCount`). Crafting
consumes exactly the ingredients listed; if the grid contains *more* than the recipe
requires, the surplus stays in the grid (Minecraft parity — the player can craft
repeatedly from a full grid).

### 3.3 Crafting recipes (v1 catalogue)

All recipes use block IDs from `Docs/BLOCK_REGISTRY.md`. Tool recipes produce
`FToolInstance` entries equipped into `UBaublesSystem` (see `design/gdd/weapons-tools.md`
§3.6 for repair).

#### 3.3.1 Hand recipes (2×2)

| Recipe | Shape (2×2) | Ingredients | Output |
|---|---|---|---|
| Oak Planks | shapeless | 1 × Wood Log (5) | 4 × Wood Planks (6) |
| Sticks | vertical pair | 2 × Wood Planks (6) | 4 × Stick |
| Torch | vertical pair | 1 × Stick + 1 × Coal (16/27) | 4 × Torch (19) |
| Crafting Table | 2×2 fill | 4 × Wood Planks (6) | 1 × Crafting Table (15) |

#### 3.3.2 Crafting Table recipes (3×3)

**Tools** (material = the tier's source block/ingot; sticks = Stick):

| Tool | Shape (3×3) | Ingredients | Output |
|---|---|---|---|
| Wooden Pickaxe | `MMM / _S_ / _S_` | 3 × Wood Planks (6) + 2 × Stick | Wooden Pickaxe |
| Stone Pickaxe | `MMM / _S_ / _S_` | 3 × Cobblestone (3) + 2 × Stick | Stone Pickaxe |
| Iron Pickaxe | `MMM / _S_ / _S_` | 3 × Iron Ingot (8) + 2 × Stick | Iron Pickaxe |
| Diamond Pickaxe | `MMM / _S_ / _S_` | 3 × Diamond (11 raw) + 2 × Stick | Diamond Pickaxe |
| Obsidian Pickaxe | `MMM / _S_ / _S_` | 3 × Obsidian + 2 × Stick | Obsidian Pickaxe |
| Wooden Shovel | `_M_ / _S_ / _S_` | 1 × Wood Planks (6) + 2 × Stick | Wooden Shovel |
| Stone Shovel | `_M_ / _S_ / _S_` | 1 × Cobblestone (3) + 2 × Stick | Stone Shovel |
| Iron Shovel | `_M_ / _S_ / _S_` | 1 × Iron Ingot (8) + 2 × Stick | Iron Shovel |
| Diamond Shovel | `_M_ / _S_ / _S_` | 1 × Diamond (11) + 2 × Stick | Diamond Shovel |
| Obsidian Shovel | `_M_ / _S_ / _S_` | 1 × Obsidian + 2 × Stick | Obsidian Shovel |
| Wooden Axe | `MM_ / MS_ / _S_` | 3 × Wood Planks (6) + 2 × Stick | Wooden Axe |
| Stone Axe | `MM_ / MS_ / _S_` | 3 × Cobblestone (3) + 2 × Stick | Stone Axe |
| Iron Axe | `MM_ / MS_ / _S_` | 3 × Iron Ingot (8) + 2 × Stick | Iron Axe |
| Diamond Axe | `MM_ / MS_ / _S_` | 3 × Diamond (11) + 2 × Stick | Diamond Axe |
| Obsidian Axe | `MM_ / MS_ / _S_` | 3 × Obsidian + 2 × Stick | Obsidian Axe |
| Wooden Hoe | `MM_ / _S_ / _S_` | 2 × Wood Planks (6) + 2 × Stick | Wooden Hoe |
| Stone Hoe | `MM_ / _S_ / _S_` | 2 × Cobblestone (3) + 2 × Stick | Stone Hoe |
| Iron Hoe | `MM_ / _S_ / _S_` | 2 × Iron Ingot (8) + 2 × Stick | Iron Hoe |
| Diamond Hoe | `MM_ / _S_ / _S_` | 2 × Diamond (11) + 2 × Stick | Diamond Hoe |
| Obsidian Hoe | `MM_ / _S_ / _S_` | 2 × Obsidian + 2 × Stick | Obsidian Hoe |
| Wooden Sword | `_M_ / _M_ / _S_` | 2 × Wood Planks (6) + 1 × Stick | Wooden Sword |
| Stone Sword | `_M_ / _M_ / _S_` | 2 × Cobblestone (3) + 1 × Stick | Stone Sword |
| Iron Sword | `_M_ / _M_ / _S_` | 2 × Iron Ingot (8) + 1 × Stick | Iron Sword |
| Diamond Sword | `_M_ / _M_ / _S_` | 2 × Diamond (11) + 1 × Stick | Diamond Sword |
| Obsidian Sword | `_M_ / _M_ / _S_` | 2 × Obsidian + 1 × Stick | Obsidian Sword |

**Blocks:**

| Output | Shape (3×3) | Ingredients |
|---|---|---|
| Iron Block (8) | 3×3 fill | 9 × Iron Ingot (8) |
| Gold Block (10) | 3×3 fill | 9 × Gold Ingot (10) |
| Diamond Block (12) | 3×3 fill | 9 × Diamond (11) |
| Emerald Block (14) | 3×3 fill | 9 × Emerald (13) |
| Coal Block | 3×3 fill | 9 × Coal (16/27) |
| Wood Planks (6) | 2×2 fill | 4 × Wood Log (5) → 16 planks (4 per log × 4) |
| Furnace (16) | ring (8 cobblestone) | 8 × Cobblestone (3) |
| Crafting Table (15) | 2×2 fill | 4 × Wood Planks (6) |

**Containers & furniture:**

| Output | Shape (3×3) | Ingredients |
|---|---|---|
| Barrel (20) | ring (6 planks + 2 slabs) | 6 × Wood Planks (6) (slabs not implemented; use 6 planks) |
| Fence (22) | `P_P_ / P_P_ / _P_` | 4 × Wood Planks (6) + 2 × Stick |
| Door (24) | `PP_ / PP_ / PP_` | 6 × Wood Planks (6) |
| Chest (not in registry — future) | ring (8 planks) | 8 × Wood Planks (6) |

**Repair (Crafting Table only):**

| Input | Ingredient | Effect |
|---|---|---|
| Any damaged tool | 1 × matching material unit | `+25 %` of `MaxDurability`, capped at `MaxDurability` |
| Broken tool (durability 0) | 4 × matching material unit | Full restore |

Material per tier: Wood → Wood Planks (6); Stone → Cobblestone (3); Iron → Iron Ingot (8);
Diamond → Diamond (11); Obsidian → Obsidian.

### 3.4 Smelting (Furnace)

The Furnace has three slots: **Input**, **Fuel**, **Output**.

**Flow:**

1. Player places a smeltable item in the Input slot and a fuel in the Fuel slot.
2. If `FurnaceBurnTimeRemaining ≤ 0`, the furnace consumes one fuel unit, setting
   `FurnaceBurnTimeRemaining = GetFuelBurnDuration(FuelBlockID)` and
   `FurnaceTotalBurnTime = same`.
3. The furnace cooks the input item for `CookTime` (default 8.0 s) per item.
   `FurnaceSmeltProgress` advances from 0.0 to 1.0 over `CookTime`.
4. When `FurnaceSmeltProgress ≥ 1.0`, the output item is produced into the Output slot
   (if it has room; otherwise smelting pauses). Input count decrements by 1.
   `FurnaceSmeltProgress` resets to 0.0 for the next item.
5. Fuel burn time decrements every tick (`TickFurnace(DeltaTime)`). When fuel runs out,
   smelting pauses until new fuel is added.
6. The furnace front texture swaps to `furnace_front_on` (slice 20) while burning and
   back to `furnace_front` (slice 19) when idle.

**Smelting recipes (v1):**

| Input | Output | Cook time |
|---|---|---|
| Iron Ore (7) | Iron Ingot/Block (8) | 8.0 s |
| Gold Ore (9) | Gold Ingot/Block (10) | 8.0 s |
| Wood Log (5) | Charcoal/Coal Block (16) | 8.0 s |
| Sand (18) | Glass (21) | 8.0 s |
| Cobblestone (3) | Stone (4) | 8.0 s |

**Target schema (per `items.md` §3.6):** `FSmeltRecipe` gains `FItemRef Input` and
`FItemRef Output` so the furnace can smelt non-block items (e.g. Rotten Flesh →
Cooked Meat, Spider Eye → Fermented Spider Eye). The block-to-block cases above remain
the default rows.

### 3.5 Fuels

| Fuel | Block ID | Burn duration | Items smelted (at 8 s/item) | Notes |
|---|---|---|---|---|
| Coal / Charcoal | 16, 27 | 80.0 s | 10.0 | Phase-2 staple; best early fuel |
| Wood Log | 5 | 15.0 s | 1.875 (~2 partial) | Renewable; also smeltable to charcoal |
| Wood Planks | 6 | 10.0 s | 1.25 (~1) | Emergency fuel; wasteful |
| (Future) Biofuel | — | 120 000 EU equivalent | — | Phase-3; see `design/gdd/light-and-power.md` §3.4 |

- `IsValidFuel(BlockID)` returns `GetFuelBurnDuration(BlockID) > 0.0f`.
- A fuel unit is consumed **only when the furnace needs to start or continue burning**
  and `FurnaceBurnTimeRemaining ≤ 0`. If the furnace is already burning, the fuel slot
  is not touched until the current burn expires.
- **Fuel is not consumed if the output slot is full.** The furnace waits (burn timer
  paused) until the player collects the output. This prevents wasting fuel on a blocked
  output.

### 3.6 User interface

**Crafting Table panel (`WBP_CraftingTable`):**
- 3×3 grid of slots (left), output slot (right), with an arrow indicating the recipe
  result.
- Hovering the output slot shows the result name and quantity (`FText`).
- Clicking the output slot consumes ingredients and adds the result to the player
  inventory (or the hovered slot if the inventory panel is open).
- The 2×2 hand grid is a sub-panel of the inventory (`WBP_Inventory`) with the same
  layout but smaller.

**Furnace panel (`WBP_Furnace`):**
- Three slots: Input (top), Fuel (bottom-left), Output (bottom-right).
- Progress arrow between Input and Output fills left-to-right as `FurnaceSmeltProgress`
  advances.
- Fuel flame icon next to the Fuel slot drains as `FurnaceBurnTimeRemaining /
  FurnaceTotalBurnTime` decreases.
- The furnace front texture (`furnace_front_on`, slice 20) is shown in-world while
  burning, providing a visual cue even when the panel is closed.

**Shared rules:**
- All text is `FText` / `NSLOCTEXT` for localization.
- Slot icons resolve via `AChunkActor::GetBlockIconTexture(BlockID)` for block items.
- The UI never owns game state; it reads from `UCraftingSmeltingSystem` and sends
  intent (place item, take result) via delegates.

---

## 4. Formulas

### 4.1 Fuel burn & smelting efficiency

Let `CookTime` = time to smelt one input item (default 8.0 s, tunable per recipe).

```
ItemsPerFuel = Floor(FuelBurnDuration / CookTime)
FuelBurnDuration values:
    Coal/Charcoal: 80.0 s  →  80 / 8 = 10.0 items
    Wood Log:      15.0 s  →  15 / 8 = 1.875 → 1 full item + 0.875 partial
    Wood Planks:   10.0 s  →  10 / 8 = 1.25   → 1 full item + 0.25 partial
```

Partial burn time carries over: if a log burns for 15 s and one item takes 8 s, the
remaining 7 s of burn time applies to the next item (no fuel wasted on item boundaries).

```
FurnaceBurnTimeRemaining(t+Δt) = max(0, FurnaceBurnTimeRemaining(t) - Δt)
FurnaceSmeltProgress(t+Δt)     = FurnaceSmeltProgress(t) + Δt / CookTime
```

When `FurnaceSmeltProgress ≥ 1.0`:
```
OutputCount += 1
InputCount  -= 1
FurnaceSmeltProgress = FurnaceSmeltProgress - 1.0   (carry-over remainder)
```

### 4.2 Fuel efficiency ranking

```
Efficiency (items per fuel unit, normalized by "cost to obtain"):
    Coal:    10.0 items / 1 coal ore   (mined, not crafted)
    Log:     1.875 items / 1 log      (mined, also yields 4 planks)
    Planks:  1.25 items / 1 plank     (crafted from 0.25 log)
```

A log is worth more as fuel than as 4 planks-as-fuel (1.875 vs 4 × 1.25 = 5.0 items),
but planks are the emergency option when no coal or logs are available.

### 4.3 Recipe matching

**Shape-based match:**

```
For recipe R with pattern P (Wp × Hp cells):
    For offset (ox, oy) in [0, GridW - Wp] × [0, GridH - Hp]:
        Match = true
        For each cell (x, y) in P:
            if P[x,y] != 0 and Grid[ox+x, oy+y] != P[x,y]: Match = false
            if P[x,y] == 0 and Grid[ox+x, oy+y] != 0:     Match = false
        For each cell outside P's footprint within the grid:
            if Grid[x,y] != 0: Match = false   (no extra ingredients)
        if Match: return R
```

**Shapeless match:**

```
GridItems = multiset of non-empty (BlockID/ItemID, count) in the grid
RecipeItems = multiset of recipe ingredients
if GridItems == RecipeItems: return R
```

**Priority:** shape-based first (by ingredient count desc, then recipe ID asc),
then shapeless (by ingredient count desc, then recipe ID asc).

### 4.4 Repair cost

```
RepairAmount = 0.25 × MaxDurability  (per material unit)
UnitsNeeded  = ceil((MaxDurability - CurrentDurability) / (MaxDurability × 0.25))
FullyBroken: UnitsNeeded = 4
```

See `design/gdd/weapons-tools.md` §4.4 for the full durability formula.

### 4.5 Smelting throughput

```
Throughput (items/hour) = 3600 / CookTime = 3600 / 8 = 450 items/hour per furnace
FuelConsumption (coal/hour) = Throughput / 10 = 45 coal/hour at full throughput
```

A single furnace with coal can smelt 450 items per hour — enough for early game but
a bottleneck for large builds, incentivizing multiple furnaces.

---

## 5. Edge Cases

| Case | Resolution |
|---|---|
| Empty crafting grid | No recipe matches; output slot stays empty |
| Grid has extra items beyond a recipe's requirements | Recipe still matches if the core pattern is present and no *conflicting* items exist in the pattern footprint; surplus items outside the footprint remain in the grid |
| Two recipes could match the same grid | Shape-based takes priority over shapeless; within the same mode, higher ingredient count wins; ties broken by recipe ID ascending |
| Furnace input is not smeltable | `GetSmeltResult` returns 0; the furnace refuses to cook; UI shows "This item cannot be smelted" |
| Furnace has no fuel and burn time is 0 | `SmeltItem` checks `FurnaceBurnTimeRemaining ≤ 0` and attempts to consume fuel from the Fuel slot; if invalid, returns false with "Invalid fuel!" message |
| Furnace output slot is full | Smelting pauses (burn timer continues but no item is produced); resumes when the player collects the output. Fuel is not wasted on a blocked output |
| Furnace fuel runs out mid-smelt | `FurnaceBurnTimeRemaining` reaches 0; smelting pauses; `FurnaceTotalBurnTime` resets to 0. Partial smelt progress is preserved. Adding new fuel resumes cooking from the preserved progress |
| Player removes fuel from a burning furnace | Burn timer stops; partial progress preserved; furnace front reverts to idle texture |
| Player removes input from a cooking furnace | Smelt progress resets to 0; output is not produced; fuel burn continues (wasted) until it expires |
| Crafting with a full inventory | Output cannot be collected; ingredients are not consumed; UI shows "Inventory full" |
| Repairing a tool at max durability | No-op; UI disables the repair action; no materials consumed |
| Repairing a broken tool (durability 0) | Allowed; requires 4 material units for a full restore |
| Crafting a tool when the Baubles slot is occupied | The crafted tool is added to the inventory as an item, not auto-equipped. The player equips it manually via the Baubles UI (open question: should it auto-equip? See §10) |
| Smelting a non-block item (target schema) | Supported via `FItemRef`; the furnace UI filters valid inputs based on `GetSmeltResult` |
| Coal Ore (27) used as fuel | Treated as fuel (80 s) — matches Coal Block (16). Note: Coal Ore is also smeltable? No — Coal Ore is not in the smelt recipe table; only Coal Block (16) is produced by smelting logs. Coal Ore is mined directly |
| Furnace front texture desync | The in-world furnace mesh updates its front face slice (19 ↔ 20) based on `FurnaceBurnTimeRemaining > 0`, polled on the next mesh rebuild or via a material parameter |

---

## 6. Dependencies

| System | Why | Status |
|---|---|---|
| `UCraftingSmeltingSystem` | Recipe validation, fuel burn, smelting progress | Implemented (block-only, no ingredients) |
| `AWorldGenerator::InteractWithTargetBlock` | Proximity right-click opens crafting/furnace panels | Implemented |
| `UBaublesSystem` | Tool crafting output, repair target, durability | Implemented |
| `FToolInstance` / `DT_Tools` | Tool data, durability, tier stats | Migrating (ADR-0007) |
| `UQuickSlotsInventorySystem` | Inventory storage, stacking, slot refresh | Implemented (9 slots) |
| `AC_Inventory` (Blueprint) | Main inventory UI, 27 slots | Exists, undocumented in C++ |
| `AChunkActor::GetBlockIconTexture` | Slot icon resolution | Implemented |
| `design/gdd/items.md` | `FItemRef`, `FCraftRecipe`, `DT_Recipes`, item catalogue | Draft |
| `design/gdd/weapons-tools.md` | Tool tiers, durability, repair rules | Draft |
| Save/load system | Furnace state (burn timer, progress, slots) persistence | Not implemented |

---

## 7. Tuning Knobs

| Knob | Default | Safe range | Effect |
|---|---|---|---|
| `CookTime` (per smelt recipe) | 8.0 s | 4.0–16.0 s | Smelting speed; affects fuel efficiency |
| `CoalBurnDuration` | 80.0 s | 40.0–160.0 s | Coal fuel efficiency (items per coal) |
| `LogBurnDuration` | 15.0 s | 8.0–30.0 s | Wood log fuel efficiency |
| `PlankBurnDuration` | 10.0 s | 5.0–20.0 s | Plank fuel efficiency (emergency) |
| `RepairFractionPerUnit` | 0.25 | 0.10–0.50 | Repair economy (units needed) |
| `MaxStackSize` | 64 | 16–999 | Inventory pressure |
| `CraftingGridSize_Hand` | 2×2 | Fixed (Minecraft parity) | Hand-craftable recipes |
| `CraftingGridSize_Table` | 3×3 | Fixed (Minecraft parity) | Full recipe set |
| `FurnaceProximityRange` | 4 blocks (400 UU) | 2–6 blocks | Interaction distance |
| `ShapeBasedPriority` | true | true/false | Whether shaped recipes shadow shapeless |
| `AllowRotations` | false | true/false | Whether recipe shapes can be rotated (v1: no) |
| `AllowMirrors` | false | true/false | Whether recipe shapes can be mirrored (v1: no) |

---

## 8. Acceptance Criteria

1. Right-clicking a Crafting Table within 4 blocks opens the 3×3 crafting panel.
2. Right-clicking a Furnace within 4 blocks opens the smelting panel.
3. A 2×2 hand grid correctly matches shapeless recipes (planks, sticks, torches).
4. A 3×3 table grid correctly matches shape-based recipes (pickaxe, shovel, axe, hoe, sword).
5. A shape-based recipe does not match if extra items are present in the pattern footprint.
6. A shapeless recipe matches regardless of item position.
7. Crafting consumes exactly the listed ingredients and produces the correct output quantity.
8. Surplus ingredients remain in the grid after crafting.
9. Smelting Iron Ore with Coal produces Iron Ingot after 8.0 s.
10. Coal (80 s) smelts exactly 10 items at 8 s/item with no wasted burn time (carry-over).
11. A Wood Log (15 s) smelts 1 full item + 7 s of partial progress toward the next.
12. A Furnace with a full output slot pauses smelting without consuming fuel.
13. Removing fuel from a burning furnace stops the burn timer and preserves partial progress.
14. `IsValidFuel` returns true for Block IDs 16, 27, 5, 6 and false for all others.
15. `GetSmeltResult` returns the correct output for inputs 7, 9, 5, 18, 3 and 0 for all others.
16. Repairing a tool at a crafting table restores 25 % of `MaxDurability` per material unit.
17. A fully broken tool requires 4 material units for a full repair.
18. All recipe definitions resolve their ingredient and output IDs at cook time; a dangling ID fails the build.
19. The furnace front texture swaps to `furnace_front_on` while burning and back when idle.
20. An automation test covers: shape-based matching, shapeless matching, priority ordering, fuel burn carry-over, and output-slot blocking.

---

## 9. Build order (implementation plan)

1. **`FCraftRecipe` / `FSmeltRecipe` schema** with `FItemRef` (per `items.md` §3.6) + validation tests.
2. **Recipe matching engine** — shape-based sliding window + shapeless multiset, priority ordering.
3. **`DT_Recipes`** with the v1 catalogue (hand + table + furnace).
4. **Crafting Table UI** (`WBP_CraftingTable`) — 3×3 grid, output slot, hover tooltip.
5. **Hand crafting UI** — 2×2 sub-panel in the inventory.
6. **Furnace UI** (`WBP_Furnace`) — input/fuel/output slots, progress arrow, fuel flame.
7. **Furnace tick logic** — `TickFurnace` with cook progress, fuel carry-over, output blocking.
8. **Repair recipe** — tool + material → durability restore.
9. **In-world furnace texture swap** (front face slice 19 ↔ 20).
10. **Save/load** for furnace state (burn timer, progress, slot contents).
11. **Automation tests**: matching, priority, fuel efficiency, repair costs, edge cases.

---

## 10. Open questions

- Should a crafted tool auto-equip into the matching Baubles slot if that slot is empty,
  or always go to the inventory? Auto-equip is friendlier but may surprise the player.
- Are recipe discoveries gated (player must find a Blueprint item) or all known from the
  start? The `Item.Blueprint` item exists in `items.md` but its role is undefined.
- Should smelting produce XP (Minecraft: 0.1–1.0 XP per item smelted)? If so, the
  furnace needs to spawn XP orbs on completion.
- Should there be a "fast smelting" mechanic (e.g. blast furnace for metals, smoker for
  food) or is one furnace type sufficient for v1?
- How are non-block item pickups rendered in the world? The furnace can produce
  non-block items (cooked meat, glass) that need a pickup mesh or billboard.
- Should crafting have a time delay (crafting animation) or remain instant? Instant
  is faster but removes the "commitment" feeling of smelting.
- Does the 27-slot main inventory live in C++ or stay in `AC_Inventory` Blueprint?
  This affects where the crafting grid state is stored.
