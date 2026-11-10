# ADR-0003: Block Crack Overlay Is Driven by the Blueprint Mining Timer

## Status
Accepted

## Context

### Problem Statement
Mining had no progressive visual feedback: a block stayed pristine for its whole break
time and then vanished. `ABlockCrackActor` and `M_BlockCrack` were added in `b6571ed`, but
nothing fed them progress, so no crack ever appeared in game.

The first implementation drove overlays from `AWorldGenerator::Tick` by iterating the active
tasks of `UMiningQueueSystem`. The playable build never populates that queue: mining is
scripted in Blueprint — `BP_HorrorCharacter` runs an `IncrementBreakTimer` timer and forwards
the accumulated `BreakTime` to `AC_DestroySystem::StartMining`. The C++ hook therefore never
fired.

Two secondary defects were found while validating in PIE:

- `M_BlockCrack` derived its opacity from `OneMinus(texture)`. The `destroy_stage_*` textures
  do **not** store the crack mask in their luminance: measured on `destroy_stage_9`
  (32×32), RGB spans 0–176 while alpha is either 0 or 255 (mean 195). `OneMinus(RGB)` leaves
  the transparent background opaque and the cracks themselves transparent — the exact
  inverse of the intent.
- The reset path was wired to `Event ResetBreakEvent`, a custom event that nothing in
  `BP_HorrorCharacter` calls. The real cancel path is
  `IA_Break.Canceled|Completed → Clear and Invalidate Timer by Handle`. Overlays were never
  destroyed on release.

### Constraints
- Mining state already lives in Blueprint (`BreakTime`, `LastTargetedBlockVoxel`,
  `Current_Mining_Force` on `BP_HorrorCharacter`). C++ must not duplicate it.
- No per-frame polling: the project forbids unnecessary `Tick`.
- The overlay must never block line traces or physics.
- `destroy_stage_0..9` are used for this overlay only; `AChaosBlockDestructionActor` debris
  is a separate system and is out of scope.
- Progress must reach stage 9 exactly when the block actually breaks.

### Requirements
- The overlay appears on the first mining tick, advances through 10 stages, and disappears
  when mining is cancelled and when the block breaks.
- Cube blocks and torches (crossed quads) are both supported.
- Cost is near zero when nothing is being mined.
- Works without the player having to hand-edit a Blueprint for every new block type.

## Decision

`AWorldGenerator` exposes a narrow Blueprint-callable bridge. Blueprint remains the single
source of truth for progress; C++ owns presentation.

| Function | Role |
| --- | --- |
| `UpdateCrackMining(FIntVector VoxelCoord, float Progress01)` | spawns or advances the overlay of one voxel |
| `ClearCrackMining()` | destroys every live overlay |
| `GetBlockBreakTimeAtVoxel(FIntVector, float MiningForce = 0.0f)` | `BlueprintPure`; denominator of the progress ratio |

Progress is computed in Blueprint as:

```
Progress01 = BreakTime / GetBlockBreakTimeAtVoxel(LastTargetedBlockVoxel, Current_Mining_Force)
```

`GetBlockBreakTimeAtVoxel` deliberately reuses the exact formula the break test uses
(`UBaublesSystem::GetBestMiningForce` → `AWorldGenerator::GetToolMiningForce` →
`UMiningQueueSystem::CalculateBreakTime` over the `Durability` column of
`Block_DataTable`), so numerator and denominator share one definition and stage 9 lands on
the same frame as the break.

Wiring in `BP_HorrorCharacter`:

- `IncrementBreakTimer` → `Set BreakTime` → `UpdateCrackMining`
- `IA_Break.Canceled|Completed` → `Clear and Invalidate Timer by Handle` → `ClearCrackMining`
  → `Set BreakTime = 0`
- `AWorldGenerator::BreakBlockAtVoxel` destroys the overlay of the voxel it turns to air.

`ABlockCrackActor` is a pure function of progress: `PrimaryActorTick.bCanEverTick = false`,
one `UProceduralMeshComponent` with collision disabled, one dynamic material instance over
`M_BlockCrack`, and the texture parameter is only written when the stage index changes.

`M_BlockCrack` is Masked / Unlit / TwoSided with
`OpacityMask = CrackTex.A` and `Emissive = CrackTex.RGB × 0.55`, which reproduces the vanilla
"dark cracks over the untouched block" look without lighting the overlay.

### Key Interfaces
```cpp
// WorldGenerator.h
UFUNCTION(BlueprintCallable) void UpdateCrackMining(const FIntVector& VoxelCoord, float Progress01);
UFUNCTION(BlueprintCallable) void ClearCrackMining();
UFUNCTION(BlueprintPure)     float GetBlockBreakTimeAtVoxel(const FIntVector& VoxelCoord, float MiningForce = 0.0f) const;

// Broadcast only when the displayed stage changes; the overlay itself is keyed per voxel.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCrackStageChanged, FIntVector, VoxelCoord, int32, Stage);

// BlockCrackActor.h
void InitializeCrackOverlay(const FIntVector& InVoxelCoord, uint8 InBlockID, float InBlockScale, const FVector& InWorldCenter);
bool UpdateCrackStage(float Progress);        // true when the stage index changed
static int32 GetStageFromProgress(float Progress); // 0..9, FloorToInt(Clamp(Progress,0,1) * 10)
```

## Alternatives Considered

### Alternative 1: Drive overlays from `UMiningQueueSystem`
- **Description**: Keep the `AWorldGenerator::Tick` loop over active mining tasks.
- **Pros**: All logic in C++, no Blueprint change.
- **Cons**: `UMiningQueueSystem` is never populated by the playable build.
- **Rejection Reason**: Verified in PIE that no overlay was ever spawned through this path.

### Alternative 2: Poll Blueprint state from `AWorldGenerator::Tick`
- **Description**: Read `BreakTime` / `LastTargetedBlockVoxel` off the pawn every frame and
  drive the overlay from C++.
- **Pros**: No Blueprint edit; guaranteed to run.
- **Cons**: Per-frame cost while idle, duplicated mining state, and property access by name
  through `FindFProperty` is fragile against renames.
- **Rejection Reason**: The Blueprint already knows the progress; polling would recreate a
  second source of truth next to it.

### Alternative 3: Opacity mask from luminance (`OneMinus`)
- **Description**: Derive the mask from RGB, as a plain grayscale mask would allow.
- **Measured**: On `destroy_stage_1/4/9` the RGB maximum is 126/162/176 while alpha reaches
  255 with mean 23.9/83.7/195.2 — the mask is the alpha channel.
- **Rejection Reason**: `OneMinus(RGB)` renders the background opaque and the cracks
  transparent, i.e. the inverse. Confirmed invisible in PIE before the rewrite.

## Consequences

### Positive
- Mining now has continuous feedback: stage 3 measured at `BreakTime = 0.3 s`, stage 6 at
  `0.6 s` on a Durability-1 block, stage 9 immediately before the break.
- No `Tick` is added anywhere; the overlay costs nothing when idle and one texture parameter
  write per stage change while mining.
- The overlay never blocks gameplay: no collision, and line traces ignore it because it is
  spawned without a collision profile.
- `GetBlockBreakTimeAtVoxel` makes the break-time formula reachable from Blueprint, removing
  the duplicated arithmetic that previously lived only in the Blueprint graph.

### Negative
- The progress contract is now split across two languages: a Blueprint edit can silently
  break the overlay even though C++ still compiles.
- `UpdateCrackMining` depends on `World Gen Ref`, a Blueprint variable filled from
  `GetActorOfClass` in `BeginPlay`. If it resolves to `None`, the call becomes a no-op and the
  failure is only visible as an "Accessed None" log line.

### Risks
- **A future mining rewrite bypasses the Blueprint timer.** Mitigation: the bridge is a plain
  `BlueprintCallable` API — any new driver only has to call `UpdateCrackMining`.
- **Stale overlays if a reset path forgets `ClearCrackMining`.** Mitigation:
  `BreakBlockAtVoxel` and `UpdateCrackMining`'s invalid-voxel early-out both destroy the
  overlay they own, so a missed cancel degrades to "crack until the block breaks" instead of
  a permanent artefact.
- **Material regression.** Mitigation: the mask channel is documented here; an automation
  test cannot currently cover material wiring, so the validation below is manual/measured.

## Performance Implications
- **CPU**: no per-frame work. While mining, one Blueprint call per mining tick plus one
  `GetVoxelAt` lookup; one texture parameter write per stage transition (≤ 10 per block).
- **Memory**: one actor and one `UProceduralMeshComponent` (six quads for cubes, four
  double-sided crossed quads for torches) per voxel being mined. Practically one at a time.
- **Load Time**: none; textures are referenced by path from the actor constructor.
- **Network**: not applicable — the overlay is local cosmetic feedback and is not replicated.

## Migration Plan
1. `ABlockCrackActor` + `M_BlockCrack` (done, `b6571ed`).
2. Add the `AWorldGenerator` bridge and drop the `Tick`-driven overlay loop (done).
3. Rewire `BP_HorrorCharacter`: `UpdateCrackMining` on increment, `ClearCrackMining` on the
   real cancel path rather than the dead `ResetBreakEvent` (done).
4. Fix `M_BlockCrack` to read the mask from the texture alpha (done).
5. Next: torch-crossed geometry and multi-voxel mining queues share the same actor; nothing
   else is planned.

## Validation Criteria
Measured in PIE on `Lvl_FirstPersonTest`, holding `IA_Break` (`LeftMouseButton`) aimed at a
Durability-1 Grass block:

- Before mining: 0 `ABlockCrackActor`, `BreakTime = 0.0`.
- `BreakTime = 0.3`: 1 overlay, `CurrentStage = 3`.
- `BreakTime = 0.6`: 1 overlay, `CurrentStage = 6`.
- After the block breaks: the targeted voxel moves to the block below and the overlay
  re-targets it; the broken voxel's overlay is gone.
- On release: `BreakTime = 0.0` and 0 overlays.
- `ClearCrackMining()` from Python: overlay count returns to 0.
- Automation: `MinecraftDeWish.Mining.CrackStageFromProgress` (plus
  `MinecraftDeWish.Mining.AcceptsNegativeHorizontalCoordinates` and
  `MinecraftDeWish.Mining.RejectsBelowWorldAndAir`) pass in a headless run.

## Related Decisions
- `Docs/architecture/adr-0001-lumen-mandatory.md`
- `Docs/architecture/adr-0002-generated-vs-player-voxel-writes.md`
- `Docs/SYSTEMS_REFERENCE.md` §1 and §14
