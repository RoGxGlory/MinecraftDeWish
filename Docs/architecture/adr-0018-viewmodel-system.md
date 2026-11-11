# ADR-0018: First-Person Viewmodel and Tool Equipment System

## Status

Accepted

## Date

2026-11-11

## Deciders

Technical Director, Technical Artist

## References

- `Docs/SYSTEMS_REFERENCE.md` (§6 Baubles, §7 FToolInstance)
- `ADR-0005` (combat/ability architecture, montage notify contract)
- `ADR-0006` (equipment presentation: static-mesh tools on sockets, skeletal arms)
- `ADR-0007` (tools data model: DT_Tools)
- `Source/MinecraftDeWish/Public/EquippedToolActor.h`
- `Source/MinecraftDeWish/Public/PlayerViewModel.h`
- `Source/MinecraftDeWish/Public/ToolAnimNotifies.h`
- `Source/MinecraftDeWish/Public/BaublesSystem.h`
- `.opencode/rules/ue-cpp.md`, `.opencode/rules/ue-blueprint.md`

## Context

### Problem Statement

The overhaul needs first-person tools and weapons with production-quality feedback.
Today `FToolInstance` carries only a 2D icon; there is no viewmodel, no animation and
no equip/unequip flow. The visual identity is Minecraft-like silhouettes with
post-apocalyptic detailing (design pillar D13=B), so heavy skeletal rigs per tool
are wasteful: tools do not deform, and tier differences are mesh/material/value
only. The system must present a skeletal first-person arms mesh with a tool
attached to a socket, play per-tool-class swing montages, drive mining cadence from
animation notifies (ADR-0003), and auto-select the best tool for the targeted
block without flicker at block boundaries.

### Constraints

- PC Windows only, single player, Unreal Engine 5.8.
- Target 60 FPS (16.6 ms frame budget); gameplay systems stay under 4 ms/frame.
- No third-party rendering plugin without an ADR; Chaos physics.
- Studio rule: tools/weapons are StaticMesh, not SkeletalMesh (ADR-0006).
- Studio rule: no hardcoded balance values; static tool data lives in `DT_Tools`
  (ADR-0007). Animation and mesh references are soft references resolved at equip
  time.
- Mining cadence must not desync from the crack overlay (ADR-0003) or durability
  consumption (ADR-0005).

### Requirements

- Skeletal first-person arms mesh with `bOnlyOwnerSee`, attached to the camera.
- Persistent tool actor whose `UStaticMeshComponent` mesh is swapped on equip;
  never spawn/destroy per tool.
- Tool attaches to socket `weapon_r` on the arms skeleton.
- Montages are per tool class (Pickaxe/Shovel/Axe/Hoe/Sword), not per tier,
  with sections Windup/Impact/Recovery.
- Notifies: `AnimNotify_ActionWindow`, `AnimNotify_PlayCue`,
  `AnimNotify_ActionComplete`, `AnimNotifyState_MiningLoop`.
- Layered blend on `spine_01` so locomotion continues under a swing.
- Mining cadence driven by the impact notify calling `QueueBlockBreakAtVoxel`.
- Auto-tool selection emits `OnActiveToolChanged(EToolType)` when the targeted
  block category changes, with a 150-200 ms anti-bounce to avoid flicker at block
  boundaries (ADR-0006; exact timing is a game-feel decision, default 175 ms).
- Pivot convention: grip centre, handle along local +Z.

## Decision

1. **`AEquippedToolActor` is a single, persistent actor** owning one
   `UStaticMeshComponent` (`ToolMeshComponent`). Its mesh is swapped via
   `EquipMesh` / `EquipMeshFromSoftRef` / `Unequip`. State
   (`CurrentToolType`, `CurrentToolTier`, `bIsEquipped`) is recorded on the actor.
   Blueprint hooks `OnToolEquippedBP` / `OnToolUnequippedBP` fire after the swap.
   The actor is never spawned or destroyed per tool, keeping animation state warm
   and avoiding equip hitches.
2. **`APlayerViewModel` is a dedicated first-person actor** owning:
   - `ArmsMeshComponent`: `USkeletalMeshComponent` (soft-ref `ArmsMeshAsset`),
     `bOnlyOwnerSee = bOnlyOwnerSeeArms`, attached to the camera.
   - `EquippedToolActor`: a persistent `AEquippedToolActor` spawned in `BeginPlay`
     and attached to socket `ToolSocketName` (default `weapon_r`).
   - `SwingMontages`: `TMap<EToolType, TSoftObjectPtr<UAnimMontage>>` keyed by
     tool class (not tier).
   - `ToolDefinition`: `UToolDefinition` (DT_Tools wrapper, ADR-0007) for
     mesh/icon/montage resolution.
   - `BaublesSystem`: `UBaublesSystem` whose `OnActiveToolChanged` drives
     `OnActiveToolChangedHandler` → `SyncEquippedToolFromActive`.
   API: `EquipToolMesh`, `UnequipTool`, `PlaySwingMontage`, `StopSwingMontage`,
   `SyncEquippedToolFromActive`, `OnActiveToolChangedHandler`.
3. **Tools and weapons are `StaticMesh`**, not `SkeletalMesh`. No deformation, no
   skinning cost. Pivot at grip centre, handle along local +Z. Animation lives on
   the arms, not the tool (ADR-0006 §1-2).
4. **Montages are per tool class**, not per tier, with sections Windup/Impact/
   Recovery. Tier changes mesh, material and values, not animation
   (ADR-0005 §4). The per-class montage is resolved from `DT_Tools` via
   `FToolInstance::GetSwingMontage` / `FToolRow::SwingMontage`.
5. **Notify-driven mining.** `AnimNotify_ActionWindow` opens the damage/mining
   window and notifies the owning actor if it implements `IToolActionReceiver`.
   `AnimNotify_PlayCue` fires a named VFX/SFX cue (`CueName`) at the correct
   frame. `AnimNotify_ActionComplete` signals full recovery. `AnimNotifyState_MiningLoop`
   sustains the mining loop (begin/tick/end) while the section plays. The impact
   notify calls `QueueBlockBreakAtVoxel` so the crack overlay (ADR-0003) and
   durability consumption cannot desync from the visual (ADR-0006 §6).
6. **Layered blend on `spine_01`** via `LayeredBlendPerBone` so locomotion
   continues under a swing. Full-body first, layered second (ADR-0006 §5).
7. **Auto-tool selection** is owned by `UBaublesSystem`: `RequestAutoToolSelection`
   commits only after `AutoSelectAntiBounceSeconds` (default 0.175 s) of stability;
   `CommitAutoToolSelection` / `CancelAutoToolSelection` allow forcing/cancelling.
   On commit, `OnActiveToolChanged` broadcasts and `APlayerViewModel` swaps the
   mesh via `SyncEquippedToolFromActive`. This avoids flicker at block boundaries
   (ADR-0006 §7).
8. **Data-driven resolution.** Mesh, icon, montage and impact cue come from
   `DT_Tools` through `UToolDefinition` (ADR-0007); `FToolInstance` keeps only
   runtime state (`ToolId`, `ToolType`, `ToolTier`, durability, `InstanceId`).
   Soft references (`TSoftObjectPtr<UStaticMesh>`, `TSoftObjectPtr<UAnimMontage>`)
   are loaded on equip, never hard-coded paths in code (ADR-0007, ADR-0006).

### Architecture Diagram

```
APlayerCharacter (camera)
  └── APlayerViewModel (bOnlyOwnerSee arms)
        ├── USkeletalMeshComponent ArmsMeshComponent  (soft ref ArmsMeshAsset)
        │     └── socket "weapon_r"
        │           └── AEquippedToolActor (persistent)
        │                 └── UStaticMeshComponent ToolMeshComponent  (mesh swapped on equip)
        ├── SwingMontages: TMap<EToolType, TSoftObjectPtr<UAnimMontage>>
        ├── ToolDefinition: UToolDefinition (DT_Tools wrapper)
        └── BaublesSystem: UBaublesSystem
              └── OnActiveToolChanged ──► APlayerViewModel::OnActiveToolChangedHandler
                                          └── SyncEquippedToolFromActive
                                                └── EquipToolMesh (resolve mesh from DT_Tools)
```

Notify flow (per swing montage):

```
PlaySwingMontage(ToolType)
  → montage sections Windup / Impact / Recovery
        Windup  : (no gameplay effect)
        Impact  : AnimNotify_ActionWindow   → opens damage/mining window (IToolActionReceiver)
                  AnimNotify_PlayCue        → VFX/SFX at impact frame
                  impact notify            → QueueBlockBreakAtVoxel (mining cadence)
        Recovery: AnimNotify_ActionComplete → action finished
        hold    : AnimNotifyState_MiningLoop (begin/tick/end) sustains mining
```

### Key Interfaces

**`AEquippedToolActor`** (`Source/.../Public/EquippedToolActor.h`)

- `ToolMeshComponent`: `TObjectPtr<UStaticMeshComponent>` (VisibleAnywhere).
- State: `CurrentToolType` (EToolType), `CurrentToolTier` (EToolTier),
  `bIsEquipped` (bool).
- API: `EquipMesh(UStaticMesh*, EToolType, EToolTier)`,
  `EquipMeshFromSoftRef(const TSoftObjectPtr<UStaticMesh>&, EToolType, EToolTier)`,
  `Unequip()`, `IsEquipped()`.
- Hooks: `OnToolEquippedBP(EToolType, EToolTier)`, `OnToolUnequippedBP()`
  (BlueprintImplementableEvent).

**`APlayerViewModel`** (`Source/.../Public/PlayerViewModel.h`)

- Components: `ArmsMeshComponent` (`TObjectPtr<USkeletalMeshComponent>`),
  `EquippedToolActor` (`TObjectPtr<AEquippedToolActor>`).
- Config: `ArmsMeshAsset` (TSoftObjectPtr<USkeletalMesh>), `ToolSocketName`
  (FName, default `weapon_r`), `bOnlyOwnerSeeArms` (bool, default true).
- Montages: `SwingMontages` (TMap<EToolType, TSoftObjectPtr<UAnimMontage>>).
- Data: `ToolDefinition` (TObjectPtr<UToolDefinition>),
  `BaublesSystem` (TObjectPtr<UBaublesSystem>).
- API: `EquipToolMesh`, `UnequipTool`, `PlaySwingMontage(EToolType, float PlayRate)`,
  `StopSwingMontage(float BlendOut)`, `SyncEquippedToolFromActive`,
  `OnActiveToolChangedHandler(EToolType)`, `GetEquippedToolActor()`.

**`ToolAnimNotifies`** (`Source/.../Public/ToolAnimNotifies.h`)

- `UAnimNotify_ActionWindow` — opens damage/mining window; `ActionTag` (FName).
- `UAnimNotify_PlayCue` — fires VFX/SFX cue; `CueName` (FName).
- `UAnimNotify_ActionComplete` — signals action finished.
- `UAnimNotifyState_MiningLoop` — sustained mining loop (begin/tick/end).

**`UBaublesSystem` auto-select** (`Source/.../Public/BaublesSystem.h`)

- Delegate: `FOnActiveToolChanged` (EToolType).
- State: `ActiveToolType`, `PendingAutoToolType`, `bHasPendingAutoTool`,
  `AutoSelectTimerHandle`.
- Tuning: `AutoSelectAntiBounceSeconds` (float, ClampMin 0.1, ClampMax 0.5,
  default 0.175).
- API: `RequestAutoToolSelection(EToolType)`, `CommitAutoToolSelection()`,
  `CancelAutoToolSelection()`, `GetActiveToolType()`.

**Enums**

- `EToolType`: None, Pickaxe, Shovel, Axe, Hoe, Sword.
- `EToolTier`: None, Wood, Stone, Iron, Diamond, Obsidian.

### Alternatives Considered

- **SkeletalMesh per tool** (one rigged mesh per tool, skinned to the arms):
  rejected. Deformation cost and authoring burden for no visual gain at the
  chosen silhouette style (D13=B); tools do not bend. StaticMesh swap gives the
  same look at a fraction of the cost.
- **Spawn/destroy tool actors per equip**: rejected. Equip hitches and loss of
  animation state; persistent actor keeps the swap instant and the transform
  warm (ADR-0006 §2).
- **Per-tier montages** (one montage per tool+tier): rejected. Tier changes
  mesh/material/values, not animation; 5 montages instead of 25, and the
  animation set stays stable across balance passes (ADR-0005 §4).
- **Blueprint timer-driven mining cadence**: rejected. Desyncs the crack overlay
  (ADR-0003) and durability consumption from the visual; the impact notify is
  the single source of truth (ADR-0006 §6).
- **Immediate auto-tool switch on block change (no anti-bounce)**: rejected.
  Flickers at block boundaries where the targeted category changes every frame;
  the 150-200 ms stability window (default 175 ms) commits only sustained
  changes (ADR-0006 §7).

## Consequences

### Positive

- One draw call per tool, no skinning: 25 meshes stay cheap; equip is instant.
- Animation set is 5 montages (per tool class), not 25 (per tool+tier).
- Persistent tool actor keeps animation state warm and avoids equip hitches.
- Notify-driven mining keeps crack overlay, durability and visual in sync.
- Layered blend lets locomotion continue under a swing.
- Auto-select anti-bounce removes flicker at block boundaries.
- Mesh/montage/cue resolution is data-driven via `DT_Tools`, so designers tune
  without recompiling (ADR-0007).

### Negative

- Tools cannot deform (acceptable for the chosen silhouette style); tier
  differences are mesh/material/value only.
- Montages need assembly: sections Windup/Impact/Recovery and notifies must be
  authored per tool class; a missing notify silently breaks mining cadence.
- Mesh/montage soft references load on equip; first equip of a new tool can hitch
  if not preloaded.
- `IToolActionReceiver` must be implemented by any actor that wants action
  windows; missing implementation means no damage/mining effect.
- Two sources of truth for tool identity (actor state + Baubles slot) must be
  kept in sync via `SyncEquippedToolFromActive`.

### Risks

- **Double durability consumption**: notify-driven mining (ADR-0003) plus the
  existing `Progress01`/`GetBlockBreakTimeAtVoxel` path could both consume
  durability. Mitigation: integration test required (ADR-0006); mining is
  committed only from the impact notify.
- **Notify timing desync**: if the Impact notify is placed off-frame, mining
  cadence drifts from the visual. Mitigation: author notifies at the impact frame
  and validate in PIE with `stat unit`.
- **Mesh load hitch on first equip**: soft ref `LoadSynchronous` in
  `EquipMeshFromSoftRef`. Mitigation: preload tool meshes at level load or
  restrict to async load with a fallback.
- **Socket transform mismatch**: tool pivot must be grip centre, handle along
  +Z; a wrong pivot makes the tool float or clip. Mitigation: asset creation
  guide + in-editor preview.
- **Auto-select bounce tuning**: 175 ms default may feel sluggish or still flicker;
  the value is a game-feel knob (ClampMin 0.1, ClampMax 0.5) for the game
  designer.
- **`bOnlyOwnerSee` arms seen by others in multiplayer**: out of scope for
  single-player; if multiplayer ships, revisit visibility (ADR-0006 §3).

## Performance Implications

- One draw call per equipped tool; no skinning on tools (StaticMesh only).
- Arms skeletal mesh is a single `USkeletalMeshComponent` with `bOnlyOwnerSee`
  (rendered for one player only in single-player).
- Montage playback cost is per-actor and bounded to one active montage.
- Soft references defer mesh/montage load; first equip may cost a frame; prefer
  async load + preload for shipped content.
- No `Tick` on the viewmodel; updates are event-driven (equip, montage notify,
  auto-select commit).
- Auto-select timer is a single `FTimerHandle` per `UBaublesSystem`, fired at
  most once per stability window.

## Migration Plan

1. Implement `AEquippedToolActor` with mesh swap and Blueprint hooks (done).
2. Implement `APlayerViewModel` with arms mesh, socket attachment, montage
   playback and Baubles sync (done).
3. Implement `ToolAnimNotifies` (ActionWindow, PlayCue, ActionComplete,
   MiningLoop) (done).
4. Wire `UBaublesSystem::OnActiveToolChanged` to viewmodel sync with 175 ms
   anti-bounce (done).
5. Author per-tool-class montages with Windup/Impact/Recovery sections and
   notifies; assign soft refs in `DT_Tools`.
6. Validate notify-driven mining against `Progress01`/`GetBlockBreakTimeAtVoxel`
   (ADR-0003) in an integration test (no double durability).
7. Preload tool meshes/montages at level load to remove first-equip hitches.
8. Add automation tests: equip swap, montage play/stop, auto-select anti-bounce,
   notify fires at impact.

## Validation Criteria

- Equip swaps the mesh instantly without hitches after preload.
- Arms render only for the owning player (`bOnlyOwnerSee`).
- Tool attaches to `weapon_r` with correct pivot (grip centre, handle +Z).
- Swing plays the per-class montage; Impact notify opens the mining window and
  triggers `QueueBlockBreakAtVoxel`.
- Crack overlay and durability stay in sync with the visual impact frame.
- `MiningLoop` sustains mining while the section plays and ends cleanly.
- Layered blend keeps locomotion playing under a swing.
- Auto-select commits only after 175 ms of stable candidate type; no flicker at
  block boundaries.
- Mesh/icon/montage resolve from `DT_Tools`; no hard-coded paths in code.
- Automation tests pass for equip swap, montage playback, anti-bounce and notify
  firing.

## Related Decisions

- `ADR-0005` — combat/ability architecture (montage notify contract, damage
  funnel).
- `ADR-0006` — equipment presentation (static-mesh tools on sockets, skeletal
  arms, montage contract, auto-select).
- `ADR-0007` — tools data model (`DT_Tools` replaces hardcoded values).
- `ADR-0003` — block crack overlay driver (notify-driven mining).
- `design/gdd/` — tool/weapon design docs (when authored).
- `Docs/SYSTEMS_REFERENCE.md` §6 (Baubles), §7 (FToolInstance).
