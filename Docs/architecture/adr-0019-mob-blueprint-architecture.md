# ADR-0019: Mob Blueprint Architecture — C++ Base, Blueprint Content, StateTree Decisions

## Status
Accepted

## Date
2026-11-11

## Deciders
Technical Director, with UE Blueprint Specialist and Technical Artist

## References
- `Docs/architecture/adr-0009-mob-behavior-architecture.md` (behavior decisions, spawner, StateTree)
- `Docs/architecture/adr-0010-mob-skeletons.md` (shared skeleton, animation sharing, budget)
- `Docs/MOB_BLUEPRINT_ARCHITECTURE.md` (asset-by-asset Blueprint wiring guide)
- `Source/MinecraftDeWish/Public/MobCharacter.h` (`AMobCharacter` C++ base)
- `Source/MinecraftDeWish/Public/MobAIController.h` (`AMobAIController`, StateTree + perception)
- `.opencode/rules/ue-blueprint.md`, `.opencode/rules/ue-cpp.md`, `.opencode/rules/ue-ui.md`

## Context

### Problem Statement

`AMobCharacter` exists in C++ with health, damage, drops, XP, sunlight burn,
spider climbing, and a hand-written state machine inside `Tick`. The spawner
(`UMobSpawnerSystem::SpawnMob`) instantiates the base class with an `EMobType`.
Mobs are therefore invisible, unanimated, and their behavior cannot be tuned
without recompiling. The overhaul requires four visible, animated, hostile mobs
(Zombie, Skeleton, Spider, Creeper) whose models, materials, animations, and
behavior can be iterated by designers in the Unreal Editor.

The studio rules (`.opencode/rules/ue-blueprint.md`) put systems, performance-
critical loops, replication, and save/load in C++, and content assembly, level
wiring, visual effects, and designer tuning in Blueprint. The `GameplayStateTree`
plugin is already enabled, and `AMobAIController` already owns a
`UStateTreeAIComponent` plus `UAIPerceptionComponent`. A shared skeleton
(`SK_Mob_Base`, 7 bones, rigid weights) and a shared animation blueprint
(`ABP_Mob_Base`) are specified by ADR-0010.

The open question this ADR resolves is **where the boundary between C++ and
Blueprint sits for mobs**, and **which concrete assets constitute the mob content
layer**, so that every mob is a Blueprint child of `AMobCharacter` carrying
mesh, material, animation, and tunables, driven by a single StateTree, without
duplicating logic.

### Constraints

- Engine: Unreal Engine 5.8, Chaos physics, Lumen (ADR-0001).
- `AMobCharacter` has `bCanEverTick = false`; all updates are timer-driven
  (sunlight burn @ 1.0 s, mob update @ 10 Hz). No per-frame behavior decisions
  may be added back to the character.
- `AMobAIController` is the only owner of perception and StateTree; the
  character exposes behavior helpers (`StartPatrol`, `ChaseTarget`,
  `PerformAttack`, `FleeFrom`, `StartFuse`, `CancelFuse`, `UpdateFuse`) as
  `BlueprintCallable` entry points for StateTree tasks.
- Shared skeleton `SK_Mob_Base` (7 bones including root, rigid weights) covers
  all humanoid mobs; Spider keeps a separate skeleton (ADR-0010). Note: ADR-0010
  states "6 bones"; the exported skeleton counts the root bone, giving 7 named
  bones (root, spine, head, arm_l, arm_r, leg_l, leg_r). Keep 7 (root at feet)
  for correct locomotion origin (see `Docs/MOB_BLUEPRINT_ARCHITECTURE.md` ��0).
- Shared `ABP_Mob_Base` locomotion state machine driven by `Speed`,
  `bIsFalling`, and `CurrentAIState` exposed as `BlueprintReadOnly` from C++.
- Performance budget: 60 FPS / 16.6 ms frame, gameplay < 4 ms; 20+ mobs must
  stay inside budget via `UAnimBudgetAllocatorComponent` + significance manager
  (ADR-0010).
- Textures: 128x128, Nearest filter, mipmaps ON (Patrix entity tier).
- Nanite OFF on mob meshes; 3 LODs; dithered fade ON.
- No third-party plugin without an ADR; only `ModelingToolsEditorMode` and
  `GameplayStateTree` are enabled.

### Requirements

- Mobs must be visible, animated, and spawnable through `UMobSpawnerSystem`
  resolving a `TSubclassOf<AMobCharacter>` from `DT_Mobs`.
- Designers must be able to tune mesh, material, animation, stats, and loot
  without recompiling C++.
- Behavior decisions must live in StateTree tasks/conditions, never in
  `AMobCharacter::Tick` or in Blueprint graphs.
- One damage path: `AMobCharacter::ApplyMobDamage` is the single entry point for
  health changes; UI and gameplay both read from `AMobCharacter` health state.
  (ADR-0009's shared vitals component is a future refactor, not yet landed;
  health currently lives directly on `AMobCharacter`.)
- Blueprint graphs must stay small (no function > ~20 nodes); logic that exceeds
  that moves to C++.
- Save/load and replication (future) must be unaffected: mob identity is the
  `EMobType` + stable DataTable row, never a Blueprint asset path.

## Decision

### 1. Layered architecture: C++ rules, Blueprint content, StateTree decisions

Three strictly separated layers, matching the studio's C++/Blueprint boundary:

| Layer | Owns | Lives in |
| --- | --- | --- |
| **Rules** | Health, damage, death, drops, XP, sunlight burn, spider climb, creeper fuse math, skeleton ranged projectiles, zombie melee, timer cadence, perception target bookkeeping | `AMobCharacter` (C++) |
| **Content** | Skeletal mesh, material instances, animation blueprint, AIController class, tunables, loot table reference | `BP_Mob_*` (Blueprint children of `AMobCharacter`) |
| **Decisions** | Patrol/chase/attack/flee/fuse state machine, transitions, perception-driven target selection | `ST_Mob_Behavior` + `AMobAIController` (StateTree) |

`AMobCharacter` is `BlueprintType`/`Blueprintable` and exposes behavior helpers
as `BlueprintCallable` so StateTree tasks (and, where needed, designer hooks via
`BlueprintNativeEvent`) can invoke them. It never calls a widget, never owns a
mesh, and never decides which state to enter. `BP_Mob_*` never overrides
behavior logic; it only assembles components and sets class defaults.

### 2. Blueprint child hierarchy

```
AMobCharacter (C++)
└── BP_Mob_Base (Blueprint, Content/Blueprints/Mobs/)
    ├── BP_Mob_Zombie       (MobType = Zombie,   MI_Mob_Zombie)
    ├── BP_Mob_Skeleton     (MobType = Skeleton, MI_Mob_Skeleton)
    ├── BP_Mob_Spider       (MobType = Spider,    own skeleton per ADR-0010)
    └── BP_Mob_Creeper      (MobType = Creeper,   MI_Mob_Creeper)
```

- `BP_Mob_Base` carries the shared `SkeletalMeshComponent` (mesh =
  `SK_Mob_Base`, anim class = `ABP_Mob_Base`, material = `M_Mob_Base`) and the
  inherited capsule. It sets `MobType = Zombie` as the default, overridden per
  child.
- Each child sets only `MobType` and its material instance (`MI_Mob_*`). No
  child re-implements health, AI, or drops.
- `bCanEverTick` is already false in C++; Blueprint cannot re-enable it.
- `AIControllerClass` on every mob BP is set to `BP_MobAIController`
  (a Blueprint child of `AMobAIController`), which owns the
  `UStateTreeAIComponent` with `StateTree = ST_Mob_Behavior` and the
  `UAIPerceptionComponent` (sight + hearing, range from `FMobDefinition`).

### 3. Shared animation layer

- One skeleton `SK_Mob_Base` (7 bones, rigid weights) for all humanoid mobs;
  Spider uses a separate skeleton (ADR-0010).
- One animation blueprint `ABP_Mob_Base` (parent `UAnimInstance`) with a
  locomotion state machine (Idle / Walk / Run / Attack / HitReact / Death)
  driven by `Speed`, `bIsFalling`, and `CurrentAIState` exposed from C++ as
  `BlueprintReadOnly`.
- Animation sequences (`A_Mob_Idle/Walk/Run/Attack/HitReact/Death`) are
  authored once and shared across humanoid mobs; per-mob differences are
  montages and material/parameter tweaks, not skeletons or ABPs.
- Derived ABPs are permitted only where a mob genuinely differs (Spider).
- `UAnimBudgetAllocatorComponent` + significance manager are mandatory from the
  start (ADR-0010), so 20+ mobs reduce tick rate and LOD by distance.

### 4. StateTree as the single decision authority

- `ST_Mob_Behavior` (one asset, `UStateTreeAIComponent` schema) owns all
  behavior decisions: Patrol, Chase, Attack, Flee, Fuse (creeper). Transitions
  are driven by perception (`UAIPerceptionComponent`) and by C++-exposed state
  (`CurrentAIState`, health, fuse timer).
- `AMobAIController` owns perception and the StateTree; it never lives on the
  character. An AIController Blueprint (`BP_MobAIController`) is required to
  assign the StateTree asset and perception config.
- `AMobCharacter` exposes `BlueprintCallable` helpers (`StartPatrol`,
  `ChaseTarget`, `PerformAttack`, `FleeFrom`, `StartFuse`, `CancelFuse`,
  `UpdateFuse`, `GetPerceptionTarget`) that StateTree tasks call; it also
  exposes `CurrentTarget` (weak) and `CurrentAIState` for conditions.
- `TickAIStateMachine` and the legacy `ProcessWander/Chase/Attack/Explosion`
  paths are retained only as compatibility shims and are not the authority; new
  behavior must be authored in StateTree.
- `bCanEverTick = false` is preserved; the 10 Hz `MobUpdateTimer` updates
  `Speed`/`bIsFalling`/`CurrentAIState` for the ABP and spider climb, nothing
  else.

### 5. Data-driven content resolution

- `DT_Mobs` (row struct `FMobTableRow`) maps `EMobType → MobClass, mesh, ABP,
  stats, loot`. `UMobSpawnerSystem::SpawnMob` takes a
  `TSubclassOf<AMobCharacter>` resolved from this table.
- `MobClasses` map on the `WorldGenerator` (Zombie→`BP_Mob_Zombie`, etc.) and
  the optional `MobDataTable` are the wiring points; no mob class is hardcoded
  in C++.
- `EMobType` + `GetMobTexturePaths` (hardcoded paths) are removed per
  ADR-0009; stats come from `FMobDefinition` populated by `InitializeMob`.
- Mob identity for save/load and replication is the stable `EMobType` + DataTable
  row id, never a Blueprint asset path.

### 6. UI and presentation boundary

- UI never references a mob widget or mesh; it reads health/state through
  `AMobCharacter` properties and sends intent via delegates
  (`.opencode/rules/ue-ui.md`). (No shared vitals component yet; health lives
  on `AMobCharacter`.)
- Damage feedback (hit react, death) is driven by `OnMobDied` and
  `ApplyMobDamage` return value, not by Blueprint polling.

## Consequences

### Positive

- **Designer-tunable content**: mesh, material, ABP, tunables, and loot are
  edited in `BP_Mob_*` without recompiling; stats flow from `DT_Mobs`/
  `FMobDefinition`.
- **Shared animations**: one skeleton + one locomotion set cover all humanoid
  mobs; production cost is divided by ~4 (ADR-0010).
- **Single decision authority**: StateTree owns behavior; `AMobCharacter` stops
  accumulating per-mob special cases; `Tick` stays disabled.
- **Single damage path**: `ApplyMobDamage` is the only entry point for health
  changes; UI and gameplay agree (health lives on `AMobCharacter`).
- **Performance**: timer-driven updates (10 Hz mob update, 1 Hz sunlight burn)
  plus animation budgeting keep 20+ mobs inside the 4 ms gameplay budget.
- **Extensibility**: a new mob is a new `BP_Mob_*` child + a `DT_Mobs` row +
  (optionally) a StateTree branch, with no C++ change when rules are unchanged.
- **Save/replication safety**: identity is `EMobType` + row id, not an asset
  path, so renames do not break saves.

### Negative

- **StateTree learning curve**: authors must learn StateTree tasks/conditions
  and the `UStateTreeAIComponent` schema; Blueprint-only designers need
  onboarding. Mitigation: `BP_MobAIController` is the single assignment point
  and `ST_Mob_Behavior` is the only StateTree asset in v1.
- **Two-asset wiring**: every mob needs both a `BP_Mob_*` and a `DT_Mobs` row
  kept in sync; mismatched `MobClass` breaks spawning. Mitigation: an
  automation test validates `DT_Mobs` rows resolve to valid classes.
- **Shared-skeleton constraint**: all humanoid mobs share proportions; a mob
  needing a unique silhouette requires a skeleton variant (ADR-0010 risk).
- **Retained legacy paths**: `TickAIStateMachine`/`Process*` shims remain in
  C++ until fully migrated, creating a risk of duplicate logic. Mitigation:
  they are documented as compatibility-only and must not be extended.
- **Asset creation is manual**: UE assets (mesh, ABP, StateTree, DT) cannot be
  generated headlessly; `Docs/MOB_BLUEPRINT_ARCHITECTURE.md` is the checklist.

### Risks

- **Blueprint spaghetti**: a `BP_Mob_*` function exceeding ~20 nodes signals a
  move to C++. Mitigation: code review + `/code-review` on Blueprint-facing
  changes; no logic in content Blueprints.
- **StateTree/C++ drift**: if `AMobCharacter` helpers and StateTree tasks
  disagree on cooldowns or fuse timing, behavior becomes non-deterministic.
  Mitigation: helpers are the single source of truth; StateTree only calls them.
- **Animation budget overrun**: 20+ mobs with full-rate ABPs can exceed budget.
  Mitigation: `UAnimBudgetAllocatorComponent` + significance manager mandatory;
  verify with `stat game`/Unreal Insights at 20+ mobs (checklist in
  `Docs/MOB_BLUEPRINT_ARCHITECTURE.md`).
- **Perception cost**: sight+hearing on every mob AIController. Mitigation:
  range from `FMobDefinition`, update on a throttle, profile with
  `Net PktLag`/latency simulation if multiplayer is added later.
- **Shared skeleton rigidity**: rigid weights prevent smooth deformation; any
  mob needing soft skinning needs a new skeleton. Mitigation: keep base skeleton
  as the common denominator; variants are additive.
- **UE 5.8 API drift**: `UStateTreeAIComponent`/perception APIs may differ
  from the 5.7 reference. Mitigation: verify against installed engine source
  (`F:\Developpement\UE_5.8\Engine\Source`) before shipping.

## Performance Implications

- **CPU**: behavior decisions run in StateTree at perception cadence, not per
  frame; mob update @ 10 Hz; sunlight burn @ 1 Hz. No `Tick` on mobs.
- **GPU**: 128x128 Nearest textures, Nanite OFF, 3 LODs with dithered fade,
  shared skeleton keeps draw-call and skinning cost flat per mob count.
- **Memory**: one shared skeleton + one ABP + one texture set reused across
  humanoid mobs; per-mob cost is mesh instance + material instance only.
- **Load time**: `DT_Mobs` rows resolve `TSubclassOf` lazily; spawner loads
  mob classes on first spawn. Soft references (`TSoftObjectPtr`) preferred for
  assets not always needed.
- **Network**: not applicable for v1 (single player); replication changes
  require an ADR if they alter ownership/authority/save data.

## Alternatives Considered

### Alternative 1: Behavior Tree instead of StateTree

- **Description**: Drive mob decisions with `UBehaviorTree` + `UBTTaskNode`s
  instead of `UStateTreeAIComponent`, reusing the existing `AAIController`
  perception setup.
- **Pros**: Familiar to most UE developers; large ecosystem of task nodes;
  well-documented.
- **Cons**: StateTree is the modern, data-driven replacement already enabled via
  `GameplayStateTree`; Behavior Tree tasks are code-heavy and harder for
  designers to tune; running both is explicitly forbidden (ADR-0009).
- **Rejection Reason**: StateTree is preferred and the plugin is already on.
  Behavior Tree remains an acceptable fallback only if the team cannot adopt
  StateTree; pick one, never both.

### Alternative 2: C++ state machine (status quo)

- **Description**: Keep `TickAIStateMachine` + `ProcessWander/Chase/Attack/
  Explosion` in `AMobCharacter`, adding per-mob branches.
- **Pros**: No new authoring surface; deterministic; no plugin dependency.
- **Cons**: Blocks designer iteration; accumulates special cases per mob type;
  contradicts the studio rule that content/behavior decisions live in
  Blueprint/data; `Tick`-based updates fight the `bCanEverTick = false` goal.
- **Rejection Reason**: Rejected by ADR-0009. Retained only as a compatibility
  shim, not extended.

### Alternative 3: Unique skeleton and ABP per mob

- **Description**: Author a separate skeleton and animation blueprint for each
  of Zombie/Skeleton/Spider/Creeper.
- **Pros**: Full per-mob silhouette and animation freedom.
- **Cons**: 4x authoring cost; no visual gain at the chosen low-poly stylized
  style; breaks the animation budgeting assumption.
- **Rejection Reason**: Rejected by ADR-0010. Shared skeleton is the common
  denominator; variants are additive.

### Alternative 4: Logic in Blueprint children (`BP_Mob_*`)

- **Description**: Put health, damage, AI decisions, and drops directly in
  Blueprint graphs of `BP_Mob_*`.
- **Pros**: Fastest iteration; no C++ compile.
- **Cons**: Violates the studio C++/Blueprint boundary; duplicates logic across
  four BPs; untestable; performance risk in Blueprint VM; save/load and
  replication become asset-path dependent.
- **Rejection Reason**: Rejected. `BP_Mob_*` is content-only; rules live in
  `AMobCharacter`.

## Validation Criteria

- `BP_Mob_Base`, `BP_Mob_Zombie`, `BP_Mob_Skeleton`, `BP_Mob_Spider`,
  `BP_Mob_Creeper` all spawn in PIE with visible mesh and playing animation.
- `ST_Mob_Behavior` drives patrol/chase/attack (and fuse for Creeper) with no
  C++ `Tick` (verify `bCanEverTick == false` and no behavior in `Tick`).
- `stat game` shows mob cost within budget at 20+ mobs; Unreal Insights capture
  attached to the PR.
- No Blueprint graph exceeds 20 nodes per function.
- `DT_Mobs` rows resolve to valid `TSubclassOf<AMobCharacter>` (automation
  test).
- Save/load round-trips a mob by `EMobType` + row id, not asset path.
- Textures 128x128, Nearest filter, mipmaps ON; Nanite OFF; 3 LODs present.

## Related Decisions

- `Docs/architecture/adr-0009-mob-behavior-architecture.md`
- `Docs/architecture/adr-0010-mob-skeletons.md`
- `Docs/architecture/adr-0008-blender-pipeline.md`
- `Docs/MOB_BLUEPRINT_ARCHITECTURE.md`
- `Docs/SYSTEMS_REFERENCE.md` §10–11
- `.opencode/rules/ue-blueprint.md`, `.opencode/rules/ue-cpp.md`,
  `.opencode/rules/ue-data.md`, `.opencode/rules/ue-ui.md`
