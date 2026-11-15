# 🎮 MinecraftDeWish — v0.3.0-alpha Pre-Release



> **The Lamplighter, Horror Art & Block Cracking Update**

> *A high-performance C++ Minecraft-inspired voxel sandbox built with Unreal Engine 5.8.*



---



## 📢 Welcome to the Third Pre-Release of MinecraftDeWish!



**v0.1** proved the engine could stream, mesh, and simulate a voxel world at speed.

**v0.2** made that world feel like *terrain* instead of a grid, with biome cross-fade, atmospheric fog, and physical debris. 

**v0.3** is about making the world *read* as a place you are not welcome in: a real mining feedback loop, a rebuilt main menu and UI kit, the first original art pass, and the Weeping Angel — the game's predator — arriving as a sculpted, non-voxel statue in a voxel world.



Everything below is implemented and playable in this build.



---



## 🧱 Key Feature Highlights



### ⛏️ 1. Vanilla-Style Block Cracking Overlay

- **Progress-driven cracks**: mining a block now shows the canonical 10-stage crack overlay (`destroy_stage_0` … `destroy_stage_9`) driven by mining progress, instead of a single static damage decal.

- **Blueprint-routed**: the crack overlay is selected and applied through the Blueprint mining path, so designers can tune thresholds without touching C++.

- **Single active overlay**: only the block currently being mined shows a crack; the previous multi-overlay stacking bug is fixed.

- **Architectural record**: the driver is documented in `ADR-0003` (`Docs/architecture/adr-0003-block-crack-overlay-driver.md`).

- **Material pass**: the crack material was reworked to composite cleanly over the global terrain material (`M_Global`) without z-fighting.



### 📋 2. Lamplighter Menu & UI Systems

- **Rebuilt main menu** (`Lvl_MainMenu`) with a unified voxel menu material set and a fixed hero camera.

- **UI kit foundations**: `WBP_Common_*`, Title, Pause, Settings and HUD restyle scaffolding, plus a CommonUI-based input routing path.

- **Weeping Angel as a menu hero**: the Angel's separated right arm is used as a visual option selector in the main menu (pointing arm), tying the predator's silhouette into the first thing the player sees.

- **Block info overlay**: real-time targeted block name and remaining durability now read consistently from the shared targeted-voxel helpers.



### 🗿 3. Weeping Angel — First Asset Pass

- **8 statue poses** authored as separate static meshes: `HandsOnFace`, `Pointing`, `HeadTurned`, `MidStride`, `Crouched`, `WingsHalfOpen`, `ArmsCrossed`, `Reaching`.

- **Sculpted, non-voxel form**: stone/plaster, bone-white `#E3DCCB`, high roughness, no metal, no emissive, no VFX — the only smooth shape in a blocky

  world.

- **Separated right arm** (`SM_WeepingAngel_Point_RightArm`): the pointing pose's right arm was split out of the body mesh so it can be reused independently as a menu selector and animated/posed on its own.

- **Materials & textures**: `M_WeepingAngel_Body` / `_Face` / `_Wings` with `T_WeepingAngel_*_D` and `_N` maps; a specular pass is still outstanding (see §8 of `Docs/art/art-bible.md`).

- Full design: `design/gdd/weeping-angels.md` and [Docs/weeping-angel-asset.md](https://github.com/RoGxGlory/MinecraftDeWish/blob/main/Docs/weeping-angel-asset.md).



### 🎨 4. First Original Art Pass

- **Ruin blocks, props and wood pickaxe**: the first in-house assets, authored in Blender, exported as FBX, and brought in at 64×64 PBR (albedo + normal +

  specular), following the `Docs/technical-contract.md` pipeline.

- **Mob assets & horror character blueprint**: first mob meshes and a reworked horror character blueprint, plus the Mobs first-asset Blender file.

- **UE5.8 art & systems foundation**: chunk store, meshing pipeline, render policy and weather assets documented via ADRs 0004–0019.



### 🧱 5. Block Placement & Inventory Handling

- **Placement pipeline**: placing blocks from the hotbar now resolves the targeted face and hit point through the shared voxel helpers, consistent with mining, debris, and block info.

- **Inventory handling**: dropped-item pickup, quick-slot fade, and context-aware drop behaviour refined; anti-reabsorption cooldown preserved.



### 🛠️ 6. Hardened Voxel Systems & Horror Direction

- **Horror direction document** plus hardened voxel systems: biomes, generation, and mob spawning documented and tightened.

- **EngineWire plugin conventions** documented; generated Visual Studio solution files and the EngineWire plugin are now ignored/untracked, keeping the tree clean.

- **OpenCode studio framework** added to the repository (`.opencode/`) for project-level coordination, rules, and agent workflows.



---



## 🎮 Default Controls & Keybindings



| Action | Key / Input | Description |

|---|---|---|

| **Move** | `Z` `Q` `S` `D` | Character movement |

| **Look** | `Mouse` | First-person camera look |

| **Jump** | `Space` | Jump |

| **Mine / Attack** | `Left Mouse Button` | Mine targeted voxel or attack mobs |

| **Place / Interact** | `Right Mouse Button` | Place selected block or open Crafting Table / Furnace (WIP) |

| **QuickSlots 1–9** | `Scroll Wheel` | Select active hotbar item |

| **Drop Item** | `A` (`IA_DropItem`) | Drop highlighted item (world) or hovered item (menu) (WIP) |

| **Baubles Menu** | `B` (`IA_Baubles`) | Open dedicated tool equipment screen (WIP) |

| **Inventory** | `E` | Open player inventory and crafting grid (WIP) |

| **Block Info** | `Hover Raycast` | Real-time targeted block name and remaining durability |



---



## 💻 Technical Requirements & Setup



### Prerequisites

- **Unreal Engine**: 5.8

- **Platform**: Windows 10/11 64-bit

- **Compiler**: Visual Studio 2022+ with MSVC v143+ toolchain

- **Build System**: Configured with Unreal Build Accelerator (UBA) for sub-5-second local compiles



### Quick Start

1. Clone the repository:

   ```bash

   git clone https://github.com/RoGxGlory/MinecraftDeWish.git

   cd MinecraftDeWish

   ```

2. Generate Visual Studio project files:

   - Right-click `MinecraftDeWish.uproject` → **Generate Visual Studio project files**

3. Open `MinecraftDeWish.sln` and compile (`Development Editor | Win64`).

4. Press **F5** or open the project in Unreal Editor 5.8 and hit **Play (PIE)**!



---



## 🗺️ Roadmap — Next 5 Weeks



Short-term plan (≈ 1.25 months), building on v0.2's outstanding items plus the new horror-art track. Milestones are indicative and may shift with playtest feedback.



### Weeks 1 — Weeping Angel implementation

- [ ] Wire the 8 statue poses into `AMobCharacter` / Angel actor with the quantum-lock gate (observed = frozen, unobserved + dark = moves).

- [ ] Implement the 0.15 s observation latch, breaching (5 s/block, all blocks except Bedrock), and contact-kill.

- [ ] Material fix: base colour → exact `#E3DCCB`, add the missing `_S` specular map, kill the green bias (see `Docs/art/art-bible.md` §8.1).

- [ ] Re-capture `Angel_25b_fog.png` and validate `L_statue ≥ 1.4 × L_fog`.



### Weeks 2 — Lamplighter UI completion

- [ ] Finish Title / Pause / Settings screens and the HUD restyle.

- [ ] First-person hands + 5 tool classes (wood tier) with 5 swing animations.

- [ ] Baubles equipment screen polish and inventory/crafting grid (WIP → done).



### Weeks 3 — Vanilla mobs & spawning

- [ ] Shared rig → Zombie + Skeleton, then Creeper, then Spider.

- [ ] Dynamic spawning engine (light level < 7, despawn beyond 128 m) and XP orbs.

- [ ] Crafting table (3×3) and furnace (multi-fuel) interaction pass.



### Weeks 4 — Ruins, POIs & decals

- [ ] 12 ruin variants + decal kit + overgrowth props.

- [ ] POI prop kits (abandoned hospital, police station, village).

- [ ] 5 loot props (flesh, bone, string, eye, arrow).



### Weeks 5 — Weather, sound & networking prep

- [ ] Weather FX (rain/snow) driven by the fog/atmosphere system; Angel weather modulation (Fog/Ash Storm aggression).

- [ ] Sound design pass: voxel step sounds, block-break impacts, mob vocalisations, Angel stone-grind cue.

- [ ] Liquid simulation (flowing water & lava) and container storage (chests/barrels).

- [ ] Dedicated multiplayer server networking groundwork.

- [ ] Release candidate for **v0.4.0-beta**.



Carried over from v0.2 and still open: remaining biomes (Snowy Peaks, Swamps, Deep, Oceans), weather pass, liquids, containers, sound, multiplayer.



---



## 🤖 Use of AI



AI has only been used to generate documentation, comment code, and produce implementation plans. All complex implementation was made by hand.



*Enjoy playing and exploring MinecraftDeWish! Feel free to open issues, submit feedback, or contribute pull requests.*