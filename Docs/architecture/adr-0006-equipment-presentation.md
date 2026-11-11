# ADR-0006 — Equipment presentation: static-mesh tools on sockets, skeletal arms, montage contract

**Status:** Accepted
**Date:** 2026-11-10
**Deciders:** Technical Director, Technical Artist
**References:** Docs/SYSTEMS_REFERENCE.md (§6 Baubles), ADR-0005

## Context

The overhaul needs first-person tools and weapons with production-quality
feedback. Today `FToolInstance` carries only a 2D icon; there is no viewmodel,
no animation, no equip/unequip flow. The visual identity is Minecraft-like
silhouettes with post-apocalyptic detailing (D13=B), so heavy skeletal rigs per
tool are wasteful.

## Decision

1. **Tools and weapons are StaticMesh, not SkeletalMesh.** No deformation, no
   skinning cost. Pivot at the grip centre, handle along local +Z. Animation
   lives on the arms, not the tool.
2. **One `AEquippedToolActor`** with a `UStaticMeshComponent` whose mesh is
   swapped on equip. Never spawn/destroy per tool: keeps animation state warm and
   avoids equip hitches.
3. **First-person arms** are a dedicated `USkeletalMeshComponent` with
   `bOnlyOwnerSee`, attached to the camera. The tool attaches to socket
   `weapon_r`.
4. **Montages are per tool class**, not per tier. Sections Windup/Impact/Recovery.
   Notifies: `AnimNotify_ActionWindow` (damage window), `AnimNotify_PlayCue`
   (VFX/SFX at the correct frame), `AnimNotify_ActionComplete`,
   `AnimNotifyState_MiningLoop`.
5. **Layered blend** on `spine_01` via `LayeredBlendPerBone` so locomotion
   continues under a swing. Full-body first, layered second.
6. **Mining cadence** is driven by the impact notify, not by the BP timer.
   `QueueBlockBreakAtVoxel` is called from the notify so the crack overlay
   (ADR-0003) and durability consumption cannot desync from the visual.
7. **Auto-tool selection** emits `OnActiveToolChanged(EToolType)` when the
   targeted block category changes, with a 150-200 ms anti-bounce to avoid
   flicker at block boundaries. The exact timing is a game-feel decision for
   the game designer.

## Consequences

- **Positive:** 25 meshes stay cheap (one draw call each, no skinning); equip
  is instant; animation set is 5 montages, not 25.
- **Negative:** tools cannot deform (acceptable for the chosen silhouette
  style); tier differences are mesh/material/value only.
- **Risk:** notify-driven mining must be validated against the existing
  `Progress01`/`GetBlockBreakTimeAtVoxel` path (ADR-0003) to avoid double
  durability consumption. Integration test required.

## Alternatives considered

- **SkeletalMesh per tool:** rejected. Deformation cost and authoring burden
  for no visual gain at the chosen style.
- **Spawn/destroy tool actors:** rejected. Equip hitches and state loss.