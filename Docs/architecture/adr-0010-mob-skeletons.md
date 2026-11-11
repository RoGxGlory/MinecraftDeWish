# ADR-0010 — Mob skeletons and animation sharing

**Status:** Accepted
**Date:** 2026-11-10
**Deciders:** Technical Director, Technical Artist
**References:** ADR-0008 (Blender pipeline), ADR-0009 (mob behavior)

## Context

The overhaul needs four visible mobs with animations. Authoring a unique skeleton
and animation set per mob would multiply production cost and break the budget.
The project already uses a single humanoid template (mannequin) for the player.

## Decision

1. **One shared skeleton `SK_Mob_Base`** for all humanoid mobs (Zombie,
   Skeleton, and future humanoid mobs). Six bones, rigid weights (100% per part).
   The Spider uses a separate skeleton (8 legs).
2. **Animation sharing** via the shared skeleton and retargeting. One locomotion
   set (idle, walk, run, attack, hit react, death) covers all humanoid mobs;
   per-mob differences are animation montages and parameters, not skeletons.
3. **Budget:** `UAnimBudgetAllocatorComponent` + significance manager. Mobs
   beyond a distance threshold reduce tick rate and LOD.
4. **Texture resolution:** 128x128 for mob textures (Patrix entity tier), with
   Nearest filter and mipmaps. Normal and specular maps included (LabPBR).
5. **Rigging:** rigid weights only (no soft skinning) to keep the skeletal mesh
   simple and the animation budget low. Blend shapes only if a mob needs facial
   expression (none planned for v1).

## Consequences

- **Positive:** 4 mobs share 1 skeleton and 1 locomotion set; production cost
  is divided by 4; retargeting is trivial.
- **Negative:** all humanoid mobs have the same proportions (acceptable for
  the low-poly stylized style, D8=Option 1).
- **Risk:** if a mob needs a unique silhouette (e.g. a hunchback), it may
  require a skeleton variant. Keep the base skeleton as the common denominator.

## Alternatives considered

- **Unique skeleton per mob:** rejected. 4x the authoring cost, no visual gain
  at the chosen style.
- **Vertex animation:** rejected. No blend shapes, no morph targets; skeletal
  is the standard and supports gameplay-driven animation.