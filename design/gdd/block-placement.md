# GDD — Block Placement

| Field | Value |
|---|---|
| Status | Draft (2026-11-11) |
| System owner | systems-designer / gameplay-programmer |
| Related | `Docs/SYSTEMS_REFERENCE.md` §1, §3, `Docs/GAME_DESIGN.md` §3, `design/gdd/block-interaction.md` §3.6, `design/gdd/inventory-quickslots.md` §3.2 §3.4, `design/gdd/items-loot.md`, `design/gdd/audio-system.md` §3.1, `design/gdd/weeping-angels.md` §3.4, `Docs/BLOCK_REGISTRY.md` |
| Source of truth for current code | `WorldGenerator.h/.cpp` (`PlaceBlock`, `TraceForBlock`, `BreakBlockAtVoxel`), `ChunkActor.h/.cpp` (`SetBlockWithMeta`, `AddTorchMesh`, `BlockMetadata`), `QuickSlotsInventorySystem.h/.cpp` |
| Decisions | ADR-0001 (voxel scale 100 UU = 1 block), ADR-0002 (chunk modification flagging), ADR-0005 (custom action layer) |
| Supersedes | `block-interaction.md` §3.6 (placement subsection) — this document is authoritative for placement rules |

---

## 1. Overview

Block Placement is the **inverse verb** of mining: the player right-clicks a surface and a
block materialises in the empty voxel adjacent to the face they are looking at. It is the
system that turns the inventory's stored resources back into **permanent geometry** — walls,
floors, pillars, light sources — and therefore the system that turns the player's resources
into safety.

Placement is a **validated write**, not a free edit. Every request runs a ten-gate pipeline
(reach, item validity, cooldown, world bounds, chunk residency, replaceability, support,
attachment, pawn overlap) before a single voxel changes, and the write is **server-authoritative
and atomic**: either the voxel is set, the chunk is remeshed, the item is consumed, the noise
is emitted and the feedback plays, or nothing happens at all.

The system owns exactly four responsibilities:

1. **Target resolution** — turning a crosshair hit into the *placement voxel* (the empty
   neighbour of the hit face), derived by integer stepping rather than epsilon sampling.
2. **Validation** — the ordered gate pipeline in §3.3, with a named failure reason for every
   rejection so the UI can stay silent while telemetry stays precise.
3. **Commit** — voxel write with orientation metadata, chunk + boundary remesh, delta save,
   quick-slot consumption, replaced-block drop, noise emission, audio/visual feedback.
4. **Cascade** — popping attached blocks (torches) whose support was removed, whether by a
   break, an explosion, or a neighbouring placement.

Placement deliberately has **no creative mode, no infinite blocks and no undo**. In a horror
game where every wall is a resource decision, the cost of a misplaced block is the block.

---

## 2. Player Fantasy

> *"One click and the hole is closed. That click was also the loudest thing I've done in ten
> minutes."*

The fantasy is **deliberate construction under threat**:

- **Placement is a decision, not a reflex.** The block you place is the block you mined, paid
  for with durability and noise. Nothing is free, so every wall segment is a small bet.
- **The world accepts your edit.** A placed block is indistinguishable from generated
  terrain — same material, same collision, same light occlusion, same biome fade. The player
  never feels they are decorating; they are *editing the world*.
- **Geometry is the answer to fear.** A torch is a claim on the dark. A two-block wall is
  enough to break an Angel's sightline. Blocking a doorway is a real, physical act of buying
  time — and it is exactly what the Angel can breach through (`weeping-angels.md` §3.8).
- **The click is audible.** Placing a block is quieter than breaking one, but it is not
  silent. In a game where sound is the primary tell, a player building a panic wall is
  broadcasting their position. Safety and stealth pull in opposite directions, and the
  player has to choose.
- **Nothing is lost to a mistake.** A block placed into a torch pops the torch and drops it.
  A block that cannot be placed consumes nothing and says nothing. The system never punishes
  the player for the game's own ambiguity.

---

## 3. Detailed Rules

### 3.1 Input and interaction priority

Placement is bound to the **secondary action** (`IA_SecondaryAction`, right mouse button).
Because right-click is also the interaction verb, a single press resolves through a strict
priority chain — the **first** matching branch wins and the others are not evaluated:

| Priority | Condition | Result |
|---|---|---|
| 1 | Targeted block is an interactive block (Crafting Table 15, Furnace 16) **and** within the interaction system's own distance | Open the panel (`InteractWithTargetBlock`) |
| 2 | Crosshair hits a valid surface **and** the selected quick slot holds a placeable block | Place the block (§3.3–§3.10) |
| 3 | Otherwise | Silent no-op |

Rules:

- The chain is evaluated **on press**, not on hold. Holding right-click does not auto-repeat
  interaction; placement repetition is governed by `PlaceCooldown` (§3.9).
- Priority 1 exists so that a player carrying a stack of planks cannot accidentally wall
  themselves out of a crafting table they are looking at. Interaction always wins on an
  interactive target.
- **The interaction distance is not owned by this system.** `block-interaction.md` §3.7
  specifies 400 cm from the camera; the current code uses `BlockScale × 4.5` = 450 cm measured
  from the **pawn** (`WorldGenerator.cpp:2556`, `:2571`). That drift is tracked against
  `block-interaction.md` and must not be duplicated here. What matters for placement is the
  **boundary**: any target inside the interaction distance is claimed by priority 1, so
  placement can never win on an interactive block, even if the player wanted to place against
  it.
- **Containers are not yet interactive.** Barrel (20) is a placeable block but
  `InteractWithTargetBlock` handles only Crafting Table and Furnace today; chest, crate,
  barrel and backpack panels are specified in `inventory-quickslots.md` §3.8 and not
  implemented. Until they land, right-clicking a placed Barrel places the held block against
  it instead of opening it.
- Priority 2 requires a **hit**. Right-clicking while looking at the sky is a no-op — there is
  no floating placement.
- Placement is disabled while any modal panel is open (inventory, Baubles, container, menu).
  The UI owns the cursor in that state; gameplay verbs are suppressed.
- Placement is disabled while dead (`UPlayerVitalSystem::bIsAlive == false`).

### 3.2 Target resolution

The placement voxel is derived from the crosshair line trace, never from the player's body:

1. `AWorldGenerator::TraceForBlock(Camera, MaxDistance, OutBreakPos, OutPlacePos, OutHitBlockID)`
   traces on `ECC_WorldStatic` from the camera location along the camera forward vector,
   ignoring the world generator and the player pawn.
2. Only hits on an `AChunkActor` are accepted. Hits on mobs, pickups, debris, the view model
   or any other actor are discarded (the trace channel already excludes most of them).
3. The **hit voxel** is recovered by stepping inward along the impact normal, then the
   **placement voxel** is derived by integer stepping along the dominant axis of the normal
   (§4.2). The integer step is normative: it is exact for axis-aligned cube faces and removes
   the corner-grazing ambiguity of sampling outward with an epsilon.
4. `LastTargetedHitNormal` and `LastTargetedHitPoint` are cached on the world generator. They
   are consumed by placement (for the attachment face), by the crack overlay, and by the
   debris burst direction.
5. **One trace serves both verbs.** The trace length is `MiningReach` (500 cm) because mining
   needs the longer reach; placement then enforces the shorter `PlacementReach` (400 cm) in
   G1 (§3.3). The practical consequence is a deliberate 1-block band (400–500 cm) where the
   player can mine a surface but cannot place against it. Placement must therefore **not**
   assume the trace already bounds reach — G1 is the authority.

The **selected quick slot** supplies the block: `SelectedSlotIndex = SelectedID - 1`
(`inventory-quickslots.md` §3.2). Only quick slots `0..8` are placement sources. The main
inventory and the Baubles tool slots are never consulted — a block in main inventory slot 12
cannot be placed without first moving it to the quickbar.

### 3.3 Validation pipeline

Validation is an **ordered, fail-fast** gate list. The order is fixed: cheap scalar checks
before world queries, world queries before the expensive capsule test. A rejection returns
`false` and a named reason; it never partially mutates state.

| Gate | Test | Failure reason |
|---|---|---|
| **G1 Reach** | `Distance(ImpactPoint, CameraLocation) ≤ PlacementReach + ReachTolerance` | `OutOfReach` |
| **G2 Block valid** | `BlockID ≠ Air` and `IsPlaceable(BlockID)` | `NotPlaceable` |
| **G3 Slot has item** | Selected slot is non-empty, `Kind == Block`, `BlockID` matches | `NoItem` / `ItemMismatch` |
| **G4 Cooldown** | `Time − LastPlaceTime ≥ PlaceCooldown` | `Cooldown` |
| **G5 Bounds** | `0 ≤ V.Z < ChunkHeight` (64); X/Y unbounded | `OutOfBounds` |
| **G6 Chunk loaded** | The owning chunk exists in `ActiveChunks` | `ChunkUnloaded` |
| **G7 Replaceable** | `Replaceable(Block(V))` | `Occupied` |
| **G8 Support** | `HasSupport(V)` — at least one face-adjacent support block | `NoSupport` |
| **G9 Attachment** | `AttachOK(BlockID, V, f)` — always true except for attaching blocks | `BadAttach` |
| **G10 Pawn overlap** | `¬OverlapsAnyPawn(V)` | `BlockedByPawn` |

Rules:

- **G2** uses the registry's placeable flag. Air, Bedrock and Water are not placeable items;
  every other block ID in `Block_DataTable` is placeable unless the row opts out.
- **G3** is re-evaluated immediately before commit (§3.10), not only at gate time, so a
  same-frame slot change cannot produce a free block.
- **G5** rejects the bedrock floor (`Z < 0`) and the build ceiling (`Z ≥ 64`). There is no
  vertical build limit message; placement above the ceiling is simply a silent no-op.
- **G7** and **G8** are distinct: *Occupied* means "something is already there", *NoSupport*
  means "the space is empty but floating".
- **G10** covers **every pawn**, not only the placing player. See §3.7 for the rationale.

### 3.4 Support and adjacency ("not in the air")

A block cannot be placed floating. The target voxel must share at least one **face** with a
block that can physically carry it:

$$\text{HasSupport}(V) = \exists\, d \in D_6 : \text{IsSupportBlock}(\text{Block}(V + d))$$

with $D_6 = \{(\pm1,0,0), (0,\pm1,0), (0,0,\pm1)\}$ and

$$\text{IsSupportBlock}(b) = (b \neq \text{Air}) \land (b \neq \text{Water}) \land \neg\text{IsNonSolidBlock}(b)$$

- `IsNonSolidBlock` is `FBlockHelpers::IsNonSolidBlock` — true for Torch only in v1
  (`block-interaction.md` §3.8).
- **Water does not support** because it has `QueryOnly` collision with no physics body
  (`Docs/BLOCK_REGISTRY.md` row 26). Placing a bridge over water therefore requires placing
  against the shore, not against the water surface — matching the "no liquid simulation"
  constraint.
- **Diagonal support does not count.** A block touching only the edge or corner of a solid
  block is rejected. This is the single rule that prevents staircase-of-one-block exploits
  and keeps the world buildable only along connected surfaces.
- In practice G8 is almost always satisfied for the trace path, because the placement voxel is
  by construction the neighbour of the block that was hit. G8 is the normative rule for
  **programmatic** placement (Blueprint, world generation, Angel breach repair, tests) and the
  safety net for the trace path when a surface is replaced between the trace and the commit.

### 3.5 Attachment constraints (torches and future attached blocks)

Some blocks are not free-standing: they are *attached* to a specific neighbouring face. In v1
the only attached block is the **Torch (ID 19)**.

The attachment face $f$ is the direction **from the new voxel toward its support**, derived
from the hit normal: $f = -\text{AxisStep}(N)$ (§4.2). For a top-face hit on the block below,
$f = (0,0,-1)$ — the torch sits on the floor.

$$\text{AttachOK}(\text{Torch}, V, f) = \text{IsSupportBlock}(\text{Block}(V + f)) \land f \neq (0,0,+1)$$

| $f$ | Mount | Allowed | Meta |
|---|---|---|---|
| $(0,0,-1)$ | Floor (support below) | Yes | `0` |
| $(+1,0,0)$ | Wall on −X side | Yes | `1` |
| $(-1,0,0)$ | Wall on +X side | Yes | `2` |
| $(0,+1,0)$ | Wall on −Y side | Yes | `3` |
| $(0,-1,0)$ | Wall on +Y side | Yes | `4` |
| $(0,0,+1)$ | Ceiling (support above) | **No** | — |

Rules:

- **Ceiling mounting is forbidden.** A torch under a ceiling has no visual anchor in the
  crossed-plane mesh and reads as floating. The rejection is silent.
- The support must be a **support block** per §3.4 — a torch cannot hang off another torch,
  and a torch cannot be placed against water.
- **Orientation metadata.** The attachment face is written to `BlockMetadata` as
  `MetaRotation` via `AChunkActor::SetBlockWithMeta`. Torches never enter the cube-rotation
  path (`ChunkActor.cpp:633` branches to `AddTorchMesh` before the rotation lookup), so the
  torch encoding `0..4` coexists with the cube encoding `0..3` without collision.
  **Implementation gap:** `AddTorchMesh` currently takes only `(MeshData, BlockPos,
  TextureIndex)` and always builds two vertical crossed planes at the voxel centre. It must
  accept the meta value and (a) offset the geometry toward the wall by `0.35 × BlockScale`
  and (b) tilt/rotate the planes for wall mounts. Until that lands, `AttachOK` should be
  narrowed to floor mounts only (§7, `bAllowWallTorches`).
- **Non-attaching blocks** return `AttachOK = true` unconditionally. The gate exists so that
  future attached blocks (ladders, signs, wall-mounted lights) inherit the pipeline for free.
- Attached blocks are always **non-solid** (`IsNonSolidBlock`): they never block movement, so
  an attached block can never satisfy G10 on its own account and never carries another
  attached block.

### 3.6 Replaceable blocks

The target voxel may already contain something, provided it is **replaceable**:

$$\text{Replaceable}(b) = (b = \text{Air}) \lor (b = \text{Water}) \lor (b = \text{Torch})$$

| Replaced block | Effect |
|---|---|
| Air (0) | Normal placement |
| Water (26) | Water is displaced; no drop (water is not an item) |
| Torch (19) | Torch is destroyed **and drops** as an `ABlockItemPickup` at the voxel centre |

- Replacing a torch **never silently deletes it**. The drop reuses
  `AWorldGenerator::SpawnBlockItemDrop` with the pre-commit block ID, so the "nothing is
  wasted" rule of `inventory-quickslots.md` §2 holds.
- Every other block, including Leaves (17), Glass (21) and Fence (22), is **not** replaceable.
  The player must break it first. This keeps the placement verb from becoming a stealth
  deletion tool for player-built structures.
- Replaceability is a **registry property**, not a code constant: the DataTable column
  `bIsReplaceable` defaults to `false` and is set true for Air, Water and Torch. Adding a
  replaceable block later requires no code change.

### 3.7 Pawn overlap

A block may not be placed inside a living body.

$$\text{Overlaps}(V, P) = \text{Dist}\big(\text{Segment}(P),\, \text{Box}(V)\big) < R_P + \text{Skin}$$

- `Segment(P)` is the pawn capsule's central vertical segment, `Box(V)` the voxel's 100 UU
  AABB, `R_P` the pawn capsule radius, `Skin = 2 cm` a safety margin.
- `OverlapsAnyPawn(V)` is true if **any** pawn on the server overlaps — the placing player,
  another player, or a mob.
- **Why every pawn, not just the player:** allowing placement inside a mob would let the
  player trap a zombie or creeper in a solid block for a free, silent kill. That is both an
  exploit and a tonal failure — the horror comes from mobs being *able* to reach you.
- **Why a skin margin:** the capsule and the AABB can touch exactly (distance 0) on a shared
  face when the player stands flush against a wall. Without a margin, a player pressed against
  a wall would be unable to place on that wall — the most common placement gesture in the
  game.
- Overlap is tested against the **voxel AABB**, not the voxel centre. A player whose capsule
  clips a corner of the target voxel blocks the placement, even if the centre is clear.
- Pawn **suffocation** (damage from being inside a block) is **not** implemented and is out of
  scope. G10 makes the situation unreachable through placement; the Angel breach
  (`weeping-angels.md` §3.8) writes voxels directly and is exempt by design.

### 3.8 Inventory interaction

Placement consumes **exactly one item** from the selected quick slot, and only on a
**successful** commit.

- **Source.** `SelectedSlotIndex = SelectedID - 1`. Never the main inventory, never Baubles.
- **Eligibility.** The slot must hold `FItemRef{Kind = Block, BlockID = X}` with
  `ItemCount ≥ 1`, and `X` must equal the block being placed. The placed block is always the
  slot's block — the player cannot place a block they are not holding.
- **Non-block items.** A slot holding a non-block item (`Kind == Item`, e.g. medicine) makes
  placement a silent no-op. The right-click still falls through to nothing; it does not
  "use" the item (`items-loot.md` §3.12).
- **Consumption.** `ItemCount -= 1`. When it reaches `0` the slot is cleared
  (`Ref = None`, `ItemCount = 0`) and the widget is refreshed with opacity `0.0` / `Hidden`
  (`inventory-quickslots.md` §3.10). `SelectedID` is **not** auto-advanced — the player keeps
  the now-empty selection, exactly as after dropping the last item.
- **Refresh.** `RefreshQuickSlotVisual` runs for the touched slot only; the remaining eight
  slots are untouched.
- **No refund path.** There is no "pick the block back up" shortcut: breaking a placed block
  runs the normal mining flow (break time, durability, debris, drop), so a misplaced block
  costs the mining time to remove it. This is intentional — it is what makes the wall a
  decision.
- **Consumption order.** The world write happens **before** the decrement (§3.10). If the
  write fails, the item is untouched. If the write succeeds, the decrement is guaranteed
  because G3 was re-checked on the same frame on the authoritative server.

### 3.9 Cooldown and repetition

- `PlaceCooldown` defaults to **0.20 s** (4 ticks at 60 FPS), measured between two
  **successful** commits. A rejected placement does **not** start the cooldown, so a player
  holding right-click against a bad surface does not lose their next valid attempt.
- Holding right-click does **not** auto-repeat in v1. Each placement is one press. Auto-repeat
  is a tuning knob (`bAllowHoldToPlace`) and is off by default, because continuous placement
  is both a remesh-spam risk and a builder's crutch in a game about deliberate action.
- The cooldown exists to bound the cost of a remesh per placement. A chunk remesh is the most
  expensive operation in the commit path (§3.10); without a cooldown a held button can
  request a remesh every frame.

### 3.10 Commit sequence

On a fully validated request, the commit runs as one atomic block. The order is fixed:

1. **Snapshot** the replaced block ID (`Block(V)` before the write). Needed for the drop and
   for the support cascade.
2. **Re-check G3** against the live slot (same frame, authoritative).
3. **Write** the voxel: `Chunk->SetBlockWithMeta(LocalX, LocalY, LocalZ, BlockID, Meta)`.
   This flags the chunk `bIsModified` (ADR-0002), which is what makes the edit persistent.
4. **Remesh** the owning chunk, then each **boundary neighbour**: if `LocalX ∈ {0, Size-1}`
   remesh the chunk on that side, same for `LocalY`. This mirrors `BreakBlockAtVoxel`
   (`WorldGenerator.cpp:1626-1646`) so a block placed on a chunk seam does not leave a
   visible hole in the neighbour's culled faces.
5. **Save** the chunk delta (`SaveChunkDelta`). Placement is persistent immediately, not at
   world save.
6. **Consume** one item from the selected quick slot (§3.8) and refresh the widget.
7. **Drop** the replaced block if it was droppable (Torch) at the voxel centre.
8. **Emit noise** (§3.11) at the voxel centre.
9. **Feedback** — play the placement cue and the view-model place animation (§3.11).
10. **Cascade** — run the support-loss check on the six neighbours (§3.12).

Rules:

- Steps 1–5 are the **world transaction**; steps 6–10 are **consequences**. A failure in the
  world transaction aborts the whole commit with no consequence applied. A failure in a
  consequence (e.g. the SFX asset is missing) is logged and does not roll back the world.
- The commit is **idempotent-safe against double input**: the cooldown timestamp is written at
  step 3, inside the transaction, so a second press in the same frame fails G4.
- No placement ever runs on a client. See §3.13.

### 3.11 Feedback: audio, visual, noise

Placement must **look and sound like it happened**, because the block itself is often behind
the player's hand or at the edge of the screen.

**Audio** (`audio-system.md` §3.1):

| Event | Cue | Notes |
|---|---|---|
| Block placed | `SFX_BlockPlace_<Material>` | Material = the placed block's category (stone, wood, dirt, sand, glass). Softer and shorter than the matching break cue. Pitch ±3 % random. |
| Torch placed | `SFX_Torch` | Same cue as torch pickup. Distinct because a torch is a light claim. |

- The cue is resolved through the existing `APlayerViewModel::HandlePlayCue(FName CueName)`
  path (`/Game/Audio/SFX/SFX_<CueName>`), so placement reuses the same audio plumbing as the
  swing cues rather than adding a new subsystem.
- The sound is played **at the voxel location**, not on the player, so it attenuates and
  occludes correctly when the player places around a corner.

**Visual:**

- The **chunk remesh is the primary visual confirmation** — the block simply appears.
- The **view-model place animation** plays for ~60 ms (a short forward press on the held
  block). It is a `UAnimMontage` hook, same mechanism as the swing montages; it is
  cosmetic and must never gate the commit.
- A **placement ghost** (wireframe preview of the target voxel) is *desirable* but **deferred
  to v2**. v1 relies on the crosshair plus the block's own preview in the selected slot. If
  the ghost lands later, it must be driven from the same `TraceForBlock` result and must be
  hidden when G1–G10 would fail.
- **Rejected placement gives no visual feedback.** No red flash, no shake, no error text. The
  player learns the rules by the world refusing to change. This is deliberate: a rejection is
  usually "you are looking at nothing" or "you are holding nothing", and both are already
  visible on screen.

**Noise** (`weeping-angels.md` §3.4):

$$\text{NoiseLoudness}(b) = \begin{cases} 2 & b = \text{Torch} \\ 6 & \text{otherwise} \end{cases}$$

- A `NoiseEvent` is emitted at the voxel centre with that loudness, using the same event type
  as mining. The Angels' hearing formula (`EffectiveNoiseRadius = Loudness − 4 ×
  SolidBlocksBetween`) then applies unchanged.
- **Rationale for 6:** breaking a block is 12 and a torch is 2. A placed block is a
  *deliberate, controlled* action with a soft landing, so it should be roughly half as loud as
  the destructive act that produced the block. 6 still puts it above walking (4), so building
  in the dark is a real risk.
- A torch is 2, matching the existing table, so placing light remains the quietest way to
  claim ground — and therefore the correct horror-economy choice: light is cheap in noise but
  expensive in visibility.

### 3.12 Support-loss cascade

Attached blocks are destroyed when the block they rest on disappears, whatever the cause.

```
OnBlockRemoved(V):
    for each d in D6:
        T = V + d
        if Block(T) == Torch:
            f = DecodeAttachMeta(Meta(T))
            if not IsSupportBlock(Block(T + f)):
                BreakBlockAtVoxel(T)     // recursive; drops the torch
```

- The check is **per-neighbour and meta-driven**, so it works for wall torches as well as
  floor torches. It generalises the current `BreakBlockAtVoxel` special case, which only pops
  the torch directly **above** the broken block (`WorldGenerator.cpp:1668-1680`) and would
  leave wall torches floating.
- The cascade is invoked from **every** block-removal path: player mining, Creeper crater,
  Angel breach, mob-griefing, and the placement that replaced a support block.
- Recursion depth is bounded at 1 in practice (a torch cannot support a torch), but the
  implementation must still guard against cycles because a future attached block might chain.
- A popped torch **drops** as a pickup. It is a world event, not a loss.

### 3.13 Multiplayer (server authority)

Placement is a **server-authoritative** transaction. The client sends an *intent*, never a
result.

```
Client: ServerPlaceBlock(VoxelCoord, BlockID, Meta, ImpactPoint, ImpactNormal)
Server: re-derive the trace from the server camera state
        re-run G1..G10 with ReachTolerance
        commit (§3.10) or reject with a reason
        replicate the voxel change (chunk delta / block change RPC)
```

- The client's `VoxelCoord` is a **hint**. The server recomputes the placement voxel from its
  own copy of the camera transform and impact point and rejects a mismatch beyond
  `ReachTolerance`. A client cannot place a block 40 m away by lying about the coordinate.
- The server validates **G3** against its own inventory state. A client that has already
  consumed the item locally must be reconciled: the server is the authority on `ItemCount`,
  and the local decrement is a prediction that is rolled back on rejection.
- Only the **world change** is replicated. The SFX, the view-model animation and the debris of
  the replaced torch are cosmetic and play client-locally (multicast or local cue).
- Single-player (`NetMode == Standalone`) runs the identical code path with
  `ReachTolerance = 0`, so there is no separate "offline" branch to diverge.

---

## 4. Formulas

### 4.1 Voxel coordinate conversion

$$\text{VoxelFromWorld}(W) = \left(\left\lfloor \frac{W_x}{S} \right\rfloor, \left\lfloor \frac{W_y}{S} \right\rfloor, \left\lfloor \frac{W_z}{S} \right\rfloor\right)$$

| Symbol | Meaning | Value / Range |
|---|---|---|
| $W$ | World location, UU | any |
| $S$ | `BlockScale`, block edge in UU | `100` (ADR-0001) |
| Output | Integer voxel index | $Z \in [0, 64)$; $X, Y$ unbounded |

Floor, not round: the voxel containing the point. Matches
`AWorldGenerator::WorldLocationToVoxelCoord` (`WorldGenerator.cpp:605`).

*Example:* $W = (1050, 1100, -5)$ → $(\lfloor 10.5 \rfloor, \lfloor 11 \rfloor, \lfloor -0.05
\rfloor) = (10, 11, -1)$ → $Z = -1$ fails G5.

### 4.2 Placement voxel from a crosshair hit

$$\text{BreakVoxel} = \text{VoxelFromWorld}\big(\text{ImpactPoint} - N \cdot 0.25 S\big)$$

$$\text{PlaceVoxel} = \text{BreakVoxel} + \text{AxisStep}(N)$$

$$\text{AxisStep}(N) = \begin{cases}
(\operatorname{sign} N_x, 0, 0) & |N_x| \ge |N_y| \ \land\ |N_x| \ge |N_z| \\
(0, \operatorname{sign} N_y, 0) & |N_y| > |N_x| \ \land\ |N_y| \ge |N_z| \\
(0, 0, \operatorname{sign} N_z) & \text{otherwise}
\end{cases}$$

| Symbol | Meaning |
|---|---|
| $N$ | `ImpactNormal`, unit vector of the hit face |
| $\text{ImpactPoint}$ | World-space hit position, UU |
| $S$ | `BlockScale` = 100 |
| $0.25 S$ | Inward sampling offset (25 UU), used only to recover `BreakVoxel` |

For an axis-aligned voxel face, $N$ is exactly one of $\pm X, \pm Y, \pm Z$, so `AxisStep` is a
single unit step and the result is exact. The 0.25 offset only recovers the hit voxel; the
placement voxel is never derived by sampling, which is what removes the corner-grazing error
of the current outward-sampling implementation (`WorldGenerator.cpp:1896-1905`).

*Example:* `ImpactPoint = (1050, 1100, 1000)`, $N = (-1, 0, 0)$.
`BreakVoxel = floor((1050 + 25)/100, 11, 10) = (10, 11, 10)`.
`AxisStep(N) = (-1, 0, 0)` → `PlaceVoxel = (9, 11, 10)`.

*Example (corner graze):* `ImpactPoint = (1099.5, 1100.5, 1000)`, $N = (0, 0, 1)$ (top face).
Inward sample → $Z = 9.75$ → `BreakVoxel = (10, 11, 9)`. `PlaceVoxel = (10, 11, 10)`. The
outward-sampling method would sample $Z = 10.25$ → also $(10,11,10)$, but at
`ImpactPoint.X = 1099.9` outward sampling with $N=(1,0,0)$ would land on the *next* voxel.
Integer stepping has no such failure mode.

### 4.3 Placement reach

$$\text{ReachOK} = \big| \text{ImpactPoint} - \text{CameraLocation} \big| \le R_{\text{place}} + T_{\text{net}}$$

| Symbol | Meaning | Value | Safe range |
|---|---|---|---|
| $R_{\text{place}}$ | `PlacementReach` | 400 cm (4 blocks) | 300–500 cm |
| $T_{\text{net}}$ | `ReachTolerance` | 25 cm (network), 0 (standalone) | 0–50 cm |
| $\text{CameraLocation}$ | First-person camera position (eye) | ≈ 165 cm above feet | — |

Measured **eye → impact point**, not eye → voxel centre. The impact point is the exact surface
the player clicked, so a block at the far corner of a 4-block voxel is not penalised for the
half-block of extra diagonal distance its centre would imply.

*Example:* eye at $Z = 165$, looking horizontally at a wall face at $X = 390$ → distance 390
≤ 400 → pass. At 415 → fail (standalone).

### 4.4 Support / adjacency

$$\text{HasSupport}(V) = \bigvee_{d \in D_6} \text{IsSupportBlock}\big(\text{Block}(V + d)\big)$$

$$\text{IsSupportBlock}(b) = (b \neq 0) \land (b \neq 26) \land (b \neq 19)$$

| Block ID | Is support? | Why |
|---|---|---|
| 0 Air | No | Nothing to carry a block |
| 1–18, 20–25, 27 | **Yes** | Solid, `QueryAndPhysics` |
| 19 Torch | No | Non-solid (`IsNonSolidBlock`) |
| 26 Water | No | `QueryOnly`, no physics body |

*Example:* $V$ with neighbours $(0,0,-1) = 4$ (Stone) and all others Air → `HasSupport` true.
If the only neighbour were $19$ (Torch) → false, placement rejected with `NoSupport`.

### 4.5 Torch attachment

$$f = -\text{AxisStep}(N)$$

$$\text{AttachOK}(\text{Torch}, V, f) = \text{IsSupportBlock}\big(\text{Block}(V + f)\big) \land f \neq (0,0,+1)$$

$$\text{Meta} = \begin{cases}
0 & f = (0,0,-1) \quad \text{(floor)} \\
1 + \text{Index}(f) & f \in \{(+1,0,0), (-1,0,0), (0,+1,0), (0,-1,0)\} \to 1..4
\end{cases}$$

| Symbol | Meaning |
|---|---|
| $f$ | Direction from the torch voxel **to** its support block |
| $N$ | `ImpactNormal` of the hit face |
| $\text{Meta}$ | `BlockMetadata` value written by `SetBlockWithMeta` |

*Example:* hitting the **top** face of a stone block at $B = (5,5,10)$ gives $N = (0,0,1)$,
`PlaceVoxel` $= B + (0,0,1) = (5,5,11)$, $f = (0,0,-1)$ → floor mount, `Meta = 0`. ✓

*Example:* hitting the $+X$ face of a wall at $B = (5,5,10)$ gives $N = (1,0,0)$,
`PlaceVoxel` $= (6,5,10)$, $f = (-1,0,0)$ → wall mount on the $+X$ side, `Meta = 2`. ✓

*Example:* hitting the **bottom** face of an overhang gives $N = (0,0,-1)$,
`PlaceVoxel` $= B + (0,0,-1)$, $f = (0,0,+1)$ → ceiling → `AttachOK` false, rejected. ✓

### 4.6 Pawn overlap

$$\text{Box}(V) = \prod_{a \in \{x,y,z\}} \big[V_a S,\ (V_a + 1) S\big]$$

$$\text{Segment}(P) = \Big[C_P - (0,0,H_P - R_P),\ C_P + (0,0,H_P - R_P)\Big]$$

$$\text{Overlaps}(V,P) = \text{Dist}\big(\text{Segment}(P), \text{Box}(V)\big) < R_P + \text{Skin}$$

| Symbol | Meaning | Value |
|---|---|---|
| $C_P$ | Pawn capsule centre (world) | feet + $(0,0,H_P)$ |
| $R_P$ | Capsule radius | 34 cm (`BP_HorrorCharacter` default) |
| $H_P$ | Capsule half-height | 88 cm (`BP_HorrorCharacter` default) |
| $\text{Skin}$ | Safety margin | 2 cm |
| $\text{Dist}(A,B)$ | Min distance between segment $A$ and box $B$ | UU |

$\text{Dist}$ is the standard two-way clamp: closest point on the box to the segment, then
closest point on the segment to that box point. Both endpoints inside the box ⇒ 0.

*Example:* player feet at origin, $C_P = (0,0,88)$, `Segment` $= [(0,0,54), (0,0,122)]$.
Target $V = (0,0,0)$, `Box` $= [0,100]^3$. The segment starts inside the box → `Dist = 0` <
$34 + 2$ → **blocked**. A player cannot place the block they are standing in.

*Example:* player feet at $Z = 100$, $C_P = (0,0,188)$, `Segment` $= [(0,0,154), (0,0,222)]$.
Target $V = (0,0,0)$ (`Box` up to $Z = 100$). Closest segment point is $(0,0,154)$, closest box
point $(0,0,100)$ → `Dist = 54` ≥ 36 → **allowed**. A player standing on a block can place the
block under their feet.

### 4.7 Consumption

$$\text{SlotAfter} = \begin{cases}
(\text{Ref}, \text{Count} - 1) & \text{Count} > 1 \\
\varnothing \text{ (empty)} & \text{Count} = 1
\end{cases}$$

$$\text{PlacementsPerSecond}_{\max} = \frac{1}{\max(\text{PlaceCooldown},\ \Delta t_{\text{input}})}$$

*Example:* `PlaceCooldown = 0.20 s` → at most 5 placements per second, i.e. 5 chunk remeshes
per second from placement. A full 64-stack lasts ≥ 12.8 s of continuous building.

### 4.8 Noise emission

$$\text{NoiseLoudness}(b) = \begin{cases} 2 & b = 19 \text{ (Torch)} \\ 6 & \text{otherwise} \end{cases}$$

$$\text{EffectiveNoiseRadius} = \text{NoiseLoudness} - 4 \times \text{SolidBlocksBetween}$$

(from `weeping-angels.md` §4.6; `HearingWallPenalty = 4` blocks per solid block.)

*Example:* a stone block placed with 2 solid blocks between it and an Angel →
$6 - 4 \times 2 = -2$ → inaudible. The same block with no obstruction → radius 6 blocks. This
is why building a *thicker* wall is both safer and quieter.

### 4.9 Validation (master predicate)

$$\text{CanPlace} = \text{ReachOK} \land \text{IsPlaceable}(b) \land \text{SlotHas}(b) \land \text{CooldownElapsed} \land \text{InBounds}(V) \land \text{ChunkLoaded}(V) \land \text{Replaceable}(\text{Block}(V)) \land \text{HasSupport}(V) \land \text{AttachOK}(b, V, f) \land \neg\text{OverlapsAnyPawn}(V)$$

All ten terms are required. The gate order in §3.3 is the evaluation order; the predicate is
the specification.

---

## 5. Edge Cases

| Case | Resolution |
|---|---|
| Right-click while looking at the sky | No trace hit → silent no-op. No floating blocks. |
| Right-click with an empty selected slot | G3 fails (`NoItem`) → silent no-op |
| Right-click holding a non-block item (medicine) | G3 fails (`ItemMismatch`) → silent no-op; the item is not consumed or used |
| Selected slot holds a different block than intended | Impossible: the placed block **is** the slot's block. There is no separate "held block" state. |
| Right-click a Crafting Table while holding planks | Interaction wins (priority 1); the planks are not placed |
| Right-click a Crafting Table **beyond** 400 cm | Priority 1 fails on distance, priority 2 places a block if a surface is hit at ≤ 400 cm — otherwise no-op |
| Placing on the top face of a block below | Standard: new voxel above, `f = (0,0,-1)` |
| Placing on the bottom face of an overhang | Allowed for normal blocks; **rejected for torches** (ceiling mount) |
| Placing a torch on the side of a wall | Allowed (wall mount, `Meta = 1..4`) — **requires the `AddTorchMesh` meta work**; until then rejected via `bAllowWallTorches = false` |
| Placing a torch on another torch | `IsSupportBlock(Torch) = false` → `BadAttach` |
| Placing a torch on water | `IsSupportBlock(Water) = false` → `BadAttach` |
| Placing a block into water | Allowed; water is replaceable and produces **no drop** |
| Placing a block into a torch | Allowed; the torch **drops** as a pickup at the voxel centre |
| Placing a block into leaves / glass / fence | `Occupied` → rejected. Break it first. |
| Placing inside the player's own capsule | G10 fails (`BlockedByPawn`) |
| Placing inside a mob's capsule | G10 fails — prevents trapping mobs for a free kill |
| Player pressed flush against a wall | Allowed: `Skin = 2 cm` margin keeps the wall placeable |
| Placing under your own feet while standing on a block | Allowed: capsule ends 54 cm above the target voxel (see §4.6 example) |
| Placing at `Z = 63` (top of the world) | Allowed. Placing at `Z = 64` fails G5 (`OutOfBounds`). |
| Placing at `Z < 0` | Fails G5 (`OutOfBounds`). Bedrock floor is not buildable. |
| Placing in an unloaded chunk | Fails G6 (`ChunkUnloaded`). Cannot happen via trace (the trace hits a loaded chunk actor), but is reachable via Blueprint/programmatic calls. |
| Placing on a chunk seam (`LocalX == 0` or `Size-1`) | Owner chunk **and** the boundary neighbour are remeshed, so no hole appears in the neighbour's culled faces |
| Two placements in the same frame | Second fails G4 (`Cooldown`) — the timestamp is written inside the transaction |
| Placement rejected repeatedly while holding right-click | No cooldown is consumed; the next valid press succeeds immediately |
| Last item of a stack placed | Slot cleared, opacity `0.0`, `SelectedID` **not** advanced |
| Breaking the block a torch rests on | Cascade pops the torch, which drops as a pickup (§3.12) |
| Breaking a **wall** a torch is attached to | Cascade reads the torch's meta, finds `f` unsupported, pops the torch. Requires the generalised cascade; the current code only handles the torch directly above. |
| Creeper crater removes a support block | Same cascade; the torch pops and drops |
| Angel breach removes a support block | Same cascade; the torch pops and drops |
| Placed block immediately mined | Normal mining flow: break time, durability, debris, drop. No refund shortcut. |
| Placement into a voxel occupied by a mob that moved away between trace and commit | G10 is evaluated at commit time on the server, so the placement is rejected, not retro-applied |
| Client places a block out of reach (cheat) | Server recomputes the trace and rejects on G1 beyond `ReachTolerance` |
| Client places a block it does not own (cheat) | Server validates G3 against authoritative inventory; the client's local decrement is rolled back |
| Missing `SFX_BlockPlace_*` asset | Logged; the world commit still succeeds. Audio never gates gameplay. |
| Placement during a weather transition / ash storm | Unaffected. Placement is not weather-gated. |
| Placement while the inventory panel is open | Suppressed: modal UI owns the cursor and gameplay verbs |
| Placement while dead | Suppressed (`bIsAlive == false`) |
| Replacing a block with the **same** block ID | Not possible: the target must be replaceable, and no replaceable block (Air/Water/Torch) is a normal placeable block |

---

## 6. Dependencies

| System | Why | Status |
|---|---|---|
| `AWorldGenerator` | `PlaceBlock`, `TraceForBlock`, `GetVoxelAt`, `SpawnBlockItemDrop`, `SaveChunkDelta`, `LastTargetedHitNormal/HitPoint` | Implemented, **partial** — see Appendix A |
| `AChunkActor` | `SetBlockWithMeta`, `BlockMetadata`, `UpdateMesh`, `GetBlock`, `bIsModified` | Implemented; `AddTorchMesh` lacks meta support |
| `UQuickSlotsInventorySystem` | Selected slot, `ItemCount` decrement, `RefreshQuickSlotVisual` | Implemented (block-only) |
| `FQuickSlotData` / `FItemRef` | Slot identity; `Kind == Block` gate for placement | `BlockID` path implemented; `FItemRef` planned |
| `FBlockHelpers::IsNonSolidBlock` | Support/attachment classification (Torch) | Implemented |
| `Block_DataTable` | `bIsPlaceable`, `bIsReplaceable`, material category for the SFX cue | Partially implemented; new columns required |
| `APlayerViewModel::HandlePlayCue` | Placement SFX and view-model place montage | Implemented (swing cues); place cue unwired |
| `UDayNightCycleSystem` | Not a hard dependency — placement is not weather-gated | Implemented |
| `UMobSpawnerSystem` / `AMobCharacter` | G10 capsule overlap against mobs; mobs are pawns | Implemented |
| `weeping-angels.md` §3.4 §4.6 | Noise loudness and hearing radius for the placement `NoiseEvent` | Draft |
| `audio-system.md` §3.1 | `SFX_BlockPlace_<Material>`, `SFX_Torch` | Draft |
| `block-interaction.md` §3.6 | Placement was first specified there; this doc is now authoritative | Draft |
| `inventory-quickslots.md` §3.2 §3.8 §3.10 | Slot selection, consumption, opacity refresh | Draft |
| `items-loot.md` §3.12 | Non-block items cannot be placed | Draft |
| `Docs/BLOCK_REGISTRY.md` | Block IDs, collision modes, placeability | Reference |
| `Docs/architecture/` (ADR-0002) | Chunk modification flagging and delta persistence | Accepted |

---

## 7. Tuning Knobs

| Knob | Default | Safe range | Effect |
|---|---|---|---|
| `PlacementReach` | 400 cm (4 blocks) | 300–500 cm | How far the player can place. Must be ≤ `MiningReach` (500 cm) so the crosshair never places beyond what it can mine. |
| `ReachTolerance` | 25 cm (network) / 0 (standalone) | 0–50 cm | Lag allowance for server-side reach validation. Above 50 cm a client can place through thin walls. |
| `PlaceCooldown` | 0.20 s | 0–0.50 s | Anti-spam / remesh rate cap. `0` restores frame-rate placement and should only be used for testing. |
| `bAllowHoldToPlace` | `false` | bool | Whether holding right-click auto-repeats at `PlaceCooldown`. Off keeps placement deliberate. |
| `bAllowWallTorches` | `false` (until mesh support) | bool | Enables wall-mounted torches. Must stay `false` until `AddTorchMesh` honours `Meta`, otherwise torches render as floor torches inside walls. |
| `bAllowCeilingTorches` | `false` | bool | Ceiling mounts. Off: the crossed-plane mesh has no anchor for them. |
| `PlacementSkin` | 2 cm | 0–10 cm | Capsule/AABB margin in G10. `0` makes walls unplaceable when the player stands flush against them. |
| `PlaceNoiseLoudness` | 6 blocks | 0–12 | Angel attraction per placement. Must stay below `BreakNoiseLoudness` (12) so building is quieter than mining. |
| `TorchPlaceNoiseLoudness` | 2 blocks | 0–6 | Torch-specific noise; matches `weeping-angels.md` §3.4. |
| `PlacementAnimDuration` | 0.06 s | 0.03–0.15 s | View-model place press. Cosmetic only; must not gate the commit. |
| `bShowPlacementGhost` | `false` (v2) | bool | Wireframe preview of the target voxel. |
| `bDropReplacedTorch` | `true` | bool | Whether replacing a torch drops it. `false` silently deletes it — not recommended. |
| `Block_DataTable.bIsPlaceable` | `true` for most rows | bool | Per-block placement permission. |
| `Block_DataTable.bIsReplaceable` | `true` for Air/Water/Torch | bool | Per-block replaceability. |

**Coupling warnings:**

- `PlacementReach > MiningReach` is invalid: the player could place on a surface they cannot
  then reach to remove. Keep `PlacementReach ≤ MiningReach`.
- `PlaceCooldown = 0` combined with `bAllowHoldToPlace = true` requests one chunk remesh per
  frame per held button. This is the single most likely source of a frame-time spike in this
  system; leave it at `0.20 s`.
- Raising `PlaceNoiseLoudness` above `BreakNoiseLoudness` inverts the horror economy: building
  would become louder than destroying, and players would stop building.
- `PlacementSkin` above ~10 cm starts blocking legitimate wall placement at oblique angles.

---

## 8. Acceptance Criteria

1. Right-clicking a valid surface within 400 cm with a placeable block selected places that
   block in the voxel adjacent to the hit face, and the block appears in the same frame.
2. The placement voxel is derived by integer stepping (`BreakVoxel + AxisStep(N)`); a grazing
   hit within 1 UU of a face corner still selects the face that was hit.
3. Placement fails with `OutOfReach` when the impact point is more than 400 cm (+ tolerance)
   from the camera.
4. Placement fails with `NotPlaceable` for Air, Bedrock and Water as the *placed* block.
5. Placement fails with `NoItem` when the selected quick slot is empty, and with
   `ItemMismatch` when it holds a non-block item; in both cases nothing is consumed and no
   sound plays.
6. Placement fails with `Occupied` on Leaves, Glass, Fence and every other non-replaceable
   block.
7. Placing into Air succeeds; into Water succeeds with no drop; into a Torch succeeds and
   spawns a torch `ABlockItemPickup` at the voxel centre.
8. Placement fails with `NoSupport` when the target voxel has no face-adjacent support block;
   a diagonally adjacent solid block does not satisfy the rule.
9. Placement fails with `BlockedByPawn` when the target voxel's AABB is within
   `R_P + 2 cm` of the placing player's capsule segment, and also when it overlaps any mob.
10. A player standing flush against a wall can place on that wall (the `Skin` margin is
    effective).
11. A player standing on a block can place the block directly under their feet.
12. Placement at `Z = 63` succeeds; at `Z = 64` and `Z = -1` it fails with `OutOfBounds`.
13. Placement in an unloaded chunk fails with `ChunkUnloaded` and does not crash.
14. A torch placed on the top face of a solid block succeeds with `Meta = 0`; on the bottom
    face of an overhang it fails with `BadAttach`; on a wall it succeeds with the matching
    `Meta` **when** `bAllowWallTorches` is true, and fails otherwise.
15. A torch cannot be placed on Air, Water or another Torch (`BadAttach`).
16. Exactly one item is removed from the selected quick slot per successful placement; the
    slot clears and renders at opacity `0.0` when the last item is placed, and `SelectedID`
    does not auto-advance.
17. A rejected placement consumes no item, plays no sound and does not start the cooldown; the
    next valid press succeeds immediately.
18. Two placements in the same frame result in exactly one commit (`Cooldown` on the second).
19. A block placed on a chunk seam remeshes both the owner chunk and the boundary neighbour;
    no hole appears in the neighbour's culled faces.
20. The chunk containing a placed block is flagged `bIsModified` and its delta is saved
    immediately; reloading the world preserves the placed block.
21. Placement plays `SFX_BlockPlace_<Material>` at the voxel location (and `SFX_Torch` for a
    torch), and emits a `NoiseEvent` with loudness 6 (2 for a torch).
22. A missing placement SFX asset logs a warning and still commits the world change.
23. Breaking the block under a torch, or the wall a torch is attached to, pops the torch and
    drops it as a pickup.
24. Right-clicking a Crafting Table or Furnace within the interaction distance opens the panel
    instead of placing the held block, even when the selected slot holds a placeable block.
25. In multiplayer, a client requesting a placement out of reach, or without the item in the
    authoritative inventory, is rejected and its local prediction is rolled back.
26. An automation test covers: `AxisStep` for all six normals, `HasSupport` (face vs diagonal),
    `Replaceable`, torch `AttachOK` for all six directions, capsule overlap for
    stand-in / stand-on / flush-wall cases, slot consumption and slot clearing, and the
    support-loss cascade.

---

## Appendix A — Implementation status vs this specification

The current source already contains a `AWorldGenerator::PlaceBlock` implementation
(`WorldGenerator.cpp:1763-1842`). It is **not a stub**, but it implements a strict subset of
this specification. The gaps, in priority order:

| # | Gap | Current behaviour | Required by |
|---|---|---|---|
| 1 | **No inventory consumption** | `PlaceBlock` never touches `UQuickSlotsInventorySystem`; the player can place unlimited blocks from an empty hand | §3.8, AC 16 |
| 2 | **No pawn overlap check** | Placement inside the player's capsule succeeds; the player is stuck inside the block | §3.7, AC 9 |
| 3 | **Torch support is floor-only** | Only checks `VoxelZ - 1` for Air/Torch (`:1772-1792`); no wall mount, no meta | §3.5, AC 14 |
| 4 | **No cooldown** | Every press commits | §3.9, AC 18 |
| 5 | **No reach validation** | `PlaceBlock` trusts its `WorldLocation`; only the Blueprint trace bounds it | §4.3, AC 3 |
| 6 | **No placement voxel derivation** | The caller supplies `WorldLocation`; the epsilon outward sampling lives in `TraceForBlock` (`:1896-1905`) | §4.2, AC 2 |
| 7 | **No noise / SFX / view-model cue** | Silent commit | §3.11, AC 21 |
| 8 | **Cascade is torch-above-only** | `BreakBlockAtVoxel` pops only `VoxelZ + 1` (`:1668-1680`); wall torches would float | §3.12, AC 23 |
| 9 | **Replaceable set is Air-only** | `:1796-1799` rejects anything non-Air, so Water and Torch are `Occupied` | §3.6, AC 7 |
| 10 | **`bIsPlaceable` / `bIsReplaceable` are not data** | Hardcoded ID checks (`19`, `0`) instead of DataTable columns | §3.3, `ue-data.md` |
| 11 | **No multiplayer path** | Single-player only; no `ServerPlaceBlock` RPC | §3.13, AC 25 |
| 12 | **`AddTorchMesh` ignores orientation** | `(MeshData, BlockPos, TextureIndex)` only; always vertical crossed planes | §3.5 |

**Suggested landing order:** 1 → 2 → 9 → 5 → 4 → 7 → 8 → 3 → 12 → 11 → 6 → 10.
Items 1, 2 and 9 are correctness and exploit issues and should land before any feature work;
items 3 and 12 are the only ones that require mesh changes; items 6 and 10 are refactors that
should accompany the ADR for the placement pipeline.

**Related document drift to reconcile (not edited by this document):**

- `block-interaction.md` §7 lists `BlockPlaceCooldown` default `0 s`; this GDD sets `0.20 s`.
  `block-interaction.md` §7 should be updated to match, or the knob renamed, so the two
  documents do not disagree.
- `block-interaction.md` §3.6 describes placement without a reach value and without a support
  rule. §3.6 should be reduced to a pointer at this document.
- `block-interaction.md` §4.8 already defines a `TorchSupportOK` predicate with five
  neighbours (floor + 4 walls) — it is **closer** to this spec than the code is, but it omits
  the ceiling exclusion and the meta encoding. §4.5 of this document supersedes it.
