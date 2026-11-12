# REA Spec: Block Placement — Product & Outcome

**Target:** Minecraft Java Edition 1.21.x (reference behavior)
**Mode:** Clean room (observation of player-facing behavior only)
**Date:** 2026-11-12

## 1. Input Contract

### What the system receives
- **Player input:** right-click (use item) on a block face or air
- **Selected item:** the item in the player's main hand (quick slot)
- **World state:** block at target position, adjacent blocks, player position, player collision box
- **Block properties:** placeable flag, replaceable flag, support requirements, attachment rules

### Triggers
- Right-click (use) on a block face → place block adjacent to that face
- Right-click on air (no hit) → use item (not placement) — unless item is a block
- Right-click on a replaceable block (grass, snow, torch) → replace it
- Right-click on a non-replaceable block → place adjacent

## 2. Output Contract

### What the system produces
- **Visual:** block appears at target position, with correct orientation/rotation
- **Audio:** placement sound (material-specific: stone, wood, grass, etc.)
- **State change:** block ID written to world, chunk marked dirty, mesh updated
- **Item consumption:** 1 item removed from inventory (if not creative)
- **Side effects:** torches cascading when support removed, neighbor block updates (redstone, water flow), particles

### Observable behavior
- Placement is instant (no animation delay)
- Block appears with correct rotation based on player facing and block type
- Placement sound plays at the block position
- Particles (small dust) may spawn at placement
- The block's hitbox is immediately active (collision)

## 3. Behavior Matrix

| Input | Condition | Output |
|---|---|---|
| Right-click on solid block face | Item is placeable block, target empty | Block placed adjacent, item consumed, sound played |
| Right-click on solid block face | Item is placeable block, target occupied (non-replaceable) | Nothing happens (no consume) |
| Right-click on solid block face | Item is placeable block, target is replaceable (grass/torch/snow) | Replaceable block replaced, item consumed |
| Right-click on solid block face | Item is not a block (tool/weapon) | Item used (not placed) |
| Right-click on air | No block hit | Item used (not placed), unless item is block → nothing |
| Right-click on water/lava | Target is water/lava | Block placed in water (if allowed), water displaced |
| Right-click while sneaking | Player sneaking | Block placed (sneaking doesn't prevent placement) |
| Right-click at max reach | Distance > 4.5 blocks (4.5m) | Nothing happens (no consume) |
| Right-click inside player | Target voxel overlaps player AABB | Nothing happens (no consume) |
| Right-click on torch | Torch is replaceable | Torch replaced, item consumed |
| Right-click on snow layer | Snow layer is replaceable (if thin) | Snow replaced, item consumed |
| Right-click with empty hand | No item | Nothing happens |
| Right-click on bedrock | Target is bedrock | Nothing happens (bedrock is not replaceable) |

## 4. Edge Cases

| Edge case | Expected behavior |
|---|---|
| Place block inside player | Rejected (no consume, no place) |
| Place block at world boundary (Y=0 or Y=255) | Rejected (out of bounds) |
| Place block in unloaded chunk | Rejected (chunk not loaded) |
| Place torch on ceiling | Rejected (torch can't attach to ceiling) |
| Place torch on non-solid block | Rejected (no support) |
| Place block on floating support | Rejected (no adjacent solid block) |
| Place block while mining | Mining continues, placement succeeds if valid |
| Place block while sprinting | Placement succeeds (no movement penalty) |
| Place block with off-hand | Uses main hand item (off-hand not used for placement) |
| Place block in creative mode | No item consumed, block placed |
| Place block on a door | Door is not replaceable → place adjacent |
| Place block on a slab | Slab is replaceable (if same type) → replace; otherwise place adjacent |
| Place block underwater | Block placed, water displaced (if block is not water-logged) |
| Place block on a fence | Fence is not replaceable → place adjacent (on top) |
| Rapid placement (hold right-click) | Each placement has a small cooldown (~4 ticks / 0.2s) to prevent spam |

## 5. Dependencies

| System | Relationship |
|---|---|
| WorldGenerator | PlaceBlock, TraceForBlock, GetVoxelAt, SetBlock |
| ChunkActor | SetBlock, UpdateMesh, RemeshVoxelNeighbourhood |
| VoxelPlacementRules | FPlacementRules (all pure functions) |
| QuickSlotsInventorySystem | GetQuickSlotData, SetQuickSlotData, GetSelectedQuickSlotIndex |
| BlockHighlightActor | Visual feedback (ghost block) |
| Audio system | Placement sounds (SFX_BlockPlace_<Material>) |
| VoxelBenchSubsystem | Performance tracking |

## 6. Tuning Knobs

| Knob | Default | Range | Effect |
|---|---|---|---|
| `PlacementReach` | 400 cm (4 blocks) | 200-600 | How far the player can place blocks |
| `ReachTolerance` | 50 cm | 0-100 | Tolerance for reach check |
| `PlaceCooldown` | 0.20 s | 0.0-1.0 | Minimum time between placements |
| `PlacementNoiseLoudness` | 6 (default) / 2 (torch) | 1-12 | Noise level for mob awareness |
| `bAllowWallTorches` | false | true/false | Whether torches can be placed on walls |
| `bAllowCeilingTorches` | false | true/false | Whether torches can be placed on ceilings |

## 7. Acceptance Criteria

- [ ] Right-click on a solid block face places the selected block adjacent
- [ ] Right-click on a replaceable block replaces it
- [ ] Right-click on air does nothing (no consume)
- [ ] Right-click beyond 4.5 blocks does nothing (no consume)
- [ ] Right-click inside player AABB does nothing (no consume)
- [ ] Right-click with empty hand does nothing
- [ ] Right-click on non-placeable item does nothing
- [ ] Placement consumes 1 item from the quick slot
- [ ] Placement sound plays at the block position
- [ ] Placement updates the chunk mesh immediately
- [ ] Neighbor chunks remesh when placing on a border
- [ ] Torch placement on a wall works (when enabled)
- [ ] Torch cascade: removing a support block pops attached torches
- [ ] Rapid right-click has a 0.2s cooldown (no spam)
- [ ] Placement works in PIE without crashes

## 8. Implementation Status

**Code status:** `TryPlaceBlock` and all helper functions are IMPLEMENTED in `WorldGenerator.cpp`. `IA_PlaceBlock` input action created. `HandlePlacePressed` wired in `GameplayPlayerController`. 

**Remaining work:**
- Wire `IA_PlaceBlock` to a key in the input mapping context (manual in editor)
- Test in PIE: place blocks, verify consumption, sound, mesh update
- Add automation tests for the 10 gates