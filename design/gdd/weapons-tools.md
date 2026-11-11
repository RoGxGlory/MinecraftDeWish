# GDD — Weapons & Tools

| Field | Value |
|---|---|
| Status | Draft (2026-11-11) |
| System owner | game-designer / gameplay-programmer / technical-artist |
| Related | `Docs/TOOL_REGISTRY.md`, `Docs/SYSTEMS_REFERENCE.md` §6–7, `design/gdd/items.md`, `design/gdd/game-concept.md` |
| Source of truth for current code | `ToolTypes.h`, `BaublesSystem.h/.cpp`, `EquippedToolActor.h`, `PlayerViewModel.h`, `PlayerActionComponent` (ADR-0005), `DT_Tools` (ADR-0007) |
| Decisions | ADR-0005 (custom action layer), ADR-0006 (static-mesh presentation), ADR-0007 (DT_Tools data model), ADR-0003 (crack overlay driver) |

---

## 1. Overview

MinecraftDeWish uses a **dedicated five-slot equipment model** (`UBaublesSystem`): one slot
per tool class — Pickaxe, Shovel, Axe, Hoe, Sword. Each slot holds a single `FToolInstance`
identified by a stable `ToolId` resolved through the `DT_Tools` DataTable (ADR-0007). There are
**5 tool classes × 5 tiers = 25 tools**, and a tool's tier changes only its mesh, material and
numeric values — never its animation set (ADR-0006).

Tools are **StaticMesh actors** (`AEquippedToolActor`) whose mesh is swapped on equip, attached to
the `weapon_r` socket of a first-person skeletal arms mesh (`APlayerViewModel`). All gameplay
actions (mining, swinging, damage windows, durability) are driven by a **custom action layer**
(`UPlayerActionComponent` + `UCombatSubsystem`) behind a single damage funnel, keeping a future
GAS migration mechanical (ADR-0005).

Two consumers share this system:

- **Mining** — `UMiningQueueSystem` + `AWorldGenerator` query `UBaublesSystem` for the best
  mining force and consume durability on a successful break.
- **Combat** — `UCombatSubsystem::ApplyDamage` → `AMobCharacter::ApplyMobDamage`, driven by
  animation notifies, with knockback and hit reactions.

**Auto-tool selection** emits `OnActiveToolChanged(EToolType)` when the targeted block/mob
category changes, with a 175 ms anti-bounce to avoid flicker at block boundaries.

This document is the design source of truth for the tool/weapon layer. Implementation details
live in `Docs/SYSTEMS_REFERENCE.md` §6–7 and `Docs/TOOL_REGISTRY.md`.

---

## 2. Player Fantasy

> *"The right tool for the right block, in the hand I expect, doing exactly what I
> predicted."*

The fantasy is **muscle memory and reliability**, not power fantasy. The player should feel
that:

- **One glance, one tool.** Looking at stone means a pickaxe is in hand before the player
  thinks about it. No inventory fumbling, no "did I equip the right thing".
- **Tier is visible, not announced.** A diamond pickaxe reads as the only "pure" cold
  white-blue material in a world of scavenged, taped, wire-wrapped tools (art-direction §7).
  The player *feels* the upgrade through break speed and durability, never through a number.
- **Swings have weight.** A 5-set animation library (pickaxe/shovel/axe/hoe/sword) with
  Windup/Impact/Recovery sections and a layered blend on `spine_01` so locomotion continues
  under a swing. Impact lands on a notify, not on a timer, so the crack overlay and durability
  can never desync from the visual.
- **Combat is a last resort.** Swords exist and hit hard, but the horror pacing (Angels are
  not fought) means combat is for clearing mobs that gate a path, not for fun. Hit feedback is
  crisp and short: one stagger, one knockback, done.
- **Nothing is wasted.** A broken tool is unusable but **not destroyed** — it can be repaired
  at a crafting table. A bad night costs materials and time, never a diamond pickaxe.

---

## 3. Detailed Rules

### 3.1 Equipment model (Baubles)

- `UBaublesSystem` owns five `FToolInstance` slots: `PickaxeSlot`, `ShovelSlot`,
  `AxeSlot`, `HoeSlot`, `SwordSlot`.
- `EquipTool(const FToolInstance&)` routes the tool to its slot by `ToolType`. Re-equipping
  into a non-empty slot returns the previous tool (swap).
- `UnequipTool(EToolType)` removes and returns the tool; the slot becomes empty (hand fallback).
- `ToggleBaublesUI()` opens/closes the Baubles panel (`WBP_Baubles_Root`) and broadcasts
  `OnBaublesToggled`.
- Delegates: `OnToolEquipped`, `OnToolUnequipped`, `OnBaublesToggled`.
- `AEquippedToolActor` (one instance, never spawned/destroyed) swaps its
  `UStaticMeshComponent` mesh on equip. Pivot at grip centre, handle along local +Z.
- First-person arms (`APlayerViewModel`, `USkeletalMeshComponent`, `bOnlyOwnerSee`) attach the
  tool to socket `weapon_r`.

### 3.2 Tool data model (DT_Tools)

Per ADR-0007, all static tool data lives in the `DT_Tools` DataTable keyed by a stable
`ToolId` (`FName`, format `Tool.<Class>.<Tier>` e.g. `Tool.Pickaxe.Diamond`).

Columns:

| Column | Type | Notes |
|---|---|---|
| `ToolType` | `EToolType` | Pickaxe / Shovel / Axe / Hoe / Sword |
| `ToolTier` | `EToolTier` | None / Wood / Stone / Iron / Diamond / Obsidian |
| `BaseMiningForce` | `float` | Tier multiplier (see §4) |
| `DurabilityMax` | `int32` | From TOOL_REGISTRY |
| `AttackDamage` | `float` | Sword total damage (fist + bonus) |
| `MeshPath` | soft ref | StaticMesh |
| `IconPath` | soft ref | 32×32 Patrix icon |
| `SwingMontage` | soft ref | Per *class*, not per tier |
| `ImpactCue` | soft ref | VFX/SFX at impact frame |
| `MiningForceByBlockCategory` | `TMap<EBlockCategory, float>` | Specialisation multipliers (default 1.0) |

`FToolInstance` keeps only runtime state: `ToolId`, `ToolType`, `ToolTier`,
`CurrentDurability`, `MaxDurability` (cached), `InstanceId` (stable GUID for save data).
`UBaublesSystem` resolves through a `UToolDefinition` DataAsset wrapper with caching.

### 3.3 Auto-tool selection

- When the targeted block (or mob) category changes, `UBaublesSystem` emits
  `OnActiveToolChanged(EToolType)`.
- If the player owns a tool for that category, it is auto-equipped; otherwise the hand
  (no tool) is used and no durability is consumed.
- **Anti-bounce:** changes occurring within `AutoSelectDebounceMs = 175 ms` of the previous
  change are ignored, preventing flicker at block boundaries.
- Manual equip via the Baubles UI sets a short-lived override (`ManualOverrideMs = 500 ms`)
  during which auto-selection does not fire; it resumes on the next distinct target category
  after the override expires.
- Target categories: Stone (Pickaxe), Earth (Shovel), Wood (Axe), Plant/Farmland (Hoe),
  Mob (Sword). Air/no-target holds the current tool.

### 3.4 Mining

- Break time: `BreakTime = BlockDurability / EffectiveMiningForce` (seconds), computed by
  `AChunkActor::GetBlockBreakTimeAtVoxel` and `AWorldGenerator::GetBlockBreakTimeAtVoxel`.
- `EffectiveMiningForce`:
  - Hand: `1.0`.
  - Matching equipped tool: `BaseMiningForce × MiningForceByBlockCategory[BlockCategory]`.
  - Non-matching equipped tool: `1.0` (hand speed) and **no durability consumed**.
- `UMiningQueueSystem` queues the break; the number of concurrent queue slots is tier-gated
  (see §4, `MaxQueueSlots`).
- `QueueBlockBreakAtVoxel` is called from the animation impact notify (ADR-0006 §6), not from
  the Blueprint timer, so the crack overlay (ADR-0003) and durability consumption cannot desync
  from the visual.
- On a successful break, `AWorldGenerator::BreakBlockAtVoxel` calls
  `UBaublesSystem::ConsumeToolDurability(BlockID)`, deducting 1 durability from the matching
  tool. Non-matching tools are not touched.
- Crack overlay (`ABlockCrackActor`) advances `Progress01 = BreakTime / GetBlockBreakTimeAtVoxel(...)`
  and selects stage `Floor(Clamp(Progress,0,1) × 10)` ∈ [0,9].

### 3.5 Combat

- Only the Sword class deals combat damage; other tools deal fist damage (`1.0`) if used to
  hit a mob (not recommended, no bonus).
- Swing is driven by the per-class montage (Sword montage, 5 swing variants via sections).
- `AnimNotify_ActionWindow` opens a damage window during Impact; `UCombatSubsystem` performs an
  arc sweep and calls `ApplyDamage` on every mob in the volume.
- Hit detection: forward cone, range `CombatRange = 250 cm`, half-angle `CombatHalfAngle = 45°`,
  evaluated against mob capsules. Multiple mobs in the volume are all hit.
- Damage dealt: `AttackDamage` of the equipped sword (or `1.0` fist).
- Durability: `−1` per mob hit (only if a sword is equipped; fist costs nothing).
- Knockback: horizontal impulse away from the player, magnitude `KnockbackImpulse` (see §4),
  applied to the mob's movement component. Vertical component clamped to avoid launching mobs
  over walls.
- Hit reaction: mobs play a short stagger (`StaggerDuration = 0.10 s`) only if the single hit
  deals ≥ `StaggerThreshold = 4.0` HP; otherwise no reaction (avoids stun-locking with weak
  weapons).
- `AnimNotify_PlayCue` fires VFX/SFX at the correct frame (impact spark, swing whoosh).
- `AnimNotify_ActionComplete` ends the action and re-enables input buffering.

### 3.6 Durability & repair

- Mining a block: `−1` durability on the matching tool.
- Hitting a mob with a sword: `−1` durability.
- At durability `0` the tool is **broken**: unusable (treated as hand for mining/combat), auto-unequipped,
  `OnToolUnequipped` fired. The `FToolInstance` is retained (not destroyed) so it can be repaired.
- Repair at a crafting table: tool + material unit → `+25%` of `MaxDurability` per unit, capped at
  `MaxDurability`. Material per tier: Wood → planks, Stone → cobblestone, Iron → iron ingot,
  Diamond → diamond, Obsidian → obsidian (or netherite scrap). Units needed:
  `ceil((MaxDurability − CurrentDurability) / (MaxDurability × 0.25))`.
- A fully broken tool needs 4 units.

### 3.7 Montages & notifies

Per ADR-0006, montages are **per tool class (5)**, not per tier. Each montage has three sections:

| Section | Purpose |
|---|---|
| `Windup` | Anticipation frames; input can be buffered but not cancelled |
| `Impact` | Damage/mining window opens here |
| `Recovery` | Return to idle; input accepted at end |

Notifies:

| Notify | Type | Function |
|---|---|---|
| `AnimNotify_ActionWindow` | one-shot | Opens the damage/mining window for the duration of Impact |
| `AnimNotify_PlayCue` | one-shot | Spawns VFX/SFX at the correct frame |
| `AnimNotify_ActionComplete` | one-shot | Ends the action, clears state |
| `AnimNotifyState_MiningLoop` | state (begin/end) | Drives repeated mining cadence while held |

Mining cadence is driven by the impact notify, not by the Blueprint timer, so the crack
overlay and durability consumption stay in sync with the visual (ADR-0006 §6).

Layered blend on `spine_01` via `LayeredBlendPerBone` so locomotion continues under a swing
(full-body first, layered second).

---

## 4. Formulas

### 4.1 Tier stat table

| Tier | `BaseMiningForce` | `MaxDurability` | Sword `AttackDamage` | `MaxQueueSlots` | `KnockbackImpulse` (cm/s) |
|---|---|---|---|---|---|
| None (Hand) | 1.0 | ∞ | 1.0 (fist) | 1 | 0 |
| Wood | 2.0 | 59 | 3.0 (+2.0) | 2 | 320 |
| Stone | 4.0 | 131 | 4.0 (+3.0) | 3 | 360 |
| Iron | 6.0 | 250 | 5.0 (+4.0) | 4 | 400 |
| Diamond | 10.0 | 1,561 | 6.0 (+5.0) | 5 | 440 |
| Obsidian | 15.0 | 2,031 | 8.0 (+7.0) | 6 | 480 |

### 4.2 Mining speed

```
EffectiveMiningForce = BaseMiningForce(ToolTier) × MiningForceByBlockCategory[BlockCategory]
                      (if matching tool equipped and block category matches the tool's affinity)
                    = 1.0  (hand, or non-matching tool — no durability consumed)
```

```
BreakTime (s) = BlockDurability / EffectiveMiningForce
```

Example: Stone block (durability 1.50) with Iron Pickaxe (6.0×) → 1.50 / 6.0 = 0.25 s.

### 4.3 Damage

```
DamageDealt = AttackDamage(EquippedSwordTier)   if sword equipped
            = 1.0                              otherwise (fist)
```

No armour reduction in v1 (mobs have no armour). Damage is applied as a single hit per
swing window, not per frame.

### 4.4 Durability

```
OnBreak:  CurrentDurability -= 1   (matching tool only)
OnHit:    CurrentDurability -= 1   (sword only, per mob hit)
Broken:   CurrentDurability == 0  → unusable, auto-unequip
Repair:   CurrentDurability = min(MaxDurability, CurrentDurability + 0.25 × MaxDurability)
UnitsNeeded = ceil((MaxDurability - CurrentDurability) / (MaxDurability × 0.25))
```

### 4.5 XP

```
XPPerMobKill  = 5   (Zombie, Skeleton, Spider, Creeper — see MOB_REGISTRY)
XPPerBlock    = 1   (proposed; ores grant 2 — see §7 tuning)
LevelCurve:   XPToNextLevel = 7 + (Level × 3)   (UPlayerVitalSystem)
```

Mining XP is granted on `BreakBlockAtVoxel` success, scaled by block category (stone/ore
worth more than dirt). Values are tuning knobs (§7).

### 4.6 Knockback

```
KnockbackVector = normalize(MobPos - PlayerPos) × KnockbackImpulse(Tier)
                + (0, 0, min(VerticalCap, Impulse × 0.25))
VerticalCap = 150 cm/s
```

Applied as a one-shot velocity delta to the mob's movement component; not accumulated
across multiple hits in the same frame.

### 4.7 Auto-selection debounce

```
if (Now - LastAutoSelectTime < AutoSelectDebounceMs) → ignore
else → evaluate target category → equip matching tool → LastAutoSelectTime = Now
```

---

## 5. Edge Cases

| Case | Resolution |
|---|---|
| Target category changes within 175 ms | Ignored (anti-bounce); current tool stays |
| No tool owned for the target category | Hand used; no durability consumed; no auto-equip |
| Manual equip during auto-selection window | Manual override for 500 ms; auto-selection resumes after |
| Mining with a non-matching tool | Hand speed (1.0×), no durability consumed, no queue-slot bonus |
| Durability reaches 0 mid-mining | Tool breaks, is auto-unequipped; the in-flight break still completes (queued break resolves) |
| Broken tool equipped manually | Treated as hand; cannot mine faster or deal bonus damage |
| Sword swing hits 0 mobs | No durability consumed; montage still plays through Recovery |
| Sword swing hits multiple mobs | Each takes full `AttackDamage`; durability `−1` total per swing (not per mob) — **decision point, see §7** |
| Mob dies during the damage window | `ApplyDamage` returns ≤ 0; death handled by `AMobCharacter`; remaining targets still processed |
| Mining while moving | Allowed; locomotion continues via layered blend on `spine_01` |
| Tool swap while a swing is in progress | Swap is queued and applied at `AnimNotify_ActionComplete`; never mid-swing |
| `AEquippedToolActor` mesh swap failure (missing asset) | Falls back to a default mesh; validation test fails the build (ADR-0007 §5) |
| Save data references a removed `ToolId` | Migration: slot cleared, logged; tool becomes hand |
| Obsidian tool on a Wood block | Non-matching → hand speed, no durability; auto-selection would have swapped to Axe anyway |
| Repairing a fully broken tool | Allowed; `UnitsNeeded = 4` |
| Repair at max durability | No-op; UI disables the repair action |

---

## 6. Dependencies

| System | Why | Status |
|---|---|---|
| `UBaublesSystem` | 5-slot equipment, delegates, auto-selection host | Implemented |
| `FToolInstance` / `DT_Tools` / `UToolDefinition` | Tool data, durability runtime state | Migrating (ADR-0007) |
| `AEquippedToolActor` | StaticMesh swap on equip | Implemented |
| `APlayerViewModel` | Skeletal arms, `weapon_r` socket | Implemented |
| `UPlayerActionComponent` | Montage driver, notifies, damage windows | To implement (ADR-0005) |
| `UCombatSubsystem` | Damage funnel `ApplyDamage` | To implement (ADR-0005) |
| `UMiningQueueSystem` | Concurrent break queue, slot gating | Implemented |
| `AWorldGenerator` | BreakBlockAtVoxel, QueueBlockBreakAtVoxel, crack overlay | Implemented |
| `AChunkActor` | GetBlockBreakTimeAtVoxel, hand/mining force | Implemented |
| `UPlayerVitalSystem` | Health, XP, death | Implemented |
| `AMobCharacter` | ApplyMobDamage, stagger, knockback, death | Implemented |
| `ABlockCrackActor` | Progress overlay (ADR-0003) | Implemented |
| Animation montages & notifies | 5 swing sets, notifies | To author |
| `UCraftingSmeltingSystem` | Tool crafting recipes, repair | Implemented (block-only, needs ingredients) |
| `design/gdd/items.md` | Durability repair, item registry | Draft |

---

## 7. Tuning Knobs

| Knob | Default | Safe range | Effect |
|---|---|---|---|
| `AutoSelectDebounceMs` | 175 ms | 100–300 ms | Anti-bounce at block boundaries |
| `ManualOverrideMs` | 500 ms | 200–1000 ms | Manual equip stickiness |
| `CombatRange` | 250 cm | 180–320 cm | Sword reach |
| `CombatHalfAngle` | 45° | 30–60° | Swing arc width |
| `StaggerThreshold` | 4.0 HP | 2.0–6.0 HP | When mobs flinch |
| `StaggerDuration` | 0.10 s | 0.05–0.20 s | Flinch length |
| `KnockbackImpulse` (per tier) | see §4.1 | ±20 % | Push strength |
| `VerticalCap` | 150 cm/s | 100–250 cm/s | Anti-launch clamp |
| `RepairFractionPerUnit` | 0.25 | 0.10–0.50 | Repair economy |
| `XPPerBlock` | 1 | 0–3 | Mining XP faucet |
| `XPOreMultiplier` | 2 | 1–4 | Ore XP bonus |
| `BaseMiningForce` (per tier) | see §4.1 | ±25 % | Mining speed |
| `DurabilityMax` (per tier) | see §4.1 | ±50 % | Tool lifespan |
| `MaxQueueSlots` (per tier) | see §4.1 | 1–8 | Concurrent mining |
| `MiningForceByBlockCategory` | 1.0 default | 0.5–2.0 | Specialisation |

**Open decision (flagged for game-designer):** whether a single sword swing that hits N mobs
costs 1 durability total (current proposal) or N durability. 1 total rewards crowd control;
N makes crowd fights expensive. Recommend 1 total for v1, revisit after playtest.

---

## 8. Acceptance Criteria

1. Equipping a tool routes it to the correct Baubles slot and swaps the mesh on
   `AEquippedToolActor` without spawning/destroying actors.
2. Auto-selection equips the matching tool within 175 ms of a target category change, and
   does not flicker at block boundaries (debounce holds).
3. Mining a matching block consumes exactly 1 durability on the matching tool; a
   non-matching tool consumes 0 and mines at hand speed.
4. `BreakTime` matches `BlockDurability / EffectiveMiningForce` within 1 frame; the crack
   overlay reaches stage 9 on the same frame as the break.
5. A sword swing within its damage window deals `AttackDamage` to every mob in the cone
   and applies knockback; mobs below `StaggerThreshold` do not flinch.
6. At durability 0 a tool is unusable, auto-unequipped, retained (not destroyed), and
   repairable for 4 material units.
7. Repair restores 25 % of `MaxDurability` per unit, capped at `MaxDurability`.
8. All 25 `DT_Tools` rows resolve their mesh, icon and montage; a missing asset fails the
   build (ADR-0007 §5).
9. Montages are per class (5 sets), with Windup/Impact/Recovery sections and the four
   notifies; mining cadence is notify-driven, not timer-driven.
10. Save/load round-trips `ToolId` + `InstanceId` + `CurrentDurability` for all 5 slots.
11. An automation test covers: matching vs non-matching mining, durability consumption,
    auto-selection debounce, and the damage window open/close.
12. A broken tool loaded from a legacy save is migrated to `CurrentDurability = 0` without
    crash.