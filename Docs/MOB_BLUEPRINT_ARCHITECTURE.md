# Mob Blueprint Architecture (ADR-0009 / ADR-0010)

This document captures the Blueprint-side assets and wiring required to complete
the mob overhaul. C++ changes (bCanEverTick=false, timer-based updates, anim
state exposure, TSubclassOf spawner, FMobTableRow) are already in
`Source/MinecraftDeWish`. The assets below must be created in the Unreal Editor
(no Unreal MCP server is available, so they cannot be generated headlessly).

## 0. Blender source (already produced)

- `Content/Characters/Mobs/MobBase.blend` — source scene.
- `Content/Characters/Mobs/SK_Mob_Base.fbx` — exported skeleton + mesh.
- `Content/Characters/Mobs/T_Mob_Zombie.png` — 128x128 placeholder texture.
- Skeleton: 7 bones (root, spine, head, arm_l, arm_r, leg_l, leg_r), rigid weights.

> Note: ADR-0010 specifies "6 bones". The named list in ADR-0010/this task
> contains 7 entries (root + 6 deform bones). The export keeps all 7 named bones;
> if a strict 6-bone skeleton is required, drop `root` and reparent `spine`/legs
> to the armature origin. Recommendation: keep 7 (root at feet) for correct
> locomotion origin.

## 1. Import SK_Mob_Base into UE

1. In Content Browser, right-click `Content/Characters/Mobs/` -> Import.
2. Select `SK_Mob_Base.fbx`.
3. In the FBX import dialog:
   - Skeleton: create new `SK_Mob_Base`.
   - Mesh: `SM_Mob_Base` (skeletal).
   - Import Materials: yes (creates `M_Mob_Base`).
   - Import Textures: yes.
   - Nanite: OFF.
   - LOD: leave default (manual LODs added later).
4. Set texture `T_Mob_Zombie` (imported) to 128x128, Filter = Nearest, Mipmaps ON,
   in Texture properties. Assign to `M_Mob_Base` Base Color.
5. LODs: open `SK_Mob_Base` -> LOD Settings -> add LOD1 (~800 tris) and LOD2
   (~200 tris) via Simplify or manual. Dithered fade ON.

Resulting assets:
- `SK_Mob_Base` (Skeleton)
- `SK_Mob_Base` (SkeletalMesh) — note UE names the mesh after the skeleton by
  default; rename the mesh to `SK_Mob_Base_Mesh` if desired.
- `M_Mob_Base` (Material)
- `T_Mob_Zombie` (Texture2D)

## 2. ABP_Mob_Base (Animation Blueprint)

Create `Content/Characters/Mobs/ABP_Mob_Base` (parent: `UAnimInstance`).

Variables (BlueprintReadOnly, driven by C++ `AMobCharacter`):
- `Speed` (float) — read from `AMobCharacter.Speed`.
- `bIsFalling` (bool) — read from `AMobCharacter.bIsFalling`.
- `AIState` (EMobAIState) — read from `AMobCharacter.CurrentAIState`.

Locomotion state machine (states):
- `Idle` (default): play `A_Mob_Idle`.
- `Walk`: play `A_Mob_Walk` when `Speed > 0 && Speed <= 250`.
- `Run`: play `A_Mob_Run` when `Speed > 250`.
- `Attack`: play `A_Mob_Attack` montage on attack state.
- `HitReact`: play `A_Mob_HitReact` on damage.
- `Death`: play `A_Mob_Death` on death.

Transitions by `Speed` thresholds. Use `UAnimBudgetAllocatorComponent`-friendly
update rates (no per-frame heavy logic).

Animation sequences to author (retargetable across humanoid mobs):
- `A_Mob_Idle`, `A_Mob_Walk`, `A_Mob_Run`, `A_Mob_Attack`, `A_Mob_HitReact`,
  `A_Mob_Death`.

## 3. BP_Mob_Base (Blueprint child of AMobCharacter)

Create `Content/Blueprints/Mobs/BP_Mob_Base` (parent: `AMobCharacter`).

Components:
- `SkeletalMeshComponent` (Mesh) -> Skeletal Mesh = `SK_Mob_Base`, Anim Class =
  `ABP_Mob_Base`, Materials = `M_Mob_Base`.
- Capsule as inherited.

Defaults (set in Class Defaults):
- `MobType` = Zombie (overridden per child).
- `bCanEverTick` already false (C++).

Expose tunables via `EditAnywhere` (already C++): Health etc. are set by
`InitializeMob` from `FMobDefinition`.

## 4. BP_Mob_Zombie and BP_Mob_Skeleton

Create `Content/Blueprints/Mobs/BP_Mob_Zombie` (parent: `BP_Mob_Base`):
- Class Defaults: `MobType = Zombie`.
- Material instance: `MI_Mob_Zombie` (base `M_Mob_Base`, tinted sick green).

Create `Content/Blueprints/Mobs/BP_Mob_Skeleton` (parent: `BP_Mob_Base`):
- Class Defaults: `MobType = Skeleton`.
- Material instance: `MI_Mob_Skeleton` (bone-white).

Spider and Creeper get their own skeletons/ABPs later (separate skeleton per
ADR-0010).

## 5. StateTree assets

Create `Content/AI/ST_Mob_Behavior` (StateTree) using
`UStateTreeAIComponent` schema. States:
- `Patrol`: wander/idle patrol.
- `Chase`: move toward player when detected.
- `Attack`: melee/ranged attack in range.
- `Flee`: flee when low health (optional).
- `Fuse`: creeper-only fuse (or a separate ST_Creeper).

Assign `ST_Mob_Behavior` to each mob BP via an AIController Blueprint
(`BP_MobAIController`) that has a `UStateTreeAIComponent` with
`StateTree = ST_Mob_Behavior`. Set the mob BP's `AIControllerClass` to
`BP_MobAIController`.

Perception: add `UAIPerceptionComponent` to the AIController with sight config
(range = DetectionRange from FMobDefinition).

Note: UStateTreeAIComponent attaches to an AIController, not the character, so
an AIController Blueprint is required.

## 6. DT_Mobs DataTable

Create `Content/Data/DT_Mobs` (row struct = `FMobTableRow`):
- Row Zombie: MobType=Zombie, MobClass=BP_Mob_Zombie.
- Row Skeleton: MobType=Skeleton, MobClass=BP_Mob_Skeleton.
- Row Spider: MobType=Spider, MobClass=BP_Mob_Spider (future).
- Row Creeper: MobType=Creeper, MobClass=BP_Mob_Creeper (future).

Assign `DT_Mobs` to `UMobSpawnerSystem.MobDataTable` (or populate `MobClasses`
map directly). The spawner resolves `TSubclassOf<AMobCharacter>` from this data.

## 7. MobSpawnerSystem wiring

In the WorldGenerator Blueprint/instance defaults:
- `MobClasses` map: Zombie->BP_Mob_Zombie, Skeleton->BP_Mob_Skeleton, etc.
- (Optional) `MobDataTable` = DT_Mobs.

## 8. Verification checklist

- [ ] SK_Mob_Base imports with 7 bones, rigid weights.
- [ ] ABP_Mob_Base compiles with Speed/bIsFalling/AIState.
- [ ] BP_Mob_Base spawns in PIE, mesh visible, animates.
- [ ] BP_Mob_Zombie and BP_Mob_Skeleton spawn via spawner.
- [ ] StateTree drives patrol/chase/attack (no C++ Tick).
- [ ] `stat game` shows mob cost within budget at 20+ mobs.
- [ ] No Blueprint graph exceeds 20 nodes per function.
- [ ] Nanite OFF on mob mesh; 3 LODs present.
- [ ] Textures 128x128, Nearest filter, mipmaps ON.