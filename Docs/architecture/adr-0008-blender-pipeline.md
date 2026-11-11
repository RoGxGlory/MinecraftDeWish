# ADR-0008 — Blender to Unreal asset pipeline

**Status:** Accepted
**Date:** 2026-11-10
**Deciders:** Technical Director, Technical Artist
**References:** .opencode/docs/technical-preferences.md, Docs/SYSTEMS_REFERENCE.md §13

## Context

The overhaul requires a large volume of original 3D assets (mobs, tools,
props, Angel poses). Blender 5.2.3 LTS is available via the `blender-mcp`
server (single live instance, procedural generation disabled, no Poly Haven /
Sketchfab). Without a written pipeline, 100+ assets will be reworked
individually. The engine is UE 5.8; the voxel scale is 100 UU = 1 m = 1 block.

## Decision

### Scale and axes
1. Blender works in metres: 1 unit = 1 m. **1 block = 1 m = 100 UU.** FBX
   export uses "Apply Unit" with import scale 1.0. A 1x1x1 cube must arrive as
   100x100x100 UU.
2. Export: Forward `-Z`, Up `Y`, `Apply Transform` ON, so a character faces +X
   in Unreal.

### Pivots
3. Tools: origin at the grip centre, handle along local +Z.
4. Mobs: origin between the feet, on the vertical centre of mass.
5. Props: origin at the logical base (ground contact).

### Naming
6. `SM_` static, `SK_` skeletal, `A_` anim sequence, `M_`/`MI_` materials,
   `ABP_` anim blueprint, `NS_` Niagara. No spaces, no version suffixes, no
   `_New`/`_Final` (per technical-preferences.md).

### Skeletons
7. One shared skeleton `SK_Mob_Base` for all humanoid mobs (Zombie, Skeleton,
   future), 6 bones, rigid weights. Spider gets a separate skeleton (8 legs).
   Shared skeleton allows animation sharing and retargeting.

### Materials
8. New master `M_Item_PBR` (Opaque, non-masked) reusing the LabPBR decoding of
   `M_Global` (R = smoothness, G = reflectance, A = emissive). Textures 32x32
   with Nearest filter and mipmaps ON. `MI_<Master>_<Variant>` naming.

### Collision
9. Auto-generate OFF. `NoCollision` when equipped; simple box for world-dropped
   items. No convex hulls on tools.

### LODs
10. Tools: 1 LOD (200-800 tris). Mobs: LOD0 2-5k / LOD1 ~800 / LOD2 ~200
    with dithered fade. Statues: Nanite (ADR-0004).

### Automation
11. Blender export script + FBX import presets (asset `FbxImportOptions`) +
    validation script (scale, pivot, naming, LOD count, material slots). No
    manual import of more than 5 assets without the preset.

### Texture note
12. Patrix textures are 32x32 pixel art. In first-person view, a 100 UU object
    with a 32x32 texture will blur. Decision (art-director): either mips with
    Nearest + anisotropy, or dedicated 64x64 textures for weapons (negligible
    VRAM, clear visual gain). To be resolved before P3 production.

## Consequences

- **Positive:** one source of truth for all 3D production; no rework from
  scale/axis mistakes; validation catches errors before import.
- **Negative:** upfront cost of writing the export and validation scripts.
- **Risk:** single Blender instance means serialized operations; two agents
  cannot work on the same scene. Commit `.blend` sources + exports or the bus
  factor is 1.

## Alternatives considered

- **Per-asset manual import:** rejected. 100+ assets cannot be imported
  consistently by hand.
- **AI generation for all models:** rejected. Generation is one object at a
  time, 1-3 minutes, may cost credits, and cannot guarantee scale/pivot. Use
  for hero props only, after the pipeline is proven.