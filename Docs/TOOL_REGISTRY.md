# Tool & Baubles Equipment Registry — MinecraftDeWish

This document details the tool specialization system, tier progression, mining speed multipliers, durability costs, combat statistics, and Patrix PBR icon paths.

---

## Tool Categories & Block Affinities

| Tool Type | Preferred Block Types | Primary Gameplay Function |
|---|---|---|
| **Pickaxe** (`EToolType::Pickaxe`) | Stone, Cobblestone, Coal Ore, Iron Ore, Gold Ore, Diamond Ore, Emerald Ore, Iron/Gold/Diamond Blocks, Furnace | Rock excavation, mineral harvesting, masonry mining |
| **Shovel** (`EToolType::Shovel`) | Dirt, Grass Block, Sand, Gravel | Earth displacement, terrain reshaping |
| **Axe** (`EToolType::Axe`) | Wood Log, Wood Planks, Crafting Table, Barrel, Fence, Fence Gate, Door | Forestry, lumber harvesting, carpentry deconstruction |
| **Hoe** (`EToolType::Hoe`) | Farmland, Leaves, Plant crops | Soil tilling, agricultural preparation |
| **Sword** (`EToolType::Sword`) | Hostile Mobs (Zombie, Skeleton, Spider, Creeper) | Combat damage multiplier, monster defense |

---

## Tool Tier Progression

| Tool Tier | Max Durability | Mining Speed Multiplier | Attack Damage Bonus | Max Concurrent Queue Slots |
|---|---|---|---|---|
| **None (Hand)** | $\infty$ | $1.0\times$ | $+0.0$ (1.0 base) | 1 block |
| **Wood** | 59 uses | $2.0\times$ | $+2.0$ (3.0 total) | 2 blocks |
| **Stone** | 131 uses | $4.0\times$ | $+3.0$ (4.0 total) | 3 blocks |
| **Iron** | 250 uses | $6.0\times$ | $+4.0$ (5.0 total) | 4 blocks |
| **Diamond** | 1,561 uses | $10.0\times$ | $+5.0$ (6.0 total) | 5 blocks |
| **Obsidian (Netherite)** | 2,031 uses | $15.0\times$ | $+7.0$ (8.0 total) | 6 blocks |

---

## Mining Time Calculations

Break time is calculated per block as:

$$\text{BreakTime (seconds)} = \frac{\text{Block Durability}}{\text{Effective Mining Force}}$$

Where:
- When using hands (no tool equipped in Baubles): $\text{Effective Mining Force} = 1.0$.
- When a matching tool is equipped in Baubles: $\text{Effective Mining Force} = \text{Tool Mining Speed}$.
- When mining with an unmatching tool: defaults to $1.0\times$ hand speed and does not consume durability.

### Sample Break Times by Block & Tier

| Block Name | Durability | Hand ($1.0\times$) | Wood ($2.0\times$) | Stone ($4.0\times$) | Iron ($6.0\times$) | Diamond ($10.0\times$) |
|---|---|---|---|---|---|---|
| **Dirt** | 0.50 | 0.50s | 0.25s *(Shovel)* | 0.13s *(Shovel)* | 0.08s *(Shovel)* | 0.05s *(Shovel)* |
| **Stone** | 1.50 | 1.50s | 0.75s *(Pickaxe)* | 0.38s *(Pickaxe)* | 0.25s *(Pickaxe)* | 0.15s *(Pickaxe)* |
| **Wood Log** | 2.00 | 2.00s | 1.00s *(Axe)* | 0.50s *(Axe)* | 0.33s *(Axe)* | 0.20s *(Axe)* |
| **Iron Ore** | 3.00 | 3.00s | 1.50s *(Pickaxe)* | 0.75s *(Pickaxe)* | 0.50s *(Pickaxe)* | 0.30s *(Pickaxe)* |
| **Iron Block** | 5.00 | 5.00s | 2.50s *(Pickaxe)* | 1.25s *(Pickaxe)* | 0.83s *(Pickaxe)* | 0.50s *(Pickaxe)* |
| **Torch** | 0.05 | 0.05s | 0.05s | 0.05s | 0.05s | 0.05s |

---

## Patrix HD Tool Icons (`FToolInstance::GetToolIconTexture`)

All tool icons are resolved in 32×32 resolution from the Patrix texture pack:

| Tool | Wood | Stone | Iron | Diamond | Netherite (Obsidian) |
|---|---|---|---|---|---|
| **Pickaxe** | `wooden_pickaxe` | `stone_pickaxe` | `iron_pickaxe` | `diamond_pickaxe` | `netherite_pickaxe` |
| **Shovel** | `wooden_shovel` | `stone_shovel` | `iron_shovel` | `diamond_shovel` | `netherite_shovel` |
| **Axe** | `wooden_axe` | `stone_axe` | `iron_axe` | `diamond_axe` | `netherite_axe` |
| **Hoe** | `wooden_hoe` | `stone_hoe` | `iron_hoe` | `diamond_hoe` | `netherite_hoe` |
| **Sword** | `wooden_sword` | `stone_sword` | `iron_sword` | `diamond_sword` | `netherite_sword` |

Asset path pattern: `/Game/Patrix_Texture_Pack/textures/item/<Name>.<Name>`

---

## Durability Lifecycle

1. **Equipping**: When equipped into a Baubles slot via `UBaublesSystem::EquipTool`, the tool retains its remaining durability.
2. **Usage**: Upon successful voxel destruction in `AWorldGenerator::BreakBlockAtVoxel`, `UBaublesSystem::ConsumeToolDurability` is automatically called.
3. **Depletion**: When durability reaches $0$, the tool breaks, is automatically unequipped from the Baubles slot, and fires `OnToolUnequipped`.
