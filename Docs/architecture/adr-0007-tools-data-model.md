# ADR-0007 — Tools and weapons data model: DT_Tools replaces hardcoded values

**Status:** Accepted
**Date:** 2026-11-10
**Deciders:** Technical Director, with Game Designer
**References:** Docs/SYSTEMS_REFERENCE.md (§6 Baubles, §7 FToolInstance), ADR-0005, ADR-0006

## Context

`FToolInstance` stores tier multipliers as hardcoded values in `GetBaseMiningForce()`
(Wood 2x, Stone 4x, Iron 6x, Diamond 10x, Obsidian 15x). This violates the
studio rule "no hardcoded gameplay balance values". Tool icons are resolved from
hardcoded Patrix paths in `FToolInstance::GetToolIconTexture`. Durability, damage
and mesh references are not data-driven. The overhaul needs 25 tools (5 types x
5 tiers) with consistent data, save/load support and designer tuning.

## Decision

1. **Create `DT_Tools` (DataTable) keyed by a stable `ToolId` (FName).** Columns:
   `ToolType`, `ToolTier`, `BaseMiningForce`, `DurabilityMax`, `AttackDamage`,
   `MeshPath` (soft reference), `IconPath` (soft reference), `SwingMontage`,
   `ImpactCue`, `MiningForceByBlockCategory` (map BlockCategory→multiplier).
2. **`FToolInstance` keeps only runtime state:** `ToolId`, `ToolType`, `ToolTier`,
   `CurrentDurability`, `MaxDurability` (cached from DT), `InstanceId` (stable
   GUID for save data). All static data is resolved via `DT_Tools`.
3. **`UBaublesSystem` resolves tool data** through a `UToolDefinition` (DataAsset)
   wrapper around `DT_Tools`, with caching. No hardcoded paths or multipliers.
4. **Save/load** references `ToolId` + `InstanceId`, never array indices. New
   fields get default values and a migration note (per ue-data.md).
5. **Validation:** an automation test checks that every row in `DT_Tools` has a
   valid mesh, icon, and non-zero durability. Missing rows fail the build.

## Consequences

- **Positive:** corrects the existing violation; designers can tune without
  recompiling; save data is stable across balance changes.
- **Negative:** migration of existing `FToolInstance` code and icon resolution.
- **Risk:** if `DT_Tools` is not kept in sync with the mesh/icon assets, tools
  appear broken. Mitigated by the validation test and the asset lint tool.

## Alternatives considered

- **Hardcoded enum with switch:** rejected. Violates the data-driven rule and
  requires recompilation for balance changes.
- **Blueprint DataAsset per tool:** rejected. 25 assets are harder to maintain
  than one DataTable; DataTables support row-level diffing and import from CSV.