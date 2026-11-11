# GDD — Baubles Equipment System

| Field | Value |
|---|---|
| Status | Draft (2026-11-11) |
| System owner | game-designer / gameplay-programmer / technical-artist |
| Related | `Docs/SYSTEMS_REFERENCE.md` §6–7, `Docs/TOOL_REGISTRY.md`, `design/gdd/weapons-tools.md`, `design/gdd/items.md`, `design/gdd/items-loot.md`, ADR-0005, ADR-0006, ADR-0007 |
| Source of truth for current code | `BaublesSystem.h/.cpp`, `ToolTypes.h`, `ToolData.h`, `EquippedToolActor.h`, `PlayerViewModel.h` |
| Decisions | ADR-0005 (custom action layer), ADR-0006 (static-mesh presentation), ADR-0007 (DT_Tools data model), ADR-0003 (crack overlay driver) |

---

## 1. Overview

MinecraftDeWish uses a **dedicated five-slot equipment model** (`UBaublesSystem`) that keeps
tools out of the hotbar. Each tool class owns a permanent slot — Pickaxe, Shovel, Axe, Hoe,
Sword — and the system **auto-selects** the right tool from the targeted block or mob category,
swapping the held viewmodel mesh without spawning or destroying actors. Tools are **StaticMesh
actors** (`AEquippedToolActor`) attached to the `weapon_r` socket of a first-person skeletal
arms mesh (`APlayerViewModel`).

All static tool data (tier multipliers, durability, damage, mesh/icon/montage references) lives
in the `DT_Tools` DataTable keyed by a stable `ToolId` (ADR-0007); `FToolInstance` keeps only
runtime state (`ToolId`, `ToolType`, `ToolTier`, `CurrentDurability`, `MaxDurability`, `InstanceId`).
A `UToolDefinition` DataAsset wraps `DT_Tools` and provides cached, validated lookups.

Two consumers share this system:

- **Mining** — `UMiningQueueSystem` + `AWorldGenerator` query `UBaublesSystem` for the best
  mining force and consume durability on a successful break; the crack overlay (ADR-0003) keys
  off the same force.
- **Combat** — `UCombatSubsystem::ApplyDamage` → `AMobCharacter::ApplyMobDamage`, driven by
  animation notifies, with knockback and hit reactions (see `design/gdd/weapons-tools.md` §3.5).

The Baubles panel is toggled with the **B** key (`IA_Baubles`), broadcasting `OnBaublesToggled`.
Auto-selection emits `OnActiveToolChanged(EToolType)` with a 175 ms anti-bounce to avoid flicker
at block boundaries. Delegates: `OnToolEquipped`, `OnToolUnequipped`, `OnBaublesToggled`,
`OnActiveToolChanged`.

This document is the design source of truth for the **equipment container** (slots, equip/unequip,
auto-selection, UI toggle, durability lifecycle, repair, viewmodel and quick-slot integration).
Combat, montages and notifies are specified in `design/gdd/weapons-tools.md`.

---

## 2. Player Fantasy

> *"The right tool for the right block, in the hand I expect, doing exactly what I predicted."*

The fantasy is **muscle memory and reliability**, not power fantasy. The player should feel that:

- **One glance, one tool.** Looking at stone means a pickaxe is in hand before the player
  thinks about it. No inventory fumbling, no "did I equip the right thing".
- **Nothing is wasted.** A broken tool is unusable but **not destroyed** — it can be repaired
  at a crafting table. A bad night costs materials and time, never a diamond pickaxe.
- **Tier is felt, not announced.** The upgrade reads through break speed and durability, never
  through a number. The Baubles panel shows a durability bar only, no digits.
- **Tools stay out of the way.** The hotbar holds blocks and materials; tools live in their
  own panel, so the player never scrolls past a pickaxe to place dirt.
- **Horror pacing is preserved.** Tools are survival infrastructure, not weapons of power.
  Even the best tier does not make an Angel fightable; it only makes the next trip shorter.

---

## 3. Detailed Rules

### 3.1 Equipment model (slots)

`UBaublesSystem` owns five `FToolInstance` slots:

| Slot | `EToolType` | Effective against |
|---|---|---|
| `PickaxeSlot` | Pickaxe | Stone, cobblestone, ores, furnace, masonry |
| `ShovelSlot` | Shovel | Dirt, grass block, sand, gravel |
| `AxeSlot` | Axe | Wood log, planks, crafting table, barrel, fence, door |
| `HoeSlot` | Hoe | Farmland, leaves, plant crops |
| `SwordSlot` | Sword | Hostile mobs (Zombie, Skeleton, Spider, Creeper) |

Rules:

- `EquipTool(const FToolInstance&)` routes the tool to its slot by `ToolType`. Re-equipping
  into a non-empty slot **replaces** the previous tool (the previous instance is returned to
  the caller / inventory).
- `UnequipTool(EToolType)` removes and returns the tool; the slot becomes empty (hand fallback).
- `GetToolInSlot(EToolType)` / `HasToolEquipped(EToolType)` expose read-only slot state.
- `EquipToolFromToolId(FName)` and `EquipToolFromTypeTier(EToolType, EToolTier)` resolve static
  data through `UToolDefinition` and equip a fresh instance at full durability.
- `AEquippedToolActor` is a **single instance, never spawned or destroyed**. It swaps its
  `UStaticMeshComponent` mesh on equip. Pivot at grip centre, handle along local +Z.
- First-person arms (`APlayerViewModel`, `USkeletalMeshComponent`, `bOnlyOwnerSee`) attach the
  tool to socket `weapon_r`.

### 3.2 Tool data model (`DT_Tools`)

Per ADR-0007, all static tool data lives in `DT_Tools` keyed by a stable `ToolId`
(`FName`, format `<Type>_<Tier>` e.g. `Pickaxe_Diamond`).

Columns (see `FToolRow`):

| Column | Type | Notes |
|---|---|---|
| `ToolType` | `EToolType` | Pickaxe / Shovel / Axe / Hoe / Sword |
| `ToolTier` | `EToolTier` | None / Wood / Stone / Iron / Diamond / Obsidian |
| `BaseMiningForce` | `float` | Tier multiplier (see §4.2) |
| `DurabilityMax` | `int32` | From `TOOL_REGISTRY` |
| `AttackDamage` | `float` | Sword total damage (fist + bonus) |
| `MeshPath` | soft ref | `UStaticMesh` |
| `IconPath` | soft ref | 32×32 Patrix icon |
| `SwingMontage` | soft ref | Per *class*, not per tier |
| `ImpactCue` | `FName` | VFX/SFX at impact frame |
| `MiningForceByBlockCategory` | `TMap<uint8, float>` | Specialisation multipliers (default 1.0) |

`FToolInstance` keeps only runtime state: `ToolId`, `ToolType`, `ToolTier`,
`CurrentDurability`, `MaxDurability` (cached), `InstanceId` (stable GUID for save data).
`UBaublesSystem` resolves through `UToolDefinition` with caching. No hardcoded paths or
multipliers in code (ADR-0007).

### 3.3 Auto-tool selection

When the targeted block (or mob) category changes, `UBaublesSystem` emits
`OnActiveToolChanged(EToolType)`. Mapping of target category to tool type:

| Target category | Tool type | Example blocks / entities |
|---|---|---|
| Stone / masonry / ores | Pickaxe | Stone, cobblestone, coal/iron/gold/diamond ore, furnace |
| Earth / granular | Shovel | Dirt, grass block, sand, gravel |
| Wood / carpentry | Axe | Wood log, planks, crafting table, barrel, fence, door |
| Plant / farmland | Hoe | Farmland, leaves, crops |
| Mob | Sword | Zombie, Skeleton, Spider, Creeper |
| Other / air / no target | (hold current) | Torch, air, water |

Rules:

- If the player owns a tool for that category, it is auto-selected; otherwise the hand
  (no tool) is used and no durability is consumed.
- **Anti-bounce:** a candidate change is committed only after
  `AutoSelectAntiBounceSeconds = 0.175 s` of stability. Changes occurring within that window
  are coalesced — the latest candidate wins after the full window elapses. This prevents
  flicker at block boundaries.
- `RequestAutoToolSelection(EToolType)` schedules the commit via a one-shot timer;
  `CommitAutoToolSelection()` forces an immediate commit; `CancelAutoToolSelection()` clears
  any pending candidate.
- Manual equip via the Baubles UI sets a short-lived override (`ManualOverrideMs = 500 ms`)
  during which auto-selection does not fire; it resumes on the next distinct target category
  after the override expires. *(Specified; not yet implemented — see §6.)*
- Air / no-target holds the current tool; it never auto-switches to hand.
- `GetActiveToolType()` returns the committed active tool; `GetActiveTool()` returns its
  `FToolInstance` (empty if None).
- Auto-selection never consumes durability and never queues a break; it only changes the
  held tool and broadcasts `OnActiveToolChanged`.

### 3.4 Toggle UI (B key)

- Pressing **B** (`IA_Baubles`) calls `ToggleBaublesUI()`, which flips `bIsBaublesOpen`
  and broadcasts `OnBaublesToggled(bool)`.
- The Baubles panel (`WBP_Baubles_Root`) displays the five slots with their icons and
  durability bars, and allows drag/drop equip and unequip.
- While the panel is open, `IA_DropItem` operates in **inventory mode**: it drops one item
  from the mouse-hovered slot (see `UQuickSlotsInventorySystem`) instead of the selected
  quickslot.
- Opening the panel does not pause the game; the player can still be observed by Angels, so
  the panel is a deliberate risk/reward action (horror pacing).
- The panel reads from `UBaublesSystem` (slot state, durability) and sends equip/unequip
  intent back via delegates; it owns no game state.

### 3.5 Durability

- Mining a block: `−1` durability on the **matching** tool (non-matching tools mine at hand
  speed and consume nothing).
- Hitting a mob with a sword: `−1` durability per swing (see `weapons-tools.md` §3.5 for
  the 1-total-per-swing decision).
- `ConsumeToolDurability(BlockID)` resolves the preferred tool type from the block, deducts
  1 from the matching slot, and returns whether the tool broke.
- **Broken state:** `CurrentDurability == 0`. The tool is unusable (treated as hand for
  mining/combat), auto-unequipped, and `OnToolUnequipped` is fired. The `FToolInstance` is
  **retained** (not destroyed) so it can be repaired. *(Implementation currently clears the
  slot on break; design requires retention — see §6.)*
- A broken tool equipped manually is treated as hand: no mining bonus, no combat bonus,
  no durability consumed.
- Durability is never shown as a number in the HUD; only a bar on the equipped tool and in
  the Baubles panel.

### 3.6 Repair

- Repair happens at a **crafting table**: tool + material unit → `+25 %` of `MaxDurability`
  per unit, capped at `MaxDurability`.
- Material per tier: Wood → planks, Stone → cobblestone, Iron → iron ingot, Diamond →
  diamond, Obsidian → obsidian (or netherite scrap).
- Units needed: `ceil((MaxDurability − CurrentDurability) / (MaxDurability × 0.25))`.
- A fully broken tool needs 4 units.
- Repair at max durability is a no-op; the UI disables the repair action.
- Repair restores the tool to its slot with the new durability; `OnToolEquipped` is fired.

### 3.7 Viewmodel integration

- `AEquippedToolActor` holds one `UStaticMeshComponent`; on equip it swaps the mesh to the
  new tool's `MeshPath` (soft reference, loaded on demand). No spawn/destroy, no equip hitch.
- The tool attaches to socket `weapon_r` of `APlayerViewModel`'s skeletal arms.
- `APlayerViewModel` is a dedicated `USkeletalMeshComponent` with `bOnlyOwnerSee`, attached
  to the camera. It drives the 5 per-class montages (Windup / Impact / Recovery) and the
  four notifies (`AnimNotify_ActionWindow`, `AnimNotify_PlayCue`,
  `AnimNotify_ActionComplete`, `AnimNotifyState_MiningLoop`).
- `ToolDefinition` is shared between `UBaublesSystem` and `APlayerViewModel` so mesh, icon
  and montage resolve consistently.
- Layered blend on `spine_01` via `LayeredBlendPerBone` keeps locomotion running under a
  swing.
- Mining cadence is notify-driven: `QueueBlockBreakAtVoxel` is called from the impact notify,
  so the crack overlay and durability consumption cannot desync from the visual (ADR-0006 §6).

### 3.8 Quick slots integration

- Tools are **not** stored in quick slots. Quick slots (`UQuickSlotsInventorySystem`, 9 slots)
  hold blocks and items; Baubles holds tools. The two systems are independent storage.
- `UBaublesSystem` and `UQuickSlotsInventorySystem` are sibling components on the player pawn;
  gameplay code queries both (mining queries Baubles for force, placement queries quick slots
  for the selected block).
- When the Baubles panel is open, `IA_DropItem` switches to inventory mode and drops from
  the hovered quick/main slot, so a tool can never be accidentally dropped as a block.
- Quick-slot opacity rules (present → 1.0 / visible, empty → 0.0 / hidden) are unaffected
  by Baubles state.
- The active tool (from Baubles) determines `MaxQueueSlots` for `UMiningQueueSystem`,
  tier-gating concurrent mining.

---

## 4. Formulas

### 4.1 Auto-selection timing

```
if (Candidate == ActiveToolType):
    CancelAutoToolSelection()          // no-op change
else if (Pending == Candidate):
    // timer already running, do nothing
else:
    Pending = Candidate
    start one-shot timer for AutoSelectAntiBounceSeconds
    // latest candidate replaces pending; timer resets

on timer fire:
    if (Pending != ActiveToolType):
        ActiveToolType = Pending
        broadcast OnActiveToolChanged(ActiveToolType)
    Pending = None
```

```
Effective commitment delay ≥ AutoSelectAntiBounceSeconds (175 ms) from the last
distinct candidate.
```

### 4.2 Mining force

```
EffectiveMiningForce = BaseMiningForce(ToolTier) × MiningForceByBlockCategory[BlockCategory]
                     if a matching, non-broken tool is equipped for the block's category
                   = 1.0   otherwise (hand speed; no durability consumed)

BreakTime (s) = BlockDurability / EffectiveMiningForce
```

Hand force = 1.0. Tier base forces: Wood 2.0, Stone 4.0, Iron 6.0, Diamond 10.0,
Obsidian 15.0. Non-matching equipped tool → 1.0 and no durability. See
`design/gdd/weapons-tools.md` §4.2 for the full tier table and examples.

### 4.3 Durability consumption

```
OnBreak (matching tool):  CurrentDurability -= 1
OnHit  (sword swing):     CurrentDurability -= 1   (1 total per swing, not per mob)
Broken:                   CurrentDurability == 0  → unusable, auto-unequip, retain instance
ConsumeToolDurability returns true iff the tool broke on this use.
```

Non-matching tools and hand: no consumption.

### 4.4 Repair cost

```
RepairAmountPerUnit = ceil(MaxDurability × RepairFractionPerUnit)     // RepairFractionPerUnit = 0.25
CurrentDurability   = min(MaxDurability, CurrentDurability + RepairAmountPerUnit)
UnitsNeeded        = ceil((MaxDurability − CurrentDurability) / RepairAmountPerUnit)
```

`ceil` on the per-unit amount guarantees 4 units always fully repair any tier.
A fully broken tool (`CurrentDurability = 0`) needs exactly 4 units.

### 4.5 Viewmodel attach

```
on equip:  AEquippedToolActor.StaticMeshComponent->SetStaticMesh(Tool.MeshPath)
           attach to APlayerViewModel socket "weapon_r"
           (no spawn/destroy; mesh swap only)
```

---

## 5. Edge Cases

| Case | Resolution |
|---|---|
| Target category changes within 175 ms | Coalesced; latest candidate commits after the full window; no flicker |
| No tool owned for the target category | Hand used; no durability consumed; no auto-equip; `OnActiveToolChanged` still fires with the category's type only if a tool exists — otherwise active stays hand (None) |
| Manual equip during auto-selection window | Manual override for 500 ms; auto-selection resumes after (specified, not yet implemented) |
| Mining with a non-matching tool | Hand speed (1.0×), no durability consumed, no queue-slot bonus |
| Durability reaches 0 mid-mining | Tool breaks, auto-unequipped; the in-flight queued break still completes; tool retained for repair |
| Broken tool equipped manually | Treated as hand; cannot mine faster or deal bonus damage |
| Sword swing hits 0 mobs | No durability consumed; montage still plays through Recovery |
| Tool swap while a swing is in progress | Swap queued, applied at `AnimNotify_ActionComplete`; never mid-swing |
| `AEquippedToolActor` mesh swap failure (missing asset) | Falls back to default mesh; validation test fails the build (ADR-0007 §5) |
| Save data references a removed `ToolId` | Migration: slot cleared, logged; tool becomes hand |
| Repairing a fully broken tool | Allowed; `UnitsNeeded = 4` |
| Repair at max durability | No-op; UI disables the repair action |
| Baubles panel open during an Angel encounter | Game not paused; player remains observable; deliberate risk |
| `IA_DropItem` while Baubles open | Drops from hovered slot, never from a Baubles slot |
| Two tools of the same type in inventory | Both remain separate instances; no auto-merge; equip replaces slot |
| Legacy save with no `InstanceId` | Migration assigns a new GUID; durability preserved if present |

---

## 6. Dependencies

| System | Why | Status |
|---|---|---|
| `UBaublesSystem` | 5-slot equipment, delegates, auto-selection host | Implemented |
| `FToolInstance` / `DT_Tools` / `UToolDefinition` | Tool data, durability runtime state | Migrating (ADR-0007) |
| `AEquippedToolActor` | StaticMesh swap on equip | Implemented |
| `APlayerViewModel` | Skeletal arms, `weapon_r` socket, montages | Implemented |
| `UPlayerActionComponent` | Montage driver, notifies, damage windows | To implement (ADR-0005) |
| `UCombatSubsystem` | Damage funnel `ApplyDamage` | To implement (ADR-0005) |
| `UMiningQueueSystem` | Concurrent break queue, slot gating | Implemented |
| `AWorldGenerator` | BreakBlockAtVoxel, QueueBlockBreakAtVoxel, crack overlay | Implemented |
| `AChunkActor` | GetBlockBreakTimeAtVoxel, hand/mining force | Implemented |
| `UQuickSlotsInventorySystem` | Storage and widget refresh; drop routing | Implemented (9 slots, block-only) |
| `UCraftingSmeltingSystem` | Tool crafting recipes, repair | Implemented (block-only, needs ingredients) |
| `UPlayerVitalSystem` | Health, XP, death | Implemented |
| `AMobCharacter` | ApplyMobDamage, stagger, knockback, death | Implemented |
| `ABlockCrackActor` | Progress overlay (ADR-0003) | Implemented |
| Animation montages & notifies | 5 swing sets, notifies | To author |
| `design/gdd/weapons-tools.md` | Combat, montages, tier stats | Draft |
| `design/gdd/items.md` | Durability repair, item registry | Draft |

**Implementation gaps to close (flagged):**
- `ConsumeToolDurability` currently clears the slot on break; design requires retaining the
  broken `FToolInstance` for repair.
- Manual override (`ManualOverrideMs`) for auto-selection is specified but not implemented.
- Repair recipe + UI not yet wired to `UCraftingSmeltingSystem`.

---

## 7. Tuning Knobs

| Knob | Default | Safe range | Effect |
|---|---|---|---|
| `AutoSelectAntiBounceSeconds` | 0.175 s | 0.100–0.300 s | Anti-bounce at block boundaries |
| `ManualOverrideMs` | 500 ms | 200–1000 ms | Manual equip stickiness |
| `RepairFractionPerUnit` | 0.25 | 0.10–0.50 | Repair economy |
| `DurabilityMax` (per tier) | see §4.2 | ±50 % | Tool lifespan |
| `BaseMiningForce` (per tier) | see §4.2 | ±25 % | Mining speed |
| `MaxQueueSlots` (per tier) | 1–6 | 1–8 | Concurrent mining |
| `MiningForceByBlockCategory` | 1.0 default | 0.5–2.0 | Specialisation |
| `CombatRange` | 250 cm | 180–320 cm | Sword reach (see weapons-tools) |
| `CombatHalfAngle` | 45° | 30–60° | Swing arc width |

---

## 8. Acceptance Criteria

1. Equipping a tool routes it to the correct Baubles slot and swaps the mesh on
   `AEquippedToolActor` without spawning or destroying actors.
2. Auto-selection equips the matching tool within 175 ms of a stable target category change,
   and does not flicker at block boundaries (debounce holds; latest candidate wins).
3. Mining a matching block consumes exactly 1 durability on the matching tool; a
   non-matching tool consumes 0 and mines at hand speed.
4. A broken tool is unusable, auto-unequipped, **retained** (not destroyed), and repairable
   for 4 material units.
5. Repair restores 25 % of `MaxDurability` per unit (ceil-rounded), capped at `MaxDurability`.
6. All 25 `DT_Tools` rows resolve their mesh, icon and montage; a missing asset fails the
   build (ADR-0007 §5).
7. The Baubles panel toggles on B (`IA_Baubles`), broadcasts `OnBaublesToggled`, and never
   pauses the game.
8. While the panel is open, `IA_DropItem` drops from the hovered slot, never from a Baubles
   slot.
9. `OnActiveToolChanged` fires only after the anti-bounce window commits.
10. `GetActiveTool()` returns the equipped instance for the active type, or empty for hand.
11. Save/load round-trips `ToolId` + `InstanceId` + `CurrentDurability` for all 5 slots.
12. An automation test covers: slot routing, matching vs non-matching mining, durability
    consumption, auto-selection debounce, and repair unit count.
13. A broken tool loaded from a legacy save is migrated to `CurrentDurability = 0` without
    crash.