# ADR-0004 — Rendering policy: ray tracing, Nanite scope, Substrate

**Status:** Accepted
**Date:** 2026-11-10
**Deciders:** Technical Director, with Creative Director and Technical Artist
**References:** ADR-0001 (Lumen mandatory), Docs/architecture/voxel-performance-plan.md

## Context

ADR-0001 made Lumen mandatory everywhere. The visual overhaul (D1=B) requires a
second, equally binding decision: the scope of ray tracing, Nanite and Substrate.
The terrain is generated at runtime into `UProceduralMeshComponent` (never Nanite),
and `M_Global` is a Masked material sampling four `Texture2DArray`s per pixel.
The current `DefaultEngine.ini` enables `r.RayTracing=True`,
`RayTracingProxies.ProjectEnabled=True` and `r.Substrate=True` with no documented
rationale. On a runtime-remeshed world, ray tracing rebuilds bottom-level
acceleration structures on every mesh update, which is the single worst
cost/effort ratio in the project.

## Decision

1. **Ray tracing is OFF by default.** `r.RayTracing=False` on the default
   scalability preset. An opt-in "Ultra" preset may enable it for users who want
   hardware RT Lumen, at their own performance cost. Ray tracing is independent of
   Lumen (ADR-0001) and is never required for the visual identity.
2. **Nanite is ON for static props and statues only.** Terrain (procedural mesh)
   and skeletal meshes (mobs, first-person arms) stay on classic LODs. Nanite
   is used where geometry is stable and high-triangle: the Weeping Angel statue,
   props above ~20k triangles, structures. Nanite materials must be Opaque,
   TwoSided-free, and free of WPO and Pixel Depth Offset.
3. **Substrate stays in legacy authoring mode.** Materials are authored with the
   classic pins (BaseColor/Normal/Roughness/Metallic/Emissive). Substrate slabs
   are reserved for the Weeping Angel only, if a dry/wet/moss multi-layer is
   desired. Rationale: the terrain material samples three arrays per pixel; legacy
   authoring gives predictable permutations and faster iteration, and no current
   system depends on Substrate slabs.
4. **VSM shadows are disabled on small dynamic objects.** Torches, debris,
   pickups, XP orbs and the crack overlay cast no dynamic shadow. Only the sun,
   the player and mobs cast shadows.

## Consequences

- **Positive:** removes permanent BLAS rebuilds on the game thread's render
  path; frees base-pass budget for the overhaul; keeps the material pipeline
  simple enough for a small team.
- **Negative:** no RT reflections on the default preset; Nanite cannot help the
  terrain (acceptable — the terrain's cost is addressed by the proxy-scene work in
  voxel-performance-plan lot 3).
- **Risk:** if a future art direction requires RT reflections, the opt-in preset
  must be validated against the streaming mesh updates.

## Alternatives considered

- **Nanite on the terrain:** rejected. Runtime `CreateMeshSection` does not
  produce a Nanite resource, and the runtime build path is not production-grade
  in UE 5.8. The terrain's answer is proxy scene + LOD rings (plan lot 3-4).
- **Substrate everywhere:** rejected for now. No system needs slabs; the cost in
  permutation count and iteration speed is not justified. Revisit when the Angel
  material is authored.
- **Ray tracing on by default:** rejected. Conflicts with the 1060 performance
  target (D3=A) and with runtime mesh updates.