# ⛏️ MinecraftDeWish — v0.1.0-alpha Pre-Release

> **The Foundation & Patrix PBR Graphics Overhaul**  
> *A high-performance C++ Minecraft-inspired voxel sandbox built with Unreal Engine 5.8.*

---

## 🌟 Welcome to the First Pre-Release of MinecraftDeWish!

**MinecraftDeWish** combines the beloved sandbox survival loop of Minecraft with the rendering horsepower and physics simulation of **Unreal Engine 5**. 

This pre-release marks the completion of the core engine architecture: infinite multithreaded voxel streaming, dynamic hostile mob AI, interactive crafting and smelting, dedicated tool equipment mechanics, and a complete **32×32 Patrix PBR material pipeline** featuring normal maps, specular reflections, and procedural tiling breakup.

---

## 🚀 Key Feature Highlights

### 🌍 1. Infinite Procedural 3D World & Chunk Streaming
- **Multi-Octave Noise**: World generation powered by layered Perlin and Simplex noise combining continentalness, mountain erosion, and 3D cave tunnels.
- **Infinite Async Chunk Loading**: Dynamic chunk streaming around the player pawn using `UProceduralMeshComponent` with background mesh generation queues.
- **Dual-Section Mesh Physics**:
  - **Solid Mesh (`QueryAndPhysics`)**: Full collision for opaque solid blocks (Stone, Dirt, Wood, Ores, Planks).
  - **Non-Solid Mesh (`QueryOnly`)**: Overlap collision for walk-through decorative blocks like **Torches**. Players walk through torches with zero hitching while raycasts for targeting, inspecting, and breaking remain 100% responsive.
- **Chunk Delta Persistence**: Saves and restores modified, placed, and destroyed voxel blocks between play sessions.

### 🎨 2. Patrix 32×32 PBR Graphics & Procedural Tiling Breakup
- **Texture2DArray Architecture**: High-efficiency dual-UV shader (`UV0` quad coordinates + `UV1.X` slice index) sampling 37 texture slices with zero draw call explosion.
  - `Base_Color`: Albedo textures.
  - `Normals_Array`: Real-time tangent-space normal maps for cracks, stone bevels, and tactile surface relief.
  - `Specular_Array`: LabPBR channels driving Metallic and Roughness.
- **Deterministic 4-Way UV & Tangent Rotation**:
  - Eliminates the repetitive 1-meter chessboard grid across landscapes by applying deterministic 4-way rotations ($0^\circ, 90^\circ, 180^\circ, 270^\circ$) to top/bottom faces of isotropic blocks (Grass Top, Dirt, Stone, Cobblestone, Sand, Ores).
  - Normal map tangents rotate in lockstep ($+X, +Y, -X, -Y$) so lighting and sun glare remain physically accurate.
  - Directional textures (Wood bark, planks, furnaces, crafting tables) remain upright.
- **Material Smoothing**:
  - Normal softening (`FlattenNormal`) softens harsh micro-crevices without losing 3D relief.
  - Terrain roughness re-mapping gives organic matter (grass, soil, rock) a soft matte finish instead of glazed plastic.
- **High-Res 32×32 Icons**: All inventory slots, HUD hotbars, and dropped item pickups render with crisp Patrix icons.

### 🎒 3. Dedicated Baubles Tool Equipment System
- Inspired by the Minecraft *Baubles* system: equip your essential tools (**Pickaxe**, **Shovel**, **Axe**, **Hoe**, **Sword**) into specialized dedicated slots via **`B`** (`IA_Baubles`), keeping your 9 hotbar slots clear for blocks and consumables.
- **Automatic Best-Tool Resolution**: When mining a block, the engine scans your equipped Baubles tools, selects the highest-tier affinity multiplier, and automatically consumes durability from that specific tool.

### 📦 4. Dynamic Inventory, QuickSlots & 3D Item Dropping
- **Smart QuickSlot HUD (`WB_QuickSlots`)**: Empty slots automatically fade out (`Opacity = 0.0`), dynamically popping into view (`Opacity = 1.0`) when items are acquired.
- **Context-Aware Item Dropping (`Q` / `IA_DropItem`)**:
  - When walking: drops 1 item from the highlighted quickslot into the world.
  - When inside an inventory, chest, crate, or barrel: drops 1 item from the currently mouse-hovered slot.
- **Parabolic 3D Launch Physics**: Dropped items launch forward 2.5 blocks (250 cm) with ballistic arcs, surface bounce damping, and idle bobbing.
- **Anti-Reabsorption Cooldown**: 1.2-second pickup immunity prevents accidentally grabbing items while dropping them.

### 🧟 5. Hostile Mob AI & Night Survival
- **Day/Night Cycle**: 20-minute atmospheric cycle (10 min day, 10 min night) controlling sun pitch, sky light intensity, and darkness thresholds.
- **Hostile Mob Ecosystem**:
  - **Zombie**: Relentless melee attacker that catches fire and burns under open daylight.
  - **Skeleton**: Tactical ranged archer that strafes and fires arrows; burns in daylight.
  - **Spider**: Dynamic climber capable of scaling vertical voxel walls to pursue players.
  - **Creeper**: Silent stalker with an audible fuse hiss countdown, detonating to damage players and blow dynamic craters out of the voxel world.
- **Dynamic Spawning Engine**: Spawns mobs in dark areas (light level < 7), avoids torch illumination radiuses, and dynamically despawns mobs beyond 128 meters.
- **Physical XP System**: Defeated mobs drop 3D glowing polyhedral XP orbs with physics bouncing and player magnetic attraction.

### 🔨 6. Interactive Crafting & Smelting
- Right-click interaction with **Crafting Tables** (3×3 crafting recipes) and **Furnaces** (multi-fuel smelting with real-time burn progress tracking).

---

## 🎮 Default Controls & Keybindings

| Action | Key / Input | Description |
|---|---|---|
| **Move** | `W` `A` `S` `D` | Character movement |
| **Look** | `Mouse` | First-person camera look |
| **Jump** | `Space` | Jump / swim up |
| **Mine / Attack** | `Left Mouse Button` | Mine targeted voxel or attack mobs |
| **Place / Interact** | `Right Mouse Button` | Place selected block or open Crafting Table / Furnace |
| **QuickSlots 1–9** | `1` – `9` / `Scroll Wheel` | Select active hotbar item |
| **Drop Item** | `Q` (`IA_DropItem`) | Drop highlighted item (world) or hovered item (menu) |
| **Baubles Menu** | `B` (`IA_Baubles`) | Open dedicated tool equipment screen |
| **Inventory** | `E` | Open player inventory and crafting grid |
| **Block Info** | `Hover Raycast` | Real-time targeted block name and remaining durability |

---

## 📋 Technical Requirements & Setup

### Prerequisites
- **Unreal Engine**: 5.8
- **Platform**: Windows 10/11 64-bit
- **Compiler**: Visual Studio 2022+ with MSVC v143+ toolchain
- **Build System**: Configured with Unreal Build Accelerator (UBA) for sub-5-second local compiles

### Quick Start
1. Clone the repository:
   ```bash
   git clone https://github.com/YourUsername/MinecraftDeWish.git
   cd MinecraftDeWish
   ```
2. Generate Visual Studio project files:
   - Right-click `MinecraftDeWish.uproject` $\to$ **Generate Visual Studio project files**
3. Open `MinecraftDeWish.sln` and compile (`Development Editor | Win64`).
4. Press **F5** or open the project in Unreal Editor 5.8 and hit **Play (PIE)**!

---

## 🗺️ Roadmap to v0.2.0-beta

- [ ] Multi-biome generation (Deserts, Snowy Peaks, Swamps, Deep Oceans).
- [ ] Liquid simulation physics (Dynamic flowing water & lava).
- [ ] Chest, barrel, and backpack persistent container storage.
- [ ] Sound design pass (voxel step sounds, block break impacts, mob vocalizations).
- [ ] Dedicated multiplayer server networking.

---

*Enjoy playing and exploring MinecraftDeWish! Feel free to open issues, submit feedback, or contribute pull requests.*
