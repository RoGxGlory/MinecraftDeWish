# ⛏️ MinecraftDeWish — v0.2.0-alpha Pre-Release

> **The Biome Blending & Atmosphere Update**  
> *A high-performance C++ Minecraft-inspired voxel sandbox built with Unreal Engine 5.8.*

---

## 🌟 Welcome to the Second Pre-Release of MinecraftDeWish!

**v0.1** proved the engine could stream, mesh, and simulate a voxel world at speed. **v0.2** is about making that world feel like *terrain* instead of a grid.

The headline is the **biome cross-fade**: Grass, Sand, and Stone now dissolve into each other across a continuous, organic boundary instead of meeting at a hard block edge. Supporting that are a reworked tiling pipeline, a physical block-debris system, and a full atmospheric pass — height fog, distance haze, and post-processing — that finally gives the world depth and mood.

Everything below is implemented and playable in this build.

---

## 🚀 Key Feature Highlights

### 🌿 1. Seamless Biome Cross-Fade
- **Continuous material field**: Grass, Sand, and Stone are no longer a discrete lookup. Each column carries a normalized membership for all three, so a border is a *gradient*, not a cut.
- **Per-corner mesh weights**: the blend weight is stored per face corner (`VertexColor.B`) with the secondary texture slice in `UV1.Y`, so the fade interpolates smoothly across every quad instead of banding on the block grid.
- **Symmetric at the seam**: the 50% point sits on the exact contour where the block ID flips (`Temp > 0.60`, `Moisture < 0.40`, `Continental > 0.35`). Both sides of a border therefore meet at `0.5`, which is what removes the visible line. A `0.75`-block plateau pins the shared corner between two differently-classified blocks to exactly `0.5`.
- **Width measured in blocks**: the signed distance is divided by the local field gradient, so the fade is `BiomeBlendWidth` blocks wide on flat ground *and* on steep slopes. No more 40-block smears on plains and 2-block slivers on hills.
- **All six faces blend**, not just the top. Cliffs, terraces, and cut-away cave mouths fade vertically too.
- **Organic borders**: the field is warped (`BiomeBlendWarp`) and jittered (`BiomeBlendNoise`), and the block-level assignment goes through the same warped sample — so the biome contour meanders in lobes instead of tracing a clean noise curve.
- **Desert height ramp**: desert flattening now eases across the border with the smooth desert factor (`BiomeHeightRamp`) instead of switching on the discrete biome, removing the step-cliff that used to form where a desert met another biome.
- **Surface-only**: the fade is restricted to the top four blocks of each column, so cave walls, cliff faces, and deep stone keep their own material instead of fading halfway toward grass.
- **Correct colour transfer**: procedural mesh sections now use sRGB vertex-colour conversion, so a `0.5` weight reaches the shader as `0.5` rather than `sRGBToLinear(0.5) ≈ 0.21`. This was the last blocker — without it the fade was mathematically present in the mesh but visually crushed into a hard edge.

### 🌫️ 2. Atmospheric Fog & Post-Processing
- **Height-based exponential fog** integrated into the world: density falls off with altitude, so valleys and lowlands hold haze while peaks stay clear.
- **Distance haze**: far terrain softens into the horizon instead of ending at a crisp draw-distance boundary — chunk streaming now reads as atmosphere rather than pop-in.
- **Sun inscattering**: fog is tinted by the directional light, so dawn, noon, and dusk all grade the horizon differently as the day/night cycle advances.
- **Post-process volume**: a graded look (contrast, bloom balance, and colour response) sits on top of the fog so the PBR material set keeps its punch without washing out.
- **Tunable per level**: fog and grade live in the level, not in code, so the FirstPerson and Horror variants can carry different moods from the same terrain shader.

### 🧱 3. Physical Block Destruction Debris
- **Real fragments, not a particle sprite**: breaking a block spawns **32–44 textured 5–9 cm fragments** that burst outward, tumble, bounce, and settle.
- **Exact block textures**: each fragment samples the correct slice from `M_Global` — including the block's **interior** texture for freshly fractured faces, so a broken log shows bark on the outside and wood grain inside.
- **Directional impact**: the burst is biased by the hit normal and hit point, so debris kicks away from the strike.
- **Ballistic simulation**: gravity (`1350 cm/s²`), surface restitution, ground settling, and a smooth shrink/fade despawn (2.0 s tumble + 0.8 s fade).
- **Runs as one procedural mesh**: the whole burst is a single `UProceduralMeshComponent` updated on the fly — dozens of fragments for one draw call.

### 🔲 4. Tiling-Grid Breakup & Noise Blending
- **Stochastic top-face UV dithering**: every isotropic top face gets a deterministic per-block UV phase offset derived from its world position, breaking the last visible alignment grid. The array sampler's wrap mode absorbs the out-of-range UVs, so there are no seams.
- **Deterministic 4-way rotation**: top/bottom faces of isotropic blocks still rotate in 90° steps, with **tangents rotating in lockstep** so normal maps and sun glare stay physically correct.
- **Directional textures preserved**: bark, planks, furnaces, and crafting tables remain upright.
- **Noise blend mask**: natural terrain is flagged in `VertexColor.G`, and `M_Global` uses it to apply the procedural tiling breakup only where it belongs. Constructed blocks stay clean.

### 🧭 5. Block Rotation Metadata
- Blocks can now store a **rotation** alongside their ID.
- Side textures are remapped during mesh generation, so a rotated block shows the correct face on each side.
- Furnaces and other directional props are placed facing the right way (the village furnace now faces west).

### 🎯 6. Global Targeted-Voxel Helpers
- Shared raycast helpers resolve the targeted block, face, and hit point in one place, so mining, placing, debris, and block-info UI all agree on what the player is pointing at.

---

## 🎮 Default Controls & Keybindings

| Action | Key / Input | Description |
|---|---|---|
| **Move** | `Z` `Q` `S` `D` | Character movement |
| **Look** | `Mouse` | First-person camera look |
| **Jump** | `Space` | Jump |
| **Mine / Attack** | `Left Mouse Button` | Mine targeted voxel or attack mobs |
| **Place / Interact** | `Right Mouse Button` | Place selected block or open Crafting Table / Furnace ( WIP ) |
| **QuickSlots 1–9** | `Scroll Wheel` | Select active hotbar item |
| **Drop Item** | `A` (`IA_DropItem`) | Drop highlighted item (world) or hovered item (menu) ( WIP ) |
| **Baubles Menu** | `B` (`IA_Baubles`) | Open dedicated tool equipment screen ( WIP ) |
| **Inventory** | `E` | Open player inventory and crafting grid ( WIP ) |
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

## 🗺️ Roadmap to v0.3.0-beta

- [ ] Remaining biomes: Snowy Peaks, Swamps, Deep Oceans.
- [ ] Weather pass: rain and snow driven by the same fog/atmosphere system.
- [ ] Liquid simulation physics (dynamic flowing water & lava).
- [ ] Chest, barrel, and backpack persistent container storage.
- [ ] Sound design pass (voxel step sounds, block break impacts, mob vocalizations).
- [ ] Dedicated multiplayer server networking.

---

## Use of AI
AI has only been used to generate documentation, commenting code as well as making the implementation plans. However, all implementation was made by hand.

*Enjoy playing and exploring MinecraftDeWish! Feel free to open issues, submit feedback, or contribute pull requests.*
