# MinecraftDeWish

A high-performance C++ Minecraft-style voxel sandbox built on Unreal Engine 5.

---

## 🌟 Overview

**MinecraftDeWish** is a voxel engine featuring infinite procedural terrain generation, asynchronous chunk streaming, multi-tier tool progression, a dedicated **Baubles equipment system**, physical item pickups, daytime/nighttime environmental cycles, hostile mob AI with spider wall climbing and creeper explosions, and player vitals with XP progression.

---

## 🚀 Key Feature Highlights

### 1. Infinite Procedural 3D Terrain & Chunk Streaming
- Multi-octave Perlin & Simplex noise combining continentalness, mountain erosion, and 3D cave carving.
- Infinite dynamic chunk loading around the player pawn with dirty mesh rebuild queues.
- Asynchronous collision and geometry baking via `UProceduralMeshComponent`.
- Chunk delta persistence saving and restoring player modifications to disk.

### 2. Dual-Section Mesh Rendering & Non-Solid Block Physics
- **Solid Mesh**: Full physical and query collision (`QueryAndPhysics`) for opaque blocks (Stone, Dirt, Wood, Ores).
- **Non-Solid Mesh**: Query-only collision (`QueryOnly`, `ECC_Pawn` overlap) for walk-through blocks like **Torches** and seeds. Players walk smoothly through torches while line traces for targeting, inspecting, and breaking remain 100% responsive.

### 3. Dedicated Baubles Tool Equipment System
- Inspired by the Minecraft *Baubles* mod.
- Tools are equipped into dedicated slots (**Pickaxe**, **Shovel**, **Axe**, **Hoe**, **Sword**) rather than cluttering quickslots.
- **Automatic Best-Tool Selection**: When mining a block, the system automatically checks equipped tools in Baubles, applies the optimal mining speed multiplier, and consumes durability from that specific tool.
- Accessible via the **B** key (`IA_Baubles`).

### 4. Player Vitals & Experience Progression
- **Health System**: 20.0 HP (10 hearts × 2 half-hearts) with natural regeneration (1 HP per 4s).
- **XP Progression**: Minecraft-accurate leveling curve ($XP = 7 + Level \times 3$).
- **Physical XP Orbs**: Defeated mobs release glowing polyhedral XP orbs with physics bouncing, idle bobbing, and player magnet attraction.

### 5. Day/Night Cycle & Hostile Mob Ecosystem
- **Day/Night Cycle**: 20-minute continuous cycle (10 min day, 10 min night) with sun rotation and sky light modulation.
- **Hostile Mobs**:
  - **Zombie**: Melee pursuer, burns under direct daylight.
  - **Skeleton**: Ranged pursuer, burns under daylight, drops bones & arrows.
  - **Spider**: Aggressive climber that scales vertical voxel walls to reach players.
  - **Creeper**: Silent stalker with fuse hiss countdown, detonating to damage players and carve craters out of the voxel terrain.
- **Spawner System**: Dark-area-only spawning (light level < 7), torch avoidance, player distance rings (24m - 128m), and despawning.

### 6. Interactive Crafting & Smelting
- Right-click interaction with **Crafting Tables** and **Furnaces** within proximity (4 blocks).
- Multi-fuel smelting recipes with burn time tracking (Coal, Charcoal, Logs, Planks).

### 7. Inventory & QuickSlots HUD Integration
- Physical mini-block item drops in the world with magnet pull and stack merging up to 64.
- Automatic insertion into 9 quickslots with dynamic icon rendering and real-time stack count display.

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

- **Engine**: Unreal Engine 5.4+
- **Modules**: `ProceduralMeshComponent`, `UMG`, `Slate`, `SlateCore`, `AIModule`, `NavigationSystem`
- **Compiler**: Visual Studio 2022 (MSVC v143+)
- **Build**: Open `MinecraftDeWish.uproject` or build via Visual Studio / Live Coding.
