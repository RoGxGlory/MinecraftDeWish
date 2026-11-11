# Asset Provenance Registry — MinecraftDeWish (LAMPLIGHTER)

**Owner:** Technical Artist
**Date:** 2026-11-10
**Status:** Living document — updated whenever a reference pack is added or an
original asset is authored.
**Scope:** Every third-party reference pack, downloaded asset, and original
gameplay-facing asset used by the project. Gameplay-facing art must be original
or properly licensed; no third-party asset ships verbatim without an ADR and
art-director sign-off (see `Docs/art/cc0-research.md` §4).

**Storage rule:** All reference packs live under `Content/Reference/CC0/`,
which is **gitignored** (see `.gitignore`). They are never committed and never
imported into `Content/` directly. They exist for offline study only.

---

## 1. Policy

1. **Reference only.** CC0 packs are downloaded for study (palette, grunge
   patterns, atlas structure, PBR calibration). No CC0 texture ships verbatim
   unless an ADR is filed and the art-director signs off.
2. **Re-author everything.** Every final gameplay texture is painted/authoring
   in our own original style. CC0 sources are inspiration, not ingredients.
3. **No Patrix.** Patrix is not purchased and not redistributed (D2). The
   project uses original art; Patrix-style recognition is preserved through
   block *silhouettes*, not through Patrix pixels.
4. **Provenance is mandatory.** Any asset imported into `Content/` must have a
   row in §3 (original) or §4 (licensed) of this document, with source, licence,
   date, and intended usage. Orphaned assets (no row) are a release blocker.
5. **Stable IDs.** Block/mob/tool slice indices and asset IDs are never
   renumbered (save data may reference them). See `Docs/technical-contract.md`
   §7.2 and §9.3.
6. **No debug/test rows** in production DataTables. Use a separate test asset.

---

## 2. Downloaded reference packs (Content/Reference/CC0/)

Downloaded 2026-11-10. All licences verified against the source page at download
time. Packs are stored unmodified in their original archive plus an extracted
folder where applicable.

| # | Pack | Folder / archive | Source | Licence | Resolution | Coverage | Intended use |
|---|---|---|---|---|---|---|---|
| 1 | Kenney Voxel Pack | `kenney-voxel-pack.zip` + `kenney-voxel-pack/` | https://kenney.nl/assets/voxel-pack (mirror: OpenGameArt `voxel-pack-updated.zip`) | CC0 1.0 | 128×128 | Blocks + items + characters + skybox + particles | **Structural reference only** — atlas layout, slice indexing, naming, consistent palette. Style is cartoon/clean; do NOT ship verbatim in a horror game. |
| 2 | Screaming Brain — Horror Texture Pack | `screaming-brain-horror-128.zip` + `screaming-brain-horror/` (128×128) | https://screamingbrainstudios.itch.io/horror-texture-pack (mirror: OpenGameArt `sbs_-_horror_texture_pack_128x128.zip`) | CC0 1.0 | 128/256/512 | Surfaces only (brick, floor, metal, misc, stain, stone, wall) | **Palette + grunge + stain-overlay reference.** Study colour palette, grunge patterns and the transparent stain-overlay technique (`M_BlockCrack`, damage overlays, grime passes). Re-author own PBR variants (add normal + roughness + emissive). Stain overlays adaptable almost directly. |
| 3 | Cole Foster — Voxel Crypt Sprites & Tiles | `cole-foster-voxel-crypt.zip` | https://colefoster.itch.io/voxel-crypt-pack | CC0 1.0 | 32×32 tiles; creatures 48–110 px | Tiles + creatures + props | **Design reference only** (resolution below our 64×64 floor). Creature silhouettes, animation counts (4–5 anims, 3 facings), autotile rules (47-tile sets), spectral water/ichor palette. |
| 4 | Cole Foster — Voxel Dungeon Sprites & Tiles | `cole-foster-voxel-dungeon.zip` | https://colefoster.itch.io/voxel-dungeon-pack | CC0 1.0 | 32×32 tiles; creatures up to 144 px | Tiles + creatures + items + effects + UI | **Iconography + UI layout reference.** 31 item icons, nine-slice panel system, loot/effect concepts for `M_VFX_*`. Stone golem (144 px) is the only asset approaching our resolution floor. |
| 5 | ambientCG — representative PBR sets | `ambientcg/*.zip` (Concrete034, Bricks104, MetalPlates001, Gravel043, Ground111, Rust001) | https://ambientcg.com | CC0 1.0 | 2K (JPG/PNG/EXR) | Surfaces only | **PBR calibration reference.** Full PBR map sets (color, normal, roughness, displacement, AO) for calibrating our normal/roughness authoring. Photographic; not final voxel art. |
| 6 | Poly Haven — representative PBR surfaces | `polyhaven/*.jpg` (brushed_concrete, blue_metal_plate — diff/nor_gl/rough at 2K) | https://polyhaven.com/textures | CC0 1.0 | 2K (JPG) | Surfaces only | **PBR calibration + lighting reference.** `Abandoned Bakery` HDRI is a strong lighting reference for post-apo interiors. Blue Metal Plate / Brushed Concrete are rusted-metal and concrete calibration targets. |
| 7 | Golgotha Textures | `golgotha-useful.zip` | https://opengameart.org/content/golgotha-textures | CC0 1.0 | 512×512 | Surfaces only (factory, iron, stone, wood, vehicles, windows, effects) | **Palette inspiration (industrial/rust/metal).** Dated look, no PBR maps. Rust/iron/stone palette and the effects-textures sub-pack (explosions, energy) worth studying. |

### Not downloaded (documented for completeness)

| Pack | Source | Licence | Reason not downloaded |
|---|---|---|---|
| monogon — Voxel Nuclear Survival | https://maxparata.itch.io/nuclearsurvival | CC BY-ND 4.0 (paid, 40–59.95 €) | **Not usable** — paid + NoDerivatives. Included in `cc0-research.md` only as a visual style reference (screenshots on the itch.io page). Do not download or redistribute. |

---

## 3. Original assets (authored in-house)

To be populated as assets are authored. Every row must include the asset name,
type, resolution, material, author, date, and the design/registry reference.

| Asset | Type | Resolution | Material | Author | Date | Reference |
|---|---|---|---|---|---|---|
| *(placeholder)* | — | — | — | — | — | — |

**Planned priority assets** (from `design/art/art-direction.md` §11, in order):
1. Grade + post-process + LUT + sky/fog + light presets
2. `Lvl_MainMenu` rebuilt in voxels + unified menu materials
3. UI kit: `WBP_Common_*`, Title, Pause, Settings + HUD restyle
4. First-person hands + 5 tool classes (wood tier first) + 5 swings
5. Angel: 8 poses + statue readability pass + material
6. Vanilla mobs: shared rig → Zombie + Skeleton → Creeper → Spider
7. 12 ruin variants + decal kit + overgrowth props
8. 5 loot props (flesh, bone, string, eye, arrow)
9. VFX/SFX-visual pass (dust by material, creeper sodium flash)
10. POI prop kits (hospital, police station, village)

**Texture slice budget:** current 37 slices (0–36), budget 128. New slices added
for the 12 ruin variants and new block types. Never renumber existing slices
(save data may reference them). Every new slice needs a matching `_n` normal
and `_s` specular map (`Docs/technical-contract.md` §7).

---

## 4. Licensed (non-CC0) assets

None currently. Any future licensed asset requires an ADR in `Docs/architecture/`
and an entry here with licence terms, attribution requirements, and shipping
restrictions.

---

## 5. Naming and storage conventions (asset pipeline)

- Blueprint actors: `BP_<Name>`; widgets: `WBP_<Name>`; data assets: `DA_<Name>`;
  data tables: `DT_<Name>`; materials: `M_<Name>`; instances: `MI_<Name>`;
  textures: `T_<Name>`; static meshes: `S_<Name>`; skeletal meshes: `SK_<Name>`;
  animations: `A_<Name>`; animation blueprints: `ABP_<Name>`; Niagara: `NS_<Name>`.
- No spaces, no version suffixes, no `_New` / `_Final`. ASCII alphanumeric +
  underscores only.
- Auto-generate collision is OFF for all imported meshes; collision authored
  explicitly (`Docs/technical-contract.md` §5).
- Blender export: Forward `-Z`, Up `Y`, `Apply Transform` ON; 1 unit = 1 m;
  FBX import scale 1.0 (`Docs/technical-contract.md` §6).
- Texture compression: `TC_DEFAULT` (sRGB) for base color/specular, `TC_NORMALMAP`
  for normals. Nearest filter mandatory for pixel-art textures; mipmaps ON.

---

## 6. Verification checklist (before any asset enters Content/)

- [ ] Provenance row exists in §3 or §4 of this document.
- [ ] Licence permits the intended use (commercial, modified, redistributed).
- [ ] ADR filed if the asset is third-party and ships verbatim.
- [ ] Art-director sign-off recorded.
- [ ] Naming follows §5.
- [ ] Collision authored explicitly (no auto-generate).
- [ ] Texture resolution matches `Docs/technical-contract.md` §6.4.
- [ ] Normal + specular maps present for terrain slices.
- [ ] Automation test or PIE smoke test updated where applicable.

---

*Last updated: 2026-11-10. Owner: Technical Artist.*
