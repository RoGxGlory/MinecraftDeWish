# ADR-0009 — Mob behavior architecture: C++ base, Blueprint content, StateTree decisions

**Status:** Accepted
**Date:** 2026-11-10
**Deciders:** Technical Director, with UE Blueprint Specialist
**References:** Docs/SYSTEMS_REFERENCE.md (§10-11), .opencode/rules/ue-blueprint.md, .opencode/rules/ue-cpp.md

## Context

`AMobCharacter` currently holds a hand-written state machine in `Tick`
(`ProcessWander/Chase/Attack/Explosion`) and `UMobSpawnerSystem::SpawnMob`
instantiates the base class with an `EMobType`. The overhaul needs visible mobs
with models, animations and behaviors in Blueprint, while the studio rules put
systems in C++ and content in Blueprint. The `GameplayStateTree` plugin is
already enabled.

## Decision

1. **`AMobCharacter` (C++) owns the rules only:** health, damage, drops, XP,
   sunlight burn, death. Zero behavior decisions. `bCanEverTick` set to false.
2. **Behavior decisions move to `UStateTreeAIComponent`** (plugin already
   enabled). States: patrol, chase, attack, flee, fuse (creeper). Frequency is
   reduced; perception uses `UAIPerceptionComponent`.
3. **`UMobSpawnerSystem::SpawnMob` takes a `TSubclassOf<AMobCharacter>`**
   resolved from a `DT_Mobs` DataTable (`EMobType → MobClass, mesh, ABP, stats,
   loot`). `EMobType` + `GetMobTexturePaths` (hardcoded paths) are removed.
4. **Content lives in Blueprint children:** `BP_Mob_Zombie`, `BP_Mob_Skeleton`,
   `BP_Mob_Spider`, `BP_Mob_Creeper`, all children of `AMobCharacter`. They
   carry mesh, materials, animation blueprint, tunables and loot table reference.
5. **Shared animation blueprint** `ABP_Mob_Base` with a locomotion state machine
   driven by `Speed`/`bIsFalling`/state enum exposed as `BlueprintReadOnly` from
   C++. Derived ABPs only where a mob genuinely differs (Spider).
6. **Shared vitals component** (`UVitalsComponent`) for player and mobs, so
   there is one damage path instead of two.
7. **Animation budgeting:** `UAnimBudgetAllocatorComponent` + significance
   manager from the start, so 20+ mobs stay inside frame budget.

## Consequences

- **Positive:** behavior is data-driven and designer-tunable; mobs are visible
  and animated; `AMobCharacter` stops accumulating special cases.
- **Negative:** StateTree authoring has a learning curve; the migration touches
  the spawner and the mob base class.
- **Risk:** Behavior Tree is the alternative if the team is more comfortable
  with it. StateTree is the modern path and the plugin is already on. Pick one,
  never both.

## Alternatives considered

- **Keep the C++ state machine:** rejected. It blocks designer iteration and
  accumulates special cases per mob type.
- **Behavior Tree:** acceptable fallback, but StateTree is preferred.