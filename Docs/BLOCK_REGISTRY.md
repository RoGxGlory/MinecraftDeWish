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
| **17** | `Leaves` | Oak Leaves | Hoe / Hand | 0.2 | QueryAndPhysics | Yes | Canopy foliage (maps to `oak_leaves`) |
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

## Patrix PBR Texture & Icon Resolution

Icons in HUD (`WB_QuickSlots`), inventory slots, and dropped item pickups are resolved directly from the Patrix texture pack at 32×32 resolution via `AChunkActor::GetBlockIconTexture`:

```cpp
// Primary path: /Game/Patrix_Texture_Pack/textures/block/<Name>.<Name>
// Fallback:     /Game/Textures/Blocks/<Name>.<Name>
```

### Texture2DArray 37-Slice Mapping (`Global_Textures_Array`)

All voxel faces encode their texture index in vertex buffer attribute `UV1.X`:

| Slice | Texture Name | Normal Map (`_n`) | Specular Map (`_s`) |
|---|---|---|---|
| 0 | `barrel_bottom` | `barrel_bottom_n` | `barrel_bottom_s` |
| 1 | `barrel_side` | `barrel_side_n` | `barrel_side_s` |
| 2 | `barrel_top` | `barrel_top_n` | `barrel_top_s` |
| 3 | `barrel_top_open` | `barrel_top_open_n` | `barrel_top_open_s` |
| 4 | `coal_block` | `coal_block_n` | `coal_block_s` |
| 5 | `coal_ore` | `coal_ore_n` | `coal_ore_s` |
| 6 | `cobblestone` | `cobblestone_n` | `cobblestone_s` |
| 7 | `crafting_table_front` | `crafting_table_front_n` | `crafting_table_front_s` |
| 8 | `crafting_table_side` | `crafting_table_side_n` | `crafting_table_side_s` |
| 9 | `crafting_table_top` | `crafting_table_top_n` | `crafting_table_top_s` |
| 10 | `dark_oak_door_bottom` | `dark_oak_door_bottom_n` | `dark_oak_door_bottom_s` |
| 11 | `dark_oak_door_top` | `dark_oak_door_top_n` | `dark_oak_door_top_s` |
| 12 | `diamond_block` | `diamond_block_n` | `diamond_block_s` |
| 13 | `diamond_ore` | `diamond_ore_n` | `diamond_ore_s` |
| 14 | `dirt` | `dirt_n` | `dirt_s` |
| 15 | `emerald_block` | `emerald_block_n` | `emerald_block_s` |
| 16 | `emerald_ore` | `emerald_ore_n` | `emerald_ore_s` |
| 17 | `farmland` | `farmland_n` | `farmland_s` |
| 18 | `farmland_moist` | `farmland_moist_n` | `farmland_moist_s` |
| 19 | `furnace_front` | `furnace_front_n` | `furnace_front_s` |
| 20 | `furnace_front_on` | `furnace_front_on_n` | `furnace_front_on_s` |
| 21 | `furnace_side` | `furnace_side_n` | `furnace_side_s` |
| 22 | `furnace_top` | `furnace_top_n` | `furnace_top_s` |
| 23 | `glass` | `glass_n` | `glass_s` |
| 24 | `gold_block` | `gold_block_n` | `gold_block_s` |
| 25 | `gold_ore` | `gold_ore_n` | `gold_ore_s` |
| 26 | `grass_block_side` | `grass_block_side_n` | `grass_block_side_s` |
| 27 | `grass_block_top` | `grass_block_top_n` | `grass_block_top_s` |
| 28 | `iron_block` | `iron_block_n` | `iron_block_s` |
| 29 | `iron_ore` | `iron_ore_n` | `iron_ore_s` |
| 30 | `oak_leaves` *(leaves)* | `oak_leaves_n` | `oak_leaves_s` |
| 31 | `oak_log` | `oak_log_n` | `oak_log_s` |
| 32 | `oak_log_top` | `oak_log_top_n` | `oak_log_top_s` |
| 33 | `oak_planks` | `oak_planks_n` | `oak_planks_s` |
| 34 | `sand` | `sand_n` | `sand_s` |
| 35 | `stone` | `stone_n` | `stone_s` |
| 36 | `torch` | `torch_n` | `torch_s` |
