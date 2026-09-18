# Game Design Document — MinecraftDeWish

---

## 1. Core Philosophy & Player Loop

**MinecraftDeWish** recreates the classic Minecraft sandbox survival experience with an enhanced equipment model (Baubles system) and modular Unreal Engine 5 C++ architecture.

```
       ┌───────────────────────────────┐
       │   Explore & Generate World    │
       └──────────────┬────────────────┘
                      │
                      ▼
       ┌───────────────────────────────┐
       │  Mine Resources & Gather XP   │
       └──────────────┬────────────────┘
                      │
                      ▼
       ┌───────────────────────────────┐
       │ Craft Tools & Smelt Ores      │
       └──────────────┬────────────────┘
                      │
                      ▼
       ┌───────────────────────────────┐
       │ Equip Baubles & Upgrade Tiers │
       └──────────────┬────────────────┘
                      │
                      ▼
       ┌───────────────────────────────┐
       │ Survive Night & Hostile Mobs  │
       └──────────────┬────────────────┘
                      │
                      └───────► (Loop)
```

---

## 2. World & Environment Systems

### 2.1 Coordinate Conventions & Voxel Scaling
- **Voxel Size**: 100 Unreal Units (UU) = 1.0 meter = 1 block.
- **Chunk Dimensions**: 16 × 16 × 64 blocks ($1600 \times 1600 \times 6400$ UU).
- **Chunk Indexing**: 2D grid coordinates $(ChunkX, ChunkY)$, with $Z$ ranging from $0$ (bedrock floor) to $63$ (sky).

### 2.2 Day / Night Cycle
- **Cycle Duration**: 20 minutes real-time (1200 seconds).
  - **Day (0.00 – 0.50)**: 10 minutes. Sun angle traverses from eastern horizon ($0^\circ$) to zenith ($90^\circ$) to western horizon ($180^\circ$). Hostile mobs exposed to sunlight burn.
  - **Night (0.50 – 1.00)**: 10 minutes. Moonlight conditions; hostile mobs spawn in unlit areas.
- **Directional Light Coupling**: `UDayNightCycleSystem` smoothly rotates the sun actor Pitch based on `TimeOfDay`.

---

## 3. Block Interaction & Mining Mechanics

### 3.1 Dual Collision Mesh
In standard Minecraft, certain blocks (such as Torches, Tall Grass, Flowers, and Seeds) cannot impede player physics movement, but must still be interactable.
- **Solid Blocks**: Spawn on section 0 with `ECollisionEnabled::QueryAndPhysics`.
- **Non-Solid Blocks**: Spawn on section 1 with `ECollisionEnabled::QueryOnly`, setting `ECC_Pawn` to `ECR_Overlap` and `ECC_WorldStatic` to `ECR_Block`. Players walk through torches without getting snagged, yet line traces hit them accurately.

### 3.2 Block Breaking Formula
$$\text{BreakTime} = \frac{\text{Block Durability}}{\text{Effective Mining Force}}$$

Where $\text{Effective Mining Force}$ is calculated automatically from the player's equipped Baubles tool matching the block's preferred category:
- Matching Tool: Multiplier based on tier (Wood: $2\times$, Stone: $4\times$, Iron: $6\times$, Diamond: $10\times$, Obsidian: $15\times$).
- Unarmed / Wrong Tool: Base multiplier ($1.0\times$).

---

## 4. The Baubles Equipment System

### 4.1 Concept
In traditional Minecraft, quickslots are constantly cluttered with tools (pickaxe, axe, shovel, sword, hoe). The Baubles system gives tools dedicated inventory equipment slots:
1. **Pickaxe Slot**: Automatically utilized when mining stone, ores, and masonry.
2. **Shovel Slot**: Automatically utilized when mining dirt, grass, sand, and gravel.
3. **Axe Slot**: Automatically utilized when chopping logs, planks, crafting tables, and wood fences.
4. **Hoe Slot**: Dedicated for agricultural soil tilling.
5. **Sword Slot**: Automatically applied to weapon melee damage.

### 4.2 Auto-Tool Selection
Players do not need to switch active hotbar slots while mining. Pointing at a stone block automatically engages the equipped Pickaxe; swinging at dirt seamlessly activates the Shovel.

### 4.3 UI Toggle
Pressing **B** (`IA_Baubles`) opens and closes the dedicated Baubles equipment panel, broadcasting `OnBaublesToggled`.

---

## 5. Crafting & Smelting Interaction

### 5.1 Proximity Interaction
Right-clicking a block within 4 blocks ($400$ UU) initiates interactive subsystems:
- **Crafting Table (Block ID 15)**: Fires `OnCraftingTableOpened` delegate to display the crafting interface.
- **Furnace (Block ID 16)**: Fires `OnFurnaceOpened` delegate to display the smelting interface.

### 5.2 Smelting Burn Rates & Fuel Efficiency
- **Coal / Charcoal**: 80 seconds burn time (8 items smelted per unit).
- **Wood Log**: 15 seconds burn time (1.5 items smelted).
- **Wood Planks**: 10 seconds burn time (1 item smelted).

---

## 6. Combat, Mobs & Player Vitals

### 6.1 Player Health & Regeneration
- **Max Health**: 20.0 HP.
- **Natural Regen**: 0.25 HP/s (1 HP every 4s) when health is below maximum and alive.
- **Death**: Player is ragdolled/frozen, death event broadcasted, and respawns on terrain surface with XP loss penalty.

### 6.2 Hostile Mob Roster
1. **Zombie**: Melee pursuer, burns in daylight, drops Rotten Flesh / Dirt.
2. **Skeleton**: Ranged pursuer, burns in daylight, drops Bones / Arrows.
3. **Spider**: Fast low-profile stalker that **climbs vertical walls** to chase the player over 1-block and multi-block obstacles.
4. **Creeper**: Silent approach; within 3 blocks, activates a 1.5s fuse timer (cancels if player backs away). On detonation, deals high radial damage and carves a 3-block spherical crater into the voxel world.

### 6.3 Physical XP Orbs
- Upon mob defeat, physical `AXPOrbPickup` polyhedral green orbs bounce into the world.
- When within 3.5m of the player, they fly towards the player via magnet attraction.
- Touching the player grants XP, advancing levels according to $XP = 7 + (Level \times 3)$.
