# CC0 / Public-Domain Texture Pack Research — LAMPLIGHTER

**Date:** 2026-11-10
**Owner:** Technical Artist
**Purpose:** Identify CC0 / public-domain texture packs that can serve as
**reference and inspiration** for the original art of LAMPLIGHTER (voxel survival
horror, post-apocalyptic). We cannot purchase Patrix, so we are looking for
free, legally clean starting points that we can study, then re-paint/re-author
into our own original art.

**Constraint reminder (from project rules):** no third-party asset may be shipped
verbatim unless its licence permits it and an ADR records the decision. All
gameplay-facing art must be original or properly licensed. This document is a
*research* artifact, not a licence to ship.

---

## 1. Search criteria

| Criterion | Requirement |
|---|---|
| Licence | CC0 (Public Domain) preferred; CC-BY acceptable if attribution tracked; no NC/ND for shippable assets |
| Style | PBR (base color + normal + roughness/specular) **or** pixel art **or** low-poly |
| Resolution | ≥ 64×64 (project uses 128×128 for blocks/mobs, Texture2DArray slices) |
| Coverage | Blocks + items + entities (full coverage ideal, partial acceptable for reference) |
| Theme | Post-apocalyptic horror (rusted metal, decay, concrete, ash, blood, grime) |
| Usability | Downloadable, well-organised, documented |

---

## 2. Summary table

| # | Pack | Source | Licence | Resolution | Coverage | Verdict |
|---|---|---|---|---|---|---|
| 1 | Kenney Voxel Pack | kenney.nl | CC0 | 128×128 | Blocks + items + characters | Inspiration only (style mismatch) |
| 2 | Screaming Brain — Horror Texture Pack | itch.io | CC0 | 128/256/512 | Surfaces only (no items/entities) | Usable for surfaces, inspiration for horror palette |
| 3 | Cole Foster — Voxel Crypt Sprites & Tiles | itch.io | CC0 | 32×32 tiles (creatures up to 110px) | Tiles + creatures + props | Inspiration only (2D top-down, below 64px threshold) |
| 4 | Cole Foster — Voxel Dungeon Sprites & Tiles | itch.io | CC0 | 32×32 tiles (creatures up to 144px) | Tiles + creatures + items + FX + UI | Inspiration only (2D top-down, below 64px threshold) |
| 5 | ambientCG | ambientcg.com | CC0 | 2K–8K PBR | Surfaces only | Reference for PBR material authoring |
| 6 | Poly Haven | polyhaven.com | CC0 | 1K–8K PBR | Surfaces only | Reference for PBR material authoring |
| 7 | Golgotha Textures | opengameart.org | CC0 | 512×512 | Surfaces only | Inspiration (industrial/rust/metal) |
| 8 | monogon — Voxel Nuclear Survival | itch.io | CC BY-ND 4.0 (paid) | Voxel (source + optimized) | Blocks + props + environment | Not usable (paid, NoDerivatives) — style reference only |

---

## 3. Detailed entries

### 1. Kenney Voxel Pack

- **Source:** https://kenney.nl/assets/voxel-pack
- **Licence:** Creative Commons CC0 (Public Domain)
- **Resolution:** 128×128
- **File count:** 190 files (PNG, SVG vector sources)
- **Coverage:** Textures, items and characters for 2D + 3D sandbox survival games.
  Includes skybox, particle effects, sun/moon. Tagged `voxel`, `tile`, `items`,
  `character`.
- **Relevance:** This is the closest CC0 pack to a "full voxel game" coverage
  (blocks + items + entities) at a resolution ≥ 64×64. It is a well-organised,
  production-ready reference for how a voxel texture atlas is structured
  (128×128 slices, consistent palette, SVG sources for re-scaling).
- **Verdict:** **Inspiration only.** The art style is bright, cartoonish and
  clean — the opposite of LAMPLIGHTER's post-apocalyptic horror tone. Use it as
  a *structural* reference (atlas layout, naming, slice indexing) and as a
  fallback for prototyping, never as final art. Do not ship Kenney textures
  verbatim in a horror game; the tonal clash would be jarring.

### 2. Screaming Brain Studios — Horror Texture Pack

- **Source:** https://screamingbrainstudios.itch.io/horror-texture-pack
- **Licence:** Creative Commons Zero v1.0 Universal (CC0)
- **Resolution:** 512×512, 256×256 and 128×128 (three sizes shipped)
- **File count:** 300 textures (100 unique × 3 sizes)
- **Coverage:** Surfaces only — no items, no entities, no UI. Categories:
  Horror Brick (14), Horror Floor (14), Horror Metal (14), Horror Misc (15),
  Horror Stain (15, transparent overlays for dirt/grime/blood), Horror Stone
  (14), Horror Wall (14). Tags include `low-poly`, `ps1`, `silent-hill`.
- **Relevance:** Directly on-theme. Rusted metal, decaying walls, grated
  floors, crumbling stone, blood/dirt stain overlays. The stain overlay category
  is a strong reference for our `M_BlockCrack` / damage overlays and for the
  grime passes on concrete/stone. Resolution ≥ 128×128 matches our block slice
  size. PS1 aesthetic (nearest filtering, low detail) is a plausible
  stylistic lane for a horror voxel game.
- **Verdict:** **Usable for surfaces + strong inspiration.** CC0, so these
  textures *can* legally be used directly as base-color inputs, but the PS1
  flat-shaded look (no normal/specular maps) will not match our PBR material
  stack (`M_Terrain_Opaque` expects base color + normal + specular arrays).
  Recommended use: study the colour palette and grunge patterns, then
  re-author our own PBR variants (add normal + roughness + emissive maps).
  The stain overlays can be adapted almost directly.

### 3. Cole Foster — Voxel Crypt Sprites & Tiles (CC0)

- **Source:** https://colefoster.itch.io/voxel-crypt-pack
- **Licence:** CC0 (free, no credit required)
- **Resolution:** 32×32 tiles; creatures 48–110 px cells; 283 tiles total
- **File count:** 24 animated undead creatures (mummies, skulls, bone hounds,
  wraiths, bats, rats, spiders, necromancers), 283 crypt tiles, 5 props
  (pulsing altar, flickering brazier, iron door, pillar, bone pile), Tiled +
  Godot 4 files, JSON atlas.
- **Coverage:** Tiles + creatures + props. **No items, no effects, no UI**
  (those are in the sibling "Voxel Dungeon" pack).
- **Relevance:** Highest thematic match for the *horror* half of our game
  (undead, crypts, spectral water, cursed ichor, spike traps). Excellent
  reference for pixel-art horror creature silhouettes, animation timing
  (4–5 animations per creature, 3 facings) and autotile rules (47-tile sets).
- **Verdict:** **Inspiration only.** Resolution is 32×32, below our ≥ 64×64
  floor and far below our 128×128 block slice target. It is also a 2D
  top-down pixel-art pack, not a 3D voxel texture set. Use it as a *design*
  reference for creature concepts, animation counts and autotile logic — not as
  a texture source. Also note: 12 of the 24 creatures are colour variants.

### 4. Cole Foster — Voxel Dungeon Sprites & Tiles (CC0)

- **Source:** https://colefoster.itch.io/voxel-dungeon-pack
- **Licence:** CC0
- **Resolution:** 32×32 tiles; creatures mostly 64 px cells (stone golem 144 px)
- **File count:** 28 animated creatures, 283 tiles, 19 props, 13 loot pickups,
  31 item icons, 17 effects, UI kit (panels, buttons, bars, slots, cursors).
  Tiled + Godot 4 files.
- **Coverage:** Tiles + creatures + items + effects + UI. The most complete
  single CC0 voxel pack found, but still 2D top-down pixel art.
- **Relevance:** The **item icons** (31) and **UI kit** are the most
  transferable part — they demonstrate a consistent icon language and a
  nine-slice panel system that maps well onto our `WBP_*` / `M_UI_*` materials.
  The loot/effect concepts (spells, explosions, healing) are a useful
  reference for our VFX categories (`M_VFX_Dust`, `M_VFX_Chip`).
- **Verdict:** **Inspiration only.** Same resolution ceiling (32×32) and 2D
  top-down limitation as the Crypt pack. Strong reference for *iconography*
  and *UI layout*, not for 3D voxel textures. The stone-golem (144 px) is the
  only asset that approaches our resolution floor.

### 5. ambientCG

- **Source:** https://ambientcg.com
- **Licence:** CC0 (Public Domain)
- **Resolution:** 1K–8K (PNG/JPG/EXR), full PBR map sets (color, normal,
  roughness, displacement, AO)
- **File count:** 2000+ materials
- **Coverage:** Surfaces only (concrete, brick, metal, wood, fabric, ground,
  gravel, grass, etc.). No items, no entities, no voxel-specific content.
- **Relevance:** This is the canonical CC0 PBR reference library. Materials
  like `Concrete034`, `Bricks104`, `Gravel043`, `Ground111` map directly onto
  our terrain block families (stone, dirt, gravel, concrete ruins). The full
  PBR map sets (color + normal + roughness + displacement) match our material
  stack (`M_Terrain_Opaque` / `M_Terrain_Masked` expect base color + normal +
  specular arrays).
- **Verdict:** **Reference for PBR material authoring.** Not horror-themed and
  not voxel-specific, but indispensable as a *technical* reference for how to
  build PBR materials and how to author normal/roughness maps. Use the
  concrete/brick/metal/gravel entries as lighting/roughness calibration targets
  when we paint our own original textures. Do not use ambientCG textures as
  final game art without heavy re-authoring — they are photographic and would
  clash with a stylised voxel look.

### 6. Poly Haven

- **Source:** https://polyhaven.com/textures
- **Licence:** CC0
- **Resolution:** 1K–8K (PNG/JPG/EXR)
- **File count:** Hundreds of PBR texture sets
- **Coverage:** Surfaces only. Notable entries: `Aerial Grass Rock`,
  `Blue Metal Plate`, `Brushed Concrete`, `Dark Paneled Wood`, `Denim Fabric`,
  plus HDRIs (e.g. `Abandoned Bakery`) and props (e.g. `Barrel 01`).
- **Relevance:** Same role as ambientCG — CC0 PBR reference. The `Abandoned
  Bakery` HDRI is a strong lighting reference for our post-apo interiors.
  `Blue Metal Plate` and `Brushed Concrete` are good calibration targets for
  rusted-metal and concrete block families. The photoscanned `Barrel 01` prop
  is a useful reference for our debris/prop pipeline.
- **Verdict:** **Reference for PBR material authoring.** Same caveats as
  ambientCG: photographic, not stylised. Use for calibration and lighting
  reference, not as final voxel art.

### 7. Golgotha Textures (OpenGameArt)

- **Source:** https://opengameart.org/content/golgotha-textures
- **Licence:** CC0 (Public Domain)
- **Resolution:** 512×512 (per-file; e.g. `factorylargerocksplain.jpg` is
  512×512)
- **File count:** Large set (factory, iron, stone, wood, vehicles, windows,
  etc. — hundreds of individual textures)
- **Coverage:** Surfaces only. Notable sub-packs: "Golgotha Effects Textures"
  (includes `evil6_color.jpg`), "Dark Camouflage Golgotha Textures".
- **Relevance:** Older, lower-fidelity texture set, but includes industrial
  and rusted-metal surfaces that are on-theme for post-apo. The "Effects
  Textures" sub-pack (explosions, energy, etc.) is a reference for our VFX
  sprite sheets. 512×512 ≥ 64×64 floor; can be downscaled to 128×128.
- **Verdict:** **Inspiration (industrial/rust/metal).** Dated look and no PBR
  maps, but the rust/iron/stone palette and the effects-textures sub-pack are
  worth studying. Not a source for final art.

### 8. monogon — Voxel Nuclear Survival

- **Source:** https://maxparata.itch.io/nuclearsurvival
- **Licence:** CC BY-ND 4.0 (Creative Commons Attribution, NoDerivatives) —
  **paid** (40 € source pack, 59.95 € optimized pack; also sold in a 49-pack
  bundle for 389.50 €)
- **Resolution:** Voxel source (`.vox`) + optimized exports (`.blend`, `.fbx`,
  Unity package). 200+ items: architecture (60), decoration (30),
  environment (25), vegetation (15), vehicles (10), varied objects (60).
- **Coverage:** Blocks + props + environment + vegetation + vehicles. The most
  complete post-apocalyptic voxel pack found, but **not CC0** and **not free**.
- **Relevance:** This is the closest thing to a "Patrix for post-apo voxel"
  that exists. The visual style (low-poly voxel, post-apo palette, rusted
  vehicles, ruined architecture) is exactly the lane LAMPLIGHTER targets.
- **Verdict:** **Not usable (paid + NoDerivatives).** Included here only as a
  *visual style reference* and to document that no free CC0 equivalent exists.
  We cannot buy it (per project constraint) and the NoDerivatives clause would
  forbid re-authoring anyway. Use the screenshots on the itch.io page as
  mood-board reference only. Do **not** download or redistribute.

---

## 4. Honest assessment

**No single CC0 pack satisfies all of our criteria.** Specifically:

- **Coverage gap:** The only CC0 packs with full blocks+items+entities coverage
  (Kenney Voxel Pack) are cartoon/clean in style. The only CC0 packs with a
  horror tone (Screaming Brain Horror Textures, Cole Foster Crypt/Dungeon) are
  either flat-shaded PS1 surfaces with no PBR maps, or 2D top-down pixel art
  below our 64×64 resolution floor.
- **PBR gap:** No CC0 pack combines (a) a post-apo horror palette, (b) PBR
  maps (normal + roughness/specular), and (c) voxel block/item/entity
  coverage. ambientCG and Poly Haven provide PBR maps but are photographic and
  generic, not horror/voxel.
- **Resolution gap:** Cole Foster's packs (the best horror voxel pixel art
  found) are 32×32, half our 64×64 floor and a quarter of our 128×128 block
  slice target.

**Conclusion:** We will not find a drop-in CC0 replacement for Patrix. The
realistic plan is:

1. **Study** the Kenney Voxel Pack for atlas structure and slice indexing
   (128×128, 190 files, consistent palette).
2. **Study** the Screaming Brain Horror Texture Pack for colour palette,
   grunge patterns and stain-overlay technique (128/256/512, CC0).
3. **Study** the Cole Foster Crypt/Dungeon packs for creature silhouettes,
   animation counts (4–5 anims, 3 facings) and autotile rules (47-tile sets).
4. **Calibrate** PBR authoring against ambientCG / Poly Haven (concrete, brick,
   metal, gravel) so our normal/roughness maps read correctly in-engine.
5. **Re-author** every final texture ourselves, in our own original style,
   using the above as reference only. No CC0 texture ships verbatim unless an
   ADR is filed and the art-director signs off.

---

## 5. Recommended next steps

1. **Art-director review:** Confirm the target style lane (PS1 flat-shaded
   horror vs. PBR voxel horror) — this decides whether the Screaming Brain
   pack's flat look is a feature or a mismatch.
2. **Download the reference packs** (Kenney, Screaming Brain, Cole Foster,
   ambientCG, Poly Haven) into `Content/Reference/CC0/` (gitignored) for
   offline study. Do **not** commit them to the repo.
3. **Create an ADR** in `Docs/architecture/` documenting the "reference only,
   re-author everything" policy before any texture is imported into
   `Content/`.
4. **Author a style guide** (`Docs/art/style-guide.md`) capturing the palette,
   grunge level, and PBR value ranges derived from the reference study.
5. **Prototype** 4–6 block textures (stone, dirt, rusted metal, cracked
   concrete, ash, gravel) at 128×128 with full PBR maps, validate in PIE
   against `M_Terrain_Opaque`, then expand.

---

## 6. Sources

- Kenney Voxel Pack — https://kenney.nl/assets/voxel-pack
- Screaming Brain Studios Horror Texture Pack — https://screamingbrainstudios.itch.io/horror-texture-pack
- Cole Foster Voxel Crypt Sprites & Tiles — https://colefoster.itch.io/voxel-crypt-pack
- Cole Foster Voxel Dungeon Sprites & Tiles — https://colefoster.itch.io/voxel-dungeon-pack
- ambientCG — https://ambientcg.com
- Poly Haven — https://polyhaven.com/textures
- Golgotha Textures — https://opengameart.org/content/golgotha-textures
- monogon Voxel Nuclear Survival — https://maxparata.itch.io/nuclearsurvival
- AssetHoard CC0 catalogue — https://assethoard.com/free-game-assets
- OpenGameArt CC0 resources — https://opengameart.org/content/cc0-resources
- Awesome CC0 list — https://github.com/madjin/awesome-cc0
