# Visual Style Guide — LAMPLIGHTER (MinecraftDeWish)

**Owner:** Technical Artist (under Art Director)
**Date:** 2026-11-10
**Status:** Accepted — derived from `design/art/art-direction.md` (Accepted) and
`Docs/technical-contract.md` (Frozen). Changes to frozen contracts require an
ADR approved by the Technical Director.
**References:** `design/art/art-direction.md`, `Docs/technical-contract.md`,
`Docs/art/material-parameters.md`, `Docs/assets-provenance.md`,
`Docs/BLOCK_REGISTRY.md`, `Docs/MOB_REGISTRY.md`, `Docs/TOOL_REGISTRY.md`.

> **Thesis (art-direction §1):** *Eroded Realism — the world is built, the Angel
> is sculpted.* Worn voxel world, sodium light, a white statue that should not
> be there. A screenshot must be identifiable without a logo.

---

## 1. Style lane (resolved decisions)

- **PBR voxel horror**, not PS1 flat-shaded. The Screaming Brain pack's flat PS1
  look is a mismatch for our material stack (`M_Terrain_Opaque` expects base
  color + normal + specular arrays). Study its palette/grunge only.
- **Original art**, not Patrix. Block *silhouettes* are preserved for muscle
  memory; pixels are ours.
- **Mobs: stylized low-poly, non-voxel**, distinct from the Angel by material
  (flesh/bone/chitin vs. stone). See art-direction §5.
- **Angel: sculpted stone/plaster**, the only non-block entity. No VFX, ever.

---

## 2. Palette

Source of truth: `design/art/art-direction.md` §2. Do not introduce new colours
without art-director approval.

| Hex | Name | World use | UI use |
|---|---|---|---|
| `#0B0E11` | Void | night sky, unlit interiors, fog | modal scrim (85%) |
| `#171C21` | Concrete | cut stone, POI walls, floors | panel surface |
| `#2A3138` | Steel | metal, roofs, bars, machines | disabled state |
| `#E3DCCB` | Bone | plaster, overcast sky, **Angel stone** | primary text |
| `#E8A33D` | Sodium | **the only warm light**: torch, lamp, furnace, generator | accent, focus, selected |
| `#C8462F` | Rust/Blood | creeper, damage, corrosion, explosion | danger, error |
| `#4FA6A0` | Signal | electricity, cameras, CRT — the only "clean" light | success, active toggle |
| `#5C6268` | Ash | fog, tarnished metal, disabled | secondary text |
| `#6B7355` | Moss | lichen, residual vegetation | — |
| `#9AA36B` | Sick green | Horror Forest, sick leaves | — |
| `#8C8577` | Dust | sand, rubble, ash on ground | — |

**Rules (non-negotiable):**
- One warm source only: sodium. Everything else is cold, desaturated, ashen.
- No saturated blue sky. Day sky is bone-ash; night is cold teal-dark.
- XP orbs desaturated to `#B9C46A` — the only "game-y" element, kept discreet.
- Rust red is reserved for danger. It never decorates.
- Sodium grade (LUT) is constant; only light intensity/filtration changes with
  weather and day/night. The global grade never brightens the world.

---

## 3. Light and grade

- Post-process + single LUT "Sodium Grade": high contrast in lows, soft rolloff
  in highs, 25–35% desaturation outside sodium.
- Ash fog reduces readability to 60–90 blocks. The Angel at 60 blocks must read
  as a silhouette, not a detail.
- The torch is the brightest object on screen, always. Visual hierarchy of the
  fantasy.
- Day/night cycle (20 min) kept; grade is constant. Phase 3 never brightens
  the world — the player installs light.
- Weather modulates mood: rain (wet surfaces, more reflections), thick fog
  (reduced visibility), ash storm (particles, wind, noise), clear sky (rare,
  unsettling by contrast). Only light intensity/filtration changes; the LUT stays.

---

## 4. Materials and PBR contract

### 4.1 LabPBR decoding (shared by all master materials)

- `Specular_Array` / specular texture: **R = smoothness** → `OneMinus` →
  Roughness; **G = reflectance** → Metallic; **A = emissive** → multiplied with
  Base Color.
- `Normals_Array` / normal texture: tangent-space normals, sampled with the
  Normal sampler.
- Base color + specular use `TC_DEFAULT` (sRGB); normals use `TC_NORMALMAP`.

### 4.2 Master materials

| Material | Blend | Used by | Texture size | Filter | Notes |
|---|---|---|---|---|---|
| `M_Global` | Masked | Chunk meshes, debris, pickups (legacy) | 32×32 slices | Nearest + mipmaps | Split into Opaque/Masked planned (perf plan lot 3). |
| `M_Terrain_Opaque` | Opaque | Future opaque terrain | 32×32 slices | Nearest + mipmaps | Early-Z friendly. Reuses LabPBR decoding. |
| `M_Terrain_Masked` | Masked | Cutout terrain (leaves, glass) | 32×32 slices | Nearest + mipmaps | `OpacityMaskClipValue` default 0.333. |
| `M_Item_PBR` | Opaque | Tools/items | 32×32 (64×64 weapons) | Nearest + mipmaps | `MI_<Master>_<Variant>` naming. |
| `M_Mob` | Opaque (default) | Mobs | 128×128 | Nearest + mipmaps | Base color + normal + specular (LabPBR). |
| `M_Statue` (Angel) | Opaque | Weeping Angel | TBD | TBD | Stone/plaster, high roughness, no metal, no emissive. Substrate slabs allowed (ADR-0004). |
| `M_BlockCrack` | Masked/Unlit/TwoSided | Crack overlay | 32×32 | Nearest | `OpacityMask = CrackTex.A`, `Emissive = CrackTex.RGB × 0.55`, clip 0.15. Mask from alpha. |
| `M_Debris_Voxel` | Masked | Debris | 32×32 | Nearest | `SliceOuter`, `SliceInner`, `DissolveAmount`. |
| `M_ItemPickup` | Masked | Pickups | 32×32 | Nearest | `SliceIndex`, `EmissiveBoost`. |
| `M_VFX_*` | Various | VFX | 32×32 | Nearest | See `Docs/art/material-parameters.md`. |

### 4.3 Material rules

- Terrain is excluded from the Lumen surface cache and ray-tracing proxies
  (ADR-0001). Only the coarse GI proxy feeds Lumen.
- Nanite materials must be Opaque, TwoSided-free, and free of WPO and Pixel
  Depth Offset (ADR-0004). Nanite is for static props and statues only.
- Substrate stays in legacy authoring mode. Slabs reserved for the Weeping
  Angel only.
- VSM shadows disabled on small dynamic objects: torches, debris, pickups, XP
  orbs, crack overlay. Only sun, player, and mobs cast shadows.
- Ray tracing OFF by default (`r.RayTracing=False`). Opt-in "Ultra" preset may
  enable it.
- Lumen mandatory on every quality level, including `@0` (Potato). The quality
  ladder degrades resolution and probe density, never the lighting method.
- Every exposed parameter of a master material must be listed in
  `Docs/art/material-parameters.md`. A parameter not listed must not be exposed.

### 4.4 PBR value guidance (calibrate against ambientCG / Poly Haven)

- **Stone/concrete:** high roughness (0.7–0.95), low reflectance (0.02–0.06),
  neutral base color (bone/concrete/steel range). Micro-normal for porosity.
- **Rusted metal:** roughness 0.4–0.7 in rusted areas, lower (0.2–0.35) on
  exposed metal; reflectance 0.3–0.5 on bare metal, near 0 on rust. Emissive
  only for lava/minerals (`EmissiveBoost`).
- **Dirt/ash/ground:** very high roughness (0.85–1.0), low reflectance
  (0.02–0.04), desaturated (ash/dust/moss range).
- **Wood (tools):** roughness 0.6–0.8, reflectance 0.04–0.06; tier reads
  through material (wood → stone → iron → diamond (cold white-blue, the only
  "pure" material) → obsidian (black glass)).
- **Angel stone:** high roughness, no metal, no emissive, `#E3DCCB`. Wetness
  and moss overlays via parameters.
- **Mobs:** matte desaturated materials (flesh/bone/chitin). Never glossy.
  Only the Angel is smooth stone.

---

## 5. Blocks and terrain

- Patrix kept as the *material truth* (player recognises their blocks). No mass
  texture rework; instead 12 "ruin" variants (cracked/mossy/sooted/scorched) of
  the most-seen blocks → new slices in the existing Texture2DArray (validate
  37→128 slice budget).
- Deferred decals: moss, soot, ash, blood, burn marks, torn posters. Best
  impact/cost ratio. No new BlockIDs.
- Vegetation: leaves and grass desaturated to olive/sick. Overgrowth props
  (ivy, roots) blocked until a non-solid mesh path exists (only torches have
  one).
- Erosion reads through material + decal + fog, never through silhouette.

---

## 6. Mobs — stylized low-poly

Not voxel-rigged, not photorealistic. Low-poly stylized horror: readable
silhouettes, simple but articulated geometry, matte desaturated materials. Mobs
are creatures of the world (flesh, bone, chitin); the Angel is stone.

| Mob | Visual read |
|---|---|
| Zombie | Torn work clothes, exposed ribs, dislocated jaw. Slow, hunched. |
| Skeleton | Stained, broken bones; half-dressed (police uniform?). |
| Spider | Low and wide, 8 articulated legs, chitin plates, tufts. Must read as a shape at night. |
| Creeper | The only organic form among voxels: swollen, moss-covered. Explosion tell = swelling + sodium cracks from within. No face. |

- One shared humanoid rig (`SK_Mob_Base`, 6 bones, rigid weights) covers
  Zombie + Skeleton. Spider gets its own skeleton (8 legs).
- Animation Blueprint: `ABP_Mob_Base` (shared locomotion state machine driven
  by `Speed` / `bIsFalling` / state enum). Derived ABPs only where a mob
  genuinely differs.
- Mob animation budget: `UAnimBudgetAllocatorComponent` + significance manager
  from the start. 20+ mobs must stay inside frame budget.

---

## 7. The Angel — the contrast is the mechanic

- Material: sculpted stone/plaster, `#E3DCCB`, high roughness, no metal, no
  emissive. Classic drapery, folded wings, **no eyes**.
- No VFX, ever. No particles, no aura, no glow. The only animation is movement
  and the grating of stone.
- No walk cycle. Only **8 statue poses** (pointing, head turned, hands on face,
  mid-stride, crouched, wings half-open, etc.) that read as "caught
  mid-gesture".
- Acceptance criterion (angels.md §8.5): a frozen statue is distinguishable
  without HUD, via three signals: smooth silhouette, material contrast, pose
  that changes between two glances.
- At 60 blocks in fog: a clear, smooth, immobile shape, backlit. Enough.
- Angel material contrast with the voxel world is the entire visual identity. If
  a player describes the Angel as "a mob", the contrast has failed.

---

## 8. Tools, weapons, hands

- 5 tools × 5 tiers = 25 meshes. Minecraft silhouettes conserved (muscle
  memory) + scavenged detailing: taped grip, wire-wrapped handle, repaired guard.
- Tier reads through material: raw wood → cracked stone → pitted iron → diamond
  (the only "pure" material, cold white-blue) → obsidian (black glass).
- First-person hands: rolled sleeves, dirty gloves, a broken watch on the wrist.
  Free narrative detail: time no longer belongs to you.
- 5 swing sets (pickaxe/shovel/axe/hoe/sword), not 25. Montages are per tool
  class, not per tier (sections Windup/Impact/Recovery).
- Tools are StaticMesh, not SkeletalMesh. No deformation, no skinning cost.
- One `AEquippedToolActor` with a swapped mesh. Never spawn/destroy per tool.

---

## 9. UI

Reference: `design/ui/ui-direction.md` (authoritative). Three additions from
art direction:
1. Baubles panel missing → `WBP_Baubles_Root`, priority 2.
2. Legacy textures `quick_slots` / `Item_BG_Panel` / `Highlight_Slot`
   (Minecraft vanilla style) are incompatible with enamel — they die with
   `WB_HUD` in phase 5.
3. Icon treatment: Patrix icon unchanged + inner shadow + 2px sodium ring on
   selection. Do not repaint 1,801 icons.

UI rules: UI never owns game state; gameplay code never calls a widget directly;
all player-facing text localizable (`FText`/`NSLOCTEXT`/string tables); no
expensive `Tick` in widgets; update on change, not every frame.

---

## 10. Naming conventions (assets)

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

Asset rules: no spaces, no version suffixes, no `_New` / `_Final`. ASCII
alphanumeric + underscores only.

---

## 11. Resolution, filter, compression by asset class

Source: `Docs/technical-contract.md` §6.4 and §7.4.

| Asset class | Resolution | Filter | Mipmaps | Compression |
|---|---|---|---|---|
| Terrain (block slices) | 32×32 | Nearest | ON | `TC_DEFAULT` (base color), `TC_NORMALMAP` (normal) |
| Tools / items | 32×32 (64×64 weapons) | Nearest | ON | `TC_DEFAULT` |
| Mobs | 128×128 | Nearest | ON | `TC_DEFAULT` (base color), `TC_NORMALMAP` (normal) |
| Statues (Angel) | TBD | TBD | TBD | TBD |
| UI icons | 32×32 | — | — | — |
| UI panels | Enamel/CRT style | — | — | — |
| VFX sprites | 32×32 | Nearest | ON | `TC_DEFAULT` |

Texture rules:
- LabPBR specular layout: R = smoothness, G = reflectance, A = emissive.
- Normal maps use `TC_NORMALMAP` compression and the Normal sampler.
- Base color and specular use `TC_DEFAULT` (sRGB).
- Nearest filter mandatory for pixel-art textures to preserve crisp edges.
- Mipmaps ON for all textures.

---

## 12. LOD policy

Source: `Docs/technical-contract.md` §4.

| Asset class | LOD count | Triangles | Notes |
|---|---|---|---|
| Tools / weapons | 1 LOD | 200–800 tris | StaticMesh, no skinning. Pivot at grip centre, handle along local +Z. |
| Mobs (humanoid) | 3 LODs | LOD0: 2–5k / LOD1: ~800 / LOD2: ~200 | Dithered fade. `UAnimBudgetAllocatorComponent` + significance manager. |
| Mobs (Spider) | 3 LODs | LOD0: 2–5k / LOD1: ~800 / LOD2: ~200 | Separate skeleton (8 legs). |
| Statues (Angel) | Nanite | N/A | Nanite ON for static props/statues only. Materials Opaque, TwoSided-free, no WPO/PDO. |
| Props (static, >20k tris) | Nanite | N/A | Nanite ON for stable high-triangle geometry. |
| Terrain | Classic LOD rings (proxy scene) | N/A | Terrain never uses Nanite (runtime `CreateMeshSection`). Rings at 2×/4×/8× with skirts + dithered fade-in. |
| Skeletal meshes (player arms, mobs) | Classic LODs | Per mob table | Skeletal meshes stay on classic LODs. |
| Debris / pickups / XP orbs | 1 LOD (or none) | Low poly | Single draw call each. |

LOD transition: dithered fade-in (no pop-in). Pop-in is the main perceived
"not smooth" before frame rate.

---

## 13. Collision policy

Source: `Docs/technical-contract.md` §5.

### Terrain blocks

| Block type | Collision | Notes |
|---|---|---|
| Solid blocks (Dirt, Stone, etc.) | `ECollisionEnabled::QueryAndPhysics` on mesh section 0 | Full collision for physics and queries. |
| Non-solid blocks (Torch) | `ECollisionEnabled::QueryOnly` on mesh section 1 | `ECC_Pawn` = `ECR_Overlap`, `ECC_WorldStatic` = `ECR_Block`. Players walk through; line traces hit. |
| Glass, Fence, Door, Fence Gate | `QueryAndPhysics` (full cube) | Not yet non-cubic. Rendered and collided as full opaque cubes until a non-cubic mesh path exists. |

### Tools and weapons

| State | Collision | Notes |
|---|---|---|
| Equipped | `NoCollision` | Visual attachment to first-person arms only. |
| World-dropped | Simple box | Auto-generate OFF. Single simple box. No convex hulls on tools. |

### Other actors

| Actor | Collision | Notes |
|---|---|---|
| `ABlockItemPickup` | Floor sweep collision | Gravity (1200 cm/s²), air drag, floor sweep. |
| `AChaosBlockDestructionActor` (debris) | None | No per-fragment collision. Ground sweeps against world geometry only. |
| `ABlockCrackActor` (overlay) | None | No collision; line traces ignore it. |
| `AXPOrbPickup` | Physics parabolic bounce + magnet | Idle bobbing, player magnet within 3.5 m. |
| Mobs | Character collision (capsule) | Standard `ACharacter` capsule. |

Collision rules: auto-generate collision OFF for all imported meshes; collision
authored explicitly; no convex hulls on tools; no trimesh per chunk (greedy
boxes for a 5×5-chunk window + DDA voxel raycasts).

---

## 14. Scale and axes

Source: `Docs/technical-contract.md` §6.

- **1 block = 1 m = 100 UU.** Blender works in metres (1 unit = 1 m). FBX export
  uses "Apply Unit" with import scale 1.0. A 1×1×1 cube must arrive as
  100×100×100 UU.
- Chunk dimensions: 16 × 16 × 64 blocks = 1600 × 1600 × 6400 UU.
- Voxel size: 100 UU.

Axes (Blender → Unreal):

| Blender | Unreal | Notes |
|---|---|---|
| Forward | `-Z` | Export setting |
| Up | `Y` | Export setting |
| `Apply Transform` | ON | Clean transform on import |

Result: a character faces **+X** in Unreal after export.

Pivots:

| Asset class | Pivot location | Axis alignment |
|---|---|---|
| Tools / weapons | Origin at grip centre | Handle along local +Z |
| Mobs | Origin between the feet | On vertical centre of mass |
| Props | Origin at logical base | Ground contact |
| Statues | Origin at base | Ground contact |

---

## 15. Texture array contract (terrain)

Source: `Docs/technical-contract.md` §7.

- Arrays: `Textures_Array` (`T_Terrain_BC`, base color), `Normals_Array`
  (`T_Terrain_N`, tangent normals), `Specular_Array` (`T_Terrain_S`, linear;
  R=smoothness, G=reflectance, A=emissive).
- Current slices: 37 (0–36). Budget: 128. No bedrock/water slice (bedrock ID 25
  → stone slice 35; water ID 26 → glass slice 23).
- New slices for ruin variants (12 planned) and new block types. Never renumber
  existing slices. Every new slice needs a matching `_n` normal and `_s`
  specular map.
- `UV1.X` encodes slice index (float representing int). `VertexColor.R` mirrors
  it (legacy). Both must agree.
- Base color array: `TC_DEFAULT` (sRGB), Nearest, mipmaps ON. Normal array:
  `TC_NORMALMAP`, Normal sampler, mipmaps ON. Specular array: `TC_DEFAULT`
  (Linear), Linear sampler, mipmaps ON.

---

## 16. Performance budgets (visual)

Source: `Docs/technical-contract.md` §8.

- Target: 60 FPS (16.6 ms frame). Gameplay systems < 4 ms/frame.
- Game thread streaming < 5 ms; worst frame < 10 ms. Upload per frame < 2 MB.
- Visible draw calls < 800 (baseline ~1200–1600).
- Fill time RD8 < 2 s, no hitch > 16 ms.
- Chunk store RAM: RD8/64h < 60 MB; RD8/192h < 140 MB.
- Mob animation: 20+ mobs inside frame budget via `UAnimBudgetAllocatorComponent`
  + significance manager.
- Profiling: Unreal Insights, `stat unit`, `stat game`, `stat gpu`, `stat memory`,
  `stat lumen`, `stat scenerendering`, `stat rhi`, `stat streaming`,
  `stat physics`, `stat gc`, `stat virtualshadowmaps`. Never rely on FPS alone.

---

## 17. Blocks and terrain (visual)

- 12 "ruin" variants (cracked/mossy/sooted/scorched) of the most-seen blocks →
  new slices in Texture2DArray.
- Deferred decals: moss, soot, ash, blood, burn marks, torn posters.
- Vegetation desaturated to olive/sick. Overgrowth props blocked until
  non-solid mesh path exists.
- Erosion reads through material + decal + fog, never through silhouette.

---

## 18. Verification

- For UI changes: screenshot before/after, or precise visual verification note.
- For performance changes: Unreal Insights captures or `stat` measurements
  before/after.
- For new textures: validate in PIE against `M_Terrain_Opaque` / `M_Mob` /
  `M_Item_PBR` before expanding.
- Every new gameplay rule needs an automation test
  (`MinecraftDeWish.<System>.<Behavior>`).

---

## 19. Creative Director confirmation (2026-11-11)

The image-analyst was re-invoked by the Creative Director to re-verify the
6 mood-board captures. Results confirm the statuses documented in
`Docs/art/art-bible.md` §18.3:

- **A1 (Angel silhouette at 60 blocks):** BLOCKED — no capture contains an Angel.
  Core acceptance criterion (angels.md §8.5) unvalidated.
- **A2 (sunset/sky palette):** PASS — sky reads as bone-ash desaturated grey-green
  (~#8E9A90–#B5BAA8); no vivid orange. LUT keeps the world desaturated.
- **A3 (torch dominance ratio):** PENDING — no sodium torch in any frame; target
  torch peak ≥ 2× sky luminance. Sodium pillar currently unproven visually.
- **A4 (fog knee):** PENDING — fog kills readability at ~10–15 blocks (vs 60–90
  target). Tune exponent/density, not colour; keep within Ash (#5C6268).
- **A5 (cave void levels):** PENDING — `000917` is an outdoor forest, not a cave.
  Unlit areas must read as Void (#0B0E11); clamp GI if over-lit.
- **A6 (early-reference hygiene):** PASS — `000820` (2026-05-23) confirmed
  pre-overhaul; excluded from P2 reference set.

**Reference set frozen for P2:** `001008` fully approved; `000733`/`000817`/`000850`
conditionally approved (subject to A3/A4/block boundaries); `000820`/`000917` excluded.

**P2 must start with P2.0** (resolve A1/A3/A4/A5) before any asset work.

*Last updated: 2026-11-11. Owner: Technical Artist (under Art Director). Creative
Director finalisation pass recorded in `Docs/art/art-bible.md` §18.*
