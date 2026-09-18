# Block Registry — MinecraftDeWish

Complete registry of all voxel block types, texture mappings, collision modes, hardness, and tool affinities.

---

## Block Definition Table

| ID | Name | Display Name | Preferred Tool | Durability | Collision Mode | Transparent | Notes |
|---|---|---|---|---|---|---|---|
| **0** | `Air` | Air | None | 0.0 | None | Yes | Empty space |
| **1** | `Dirt` | Dirt | Shovel | 0.5 | QueryAndPhysics | No | Common surface soil |
| **2** | `Grass` | Grass Block | Shovel | 0.6 | QueryAndPhysics | No | Grassy top layer |
| **3** | `Cobblestone` | Cobblestone | Pickaxe | 2.0 | QueryAndPhysics | No | Dropped from mined stone |
| **4** | `Stone` | Stone | Pickaxe | 1.5 | QueryAndPhysics | No | Common underground rock |
| **5** | `Wood_Log` | Oak Log | Axe | 2.0 | QueryAndPhysics | No | Tree trunk material |
| **6** | `Wood_Planks` | Oak Planks | Axe | 2.0 | QueryAndPhysics | No | Crafted wood material |
| **7** | `Iron_Ore` | Iron Ore | Pickaxe | 3.0 | QueryAndPhysics | No | Smelts to Iron Ingot |
| **8** | `Iron_Block` | Iron Block | Pickaxe | 5.0 | QueryAndPhysics | No | Crafted iron block |
| **9** | `Gold_Ore` | Gold Ore | Pickaxe | 3.0 | QueryAndPhysics | No | Smelts to Gold Ingot |
| **10** | `Gold_Block` | Gold Block | Pickaxe | 3.0 | QueryAndPhysics | No | Crafted gold block |
| **11** | `Diamond_Ore` | Diamond Ore | Pickaxe | 3.0 | QueryAndPhysics | No | Mined for raw diamond |
| **12** | `Diamond_Block` | Diamond Block | Pickaxe | 5.0 | QueryAndPhysics | No | Crafted diamond block |
| **13** | `Emerald_Ore` | Emerald Ore | Pickaxe | 3.0 | QueryAndPhysics | No | Rare mineral deposit |
| **14** | `Emerald_Block` | Emerald Block | Pickaxe | 5.0 | QueryAndPhysics | No | Crafted emerald block |
| **15** | `Crafting_Table` | Crafting Table | Axe | 2.5 | QueryAndPhysics | No | Right-click to craft tools |
| **16** | `Furnace` | Furnace | Pickaxe | 3.5 | QueryAndPhysics | No | Right-click to smelt ores |
| **17** | `Leaves` | Oak Leaves | Hoe / Hand | 0.2 | QueryAndPhysics | Yes | Canopy foliage |
| **18** | `Sand` | Sand | Shovel | 0.5 | QueryAndPhysics | No | Desert/beach granular block |
| **19** | `Torch` | Torch | Hand / Any | 0.05 | **QueryOnly** | Yes | Emits light; walk-through |
| **20** | `Barrel` | Barrel | Axe | 2.5 | QueryAndPhysics | No | Wooden storage block |
| **21** | `Glass` | Glass | Hand / Any | 0.3 | QueryAndPhysics | Yes | Transparent decorative block |
| **22** | `Fence` | Oak Fence | Axe | 2.0 | QueryAndPhysics | Yes | Wooden perimeter barrier |
| **23** | `Fence_Door` | Fence Gate | Axe | 2.0 | QueryAndPhysics | Yes | Operable wooden gate |
| **24** | `Door` | Wooden Door | Axe | 3.0 | QueryAndPhysics | Yes | Operable entrance barrier |
| **25** | `Bedrock` | Bedrock | Indestructible | $\infty$ | QueryAndPhysics | No | Indestructible world floor |
| **26** | `Water` | Water | Bucket | 100.0 | QueryOnly | Yes | Liquid block |
| **27** | `Coal_Ore` | Coal Ore | Pickaxe | 3.0 | QueryAndPhysics | No | Smelting fuel ore |

---

## Icon Asset Paths (`GetBlockIconTexture`)

Textures are referenced directly from Content assets for HUD rendering:

```cpp
case 1:  TEXT("/Game/Textures/Blocks/dirt.dirt")
case 2:  TEXT("/Game/Textures/Blocks/grass_block_side.grass_block_side")
case 3:  TEXT("/Game/Textures/Blocks/cobblestone.cobblestone")
case 4:  TEXT("/Game/Textures/Blocks/stone.stone")
case 5:  TEXT("/Game/Textures/Blocks/oak_log.oak_log")
case 6:  TEXT("/Game/Textures/Blocks/oak_planks.oak_planks")
case 7:  TEXT("/Game/Textures/Blocks/iron_ore.iron_ore")
case 8:  TEXT("/Game/Textures/Blocks/iron_block.iron_block")
case 9:  TEXT("/Game/Textures/Blocks/gold_ore.gold_ore")
case 10: TEXT("/Game/Textures/Blocks/gold_block.gold_block")
case 11: TEXT("/Game/Textures/Blocks/diamond_ore.diamond_ore")
case 12: TEXT("/Game/Textures/Blocks/diamond_block.diamond_block")
case 13: TEXT("/Game/Textures/Blocks/emerald_ore.emerald_ore")
case 14: TEXT("/Game/Textures/Blocks/emerald_block.emerald_block")
case 15: TEXT("/Game/Textures/Blocks/crafting_table_front.crafting_table_front")
case 16: TEXT("/Game/Textures/Blocks/furnace_front.furnace_front")
case 17: TEXT("/Game/Textures/Blocks/leaves.leaves")
case 18: TEXT("/Game/Textures/Blocks/sand.sand")
case 19: TEXT("/Game/Textures/Blocks/torch.torch")
case 20: TEXT("/Game/Textures/Blocks/barrel_side.barrel_side")
case 21: TEXT("/Game/Textures/Blocks/glass.glass")
case 22: TEXT("/Game/Textures/Blocks/oak_planks.oak_planks")
case 23: TEXT("/Game/Textures/Blocks/oak_planks.oak_planks")
case 24: TEXT("/Game/Textures/Blocks/dark_oak_door_bottom.dark_oak_door_bottom")
case 25: TEXT("/Game/Textures/Blocks/stone.stone")
case 26: TEXT("/Game/Textures/Blocks/glass.glass")
case 27: TEXT("/Game/Textures/Blocks/coal_ore.coal_ore")
```
