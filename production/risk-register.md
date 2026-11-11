# Risk Register — LAMPLIGHTER Visual Overhaul

**Last updated:** 2026-11-10
**Owner:** Producer

## Risks

| # | Risk | Probability | Impact | Mitigation | Owner | Status |
|---|---|---|---|---|---|---|
| R1 | Scope vs solo capacity (6-8 months) | High | Critical | Vertical slice first (forfait A→B); cut list per phase; phase gates; no phase without gate | Producer | Open |
| R2 | IP / licence (Patrix = Minecraft pack) | High | Critical | Art original (D2=A); provenance registry; LAMPLIGHTER branding | Creative Director | Open |
| R3 | Performance regression from art (PBR, Lumen, textures, mobs) | High | High | Budgets per asset class; automated validation; benchmark per sprint; material split | Technical Director | Open |
| R4 | Voxel renderer rewrite (lot 3) conflicts with art | Medium | High | Freeze vertex/material contract in P1; keep art renderer-agnostic | Technical Director | Open |
| R5 | Mobs start from zero (no mesh, no rig, no AnimBP) | Certain | High | Treat as greenfield; 1 reference asset end-to-end before series | Art Director + UE BP Specialist | Open |
| R6 | Blender→UE friction (scale, pivot, FBX, LOD, collision) | Medium | Medium | Pipeline v1 + reference asset in P1; export automation | Technical Artist | Open |
| R7 | Angels at 0% while being the game identity | High | High | Separate track; decide now if in scope (D7=A: visual + prep) | Creative Director | Open |
| R8 | Test/CI debt (8 tests, no CI) | High | Medium | P1 adds CI + tests for touched systems | QA Lead | Open |
| R9 | UE 5.8 > bundled 5.7 reference | Medium | Medium | Verify every API against engine source; unreal-specialist review | Technical Director | Open |
| R10 | Art direction drift | Medium | Medium | Art bible + weekly in-engine captures; creative-director sign-off | Art Director | Open |
| R11 | Bus factor / unversioned assets | Medium | High | .blend + exports committed (LFS); documented pipeline | Producer | Open |
| R12 | CC0 pack search yields nothing usable (D4) | Medium | Medium | Create original art from Patrix as reference; upscale + repaint | Technical Artist | Open |
| R13 | Weather/day-night system adds scope (D12) | Medium | Medium | Scope as P2-P3 deliverable; cut if behind schedule | Producer | Open |
| R14 | Mob low-poly style may not read as horror | Low | Medium | Validate with screenshot test; adjust material/contrast | Creative Director | Open |
| R15 | Decal crack overlay may not work on all block types | Low | Low | Fallback to quads for torches; ADR amendment | Technical Artist | Open |
| R16 | NativeTick on UUserWidget may not fire in all cases | Low | Medium | Verified in PIE (arm moves). Add fallback timer if needed. | Gameplay Programmer | Mitigated |
| R17 | ADR-0007 absent — tools data model not frozen | Medium | High | ADR-0007 created (2026-11-10). DT_Tools migration planned in P1. | Technical Director | Mitigated |
| R18 | World height migration (64 → 192 blocks) | Medium | High | Blocked until lots 1-2 complete (voxel-performance-plan). Cost tripled if done early. | Technical Director | Open |
| R19 | Lumen on GTX 1060 (entry-level GPU) | Medium | High | ADR-0001 maintained. Benchmark in P0 baseline. Scalability presets in P5. | Performance Analyst | Open |
| R20 | Solo dev / human bus factor | Medium | Critical | Vertical slice first. Cut list per phase. Risk register reviewed each sprint. | Producer | Open |

## Risk categories

- **Critical (must mitigate before P1):** R1, R2
- **High (mitigate during P1):** R3, R4, R5, R7, R11
- **Medium (monitor):** R6, R8, R9, R10, R12, R13, R14
- **Low (accept):** R15, R16

## Review cadence

Risk register reviewed at the start of each sprint. New risks added as discovered.