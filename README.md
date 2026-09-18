# MinecraftDeWish

A high-performance C++ Minecraft-style voxel sandbox built on Unreal Engine 5.

---

## 🌟 Overview

**MinecraftDeWish** is a voxel engine featuring infinite procedural terrain generation, asynchronous chunk streaming, multi-tier tool progression, a dedicated **Baubles equipment system**, physical 3D item pickups with parabolic launch dropping, full **Patrix 32×32 PBR graphics** (Normal & Specular maps), dynamic quickslot opacity, daytime/nighttime environmental cycles, hostile mob AI with spider wall climbing and creeper explosions, and player vitals with XP progression.

---

## 🚀 Key Feature Highlights

### 1. Infinite Procedural 3D Terrain & Chunk Streaming
- Multi-octave Perlin & Simplex noise combining continentalness, mountain erosion, and 3D cave carving.
- Infinite dynamic chunk loading around the player pawn with dirty mesh rebuild queues.
- Asynchronous collision and geometry baking via `UProceduralMeshComponent`.
- Chunk delta persistence saving and restoring player modifications to disk.

### 2. Full PBR Graphics & Patrix 32×32 Texture Pack
- **Voxel Terrain Shading (`M_Global`)**: Dual-UV procedural rendering (`UV0` face coordinates + `UV1.X` slice index) sampling 32×32 Texture2DArrays:
  - `Textures_Array`: Base Color.
  - `Normals_Array`: Tangent-space normal mapping for cracks, bevels, and depth.
  - `Specular_Array`: LabPBR mapping driving Roughness, Metallic, and Emissive channels.
- **High-Res Icons**: 32×32 Patrix item icons for all tools and voxel blocks.

### 3. Dual-Section Mesh Rendering & Non-Solid Block Physics
- **Solid Mesh**: Full physical and query collision (`QueryAndPhysics`) for opaque blocks (Stone, Dirt, Wood, Ores).
- **Non-Solid Mesh**: Query-only collision (`QueryOnly`, `ECC_Pawn` overlap) for walk-through blocks like **Torches**. Players walk smoothly through torches while line traces for targeting, inspecting, and breaking remain 100% responsive.

### 4. Dedicated Baubles Tool Equipment System
- Inspired by the Minecraft *Baubles* mod.
- Tools are equipped into dedicated slots (**Pickaxe**, **Shovel**, **Axe**, **Hoe**, **Sword**) rather than cluttering quickslots.
- **Automatic Best-Tool Selection**: When mining a block, the system automatically checks equipped tools in Baubles, applies the optimal mining speed multiplier, and consumes durability from that specific tool.
- Accessible via the **B** key (`IA_Baubles`).

### 5. Inventory, QuickSlots & Item Drop Mechanics
- **Dynamic QuickSlot Opacity**: Automatically toggles slot visibility in `WB_QuickSlots` (`opacity = 1.0` and `Visible` when filled; `opacity = 0.0` and `Hidden` when empty).
- **Contextual Drop Action (`IA_DropItem` / Q)**:
  - Drops selected quickslot item during regular gameplay.
  - Drops mouse-hovered item when inside an inventory or container panel (chests, crates, barrels, backpacks).
- **3D Physics Launch**: Items are launched forward 2.5 blocks (250 cm) with gravity, drag, and floor sweep collision.
- **Pickup Cooldown**: 1.2s delay prevents instant re-absorption while items are airborne.

### 6. Player Vitals & Experience Progression
- **Health System**: 20.0 HP (10 hearts × 2 half-hearts) with natural regeneration (1 HP per 4s).
- **XP Progression**: Minecraft-accurate leveling curve ($XP = 7 + Level \times 3$).
- **Physical XP Orbs**: Defeated mobs release glowing polyhedral XP orbs with physics bouncing, idle bobbing, and player magnet attraction.

### 7. Day/Night Cycle & Hostile Mob Ecosystem
- **Day/Night Cycle**: 20-minute continuous cycle (10 min day, 10 min night) with sun rotation and sky light modulation.
- **Hostile Mobs**:
  - **Zombie**: Melee pursuer, burns under direct daylight.
  - **Skeleton**: Ranged pursuer, burns under daylight, drops bones & arrows.
  - **Spider**: Aggressive climber that scales vertical voxel walls to reach players.
  - **Creeper**: Silent stalker with fuse hiss countdown, detonating to damage players and carve craters out of the voxel terrain.
- **Spawner System**: Dark-area-only spawning (light level < 7), torch avoidance, player distance rings (24m - 128m), and despawning.

### 8. Interactive Crafting & Smelting
- Right-click interaction with **Crafting Tables** and **Furnaces** within proximity (4 blocks).
- Multi-fuel smelting recipes with burn time tracking (Coal, Charcoal, Logs, Planks).

---

## 📂 Documentation Directory

| Document | Description |
|---|---|
| [GAME_DESIGN.md](Docs/GAME_DESIGN.md) | High-level game design, mechanics, formulas, and player loop |
| [SYSTEMS_REFERENCE.md](Docs/SYSTEMS_REFERENCE.md) | Comprehensive C++ subsystem API and architecture guide |
| [BLOCK_REGISTRY.md](Docs/BLOCK_REGISTRY.md) | Block IDs, textures, hardness, tool affinities, and properties |
| [MOB_REGISTRY.md](Docs/MOB_REGISTRY.md) | Mob stats, AI state machines, drop tables, and special behaviors |
| [TOOL_REGISTRY.md](Docs/TOOL_REGISTRY.md) | Tool tiers, durabilities, mining multipliers, and combat damage |

---

## 🛠️ Building & Requirements

- **Engine**: Unreal Engine 5.8
- **Modules**: `ProceduralMeshComponent`, `UMG`, `Slate`, `SlateCore`, `AIModule`, `NavigationSystem`, `EnhancedInput`
- **Compiler**: Visual Studio 2022+ (MSVC toolchain)
- **Fast Build Configuration**: Pre-configured with Unreal Build Accelerator (UBA) local executor (`bAllowXGE = false` in `BuildConfiguration.xml`) for sub-5s incremental compiles.
