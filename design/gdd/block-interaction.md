# GDD — Block Interaction (Mining, Placement, Interaction)

| Field | Value |
|---|---|
| Status | Draft (2026-11-11) |
| System owner | game-designer / gameplay-programmer / technical-artist |
| Related | `Docs/BLOCK_REGISTRY.md`, `Docs/SYSTEMS_REFERENCE.md` §1–2, §14, `design/gdd/weapons-tools.md`, `design/gdd/items-loot.md`, `design/gdd/crafting-smelting.md`, `Docs/GAME_DESIGN.md` §3 |
| Source of truth for current code | `WorldGenerator.h/.cpp`, `ChunkActor.h/.cpp`, `BlockCrackActor.h/.cpp`, `ChaosBlockDestructionActor.h/.cpp`, `XPOrbPickup.h/.cpp`, `BaublesSystem.h/.cpp`, `QuickSlotsInventorySystem.h/.cpp` |
| Decisions | ADR-0003 (crack overlay driver), ADR-0002 (SetBlockGenerated / chunk modification flagging) |

---

## 1. Overview

Block Interaction is the **core verb loop** of MinecraftDeWish: the player looks at a voxel, holds the mouse button, and the block breaks after a deterministic delay; right-clicks a block to place a new one; and right-clicks interactive blocks (Crafting Table, Furnace) to open their UI. Every other system — crafting, smelting, inventory, combat, XP — feeds or is fed by this loop.

The system is split across three cooperating actors:

- **`AWorldGenerator`** — the coordinator. Owns `BreakBlockAtVoxel`, `QueueBlockBreakAtVoxel`, `PlaceBlock`, `InteractWithTargetBlock`, the crack-overlay bookkeeping (`UpdateCrackMining`, `ClearCrackMining`), and the debris/XP spawn on a successful break.
- **`AChunkActor`** — the per-chunk mesh owner. Computes the exact break time via `GetBlockBreakTimeAtVoxel` using the `Durability` column of `Block_DataTable` and the tool's effective mining force.
- **`ABlockCrackActor`** — a separate overlay actor that draws one of ten `destroy_stage_*` textures over the targeted block so the player sees damage accumulate (ADR-0003).

Breaking a block also spawns **ballistic debris** (`AChaosBlockDestructionActor`, 32–44 fragments) and, for mob deaths, **XP orbs** (`AXPOrbPickup`) with a 3.5 m magnet. Block drops are spawned as `ABlockItemPickup` instances (see `design/gdd/items-loot.md` §3.10).

The loop is **animation-driven, not timer-driven**: mining cadence comes from the impact notify of the per-class swing montage (`weapons-tools.md` §3.7), so the crack overlay and durability consumption cannot desync from the visual.

---

## 2. Player Fantasy

> *"The block I'm looking at is already half-gone. I can feel the next hit land."*

The fantasy is **tactile predictability with a horror edge**. The player should feel that:

- **Mining is a contract.** The number on screen (break time) is the truth. A dirt block breaks in half a second with hands; a diamond ore block takes five seconds with a stone pickaxe. The player never wonders "is it stuck?" — the crack overlay answers every frame.
- **The world fights back slowly.** Breaking a block is loud, visible, and leaves debris. In a game where sound and attention attract Angels, every mining stroke is a risk decision, not just a resource decision. The player learns to mine *quietly*, in lit areas, with an escape route.
- **Placement is decisive.** A placed block is permanent geometry: it changes collision, light, and Angel sightlines. Placing a torch is an act of claiming ground; placing a wall is an act of buying time.
- **Interactions are proximity-based and unambiguous.** Right-clicking within 4 blocks of a Crafting Table or Furnace opens its panel. No "use" key ambiguity, no long menus — one gesture, one result.
- **Debris is weight, not decoration.** Fragments scatter, tumble, and settle. They tell the player "something was here" and, in horror terms, "something heard that."

---

## 3. Detailed Rules

### 3.1 Targeting

- The player's crosshair raycasts against the world every frame (via the player controller / `PlayerActionComponent`) and reports the **targeted voxel** (`FIntVector`) and the **hit normal** / **hit point**.
- `AWorldGenerator::GetTargetedBlockType()` returns the targeted block ID for UI decisions (e.g. showing the block name in the HUD).
- A target is valid only if it is a **mineable, non-air block** within the player's reach (default 5 blocks / 500 cm; see §7). Bedrock (ID 25) is targeted but never mineable.
- The targeted block drives **auto-tool selection** (`UBaublesSystem`, see `weapons-tools.md` §3.3): looking at stone equips the Pickaxe, dirt equips the Shovel, wood equips the Axe, within a 175 ms debounce.

### 3.2 Mining (block breaking)

- Mining is initiated by **holding the primary attack** (`IA_PrimaryAction`) while a valid target is under the crosshair.
- `UMiningQueueSystem` queues the break. The number of concurrent queue slots is tier-gated (see `weapons-tools.md` §4.1, `MaxQueueSlots` 1–6).
- `AWorldGenerator::QueueBlockBreakAtVoxel(VoxelX, VoxelY, VoxelZ, MiningForceOverride)` is called. If `MiningForceOverride` is 0, the world generator queries `UBaublesSystem` for the best mining force for that block.
- Break time is computed by `AChunkActor::GetBlockBreakTimeAtVoxel(MiningForce)`:
  $$\text{BreakTime} = \frac{\text{BlockDurability}}{\text{EffectiveMiningForce}}$$
- On a successful break, `AWorldGenerator::BreakBlockAtVoxel` executes:
  1. Destroys the block in the voxel grid.
  2. Updates the chunk meshes (solid + non-solid) for the affected chunk(s).
  3. Spawns `AChaosBlockDestructionActor` debris at the voxel centre, biased by the hit normal and hit point.
  4. Spawns an `ABlockItemPickup` at the block centre with a small upward+outward launch (see `items-loot.md` §3.10).
  5. Calls `UBaublesSystem::ConsumeToolDurability(BlockID)` — deducts 1 durability from the matching tool only.
  6. Grants mining XP: `XPPerBlock = 1` base, `XPOreMultiplier = 2` for ore blocks (see `items-loot.md` §4.7).
  7. Destroys the crack overlay for that voxel (it will re-target the block below on the next mining tick).
- **Durability consumption** happens only for the matching tool. A non-matching tool mines at hand speed (1.0×) and consumes 0 durability (see `weapons-tools.md` §3.4).
- Mining cadence is driven by the **impact notify** of the swing montage (`AnimNotify_ActionWindow`), not by a Blueprint timer (ADR-0006 §6). This guarantees the crack overlay and durability consumption stay in sync with the visual.

### 3.3 Crack overlay

- While a block is being mined, `AWorldGenerator::UpdateCrackMining(VoxelCoord, Progress01)` drives the `ABlockCrackActor` overlay.
- `Progress01 = BreakTime / GetBlockBreakTimeAtVoxel(LastTargetedBlockVoxel, CurrentMiningForce)` — the same formula as the break test, so stage 9 lands on the same frame as the break (ADR-0003).
- `GetStageFromProgress(Progress) = Floor(Clamp(Progress, 0, 1) × 10)`, clamped to `[0, 9]`.
- The overlay is keyed **per voxel** in `BlockCrackOverlays` (`TMap<FIntVector, TWeakObjectPtr<ABlockCrackActor>>`), but **only one may be alive**: every `UpdateCrackMining` first destroys overlays on all other voxels (`ShouldKeepCrackOverlay`). Changing target moves the overlay instead of stacking.
- Geometry:
  - **Cube blocks**: six per-face quads with 0..1 UVs, pushed `0.75 UU` outward along each face normal to win the depth test against the chunk surface without z-fighting. All six faces are generated (hidden faces are not culled because the overlay is a separate actor).
  - **Torches**: two crossed vertical diagonal planes mirroring `AChunkActor::AddTorchMesh`, each double-sided and offset `0.75 UU` along both normal directions. Local Z spans `-0.5·S .. +0.3·S` and X/Y `±0.35·S`.
  - Actor location is the block centre: `(Voxel + 0.5) × BlockScale`.
- Material: dynamic instance of `M_BlockCrack` (Masked / Unlit / TwoSided). `OpacityMask = CrackTex.A`, `Emissive = CrackTex.RGB × 0.55`, `OpacityMaskClipValue = 0.15`. The mask comes from the **alpha** channel (the source PNGs are near-greyscale in RGB while alpha is 0 or 255).
- `OnCrackStageChanged(VoxelCoord, Stage)` is broadcast only on a stage transition (not every tick), so HUD/debug consumers do not need to poll.
- `ClearCrackMining()` destroys every live overlay. Called when mining is cancelled (`IA_Break.Canceled|Completed`).
- `UpdateCrackMining` with an invalid voxel, air, or **Bedrock** destroys the overlay at that spot instead of drawing on it.
- `BreakBlockAtVoxel` destroys the overlay of the voxel it turns to air, then the next mining tick re-targets the block below.

### 3.4 Ballistic debris

- `AChaosBlockDestructionActor` spawns 32–44 small fragments per broken block, roughly 5–9 cm across (`FBlockDestructionParticle::HalfExtents` defaults to 3.5 UU).
- Fragments are **not** convex fracture cells and carry no per-fragment collision component. They share one `UProceduralMeshComponent`; buffers are rebuilt each tick, so the whole burst costs one draw call.
- **Visual texture mapping**:
  - Outer faces reuse the broken block's exact sub-UV face coordinates and `Texture2DArray` slice, so debris matches `M_Global`.
  - Freshly exposed faces sample an interior slice (Dirt under Grass, planks under logs, stone under ores), resolved by `ResolveBlockTextures()` against `Block_DataTable`.
- **Simulation** (manual integration, not the physics engine):
  - Per-tick velocity integration with `GravityStrength = 1350 cm/s²`.
  - Per-particle ground sweeps against world geometry; `bIsGrounded` latches when a fragment settles.
  - Initial burst is biased by the mining strike normal and hit point, plus upward pop and randomized angular velocity.
- **Lifecycle**: `SimulationLifetime = 2.0 s` of tumbling, then a `FadeDuration = 0.8 s` shrink-and-fade, then the actor destroys itself.
- **Tunables** (all `EditAnywhere`): `BlockScale`, `SimulationLifetime`, `FadeDuration`, `GravityStrength`.
- **Naming note**: the class is named "Chaos" but does **not** use the Chaos destruction solver. It is a hand-written ballistic debris simulation. Do not document it as Chaos destruction.

### 3.5 XP orbs

- Upon mob defeat, physical `AXPOrbPickup` polyhedral green orbs (procedural diamond octahedron mesh with glowing green-yellow vertex colors) bounce into the world.
- Dynamics: physics parabolic bounce, idle rotational bobbing, and **player magnet attraction within 3.5 m** (350 cm).
- On contact with the player, deposits experience into `UPlayerVitalSystem` (`AddXP`), advancing levels according to `XP = 7 + (Level × 3)`.
- Block mining grants XP separately (§3.2 step 6), deposited directly into `UPlayerVitalSystem` without spawning an orb. Only mob kills spawn physical orbs.

### 3.6 Placement

- `AWorldGenerator::PlaceBlock(WorldLocation, BlockID)` validates placement and updates meshes.
- **Validation rules**:
  - The target location must be **air** (ID 0) or a replaceable block.
  - The block must not overlap the player's collision capsule (no placing inside the player).
  - **Torch resting constraints**: a torch (ID 19) requires a solid block below or beside it to rest on; placement fails if no valid support exists.
  - Placement is blocked if the target voxel is outside the loaded world or in an unloaded chunk.
- On success:
  1. Sets the voxel to `BlockID` in the chunk data.
  2. Flags the chunk as modified (see ADR-0002).
  3. Re-meshes the affected chunk(s).
  4. Consumes one unit of the placed item from the selected quickslot (see `items-loot.md` §3.7).
- The placed block ID comes from the **selected quickslot** item. If the selected slot is empty or holds a non-block item, placement is a no-op (see `items-loot.md` §3.12: "a non-block item cannot be placed").
- Placement uses the **secondary attack** (`IA_SecondaryAction`) with the crosshair raycast determining the target voxel (adjacent to the hit face).

### 3.7 Interaction (crafting table, furnace)

- `AWorldGenerator::InteractWithTargetBlock()` handles proximity right-click interaction with Crafting Tables (ID 15) and Furnaces (ID 16).
- **Proximity rule**: the targeted block must be within **4 blocks (400 UU)** of the player.
- On interaction:
  - **Crafting Table (ID 15)**: broadcasts `OnCraftingTableOpened` delegate → displays the crafting interface (see `crafting-smelting.md`).
  - **Furnace (ID 16)**: broadcasts `OnFurnaceOpened` delegate → displays the smelting interface.
- Interaction is triggered by `IA_Interact` (right-click) while looking at the target block.
- If the targeted block is neither a Crafting Table nor a Furnace, the interaction is a no-op (falls through to placement if a block item is selected).
- The interaction distance (4 blocks) is a tuning knob; see §7.

### 3.8 Dual collision mesh

The chunk mesh is split into two sections with different collision profiles (see `GAME_DESIGN.md` §3.1):

| Mesh section | Blocks | Collision mode | Pawn interaction | Trace interaction |
|---|---|---|---|---|
| **SolidMesh** (section 0) | All solid blocks (Dirt, Stone, Wood, etc.) | `QueryAndPhysics` | Blocks movement | Hit by line traces |
| **NonSolidMesh** (section 1) | Non-solid blocks (Torch only in v1) | `QueryOnly` | `ECC_Pawn` → `ECR_Overlap` (players walk through) | `ECC_WorldStatic` → `ECR_Block` (line traces hit) |

- `FBlockHelpers::IsNonSolidBlock()` returns true for `Torch` and nothing else in v1. Fence, Fence Gate, Door and Glass are rendered and collided as **full opaque cubes** despite the `Transparent` flag.
- Players walk through torches without getting snagged, yet line traces (mining, interaction) hit them accurately.
- Face culling: a face is generated when the neighbouring block is transparent (`AWorldGenerator::IsBlockTransparent`). Never cull a face behind a leaf, because `M_Global` is a Masked (alpha-tested) single-sided material.

---

## 4. Formulas

### 4.1 Break time

$$\text{BreakTime (s)} = \frac{\text{BlockDurability}}{\text{EffectiveMiningForce}}$$

Where `BlockDurability` comes from the `Durability` column of `Block_DataTable` (see `Docs/BLOCK_REGISTRY.md`), and `EffectiveMiningForce` is computed as follows:

### 4.2 Effective mining force

$$
\text{EffectiveMiningForce} =
\begin{cases}
1.0 & \text{hand (no tool equipped), or non-matching tool} \\
\text{BaseMiningForce(Tier)} \times \text{MiningForceByBlockCategory[BlockCategory]} & \text{matching tool equipped}
\end{cases}
$$

- **Hand**: `1.0` (base hand mining force, `AChunkActor::GetHandMiningForce()`).
- **Matching tool**: `BaseMiningForce(Tier) × MiningForceByBlockCategory[BlockCategory]`. The tool's base force is multiplied by a category specialisation multiplier (default 1.0).
- **Non-matching tool**: `1.0` (hand speed) and **no durability consumed**.

Tier base mining forces (`FToolInstance::GetBaseMiningForce`):

| Tier | `BaseMiningForce` |
|---|---|
| None (Hand) | 1.0 |
| Wood | 2.0 |
| Stone | 4.0 |
| Iron | 6.0 |
| Diamond | 10.0 |
| Obsidian | 15.0 |

Category specialisation (`MiningForceByBlockCategory`, default 1.0 for all; examples):

| Block category | Pickaxe | Shovel | Axe | Hoe |
|---|---|---|---|---|
| Stone / Masonry | 1.0 | 0.5 | 0.5 | 0.5 |
| Earth / Sand | 0.5 | 1.0 | 0.5 | 0.5 |
| Wood / Planks | 0.5 | 0.5 | 1.0 | 0.5 |
| Plant / Farmland | 0.5 | 0.5 | 0.5 | 1.0 |

A non-matching tool's category multiplier is effectively 0 (falls back to hand speed 1.0).

### 4.3 Break time examples

| Block (durability) | Hand (1.0) | Wood Pickaxe (2.0) | Stone Pickaxe (4.0) | Iron Pickaxe (6.0) | Diamond Pickaxe (10.0) |
|---|---|---|---|---|---|
| Dirt (0.5) | 0.50 s | 0.25 s | 0.125 s | 0.083 s | 0.050 s |
| Stone (1.5) | 1.50 s | 0.75 s | 0.375 s | 0.250 s | 0.150 s |
| Iron Ore (3.0) | 3.00 s | 1.50 s | 0.750 s | 0.500 s | 0.300 s |
| Diamond Ore (3.0) | 3.00 s | 1.50 s | 0.750 s | 0.500 s | 0.300 s |
| Obsidian (—) | ∞ (indestructible) | ∞ | ∞ | ∞ | ∞ |
| Bedrock (25) | ∞ (indestructible) | ∞ | ∞ | ∞ | ∞ |
| Torch (0.05) | 0.05 s | 0.025 s | 0.0125 s | 0.008 s | 0.005 s |
| Leaves (0.2) | 0.20 s | 0.10 s | 0.05 s | 0.033 s | 0.020 s |

### 4.4 Crack stage selection

$$\text{Stage} = \text{Clamp}\left(\left\lfloor \text{Clamp}(\text{Progress01},\, 0,\, 1) \times 10 \right\rfloor,\; 0,\; 9\right)$$

Where:

$$\text{Progress01} = \frac{\text{BreakTime}}{\text{GetBlockBreakTimeAtVoxel}(\text{VoxelCoord},\; \text{CurrentMiningForce})}$$

- Stage 0 = no visible crack (0–10 % progress).
- Stage 9 = block about to break (90–100 % progress).
- Stage 9 lands on the same frame as the break because the same formula is used for both.

### 4.5 XP drop

**Block mining XP** (deposited directly, no orb spawned):

$$
\text{XPGranted} =
\begin{cases}
\text{XPPerBlock} \times \text{XPOreMultiplier} & \text{if block is an ore (IDs 7, 9, 11, 13, 27)} \\
\text{XPPerBlock} & \text{otherwise}
\end{cases}
$$

- `XPPerBlock = 1` (base).
- `XPOreMultiplier = 2` (ore blocks grant 2 XP).

**Mob kill XP** (spawned as `AXPOrbPickup` physical orbs):

$$\text{XPPerMobKill} = 5 \quad \text{(Zombie, Skeleton, Spider, Creeper)}$$

**Level curve** (from `UPlayerVitalSystem`):

$$\text{XPToNextLevel} = 7 + (\text{Level} \times 3)$$

### 4.6 Debris fragment count

$$\text{FragmentCount} = \text{RandRange}(32,\; 44)$$

- Each fragment: `HalfExtents` default 3.5 UU (≈7 cm across).
- `GravityStrength = 1350 \text{ cm/s}^2`.
- `SimulationLifetime = 2.0 \text{ s}` tumbling + `FadeDuration = 0.8 \text{ s}` shrink-and-fade = `2.8 s` total.

### 4.7 XP orb magnet

$$\text{MagnetRadius} = 350 \text{ cm} \quad (3.5 \text{ m})$$

- Magnet activates when the orb is within 350 cm of the player.
- Orbs use physics parabolic bounce + idle rotational bobbing before magnet activates.

### 4.8 Placement validation

$$\text{CanPlace(WorldLocation, BlockID)} = \text{IsAir(TargetVoxel)} \;\land\; \text{NotOverlappingPlayer(TargetVoxel)} \;\land\; \text{TorchSupportOK(BlockID, TargetVoxel)} \;\land\; \text{ChunkLoaded(TargetVoxel)}$$

Where:

$$\text{TorchSupportOK}(19, v) = \text{IsSolid}(v - (0,0,1)) \;\lor\; \text{IsSolid}(v - (1,0,0)) \;\lor\; \text{IsSolid}(v + (1,0,0)) \;\lor\; \text{IsSolid}(v - (0,1,0)) \;\lor\; \text{IsSolid}(v + (0,1,0))$$

(For non-torch blocks, `TorchSupportOK` returns true.)

### 4.9 Interaction distance

$$\text{CanInteract} = \text{Distance(PlayerPos, TargetBlockCentre)} \le 400 \text{ cm} \quad (4 \text{ blocks})$$

---

## 5. Edge Cases

| Case | Resolution |
|---|---|
| Mining Bedrock (ID 25) | Targeted but never mineable; `BreakBlockAtVoxel` is a no-op; crack overlay is destroyed at that spot (not drawn on Bedrock) |
| Mining air (ID 0) | No-op; `BreakBlockAtVoxel` on air does nothing |
| Mining an unloaded chunk | No-op; the voxel is not in any loaded chunk |
| Tool breaks mid-mining (durability reaches 0) | The in-flight queued break still completes; the tool is auto-unequipped after; subsequent mining uses hand speed |
| Non-matching tool equipped | Mines at hand speed (1.0×), no durability consumed, no queue-slot bonus |
| No tool equipped (hand) | `1.0×` speed; no durability consumed (hand has no durability) |
| Changing target mid-mining | Crack overlay moves to the new voxel; the old overlay is destroyed; mining progress on the old block is lost (no partial break persistence) |
| Mining while moving | Allowed; locomotion continues via layered blend on `spine_01` |
| Placing a block inside the player | Blocked by `NotOverlappingPlayer` check |
| Placing a torch with no support | Blocked by `TorchSupportOK` check |
| Placing a non-block item | No-op; only block items (Kind = Block) can be placed |
| Placing in an unloaded chunk | Blocked by `ChunkLoaded` check |
| Right-clicking a non-interactive block | Falls through to placement if a block item is selected; otherwise no-op |
| Right-clicking a Crafting Table / Furnace beyond 4 blocks | No-op; interaction distance check fails |
| Right-clicking a Crafting Table / Furnace within 4 blocks | Fires the corresponding delegate (`OnCraftingTableOpened` / `OnFurnaceOpened`) |
| Crack overlay z-fighting at extreme view distances | Raise the `Epsilon = 0.75f` geometry offset (compile-time constant) |
| Multiple overlays stacking | Prevented: `UpdateCrackMining` destroys all other voxels' overlays first (`ShouldKeepCrackOverlay`) |
| Debris fragment count out of range | Clamped to [32, 44] by `RandRange` |
| Water block (ID 26) mined | Drops itself as a block item; no liquid simulation; behaves as a solid cube for collision |
| Leaves block (ID 17) mined | Drops itself as a block item; 5 % chance of `Item.String` deferred (see `items-loot.md` open questions) |
| Ore block mined | Drops itself as a block item (v1); raw material items deferred until smelting produces them |
| Creeping detonation (3-block crater) | `BreakBlockAtVoxel` called 3× for the crater voxels; each spawns debris and a pickup |
| Angel breach (1 block per 5 s) | `BreakBlockAtVoxel` called per breach; no XP granted (Angel action, not player mining) |
| Crack overlay on a torch | Uses crossed-plane geometry instead of cube; follows torch shape |
| Crack overlay spawn failure (invalid voxel) | Overlay destroyed at that spot; no crash |
| Debris actor lifetime exceeded | Actor self-destructs after 2.0 s + 0.8 s fade; no orphaned actors |
| XP orb magnet range exceeded | Orb does not magnetise; stays on ground; can be picked up on direct contact |

---

## 6. Dependencies

| System | Why | Status |
|---|---|---|
| `AWorldGenerator` | Coordinator: BreakBlockAtVoxel, QueueBlockBreakAtVoxel, PlaceBlock, InteractWithTargetBlock, UpdateCrackMining, ClearCrackMining, GetBlockBreakTimeAtVoxel, GetTargetedBlockType | Implemented |
| `AChunkActor` | GetBlockBreakTimeAtVoxel, GetHandMiningForce, GetCurrentBestToolMiningForce, dual collision mesh, mesh regeneration | Implemented |
| `ABlockCrackActor` | 10-stage crack overlay (ADR-0003) | Implemented |
| `AChaosBlockDestructionActor` | Ballistic debris (32–44 fragments) | Implemented |
| `AXPOrbPickup` | XP orb with 3.5 m magnet | Implemented |
| `ABlockItemPickup` | Block drop pickup (magnet, merge, cooldown) | Implemented for blocks |
| `UBaublesSystem` | Tool mining force, durability consumption, auto-tool selection | Implemented |
| `FToolInstance` / `DT_Tools` | Tool tiers, base mining force, category multipliers | Migrating (ADR-0007) |
| `UMiningQueueSystem` | Concurrent break queue, slot gating | Implemented |
| `UQuickSlotsInventorySystem` | Selected item for placement, stack consumption | Implemented, block-only |
| `UPlayerVitalSystem` | XP deposit on break / mob kill | Implemented |
| `AMobCharacter` | Death → XP orb spawn, loot drops, Creeper crater | Implemented |
| `UCraftingSmeltingSystem` | Crafting Table / Furnace interaction targets | Implemented (block-only) |
| `design/gdd/weapons-tools.md` | Tool tiers, mining force, durability, auto-selection | Draft |
| `design/gdd/items-loot.md` | Block drops, pickup physics, inventory, XP values | Draft |
| `design/gdd/crafting-smelting.md` | Crafting Table / Furnace UI | Draft |
| `Docs/BLOCK_REGISTRY.md` | Block durability, collision modes, tool affinities | Reference |
| `Docs/TOOL_REGISTRY.md` | Tool tier stats | Reference |

---

## 7. Tuning Knobs

| Knob | Default | Safe range | Effect |
|---|---|---|---|
| `MiningReach` | 500 cm (5 blocks) | 300–700 cm | How far the player can mine/place |
| `InteractionDistance` | 400 cm (4 blocks) | 200–600 cm | Crafting Table / Furnace open range |
| `HandMiningForce` | 1.0 | 0.5–2.0 | Base hand mining speed |
| `TierBaseMiningForce[Wood..Obsidian]` | 2/4/6/10/15 | ±25 % | Tool mining speed per tier |
| `MiningForceByBlockCategory` | 1.0 default | 0.5–2.0 | Tool specialisation multiplier |
| `CrackOverlayEpsilon` | 0.75 UU | 0.5–1.5 UU | Z-fighting prevention offset |
| `DebrisFragmentCount` | 32–44 | 16–64 | Visual density of debris |
| `DebrisFragmentSize` | 3.5 UU (HalfExtents) | 2–6 UU | Fragment size |
| `DebrisGravity` | 1350 cm/s² | 800–2000 cm/s² | Fragment fall speed |
| `DebrisSimulationLifetime` | 2.0 s | 1.0–4.0 s | Tumbling duration |
| `DebrisFadeDuration` | 0.8 s | 0.3–1.5 s | Fade-out duration |
| `XPOrbMagnetRadius` | 350 cm (3.5 m) | 200–500 cm | XP orb pickup reach |
| `XPPerBlock` | 1 | 0–3 | Mining XP faucet |
| `XPOreMultiplier` | 2 | 1–4 | Ore XP bonus |
| `MaxQueueSlots[None..Obsidian]` | 1–6 | 1–8 | Concurrent mining queue depth |
| `TorchSupportRequired` | true | — | Whether torches need a support block |
| `BlockPlaceCooldown` | 0 s (none) | 0–0.5 s | Anti-spam delay between placements |

---

## 8. Acceptance Criteria

1. Mining a dirt block (durability 0.5) with hands (1.0×) breaks it in exactly 0.5 seconds.
2. Mining a stone block (durability 1.5) with an iron pickaxe (6.0×) breaks it in exactly 0.25 seconds.
3. `BreakTime` matches `BlockDurability / EffectiveMiningForce` within 1 frame; the crack overlay reaches stage 9 on the same frame as the break.
4. A non-matching tool mines at hand speed (1.0×) and consumes 0 durability.
5. A matching tool consumes exactly 1 durability per successful break.
6. The crack overlay advances through stages 0–9 and is destroyed when mining is cancelled or the block breaks.
7. Only one crack overlay is alive at any time; changing targets moves it.
8. Bedrock and air targets do not spawn a crack overlay.
9. Breaking a block spawns 32–44 debris fragments that tumble for 2.0 s, fade for 0.8 s, and self-destruct.
10. Debris fragments reuse the broken block's texture on outer faces and an interior slice on exposed faces.
11. An XP orb spawns on mob death and magnets toward the player within 3.5 m, depositing XP on contact.
12. Block mining grants 1 XP (2 for ores) deposited directly into `UPlayerVitalSystem`.
13. Placing a block on air fails; placing inside the player fails; placing a torch without support fails.
14. Right-clicking a Crafting Table or Furnace within 4 blocks fires the corresponding delegate.
15. Right-clicking a Crafting Table or Furnace beyond 4 blocks is a no-op.
16. Non-block items cannot be placed; placement is a no-op if the selected slot is empty.
17. Solid blocks use `QueryAndPhysics` collision; non-solid blocks (torch) use `QueryOnly` with `ECC_Pawn` overlap and `ECC_WorldStatic` block.
18. An automation test covers: break time formula, crack stage selection, tool matching vs non-matching, placement validation, and interaction distance.
19. Debris fragments share a single draw call (one `UProceduralMeshComponent` per burst).
20. The crack overlay texture mask comes from the alpha channel, not luminance.