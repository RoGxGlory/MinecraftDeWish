# GDD — Inventory & Quick Slots

| Field | Value |
|---|---|
| Status | Draft (2026-11-11) |
| System owner | game-designer / gameplay-programmer / ux-designer |
| Related | `Docs/SYSTEMS_REFERENCE.md` §3, §7, `Docs/GAME_DESIGN.md` §7, `design/gdd/items.md`, `design/gdd/items-loot.md`, `design/gdd/baubles-equipment.md`, `design/gdd/weapons-tools.md`, `design/gdd/crafting-smelting.md`, `design/gdd/save-load.md` |
| Source of truth for current code | `QuickSlotsInventorySystem.h/.cpp`, `BlockItemPickup.h/.cpp`, `PlayerVitalSystem.h`, `BaublesSystem.h`, `WorldGenerator.cpp` (`BreakBlockAtVoxel`, `InteractWithTargetBlock`), `AC_Inventory` (Blueprint) |
| Decisions | ADR-0001 (voxel scale 100 UU = 1 block), ADR-0005 (custom action layer), ADR-0007 (DT_Tools data model), ADR-0016 (day/night & weather) |

---

## 1. Overview

MinecraftDeWish uses a **two-tier inventory model** that splits permanent equipment
(Baubles) from consumable storage (Quick Slots + Main Inventory). The
`UQuickSlotsInventorySystem` is the single authority for the latter: a 9-slot
**quickbar** (always visible at the bottom of the HUD) plus a 27-slot **main
inventory** (opened with `Tab`), for a Minecraft-classic total of **36 slots**.
Every slot holds a stack of items identified by an `FItemRef` (block item, non-block
item, or — in the quickbar — a placeable block), capped at `MaxStackSize = 64`.

Quick slots are the **interaction surface** for the game: the selected slot drives
block placement, the drop key (`Q` / `IA_DropItem`), and the HUD preview. The main
inventory is the **storage overflow**: items flow into it when the quickbar is full,
and it is the only place where container panels (chest, crate, barrel, backpack)
can be browsed, sorted and transferred. Tools never live here — they live in
`UBaublesSystem` (see `design/gdd/baubles-equipment.md`).

The system also owns the **physical drop-and-pickup loop**: dropping a stack spawns
an `ABlockItemPickup` that arcs 2.5 blocks forward under gravity, settles, and
after a 1.2 s cooldown pulls toward the player within a 250 cm magnet radius.
Empty slots are not rendered (dynamic opacity `0.0` / hidden) so the quickbar
stays clean; non-empty slots render at full opacity. The system tracks the
mouse-hovered slot so that, when an inventory or container panel is open, the
drop key operates on the hovered slot instead of the selected quickslot.

This document is the design source of truth for **storage layout, slot selection,
stacking, drop, pickup, container interaction, hover tracking and opacity rules**.
Item taxonomy, loot tables, durability and repair are specified in
`design/gdd/items.md` and `design/gdd/items-loot.md`; crafting in
`design/gdd/crafting-smelting.md`; save persistence in
`design/gdd/save-load.md`.

---

## 2. Player Fantasy

> *"My hotbar is what I'm about to do. My inventory is what I've already decided to keep."*

The inventory fantasy is **spatial memory and deliberate scarcity**, not a
spreadsheet:

- **The hotbar is muscle memory.** Nine slots, always visible, always in the same
  order. The player selects by wheel or number without looking, the way a pianist
  finds keys. Empty slots vanish so the bar never lies about what is available.
- **The inventory is a decision, not a dump.** 36 slots, stacks of 64, no weight —
  but a full pack means choosing what to leave behind. The horror pacing depends
  on the player never feeling safe enough to hoard everything.
- **Dropping feels physical.** An item arcs out of the hand, tumbles under
  gravity, bounces, settles, and only then becomes recoverable. The world has
  weight; you cannot spam-drop to teleport items.
- **Pickup feels earned.** A 1.2 s cooldown means a dropped item cannot be
  re-absorbed mid-arc; the magnet pull at 250 cm is a reward for walking back,
  not a vacuum. Merging keeps the ground tidy without erasing the player's choice
  to drop.
- **Containers are places, not menus.** Opening a chest, crate, barrel or
  backpack is a spatial act: the player stands at the block, the panel opens, and
  `Q` drops from whatever the cursor rests on. Hover tracking makes the panel
  feel like a physical surface.
- **Nothing is wasted.** A dropped stack can be recovered; a partial fit leaves
  the remainder on the ground; a full inventory never silently deletes an item.

---

## 3. Detailed Rules

### 3.1 Storage layout

Two contiguous storage regions, owned by `UQuickSlotsInventorySystem`:

| Region | Slots | Indices (internal) | Purpose |
|---|---|---|---|
| Quick slots (hotbar) | 9 | `0..8` | Always-visible bar; selected slot drives placement, drop and HUD preview |
| Main inventory | 27 | `9..35` | Opened with `Tab`; overflow storage; container browsing/transfer |
| **Total** | **36** | `0..35` | Minecraft-classic (user decision, `items.md` §3.4) |

- `NUM_QUICK_SLOTS = 9`. Slot count is **fixed** — it is not a tuning knob.
- Main-slot count is **fixed at 27** (3 rows × 9 columns). Changing it breaks
  save data and UI layout and requires an ADR.
- Tools are **never** stored here. `UBaublesSystem` owns the 5 tool slots; the
  two systems are sibling components on the player pawn
  (`baubles-equipment.md` §3.8).
- Block items and non-block items share the same slot model; identity is carried
  by `FItemRef` (`Kind`, `BlockID`, `ItemId`). `FQuickSlotData.BlockID` is
  retained for block items so the existing widget/icon path keeps working during
  migration (`items-loot.md` §3.7).

```cpp
USTRUCT(BlueprintType)
struct FQuickSlotData
{
    FItemRef Ref;          // unified identity (Block / Item / Tool)
    uint8   BlockID = 0;  // legacy, populated for Kind == Block
    int32   ItemCount = 0;
    int32   MaxStackSize = 64; // per-slot override of the global default
};
```

### 3.2 Quick slot selection

The selected quick slot is the **single source of truth** for "what the player is
holding" in gameplay. Selection rules:

- **Mouse wheel** scrolls the selection by ±1, wrapping around
  (`SelectedID` 0 → 8 → 0). Scroll direction follows the wheel's natural
  direction (up = previous slot, down = next slot — matches Minecraft).
- **Number keys 1–9** select slot `0..8` directly, with no wraparound and no
  animation delay.
- **Selection is immediate**: `SelectedID` updates on the input event; the HUD
  highlight moves on the next frame. No commit delay.
- **Empty selection is allowed**: selecting an empty slot is valid and simply
  means "hand only" (no block to place, no item to drop). The HUD still
  highlights the empty slot so the player knows what is selected.
- **Selection persists** across inventory open/close and across container
  interactions. Closing the inventory does not reset `SelectedID`.
- Selection is **not** affected by Baubles auto-tool selection (tools live in a
  separate system). The active tool is chosen by `UBaublesSystem` from the
  targeted block; the selected quick slot is chosen by the player.
- When a selected slot becomes empty (e.g. last item dropped), `SelectedID` is
  **not** auto-advanced. The player keeps the empty selection until they move it.
  (Auto-advance would surprise the player mid-combat.)

`SelectedID` is 1-based in the existing code (`SelectedID - 1` → slot index);
the GDD uses 0-based indices and notes the offset explicitly.

### 3.3 Inventory panel (Tab)

- Pressing **Tab** (`IA_Inventory`) toggles the main inventory panel
  (`WBP_Inventory_Root` or `AC_Inventory`). The game is **not** paused while it
  is open (consistent with the Baubles panel — horror pacing, the player stays
  observable by Angels).
- While the panel is open, the mouse cursor is visible and the camera look is
  either disabled or slowed (UX decision; default: cursor free, look disabled).
- The panel renders all 27 main slots in a 3 × 9 grid, plus a read-only mirror of
  the 9 quick slots at the top (so the player can move items between the two
  regions by drag-and-drop).
- **Drag and drop**: an item stack can be dragged from any slot to any other slot.
  - Drop onto an empty slot: move the whole stack.
  - Drop onto a slot with the same `FItemRef`: merge up to `MaxStackSize`;
    overflow stays in the source slot.
  - Drop onto a slot with a different `FItemRef`: swap the two stacks.
  - Dragging outside the panel (or pressing `Esc`) cancels the drag and returns
    the stack to its origin.
- **Shift-click** (optional UX, deferred to the inventory UI spec) moves a whole
  stack to the other region in one action. v1 ships drag-and-drop only.
- Closing the panel (Tab again or Esc) commits any in-flight drag as a cancel
  and hides the cursor.

### 3.4 Stacking

- Default `MaxStackSize = 64` for both block and non-block items
  (`items-loot.md` §3.4, §4.2).
- Per-item override via `UItemDefinition.MaxStackSize`; tools use `1` (but tools
  never enter this inventory as stacks — they live in Baubles).
- A stack is identified by `(FItemRef, durability state)`. Two stacks with the
  same `FItemRef` but different remaining durability **do not merge** (edge case
  in `items-loot.md` §5). Block and non-block items carry no durability, so they
  always merge by identity.
- Stacking is the responsibility of `TryAddItemToPlayerInventory` (§3.7) and of
  the merge pass in `ABlockItemPickup` (§3.6).
- `FQuickSlotData.MaxStackSize` is a per-slot override of the global default; it
  is populated from `UItemDefinition` on insert and validated at cook time
  (`ue-data.md`).

### 3.5 Drop (`IA_DropItem`)

`DropItemFromInventory(AActor* Player, int32 SlotIndexOverride)` (SYSTEMS_REFERENCE §3):

- **Gameplay mode** (no panel open, cursor hidden): drops **exactly 1 item** from
  the currently selected quickslot (`SelectedID - 1`). If that slot is empty, the
  drop is a no-op (no pickup spawned, no animation).
- **Inventory / container mode** (panel open, cursor visible, Baubles open, or
  chest/crate/barrel/backpack open): drops **exactly 1 item** from the
  mouse-hovered slot (`GetHoveredSlotIndex()`). If no slot is hovered, the drop
  is a no-op.
- Decrements the source stack by 1; clears the slot when it reaches 0.
- Spawns an `ABlockItemPickup` at the player's drop origin and launches it with
  the forward velocity vector (§4.3). The pickup carries the dropped `FItemRef`
  and `Count = 1`.
- Dropping a whole stack (shift-drop) is **deferred** to the inventory UI spec;
  v1 drops one unit per press. This keeps the drop key predictable and prevents
  accidental full-stack loss.
- Dropping from a Baubles slot is **never** allowed: `IA_DropItem` in inventory
  mode reads only quick/main slots, never Baubles slots
  (`baubles-equipment.md` §3.4). A tool is unequipped via the Baubles panel, not
  dropped as a block.
- Dropping while the selected slot is empty, or while no slot is hovered in panel
  mode, is a silent no-op (no error, no sound). This avoids spamming the player
  with feedback on a missed keypress.

### 3.6 Pickup (magnet, cooldown, launch, merge)

`ABlockItemPickup` is the single world representation of a dropped item for both
families (block and non-block). Full entity spec in `items-loot.md` §3.11 and
SYSTEMS_REFERENCE §4.

- **Visuals:** procedural mini-voxel cube (25 cm) for block items, with texture
  coordinates matching the dropped block's Texture2DArray slice. Non-block items
  use a shared generic pickup mesh (25 cm cube with the item's icon applied as a
  material parameter) until the render question is resolved
  (`items-loot.md` §3.12). Either path satisfies the acceptance criteria.
- **Launch:** `LaunchPickup(const FVector& InVelocity, float InCooldown = 1.2f)`
  launches the pickup with an initial 3D velocity, gravity `1200 cm/s²`, air drag,
  and floor sweep collision.
- **Cooldown:** `PickupCooldown` (default 1.2 s) prevents the player from
  instantly re-picking a dropped item while it is still in mid-air or on the
  floor next to them. During cooldown the pickup **cannot** be picked up, **cannot**
  merge, and **cannot** be magnet-pulled.
- **Magnet:** after the cooldown expires and the pickup has settled
  (`bIsLaunched == false`), pickups within `MagnetRadius = 250 cm` are pulled
  toward the player with `MagnetAccel = 800 cm/s²`, capped at
  `MagnetMaxSpeed = 600 cm/s`. `AXPOrbPickup` uses the same pattern at 3.5 m for
  XP orbs; items use the shorter 2.5 m radius to keep the "arc and settle" feel
  (`items-loot.md` §3.5).
- **Pickup on contact:** after cooldown, when the pickup overlaps the player, the
  pickup flow runs:
  1. Resolve `FItemRef` from the pickup.
  2. Call `TryAddItemToPlayerInventory(Player, Ref, Count, OutRemaining)`.
  3. If `OutRemaining == 0`: destroy the pickup.
  4. If `OutRemaining > 0`: reduce the pickup's `Count` to `OutRemaining` and
     leave it on the ground (partial fit).
- **Merges:** a pickup merges with another pickup of the same identity when their
  centres are within `MergeRadius = 100 cm` and both are settled (not being
  launched). Merge is attempted on spawn and on a 0.5 s timer while settled.
  Merged count is clamped to `MaxStackSize` per pickup; overflow spawns a new
  pickup at the same location. Merge never crosses the cooldown boundary.
- **Replication (if multiplayer):** `Count` and `ItemRef` are replicated; physics
  is client-predicted cosmetic, server-authoritative pickup
  (`ue-network.md`).

### 3.7 Inventory insertion order

`TryAddItemToPlayerInventory(AActor* Player, FItemRef Ref, int32 Count,
int32& OutRemainingCount)`:

1. **Pass 1 (fill):** for each slot in `[quickslots 0..8, main 0..26]`:
   if `slot.Ref == Incoming.Ref` and `slot.Count < MaxStackSize`: take =
   `min(Incoming, MaxStackSize - slot.Count)`; `slot.Count += take`;
   `Incoming -= take`.
2. **Pass 2 (empty):** for each slot in `[quickslots 0..8, main 0..26]`:
   if the slot is empty: `slot.Ref = Incoming.Ref`;
   `slot.Count = min(Incoming, MaxStackSize)`; `Incoming -= slot.Count`;
   if `Incoming == 0`: break.
3. If no slot can accept the remainder, set `OutRemainingCount = Incoming` and
   leave the item on the ground (the pickup is **not** consumed).

Rules:

- Quick slots are filled **before** main slots in both passes. This keeps the
  hotbar useful (the player picks up into the bar first) and makes the insertion
  order deterministic and testable.
- Insertion **never** overwrites a non-empty slot with a different identity. A
  slot holding Dirt is never replaced by Stone; Stone goes to the next empty slot
  or stays on the ground.
- Partial fits are reported via `OutRemainingCount > 0`; the caller (pickup flow
  or loot spawner) is responsible for leaving the remainder on the ground or
  spawning an overflow pickup.
- The function updates the HUD/widget for every touched slot (quick slots via
  `RefreshQuickSlotVisual`, main slots via the inventory panel's refresh binding).
- `TryAddItemToPlayerInventory` is the **only** sanctioned insertion path. Direct
  slot writes (`SetQuickSlotData`) exist for migration, save/load and tests, but
  gameplay code must go through the function so stacking and ordering rules hold.

### 3.8 Containers (chest, crate, barrel, backpack)

Containers are **placeable blocks** that open a panel when right-clicked within
4 blocks (`AWorldGenerator::InteractWithTargetBlock`). Each container type has a
fixed slot count:

| Container | Block ID (per `BLOCK_REGISTRY`) | Slots | Notes |
|---|---|---|---|
| Chest | `Block.Chest` | 27 (3 × 9) | Standard storage; two adjacent chests form a double chest (54 slots) — deferred to v2 |
| Crate | `Block.Crate` | 9 (1 × 9) | Portable-ish storage; same footprint as a block, smaller capacity |
| Barrel | `Block.Barrel` | 27 (3 × 9) | Vertical storage; same capacity as a chest, different silhouette |
| Backpack | `Block.Backpack` (item, not placeable) | 9 (1 × 9) | Carried in the inventory; opens as a panel without placing a block — deferred to v2 |

Rules:

- Opening a container fires `OnContainerOpened(AActor* Container, int32 SlotCount)`
  and displays `WBP_Container_Root` with the container's slots on the left and the
  player's inventory (quick + main) on the right.
- **Drag and drop** works between the container and the player inventory with the
  same rules as §3.3 (move, merge, swap). Items never teleport; they are moved
  slot-by-slot so a full destination leaves the remainder in the source.
- **`IA_DropItem` in container mode** drops 1 item from the mouse-hovered slot
  (whether that slot is in the container or in the player inventory). The drop
  uses the same `ABlockItemPickup` launch as §3.5, spawned at the player's drop
  origin (not at the container).
- **Container state is persisted** with the world: a container's slot contents
  are saved as part of the chunk/block save data (`save-load.md`). A broken
  container (mined) **drops its contents** as pickups at the container's location
  before the container block is removed, so items are never silently lost.
- **Locking / ownership:** v1 has no locks, no ownership, no "only the opener
  can access". Any player who right-clicks an open container can browse it.
  Locks are deferred to the NPC/colony system.
- **Container capacity is fixed** and not a tuning knob. Changing it breaks save
  data and requires an ADR + migration.
- **Hover tracking** is used to make `IA_DropItem` context-aware: when the cursor
  rests on a container slot, dropping operates on that slot; when it rests on a
  player slot, on that slot. See §3.9.

### 3.9 Hover slot tracking

`UQuickSlotsInventorySystem` tracks the mouse-hovered slot so that container and
inventory panels can route `IA_DropItem` to the correct source:

- `SetHoveredInventorySlot(int32 SlotIndex, uint8 BlockID, UObject* SourceContainer)`:
  called by the UI widget when the cursor enters a slot. `SlotIndex` is the slot's
  index within its owning panel (quick/main/container). `SourceContainer` is the
  owning container actor or `nullptr` for the player inventory.
- `ClearHoveredInventorySlot()`: called when the cursor leaves all slots or the
  panel closes.
- `GetHoveredSlotIndex()`: returns the hovered slot index, or `-1` if none.
- `IsInInventoryPanel(AActor* PlayerActor)`: returns true when an inventory or
  container panel is open and the cursor is free. Used by `DropItemFromInventory`
  to choose between gameplay mode and panel mode.
- **Hover is updated on `MouseMove` / widget hover events**, not every frame, to
  avoid polling. The UI binds `OnHovered` / `OnUnhovered` on each slot widget.
- **Hover is cleared on panel close**, on drag start (the dragged stack is no
  longer "hovered"), and on `IA_DropItem` consumption (so a held key does not
  drop repeatedly from the same slot — each press consumes one hover and the UI
  re-sets it on the next hover event).
- A hovered slot index of `-1` means "no slot"; `DropItemFromInventory` treats
  this as a no-op in panel mode.

### 3.10 Dynamic opacity

Quick slots reflect item presence through opacity and visibility so the bar never
lies about what is available (SYSTEMS_REFERENCE §3, GAME_DESIGN §7.1):

- **Item present** (`ItemCount > 0`): `SetRenderOpacity(1.0f)` and
  `SetVisibility(Visible)`.
- **Empty** (`ItemCount == 0`): `SetRenderOpacity(0.0f)` and
  `SetVisibility(Hidden)`.
- `RefreshQuickSlotVisual(UUserWidget* QuickSlotsWidget, int32 SlotIndex, uint8 BlockID,
  int32 ItemCount, const FName& BlockName)` applies both the texture/name and the
  opacity rule whenever a slot changes.
- `UpdateSlotOpacity(UWidget* SlotWidget, bool bHasItem)` is the Blueprint-callable
  helper for designers who want to drive opacity from a Blueprint graph.
- `UpdateAllQuickSlotOpacities(UUserWidget* QuickSlotsWidget)` refreshes all 9
  quickslots at once (called on inventory open, on load, and on bulk changes).
- **Opacity is driven by item presence, not by selection.** The selected empty
  slot is still hidden; the selection highlight is a separate widget (border /
  glow) that remains visible even on an empty slot so the player can see what is
  selected. This separation is what keeps the bar honest.
- Main-inventory slots do **not** use the opacity rule. They are always visible
  (empty slots render as empty frames) because the inventory panel is a management
  surface, not a glanceable bar. Empty main slots render at full opacity with an
  empty-frame style.

---

## 4. Formulas

### 4.1 Slot indexing and selection

```
SelectedSlotIndex (0-based) = SelectedID - 1          // SelectedID is 1-based in code
WheelScroll:  NewIndex = (SelectedSlotIndex + Direction + 9) mod 9
              Direction = +1 (scroll down) or -1 (scroll up)
NumberKey n (1..9): SelectedSlotIndex = n - 1
```

### 4.2 Stacking

```
MaxStackSize(Item) = UItemDefinition.MaxStackSize   (default 64)
                    = 1 for tools (EItemKind::Tool, never stored here)
EffectiveFit(CurrentStack, Incoming) = min(Incoming, MaxStackSize - CurrentStack.Count)
```

### 4.3 Drop launch

```
Gravity             = 1200 cm/s²   (downward, Z-)
LaunchSpeedForward  = 250 cm/s     (initial, along player look + slight up)
LaunchSpeedUp       = 150 cm/s     (initial vertical)
AirDrag             = 0.98 per 0.1 s  (multiplicative)
FloorRestitution    = 0.3           (bounce on first contact, then settle)
SettleThreshold     = 5 cm/s        (velocity below this → bIsLaunched = false)
```

A dropped item launched forward 2.5 blocks (250 cm) at 250 cm/s forward +
150 cm/s up travels ~1.0 s before first floor contact, bounces once, and settles
within ~1.4 s — comfortably inside the 1.2 s cooldown window so it cannot be
re-picked mid-arc (`items-loot.md` §4.3).

Drop origin: the item is spawned at the player's drop point, offset
`DropForwardOffset = 50 cm` forward of the player's feet and `DropUpOffset = 80 cm`
above the floor, so it clears the player's collider and arcs onto the ground
2.5 blocks ahead. (Values are tuning knobs; see §7.)

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

### 4.6 Inventory insertion order

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

### 4.7 Container slot counts

```
ChestSlots   = 27   (3 rows × 9)
CrateSlots   = 9    (1 row × 9)
BarrelSlots  = 27   (3 rows × 9)
BackpackSlots = 9   (deferred to v2)
```

### 4.8 Pickup flow

```
on player overlap with pickup (after cooldown):
    OutRemaining = TryAddItemToPlayerInventory(Player, pickup.Ref, pickup.Count)
    if OutRemaining == 0:  Destroy(pickup)
    else:                  pickup.Count = OutRemaining   // partial fit, stays on ground
```

---

## 5. Edge Cases

| Case | Resolution |
|---|---|
| Selecting an empty quick slot | Valid; selection stays; HUD highlights the empty slot; placement and drop become no-ops until a non-empty slot is selected |
| Selected slot becomes empty (last item dropped) | `SelectedID` is **not** auto-advanced; the player keeps the empty selection |
| Wheel scroll past slot 8 | Wraps to slot 0 (and vice versa) |
| Number key 0 | No-op (no slot 0 in 1-based input; slot 9 is key 9) |
| Drop on empty selected slot (gameplay mode) | Silent no-op; no pickup spawned |
| Drop with no hovered slot (panel mode) | Silent no-op |
| Drop while Baubles panel is open | Drops from the hovered quick/main slot, never from a Baubles slot |
| Inventory full on pickup | Item stays on the ground; pickup not consumed; `OutRemainingCount > 0` |
| Partial stack fit | What fits is taken; remainder stays as a pickup with reduced `Count` |
| Two stacks of same identity, different durability | Both remain separate; no auto-merge (tools only; block/non-block items carry no durability) |
| Merge of a launched and a settled pickup | Not allowed; merge waits until both settle |
| Merge overflow | Overflow spawns a new pickup at the same location; counts never exceed `MaxStackSize` |
| Pickup cooldown vs magnet | Magnet never activates before `PickupCooldown` expires, even if settled earlier |
| Player dies near pickups | Pickups unaffected; corpse is a separate `FWorldItemRecord` |
| Two players (multiplayer) racing a pickup | Server-authoritative: first valid pickup RPC wins; the other gets `OutRemainingCount = Count` and the pickup stays |
| Container mined (broken) | Drops its contents as pickups at the container's location before the block is removed; no silent loss |
| Container opened by two players | Both browse; no locks in v1; last write wins on slot transfer (server-authoritative) |
| Drag cancelled (Esc / outside panel) | Stack returns to its origin slot; no state change |
| Drag onto a slot with a different identity | Swap the two stacks |
| Hover cleared on panel close | `ClearHoveredInventorySlot()` called; `GetHoveredSlotIndex()` returns `-1` |
| Hover during an in-flight drag | Hover is cleared on drag start; re-set on the next hover event after drop |
| Selected slot index out of range (save data) | Clamped to `[0, 8]` on load; logged as a migration warning |
| Legacy save with no `FItemRef` | Migration: `BlockID` → `FItemRef{Kind=Block, BlockID=BlockID}`; `ItemCount` preserved |
| Main-inventory slot count mismatch on load | Slots beyond the saved count are treated as empty; logged |
| Non-block item in a quick slot | Allowed (e.g. medicine in the hotbar); placement is a no-op for non-block items, drop still works |
| Block item selected but air targeted | Placement no-op; no error |
| Container with 0 items | Panel opens showing empty slots; no special case |

---

## 6. Dependencies

| System | Why | Status |
|---|---|---|
| `UQuickSlotsInventorySystem` | Storage, stacking, widget refresh, drop, hover tracking, opacity | Implemented, 9 quick slots, block-only; main slots via `AC_Inventory` |
| `ABlockItemPickup` | Pickup path, physics, cooldown, magnet, merge | Implemented for blocks only |
| `AC_Inventory` (Blueprint) | 27 main slots, drag-and-drop UI, container panels | Exists, undocumented in C++ |
| `WBP_QuickSlots` / `WBP_Inventory_Root` / `WBP_Container_Root` | HUD and panel widgets | Implemented (quick slots); inventory/container panels in progress |
| `FToolInstance` / `UBaublesSystem` | Tools are a separate, non-stacked family | Implemented |
| `UItemDefinition` / `DT_Items` | Item identity, `MaxStackSize`, icon, `PickupMesh` | Not implemented (planned, `items-loot.md` §9) |
| `FItemRef` / `FItemStack` | Unified item identity | Not implemented (planned) |
| `AWorldGenerator` | `BreakBlockAtVoxel` (block drops), `InteractWithTargetBlock` (container open), `PlaceBlock` (placement from selected slot) | Implemented |
| `AChunkActor` | Block icon resolution for slot textures | Implemented |
| `UCraftingSmeltingSystem` | Recipes consume items from inventory | Implemented, block-only, no ingredients |
| `AMobCharacter` / `UMobSpawnerSystem` | Loot drops on death (inserted via `TryAddItemToPlayerInventory`) | Implemented with block proxies |
| `UPlayerVitalSystem` | XP on kill/break (not on pickup) | Implemented |
| `AXPOrbPickup` | Reference pattern for magnet | Implemented |
| Save/load (`FWorldItemRecord`, container state) | Dropped items, corpses, container contents persist | Not implemented |
| `design/gdd/items.md` / `items-loot.md` | Item taxonomy, loot tables, stacking, pickup physics | Draft |
| `design/gdd/baubles-equipment.md` | Tool storage separation, drop routing while panel open | Draft |
| `design/gdd/crafting-smelting.md` | Crafting consumption from inventory | Draft |
| `design/gdd/save-load.md` | Persistence of slots and container state | Draft |

**Implementation gaps to close (flagged):**

- `AC_Inventory` (27 main slots) is Blueprint-only and undocumented in C++; its
  slot array, drag-and-drop and container panels need a C++ contract
  (`UInventoryComponent` or an extension of `UQuickSlotsInventorySystem`) so
  `TryAddItemToPlayerInventory` can address main slots uniformly.
- `DropItemFromInventory` currently drops from `SelectedID - 1` in gameplay mode
  and from the hovered slot in panel mode; the hover-tracking calls
  (`SetHoveredInventorySlot`, etc.) are specified but the UI binding is not yet
  wired.
- Dynamic opacity is implemented for quick slots only; main-slot and
  container-slot opacity rules (always visible) are a UI convention, not yet
  codified in a widget.
- Stacking beyond `BlockID` (non-block items) is not yet implemented; `FItemRef`
  and `FQuickSlotData.Ref` are planned (`items-loot.md` §9).

---

## 7. Tuning Knobs

| Knob | Default | Safe range | Effect |
|---|---|---|---|
| `NumQuickSlots` | 9 | **Fixed** | Hotbar width; changing breaks UI and save data |
| `NumMainSlots` | 27 | **Fixed** | Inventory size; changing breaks save data and layout |
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
| `DropForwardOffset` / `DropUpOffset` | 50 / 80 cm | ±25 % | Drop origin clearance |
| `SettleThreshold` | 5 cm/s | 2–10 cm/s | When a pickup becomes magnet-eligible |
| `ChestSlots` / `CrateSlots` / `BarrelSlots` | 27 / 9 / 27 | **Fixed** | Container capacity; changing breaks save data |
| `QuickSlotOpacityPresent` / `Empty` | 1.0 / 0.0 | 0.0–1.0 (present ≥ 0.5) | Bar legibility |
| `SelectionWrapOnWheel` | true | bool | Wraparound behaviour |
| `AutoAdvanceOnEmpty` | false | bool | Whether an emptied slot auto-advances selection |

---

## 8. Acceptance Criteria

1. The quickbar renders 9 slots; empty slots have opacity `0.0` and visibility
   `Hidden`; non-empty slots have opacity `1.0` and visibility `Visible`.
2. Mouse wheel scrolls the selection with wraparound (0 ↔ 8); number keys 1–9
   select directly.
3. Pressing `Tab` opens the 27-slot main inventory (3 × 9 grid) plus a
   read-only mirror of the 9 quick slots; the game is not paused.
4. Drag-and-drop moves, merges (same identity, up to 64) or swaps stacks between
   any two slots; cancelling returns the stack to its origin.
5. `IA_DropItem` (`Q`) in gameplay mode drops exactly 1 item from the selected
   quickslot; in panel/container mode it drops 1 item from the hovered slot.
6. A dropped item arcs ~2.5 blocks forward, bounces once, settles within ~1.4 s,
   and cannot be re-picked for 1.2 s.
7. After cooldown, a pickup within 250 cm is pulled toward the player (capped at
   600 cm/s) and picked up on contact.
8. Two pickups of the same identity within 100 cm merge into one stack (capped at
   64); a launched pickup never merges.
9. `TryAddItemToPlayerInventory` fills existing matching stacks (quick slots
   first, then main) before using empty slots, and reports the remainder via
   `OutRemainingCount`.
10. A full inventory leaves the item on the ground with a reduced count (partial
    fit); nothing is silently deleted.
11. Right-clicking a chest/crate/barrel within 4 blocks opens its panel; mining
    the block drops its contents as pickups before removal.
12. `SetHoveredInventorySlot` / `ClearHoveredInventorySlot` /
    `GetHoveredSlotIndex` / `IsInInventoryPanel` route `IA_DropItem` to the
    hovered slot in panel mode.
13. Insertion order is deterministic and covered by an automation test
    (`MinecraftDeWish.Inventory.InsertionOrder`).
14. Every `FItemRef` in every slot resolves at cook time; a dangling ID fails the
    build (`ue-data.md`).
15. A legacy save (block-only `BlockID`) loads with every slot mapped to
    `FItemRef{Kind=Block}` and no crash.
16. An automation test covers: wheel wraparound, number-key selection, empty-slot
    no-op drop, partial fit, merge radius, cooldown-vs-magnet, and container
    break-drops-contents.