# Technical Contract — MinecraftDeWish

**Status:** Frozen
**Date:** 2026-11-10
**Owner:** Technical Director
**Applies to:** All code, content, materials, and assets produced during the visual overhaul.
**References:** `Docs/SYSTEMS_REFERENCE.md`, `.opencode/docs/technical-preferences.md`, ADR-0001..ADR-0010, `Docs/architecture/voxel-performance-plan.md`, `Docs/art/material-parameters.md`, `Docs/BLOCK_REGISTRY.md`.

> This document freezes the conventions and contracts that must not change during the visual overhaul. Any change to a frozen contract requires a new ADR approved by the Technical Director. Production code and content must conform to this document; deviations are bugs.

---

## 1. Vertex Attribute Contract

Terrain and voxel-derived meshes (chunk meshes, debris, pickups, crack overlays) share a single vertex layout that the material `M_Global` / `M_Terrain_Opaque` / `M_Terrain_Masked` consumes. The contract is defined by `FChunkMeshData` (`ChunkActor.h`) and decoded by `M_Global`.

### 1.1 Attribute table

| Attribute | Channel / member | Meaning | Range |
|---|---|---|---|
| `UV0.XY` | `FChunkMeshData::UV0` | Face UVs (0..1), rotated per block and phase-shifted on isotropic tops | 0..1 (may exceed 1 in Wrap mode for stochastic tiling) |
| `UV1.X` | `FChunkMeshData::UV1.X` | Primary `Texture2DArray` slice index | 0..127 (budget) |
| `UV1.Y` | `FChunkMeshData::UV1.Y` | Secondary (neighbouring biome) slice index | 0..127 (budget) |
| `VertexColor.R` | `FChunkMeshData::VertexColors.R` | Slice index (legacy path, mirrors `UV1.X`) | 0..1 (byte 0..255) |
| `VertexColor.G` | `FChunkMeshData::VertexColors.G` | Tiling-breakup mask: `1` = natural terrain, `0` = constructed blocks | 0 or 1 (byte 0 or 255) |
| `VertexColor.B` | `FChunkMeshData::VertexColors.B` | Biome blend weight, interpolated per face corner | 0..0.5 (byte 0..128) |
| `VertexColor.A` | `FChunkMeshData::VertexColors.A` | Reserved — drives the `Alpha` input of two `LinearInterpolate` nodes in `M_Global` (base colour and normal) | 0..1 |

### 1.2 Decoding rules

- **8-bit vertex colour is sRGB-decoded.** Both `CreateMeshSection_LinearColor` calls pass `bSRGBConversion = true`. The material's `VertexColor` node decodes the stored byte as sRGB. With the default (`false`), a linear `0.5` weight reaches the shader as `sRGBToLinear(0.5) ≈ 0.21`, collapsing the fade into a hard step.
- **`VertexColor.B` is capped at `0.5`.** A block's own texture always stays dominant; the transition is symmetric on both sides of a border.
- **`UV0` is intentionally left outside `[0,1]` for stochastic tops** so the sampler's Wrap mode tiles it. Applying `Frac` collapses the quad onto a single texel.
- **No tangents** in the packed vertex layout (performance plan, lot 3). Normals are stored as `FVector` (unit length) and reconstructed or stored per-vertex.

### 1.3 Biome cross-fade (M_Global)

`M_Global` adds a second pair of array samples (`Base_Color_Blend`, `Normals_Blend`) whose slice comes from `UV1.Y`:

```
ComponentMask(G) of UV1 --> AppendVector(UV0, UV1.G) --> Base_Color_Blend / Normals_Blend
Primary BaseColor chain --Lerp(A)--> LinearInterpolate_2 --> BaseColor
Base_Color_Blend --------Lerp(B)-->            ^ (Alpha = VertexColor.B)
Primary normal chain -----Lerp(A)--> LinearInterpolate_3 --> FlattenNormal.Normal
Normals_Blend -----------Lerp(B)-->            ^ (Alpha = VertexColor.B)
```

- Blocks outside the natural surface set (Grass / Dirt / Sand / Stone) write `UV1.Y = UV1.X` and `VertexColor.B = 0`, so the extra samples resolve to the primary slice and the lerp is a no-op.
- The fade is gated to each column's top four blocks (`Z + 3 >= ColumnTopZ`), so cave walls, cliff faces, and deep stone keep their own material.
- The 50% point of the fade is derived from the signed distance to the exact thresholds `GetBiomeAt()` uses (`Temp > 0.60`, `Moisture < 0.40`, `Continental > 0.35`). Both paths sample the same `SampleBiomeFields()`.
- The fade runs `SmoothStep(0.75, 0.75 + BiomeBlendWidth, BlocksToBorder)` with `BiomeBlendWidth` measured in **blocks** (default 4).

### 1.4 Fast-leaf route (reserved, not active)

`VertexColor.A` already drives the `Alpha` input of two `LinearInterpolate` nodes in `M_Global`. The free channels are **R and G**. The reserved route for opaque fast leaves is: leaf faces write `0` into `G`, the Opacity Mask chain becomes `max(existing, 1 - G)`, and culling can then treat leaves as opaque. This change requires its own approval and a visual pass over every block type. **Do not enable leaf-to-leaf culling until this is implemented.**

---

## 2. Material Contract

### 2.1 Terrain materials

| Material | Blend mode | Two-sided | Used by | Notes |
|---|---|---|---|---|
| `M_Global` | Masked | No | Chunk meshes (solid + non-solid), debris, pickups | Legacy name; the split into Opaque / Masked is planned (performance plan, lot 3). Until the split ships, `M_Global` is Masked for all terrain. |
| `M_Terrain_Opaque` | Opaque | No | Future opaque terrain (performance plan, lot 3) | Early-Z friendly. Reuses the LabPBR decoding of `M_Global`. |
| `M_Terrain_Masked` | Masked | No | Future cutout terrain (leaves, glass) | Alpha-tested. `OpacityMaskClipValue` default 0.333. |

**LabPBR decoding (shared by `M_Global`, `M_Terrain_*`, `M_Item_PBR`, `M_Mob`):**
- `Specular_Array` / specular texture: **R = smoothness** → `OneMinus` → Roughness; **G = reflectance** → Metallic; **A = emissive** → multiplied with Base Color.
- `Normals_Array` / normal texture: tangent-space normals, sampled with the Normal sampler.

### 2.2 Master materials (Blender pipeline, ADR-0008)

| Material | Blend | Notes | Texture size | Filter |
|---|---|---|---|---|
| `M_Item_PBR` | Opaque | Reuses LabPBR decoding. `MI_<Master>_<Variant>` naming. | 32×32 (or 64×64 for weapons, TBD) | Nearest + mipmaps |
| `M_Mob` | Opaque (default) | Base color 128×128 (Patrix entity tier), normal + specular (LabPBR). | 128×128 | Nearest + mipmaps |
| `M_Statue` (Weeping Angel) | Opaque | Stone/plaster, high roughness, no metal, no emissive. Substrate slabs allowed (ADR-0004). | TBD | TBD |
| `M_BlockCrack` | Masked / Unlit / TwoSided | `OpacityMask = CrackTex.A`, `Emissive = CrackTex.RGB × 0.55`, `OpacityMaskClipValue = 0.15`. Mask comes from the **alpha** channel. | 32×32 | Nearest |
| `M_Debris_Voxel` | Masked | `SliceOuter`, `SliceInner`, `DissolveAmount`. | 32×32 | Nearest |
| `M_ItemPickup` | Masked | `SliceIndex`, `EmissiveBoost`. | 32×32 | Nearest |
| `M_VFX_*` | Various | See `Docs/art/material-parameters.md`. | 32×32 | Nearest |

### 2.3 Material rules

- **Terrain is excluded from the Lumen surface cache and ray-tracing proxies** (ADR-0001). Only the coarse GI proxy feeds Lumen.
- **Nanite materials must be Opaque, TwoSided-free, and free of WPO and Pixel Depth Offset** (ADR-0004).
- **Substrate stays in legacy authoring mode** (classic BaseColor/Normal/Roughness/Metallic/Emissive pins). Substrate slabs are reserved for the Weeping Angel only.
- **VSM shadows are disabled on small dynamic objects:** torches, debris, pickups, XP orbs, and the crack overlay cast no dynamic shadow. Only the sun, the player, and mobs cast shadows.
- **Ray tracing is OFF by default** (`r.RayTracing=False`). An opt-in "Ultra" preset may enable it.
- **Lumen is mandatory on every quality level**, including `@0` (Potato). The quality ladder degrades resolution and probe density, never the lighting method.

### 2.4 Material parameter authority

Every exposed parameter of a master material must be listed in `Docs/art/material-parameters.md`. A parameter not listed there must not be exposed. New parameters require an update to that file and, if they affect gameplay or performance, an ADR.

---

## 3. Naming Conventions

### 3.1 C++ classes and files

| Element | Convention | Example |
|---|---|---|
| Actor class | `A` prefix + PascalCase | `AChunkActor`, `ABlockCrackActor` |
| UObject / Component class | `U` prefix + PascalCase | `UQuickSlotsInventorySystem`, `UBaublesSystem` |
| Struct | `F` prefix + PascalCase | `FChunkMeshData`, `FToolInstance` |
| Enum | `E` prefix + PascalCase | `EToolType`, `EMobType` |
| Interface | `I` prefix + PascalCase | `IVoxelInteractable` |
| Header file | Class name without prefix | `ChunkActor.h` |
| Source file | Class name without prefix | `ChunkActor.cpp` |

### 3.2 Variables, functions, constants

| Element | Convention | Example |
|---|---|---|
| Variables | PascalCase; booleans start with `b` | `bIsMining`, `CurrentHealth`, `BlockScale` |
| Functions | PascalCase, verb-first | `StartMining`, `GetBlockAt`, `BreakBlockAtVoxel` |
| Constants | `static constexpr` in PascalCase, or `UPPER_SNAKE_CASE` for macros | `DEFAULT_BLOCK_SCALE`, `CHUNK_SIZE_X` |
| Log categories | `DEFINE_LOG_CATEGORY_STATIC(LogMinecraftDeWish, Log, All)` | — |

### 3.3 Assets (Blueprints, meshes, materials, textures)

| Prefix | Type | Example |
|---|---|---|
| `BP_` | Blueprint actor | `BP_Mob_Zombie`, `BP_HorrorCharacter` |
| `WBP_` | Widget Blueprint | `WBP_QuickSlots`, `WBP_Baubles_Root` |
| `DA_` | Data Asset | `DA_Tool_Pickaxe_Iron` |
| `DT_` | Data Table | `DT_Tools`, `DT_Mobs`, `DT_Blocks` |
| `M_` | Material | `M_Global`, `M_BlockCrack`, `M_Item_PBR` |
| `MI_` | Material Instance | `MI_M_Item_PBR_Wood` |
| `T_` | Texture | `T_Terrain_BC`, `T_Terrain_N` |
| `S_` | Static mesh | `S_Tool_Pickaxe_Wood` |
| `SK_` | Skeletal mesh | `SK_Mob_Base`, `SK_PlayerArms` |
| `A_` | Animation sequence | `A_Mob_Walk`, `A_Tool_Swing_Pickaxe` |
| `ABP_` | Animation Blueprint | `ABP_Mob_Base`, `ABP_PlayerArms` |
| `NS_` | Niagara system | `NS_Dust_Impact` |

**Asset rules:** no spaces, no version suffixes, no `_New` / `_Final`. All asset names use only ASCII alphanumeric characters and underscores.

### 3.4 Skeletons and animation

- One shared skeleton `SK_Mob_Base` for all humanoid mobs (Zombie, Skeleton, future). Six bones, rigid weights (100% per part).
- Spider uses a separate skeleton (8 legs).
- Animation Blueprint: `ABP_Mob_Base` (shared locomotion state machine driven by `Speed` / `bIsFalling` / state enum exposed as `BlueprintReadOnly` from C++). Derived ABPs only where a mob genuinely differs.

---

## 4. LOD Policy

| Asset class | LOD count | Triangles | Notes |
|---|---|---|---|
| **Tools / weapons** | 1 LOD | 200–800 tris | StaticMesh, no skinning. Pivot at grip centre, handle along local +Z. |
| **Mobs (humanoid)** | 3 LODs | LOD0: 2–5k / LOD1: ~800 / LOD2: ~200 | Dithered fade between LODs. `UAnimBudgetAllocatorComponent` + significance manager. |
| **Mobs (Spider)** | 3 LODs | LOD0: 2–5k / LOD1: ~800 / LOD2: ~200 | Separate skeleton (8 legs). |
| **Statues (Weeping Angel)** | Nanite | N/A (Nanite) | Nanite is ON for static props and statues only. Materials must be Opaque, TwoSided-free, free of WPO and Pixel Depth Offset. |
| **Props (static, >20k tris)** | Nanite | N/A (Nanite) | Nanite is ON for stable high-triangle geometry. |
| **Terrain** | Classic LOD rings (proxy scene) | N/A | Terrain never uses Nanite (runtime `CreateMeshSection` does not produce a Nanite resource). LOD rings at 2×/4×/8× with skirts and dithered fade-in. |
| **Skeletal meshes (player arms, mobs)** | Classic LODs | Per mob table above | Skeletal meshes stay on classic LODs. |
| **Debris / pickups / XP orbs** | 1 LOD (or none) | Low poly | Single draw call each. |

**LOD transition:** dithered fade-in (no pop-in). Pop-in is the main perceived "not smooth" before frame rate.

---

## 5. Collision Policy

### 5.1 Terrain blocks

| Block type | Collision | Notes |
|---|---|---|
| Solid blocks (Dirt, Stone, etc.) | `ECollisionEnabled::QueryAndPhysics` on mesh section 0 | Full collision for physics and queries. |
| Non-solid blocks (Torch) | `ECollisionEnabled::QueryOnly` on mesh section 1 | `ECC_Pawn` = `ECR_Overlap`, `ECC_WorldStatic` = `ECR_Block`. Players walk through; line traces hit. |
| Glass, Fence, Door, Fence Gate | `QueryAndPhysics` (full cube) | **Not yet non-cubic.** Rendered and collided as full opaque cubes until a non-cubic mesh path exists. |

### 5.2 Tools and weapons

| State | Collision | Notes |
|---|---|---|
| **Equipped** | `NoCollision` | No collision component at all. The tool is a visual attachment to the first-person arms. |
| **World-dropped** | Simple box | Auto-generate OFF. A single simple box collision. No convex hulls on tools. |

### 5.3 Other actors

| Actor | Collision | Notes |
|---|---|---|
| `ABlockItemPickup` | Floor sweep collision | Gravity (1200 cm/s²), air drag, floor sweep. |
| `AChaosBlockDestructionActor` (debris) | None | No per-fragment collision. Ground sweeps against world geometry only. |
| `ABlockCrackActor` (overlay) | None | No collision; line traces ignore it. |
| `AXPOrbPickup` | Physics parabolic bounce + magnet | Idle rotational bobbing, player magnet within 3.5 m. |
| Mobs | Character collision (capsule) | Standard `ACharacter` capsule. |

### 5.4 Collision rules

- Auto-generate collision is **OFF** for all imported meshes. Collision must be authored explicitly.
- No convex hulls on tools.
- No trimesh per chunk (performance plan, lot 4): greedy boxes for a 5×5-chunk window around the player + DDA voxel raycasts.

---

## 6. Scale and Axes

### 6.1 Scale

- **1 block = 1 m = 100 UU.**
- Blender works in metres: 1 unit = 1 m.
- FBX export uses "Apply Unit" with import scale 1.0. A 1×1×1 cube must arrive as 100×100×100 UU.
- Chunk dimensions: 16 × 16 × 64 blocks = 1600 × 1600 × 6400 UU.
- Voxel size: 100 UU (confirmed in `GAME_DESIGN.md` §2.1 and `WorldGenerator`).

### 6.2 Axes (Blender → Unreal)

| Blender | Unreal | Notes |
|---|---|---|
| Forward | `-Z` | Export setting |
| Up | `Y` | Export setting |
| `Apply Transform` | ON | Ensures clean transform on import |

Result: a character faces **+X** in Unreal after export.

### 6.3 Pivots

| Asset class | Pivot location | Axis alignment |
|---|---|---|
| Tools / weapons | Origin at the grip centre | Handle along local +Z |
| Mobs | Origin between the feet | On the vertical centre of mass |
| Props | Origin at the logical base | Ground contact |
| Statues | Origin at the base | Ground contact |

### 6.4 Texture resolution by asset class

| Asset class | Resolution | Filter | Mipmaps | Compression |
|---|---|---|---|---|
| Terrain (Patrix) | 32×32 | Nearest | ON | `TC_DEFAULT` (base color), `TC_NORMALMAP` (normal) |
| Tools / items | 32×32 (or 64×64 for weapons, TBD) | Nearest | ON | `TC_DEFAULT` |
| Mobs | 128×128 | Nearest | ON | `TC_DEFAULT` (base color), `TC_NORMALMAP` (normal) |
| Statues (Angel) | TBD | TBD | TBD | TBD |
| UI icons | 32×32 (Patrix, unchanged) | — | — | — |
| UI panels | Enamel/CRT style (see `design/ui/ui-direction.md`) | — | — | — |

**Texture rules:**
- LabPBR textures use the specular channel layout: R = smoothness, G = reflectance, A = emissive.
- Normal maps use `TC_NORMALMAP` compression and the Normal sampler.
- Base color and specular use `TC_DEFAULT` (sRGB).
- Nearest filter is mandatory for pixel-art textures to preserve crisp edges.
- Mipmaps are ON for all textures (anisotropy TBD, to be resolved before P3 production).

---

## 7. Texture Array Contract

The terrain material samples `Texture2DArray` resources. The contract is defined by `Global_Textures_Array` and consumed by `M_Global` / `M_Terrain_Opaque` / `M_Terrain_Masked`.

### 7.1 Array parameters

| Parameter | Type | Default | Notes |
|---|---|---|---|
| `Textures_Array` | `Texture2DArray` | `T_Terrain_BC` | Base color array (slice = `UV1.X`) |
| `Normals_Array` | `Texture2DArray` (Normal) | `T_Terrain_N` | Tangent-space normals |
| `Specular_Array` | `Texture2DArray` (Linear) | `T_Terrain_S` | R = smoothness, G = reflectance, A = emissive |

### 7.2 Slice budget

- **Current slices:** 37 (indices 0–36). See `Docs/BLOCK_REGISTRY.md` for the full mapping.
- **Budget:** 128 slices (validate 37→128 slice budget per `design/art/art-direction.md` §4).
- **No bedrock and no water slice.** Bedrock (ID 25) maps to `stone` (slice 35); Water (ID 26) maps to `glass` (slice 23).
- **New slices** are added for ruin variants (12 planned) and new block types. Never renumber existing slices (save data may reference them).
- Every new slice must have a corresponding normal map (`_n`) and specular map (`_s`).

### 7.3 Slice rules

- A face encodes its texture index in vertex buffer attribute `UV1.X`.
- `UV1.X` is a float in the shader but represents an integer slice index. The material samples the array with `Texture2DArray.SampleLevel` or equivalent.
- Secondary slice (`UV1.Y`) is used for biome cross-fade only. Non-surface blocks write `UV1.Y = UV1.X`.
- `VertexColor.R` mirrors `UV1.X` (legacy path). Both must agree.

### 7.4 Compression and filtering

- **Base color array (`T_Terrain_BC`):** `TC_DEFAULT` (sRGB), Nearest filter, mipmaps ON.
- **Normal array (`T_Terrain_N`):** `TC_NORMALMAP`, Normal sampler, mipmaps ON.
- **Specular array (`T_Terrain_S`):** `TC_DEFAULT` (Linear), Linear sampler, mipmaps ON.
- **Filter:** Nearest for all pixel-art textures. No bilinear/trilinear on Patrix textures.
- **Mipmaps:** ON for all textures. Anisotropy TBD (to be resolved before P3 production).

---

## 8. Performance Budgets

### 8.1 Frame and streaming budgets

| Metric | Baseline | After lots 0–2 | After lots 3–4 | Target |
|---|---|---|---|---|
| Target framerate | 60 FPS | 60 FPS | 60 FPS | 60 FPS (16.6 ms frame) |
| Gameplay systems (per frame) | — | — | — | < 4 ms |
| Game thread, streaming | to measure | < 6 ms | < 5 ms | < 5 ms |
| Game thread, worst frame | to measure | < 12 ms | < 10 ms | < 10 ms |
| Upload per frame | not measured | < 2 MB | < 2 MB | < 2 MB |
| Visible draw calls | ~1 200–1 600 | same | < 800 | < 800 |
| Fill time, RD 8 | ~5 s, 40+ hitches | < 3 s, no hitch > 16 ms | < 2 s | < 2 s |

### 8.2 Memory budgets

| Metric | Baseline | After lots 0–2 | After lots 3–4 | Target |
|---|---|---|---|---|
| Chunk store RAM, RD 8 / 64 h | ~200 MB | < 90 MB | < 60 MB | < 60 MB |
| Chunk store RAM, RD 8 / 192 h | ~600 MB | ~200 MB | < 140 MB | < 140 MB |
| VRAM | TBD | TBD | TBD | TBD (measure with Unreal Insights) |
| Memory ceiling | TBD | TBD | TBD | TBD (measure with `stat memory`) |

### 8.3 Hardware floor (ADR-0001)

| Tier | Hardware | Target |
|---|---|---|
| Minimum supported | GTX 1060 6 GB / RX 580 8 GB, 4c/8t, 8 GB RAM, SSD | 1080p, 60 fps, preset `@1` |
| Absolute floor | GTX 1050 Ti / RX 570 | 1080p, 30 fps, preset `@0`, 50–60% resolution scale, render distance 4 |
| Out of scope | Integrated GPUs (Intel UHD, AMD Vega iGPU, Apple silicon) | Not supported |

### 8.4 Mob animation budget

- `UAnimBudgetAllocatorComponent` + significance manager from the start.
- Mobs beyond a distance threshold reduce tick rate and LOD.
- 20+ mobs must stay inside frame budget.
- Shared skeleton (`SK_Mob_Base`, 6 bones, rigid weights) keeps animation cost low.

### 8.5 Profiling tools

- **Unreal Insights**, `stat unit`, `stat game`, `stat gpu`, `stat memory`, `stat lumen`, `stat scenerendering`, `stat rhi`, `stat streaming`, `stat physics`, `stat gc`, `stat virtualshadowmaps`.
- Never rely on an FPS counter alone.
- `UVoxelBenchSubsystem` for voxel streaming benchmarks (CSV output to `Saved/Profiling/`).

---

## 9. Rules That Must Not Be Violated

### 9.1 Code rules

1. **No `new` / `delete` for `UObject` instances.** Use `NewObject<>`, `CreateDefaultSubobject<>`, `TObjectPtr<>`.
2. **No raw `UObject*` members without `UPROPERTY()`** (GC will collect them).
3. **No hardcoded gameplay balance values** (damage, health, drop rates, recipes, mining forces, durabilities). Use DataTables / DataAssets (`DT_Tools`, `DT_Mobs`, `DT_Blocks`, etc.).
4. **No direct UI references from gameplay code.** Use delegates / events.
5. **No `Tick` on actors that do not need per-frame updates.** Prefer timers, delegates, event-driven systems.
6. **No string comparisons in hot paths.** Use `FName`.
7. **No direct attribute mutation outside GAS** (once the relevant system is GAS-based).
8. **No new plugins or third-party libraries without an ADR** approved by the Technical Director.
9. **No `UE_LOG(LogTemp, ...)` in production code.** Use `DEFINE_LOG_CATEGORY_STATIC(LogMinecraftDeWish, Log, All)`.
10. **Every new gameplay rule needs an automation test.** Test naming: `MinecraftDeWish.<System>.<Behavior>`.
11. **Changes to public headers need a rebuild of the editor target and a smoke test in PIE.**

### 9.2 Material and rendering rules

12. **Lumen is mandatory on every quality level.** Never disable it (ADR-0001).
13. **Ray tracing is OFF by default.** Never enable it without an opt-in preset (ADR-0004).
14. **Nanite is for static props and statues only.** Terrain and skeletal meshes use classic LODs.
15. **Substrate stays in legacy authoring mode.** Slabs reserved for the Weeping Angel only.
16. **Terrain is excluded from the Lumen surface cache and RT proxies.** Only the coarse GI proxy feeds Lumen.
17. **VSM shadows disabled on small dynamic objects** (torches, debris, pickups, XP orbs, crack overlay).
18. **No leaf-to-leaf culling** until opaque fast leaves are implemented (`VertexColor.G` route).

### 9.3 Data and content rules

19. **Every row/asset must have a stable, unique ID.** Never renumber existing IDs (save data may reference them).
20. **Define a schema (struct) for every DataTable.** Validate rows on import or in an automation test.
21. **New fields need a default value and a migration note** for existing save data.
22. **Balance changes must cite the formula or rationale** in the design doc.
23. **No debug/test rows in production DataTables.** Use a separate test asset.
24. **JSON must be valid UTF-8, no trailing commas, no comments.** Validate before committing.
25. **Curves and formulas must be documented** in the GDD (`design/gdd/`) with variable names and units.
26. **Keep block, mob, and tool definitions aligned** with `Docs/BLOCK_REGISTRY.md`, `Docs/MOB_REGISTRY.md`, `Docs/TOOL_REGISTRY.md`.

### 9.4 Voxel write rules (ADR-0002)

27. **Generated voxel writes must use `SetBlockGenerated` / `SetBlockWithMetaGenerated`.** These do not set `bIsModified`.
28. **Player voxel writes must use `SetBlock` / `SetBlockWithMeta`.** These set `bIsModified = true`.
29. **Never delete the save directory at startup.** Terrain-parameter changes are handled by a format version, never by wiping data.

### 9.5 Networking rules (single-player today, but if multiplayer is added)

30. **The server is authoritative.** Clients request actions; the server validates and replicates results.
31. **Replicate only what is necessary.** Prefer `ReplicatedUsing` with a callback over polling in `Tick`.
32. **Never let a client decide damage, loot, inventory contents, or world state.**
33. **Replication changes require an ADR** if they alter ownership, authority, or save data.

### 9.6 UI rules

34. **UI never owns game state.** It reads from a view model / subsystem and sends intent back via delegates.
35. **Gameplay code never calls a widget directly.** Use events, delegates, or a UI subsystem.
36. **All player-facing text must be localizable** (`FText` / `NSLOCTEXT` / string tables). No hardcoded strings in widgets.
37. **No expensive `Tick` logic in widgets.** Update on change, not every frame.

### 9.7 Asset and pipeline rules (ADR-0008)

38. **No manual import of more than 5 assets without the FBX import preset.**
39. **Blender export: Forward `-Z`, Up `Y`, `Apply Transform` ON.**
40. **Tools: origin at grip centre, handle along local +Z.**
41. **Mobs: origin between the feet, on the vertical centre of mass.**
42. **Props: origin at the logical base (ground contact).**
43. **One shared skeleton `SK_Mob_Base` for all humanoid mobs.** Spider gets a separate skeleton.
44. **Tools and weapons are StaticMesh, not SkeletalMesh.** No deformation, no skinning cost.
45. **One `AEquippedToolActor`** with a swapped mesh. Never spawn/destroy per tool.
46. **Montages are per tool class, not per tier.** Sections Windup/Impact/Recovery.

### 9.8 Documentation rules

47. **All game code must include doc comments on public APIs.**
48. **Every system must have a corresponding ADR** in `Docs/architecture/`.
49. **Gameplay values must be data-driven (external config), never hardcoded.**
50. **All public methods must be unit-testable** (dependency injection over singletons).
51. **Commits must reference the relevant design document or task ID.**

---

## 10. Contract Enforcement

- Any change to a frozen contract requires a new ADR approved by the Technical Director.
- Production code and content must conform to this document; deviations are bugs.
- This document is the reference for all production. When in doubt, consult this document before writing code or creating assets.
- The `Docs/architecture/` directory contains the full ADRs. This document summarizes and freezes their decisions.

---

*Last updated: 2026-11-10. Frozen by the Technical Director.*
