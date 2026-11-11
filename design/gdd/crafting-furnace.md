# GDD — Crafting Table & Furnace

| Field | Value |
|---|---|
| Status | Draft (2026-11-11) |
| System owner | game-designer / systems-designer / gameplay-programmer / ux-designer |
| Related | `Docs/BLOCK_REGISTRY.md`, `Docs/SYSTEMS_REFERENCE.md` §1, §5, `Docs/GAME_DESIGN.md` §5, `design/gdd/crafting-smelting.md`, `design/gdd/inventory-quickslots.md`, `design/gdd/items.md`, `design/gdd/weapons-tools.md` |
| Source of truth for current code | `CraftingSmeltingSystem.h/.cpp`, `WorldGenerator.cpp` (`InteractWithTargetBlock`), `BlockRegistry`, `ToolTypes.h` |
| Decisions | ADR-0007 (DT_Tools data model), ADR-0002 (generated vs edited blocks) |

---

## 1. Overview

MinecraftDeWish exposes two station blocks that gate the mid-game transformation loop:
the **Crafting Table** (Block ID 15) and the **Furnace** (Block ID 16). Both are
interacted with by **right-clicking within 4 blocks (400 UU)** of the player, which
fires the corresponding delegate on `AWorldGenerator` (`OnCraftingTableOpened` /
`OnFurnaceOpened`) and opens the matching UI panel.

- **Crafting Table** provides a **3×3 crafting grid**. It is the only station that
  can craft tools, blocks, containers, fences, doors and perform tool repair. Its
  recipes are a superset of the 2×2 hand-crafting recipes available in the player
  inventory. Crafting is **instant** (no craft-time delay in v1).
- **Furnace** provides **three slots — Input, Fuel, Output** — and converts raw
  materials into refined products (ores → ingots, logs → charcoal, sand → glass,
  cobblestone → stone). Smelting is **time-gated**: each item takes a default
  `CookTime = 8.0 s` and fuel burns according to its burn duration. The furnace
  front texture swaps between `furnace_front` (slice 19, idle) and
  `furnace_front_on` (slice 20, burning) to give an in-world cue even when the
  panel is closed.

Both stations are implemented by `UCraftingSmeltingSystem`, which validates
recipes, manages furnace fuel burn timers, and drives smelting progress. Recipes
are data-driven (target: `DT_Recipes`, see `design/gdd/items.md` §3.6); the
current C++ implementation is block-ID-only, with a target schema based on
`FItemRef` so furnaces can smelt non-block items and crafting can consume
non-block materials.

This document is the design source of truth for the **two station blocks** — their
interaction, grid/slot layout, fuels, cooking, UI, delegates and proximity rules.
Recipe catalogues and the hand 2×2 grid are specified in
`design/gdd/crafting-smelting.md`; item identity and stacking in
`design/gdd/items.md` and `design/gdd/inventory-quickslots.md`.

---

## 2. Player Fantasy

> *"The table is where I decide what to become. The furnace is where I wait."*

The two stations split the transformation fantasy into two distinct beats:

- **The Crafting Table is agency.** It is the place where raw materials become
  intent: a log becomes planks, planks become a tool, three iron ingots and two
  sticks become a pickaxe that will outlast the night. The player arranges a
  shape and the result appears instantly — the table rewards *knowledge* (which
  shape makes which tool) and *commitment* (spending scarce diamonds on a
  pickaxe is a real choice). The 3×3 grid is the largest canvas the player has;
  every recipe should be expressible as a shape the player can picture.
- **The Furnace is patience.** Smelting is the only system that asks the player
  to wait. Fuel burns, progress ticks, the result appears. It is the game's one
  "leave it and come back" mechanic, and it rewards planning: load the furnace,
  go mine, return to a full output slot. The in-world flame on the furnace front
  is the promise that work is happening while the player is away.
- **Proximity keeps it physical.** The player must stand within 4 blocks and
  right-click the block — the station is a place in the world, not a menu. This
  keeps crafting and smelting tied to the spatial loop (build a base, place the
  table, place the furnace, return to it) instead of becoming an abstract
  inventory action.

The system must never feel like a menu tax: every recipe should be legible once
seen, and every smelt should have a visible, predictable progress bar.

---

## 3. Detailed Rules

### 3.1 Proximity interaction

Right-clicking a block within **4 blocks (400 UU)** initiates the interactive
subsystem via `AWorldGenerator::InteractWithTargetBlock()`:

- **Crafting Table (Block ID 15)** — fires `OnCraftingTableOpened` delegate and
  displays the `WBP_CraftingTable` panel.
- **Furnace (Block ID 16)** — fires `OnFurnaceOpened` delegate and displays the
  `WBP_Furnace` panel.

Rules:
- The interaction is **server-authoritative** in multiplayer: the client requests
  the interaction, the server validates range (≤ 4 blocks), block type, and
  ownership, then broadcasts the open event.
- If the player is further than 4 blocks, the right-click is a no-op (no panel,
  no delegate). The player must approach the block.
- If the targeted block is not a Crafting Table or Furnace (e.g. a chest, crate,
  barrel), the corresponding container delegate fires instead
  (`OnContainerOpened`); see `design/gdd/inventory-quickslots.md` §3.8.
- Only one station panel may be open at a time. Opening a new panel closes the
  previous one and commits any in-flight drag as a cancel.
- The game is **not paused** while a panel is open (horror pacing: the player stays
  observable). The mouse cursor is freed and camera look is disabled while the
  panel is open.
- Closing the panel (Esc or re-clicking) hides the cursor and returns input to
  gameplay. Any in-flight crafting result that was not collected stays in the
  output slot.

### 3.2 Crafting Table — 3×3 grid

The Crafting Table panel (`WBP_CraftingTable`) exposes a **3×3 grid of slots**
(left) and a single **output slot** (right), with an arrow indicating the recipe
result.

- Grid size is **fixed at 3×3** (Minecraft parity). It is not a tuning knob.
- The 3×3 grid supports the full recipe set: all hand (2×2) recipes plus tools,
  blocks, containers, fences, doors and repair. A recipe tagged `Hand` can be
  crafted in either grid; a recipe tagged `Table` requires the Crafting Table.
- Recipe matching follows `crafting-smelting.md` §3.2: **shape-based (ordered)**
  recipes are tested first (sliding mask, no rotations, no mirrors in v1), then
  **shapeless (unordered)** recipes. Priority is by ingredient count descending,
  then recipe ID ascending.
- Crafting is **instant**: placing the correct ingredients makes the output slot
  populate immediately; clicking the output slot consumes the ingredients and
  adds the result to the player inventory (or the hovered slot if an inventory
  panel is open).
- **Output quantity** is fixed per recipe (`OutputCount`). If the grid contains
  more than the recipe requires, the surplus stays in the grid (Minecraft parity
  — the player can craft repeatedly from a full grid).
- **Repair (Crafting Table only):** a damaged tool + a matching material unit
  restores `+25 %` of `MaxDurability` per unit, capped at `MaxDurability`. A
  fully broken tool requires 4 material units for a full restore. Material per
  tier: Wood → Wood Planks (6), Stone → Cobblestone (3), Iron → Iron Ingot (8),
  Diamond → Diamond (11), Obsidian → Obsidian.
- Hovering the output slot shows the result name and quantity as `FText`.
- All text is `FText` / `NSLOCTEXT` for localization.

### 3.3 Furnace — input / fuel / output slots

The Furnace panel (`WBP_Furnace`) exposes **three slots**:

| Slot | Position | Purpose |
|---|---|---|
| Input | Top | Item to be smelted |
| Fuel | Bottom-left | Fuel item consumed for burn time |
| Output | Bottom-right | Smelted result (collected by the player) |

**Flow:**
1. Player places a smeltable item in the Input slot and a fuel in the Fuel slot.
2. If `FurnaceBurnTimeRemaining ≤ 0`, the furnace consumes one fuel unit, setting
   `FurnaceBurnTimeRemaining = GetFuelBurnDuration(FuelBlockID)` and
   `FurnaceTotalBurnTime = same`.
3. The furnace cooks the input item for `CookTime` (default 8.0 s) per item.
   `FurnaceSmeltProgress` advances from 0.0 to 1.0 over `CookTime`.
4. When `FurnaceSmeltProgress ≥ 1.0`, the output item is produced into the Output
   slot (if it has room; otherwise smelting pauses). Input count decrements by 1.
   `FurnaceSmeltProgress` resets to 0.0 for the next item (with carry-over
   remainder, see §4.1).
5. Fuel burn time decrements every tick (`TickFurnace(DeltaTime)`). When fuel
   runs out, smelting pauses until new fuel is added.
6. The furnace front texture swaps to `furnace_front_on` (slice 20) while burning
   and back to `furnace_front` (slice 19) when idle.

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
Cooked Meat, Spider Eye → Fermented Spider Eye). The block-to-block cases above
remain the default rows.

**UI:**
- Three slots: Input (top), Fuel (bottom-left), Output (bottom-right).
- Progress arrow between Input and Output fills left-to-right as
  `FurnaceSmeltProgress` advances.
- Fuel flame icon next to the Fuel slot drains as
  `FurnaceBurnTimeRemaining / FurnaceTotalBurnTime` decreases.
- The furnace front texture (`furnace_front_on`, slice 20) is shown in-world while
  burning, providing a visual cue even when the panel is closed.
- Slot icons resolve via `AChunkActor::GetBlockIconTexture(BlockID)` for block
  items.

### 3.4 Fuels

| Fuel | Block ID | Burn duration | Items smelted (at 8 s/item) | Notes |
|---|---|---|---|---|
| Coal / Charcoal | 16, 27 | 80.0 s | 10.0 | Phase-2 staple; best early fuel |
| Wood Log | 5 | 15.0 s | 1.875 (~2 partial) | Renewable; also smeltable to charcoal |
| Wood Planks | 6 | 10.0 s | 1.25 (~1) | Emergency fuel; wasteful |
| (Future) Biofuel | — | 120 000 EU equivalent | — | Phase-3; see `design/gdd/light-and-power.md` §3.4 |

- `IsValidFuel(BlockID)` returns `GetFuelBurnDuration(BlockID) > 0.0f`.
- A fuel unit is consumed **only when the furnace needs to start or continue
  burning** and `FurnaceBurnTimeRemaining ≤ 0`. If the furnace is already
  burning, the fuel slot is not touched until the current burn expires.
- **Fuel is not consumed if the output slot is full.** The furnace waits (burn
  timer paused) until the player collects the output. This prevents wasting fuel
  on a blocked output.
- `GetFuelBurnDuration(BlockID)`:
  - Block ID 16 (Coal Block) → 80.0 s
  - Block ID 27 (Coal Ore) → 80.0 s
  - Block ID 5 (Wood Log) → 15.0 s
  - Block ID 6 (Wood Planks) → 10.0 s
  - All others → 0.0 s (not a fuel)

### 3.5 Delegates & integration

`AWorldGenerator` broadcasts the following delegates, consumed by the UI layer:

- `OnCraftingTableOpened` — broadcast when the player right-clicks a Crafting
  Table within 4 blocks.
- `OnFurnaceOpened` — broadcast when the player right-clicks a Furnace within 4
  blocks.

The UI layer (Blueprint widgets `WBP_CraftingTable` / `WBP_Furnace`) binds to
these delegates to open the corresponding panel. The UI never owns game state; it
reads from `UCraftingSmeltingSystem` and sends intent (place item, take result)
via delegates.

The furnace tick (`TickFurnace(DeltaTime)`) is driven by `UCraftingSmeltingSystem`
and advances burn time and smelt progress. The in-world furnace mesh updates its
front face slice (19 ↔ 20) based on `FurnaceBurnTimeRemaining > 0`.

---

## 4. Formulas

### 4.1 Fuel burn & smelting efficiency

Let `CookTime` = time to smelt one input item (default 8.0 s, tunable per
recipe).

```
ItemsPerFuel = Floor(FuelBurnDuration / CookTime)
FuelBurnDuration values:
    Coal/Charcoal: 80.0 s  →  80 / 8 = 10.0 items
    Wood Log:      15.0 s  →  15 / 8 = 1.875 → 1 full item + 0.875 partial
    Wood Planks:   10.0 s  →  10 / 8 = 1.25   → 1 full item + 0.25 partial
```

Partial burn time carries over: if a log burns for 15 s and one item takes 8 s,
the remaining 7 s of burn time applies to the next item (no fuel wasted on item
boundaries).

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

A log is worth more as fuel than as 4 planks-as-fuel (1.875 vs 4 × 1.25 = 5.0
items), but planks are the emergency option when no coal or logs are available.

### 4.3 Recipe matching (3×3 Crafting Table)

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

A single furnace with coal can smelt 450 items per hour — enough for early game
but a bottleneck for large builds, incentivizing multiple furnaces.

### 4.6 Furnace burn-time conversion helpers

```
GetFuelBurnDuration(BlockID):
    16 → 80.0 s   (Coal Block)
    27 → 80.0 s   (Coal Ore)
     5 → 15.0 s   (Wood Log)
     6 → 10.0 s   (Wood Planks)
     * → 0.0 s    (not a fuel)

IsValidFuel(BlockID) = GetFuelBurnDuration(BlockID) > 0.0f
```

---

## 5. Edge Cases

| Case | Resolution |
|---|---|
| Player right-clicks a Crafting Table from > 4 blocks away | No-op; no panel, no delegate; player must approach |
| Player right-clicks a non-station block (dirt, stone) | No interaction; standard block placement/mining rules apply |
| Two panels opened in sequence | Previous panel closes and commits any in-flight drag as cancel; new panel opens |
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
| Crafting a tool when the Baubles slot is occupied | The crafted tool is added to the inventory as an item, not auto-equipped. The player equips it manually via the Baubles UI |
| Smelting a non-block item (target schema) | Supported via `FItemRef`; the furnace UI filters valid inputs based on `GetSmeltResult` |
| Coal Ore (27) used as fuel | Treated as fuel (80 s) — matches Coal Block (16). Coal Ore is not in the smelt recipe table; only Coal Block (16) is produced by smelting logs. Coal Ore is mined directly |
| Furnace front texture desync | The in-world furnace mesh updates its front face slice (19 ↔ 20) based on `FurnaceBurnTimeRemaining > 0`, polled on the next mesh rebuild or via a material parameter |
| Player dies while a panel is open | Panel closes; cursor hidden; any uncollected output stays in the slot |
| Multiplayer: second player opens the same furnace | Server-authoritative; both can browse; slot transfers are server-validated (last write wins) |

---

## 6. Dependencies

| System | Why | Status |
|---|---|---|
| `UCraftingSmeltingSystem` | Recipe validation, fuel burn, smelting progress, `TickFurnace` | Implemented (block-only, no ingredients) |
| `AWorldGenerator::InteractWithTargetBlock` | Proximity right-click opens crafting/furnace panels | Implemented |
| `AWorldGenerator::OnCraftingTableOpened` / `OnFurnaceOpened` | Delegates consumed by the UI layer | Implemented |
| `UBaublesSystem` | Tool crafting output, repair target, durability | Implemented |
| `FToolInstance` / `DT_Tools` | Tool data, durability, tier stats | Migrating (ADR-0007) |
| `UQuickSlotsInventorySystem` | Inventory storage, stacking, slot refresh | Implemented (9 slots) |
| `AC_Inventory` (Blueprint) | Main inventory UI, 27 slots, drag-and-drop | Exists, undocumented in C++ |
| `AChunkActor::GetBlockIconTexture` | Slot icon resolution | Implemented |
| `design/gdd/crafting-smelting.md` | Recipe catalogues, hand 2×2 grid, matching engine | Draft |
| `design/gdd/items.md` | `FItemRef`, `FCraftRecipe`, `DT_Recipes`, item catalogue | Draft |
| `design/gdd/weapons-tools.md` | Tool tiers, durability, repair rules | Draft |
| `design/gdd/inventory-quickslots.md` | Storage, stacking, drop/pickup, containers | Draft |
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
| `CraftingGridSize_Table` | 3×3 | Fixed (Minecraft parity) | Full recipe set |
| `FurnaceProximityRange` | 4 blocks (400 UU) | 2–6 blocks | Interaction distance |
| `ShapeBasedPriority` | true | true/false | Whether shaped recipes shadow shapeless |
| `AllowRotations` | false | true/false | Whether recipe shapes can be rotated (v1: no) |
| `AllowMirrors` | false | true/false | Whether recipe shapes can be mirrored (v1: no) |
| `FurnaceFrontSliceIdle` | 19 | Fixed (texture atlas) | In-world idle front texture slice |
| `FurnaceFrontSliceBurning` | 20 | Fixed (texture atlas) | In-world burning front texture slice |

---

## 8. Acceptance Criteria

1. Right-clicking a Crafting Table within 4 blocks opens the 3×3 crafting panel (`WBP_CraftingTable`).
2. Right-clicking a Furnace within 4 blocks opens the smelting panel (`WBP_Furnace`).
3. Right-clicking either station from beyond 4 blocks is a no-op (no panel, no delegate).
4. `OnCraftingTableOpened` / `OnFurnaceOpened` fire exactly once per valid interaction.
5. A 3×3 table grid correctly matches shape-based recipes (pickaxe, shovel, axe, hoe, sword).
6. A 3×3 table grid correctly matches shapeless recipes (planks, sticks, torches).
7. A shape-based recipe does not match if extra items are present in the pattern footprint.
8. A shapeless recipe matches regardless of item position.
9. Crafting consumes exactly the listed ingredients and produces the correct output quantity.
10. Surplus ingredients remain in the grid after crafting.
11. Smelting Iron Ore with Coal produces Iron Ingot after 8.0 s.
12. Coal (80 s) smelts exactly 10 items at 8 s/item with no wasted burn time (carry-over).
13. A Wood Log (15 s) smelts 1 full item + 7 s of partial progress toward the next.
14. A Furnace with a full output slot pauses smelting without consuming fuel.
15. Removing fuel from a burning furnace stops the burn timer and preserves partial progress.
16. `IsValidFuel` returns true for Block IDs 16, 27, 5, 6 and false for all others.
17. `GetSmeltResult` returns the correct output for inputs 7, 9, 5, 18, 3 and 0 for all others.
18. Repairing a tool at a crafting table restores 25 % of `MaxDurability` per material unit.
19. A fully broken tool requires 4 material units for a full repair.
20. The furnace front texture swaps to `furnace_front_on` while burning and back when idle.
21. The progress arrow in `WBP_Furnace` fills left-to-right as `FurnaceSmeltProgress` advances.
22. The fuel flame icon drains as `FurnaceBurnTimeRemaining / FurnaceTotalBurnTime` decreases.
23. All recipe definitions resolve their ingredient and output IDs at cook time; a dangling ID fails the build.
24. An automation test covers: shape-based matching, shapeless matching, priority ordering, fuel burn carry-over, and output-slot blocking.