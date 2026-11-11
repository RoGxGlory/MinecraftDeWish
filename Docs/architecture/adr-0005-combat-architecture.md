# ADR-0005 — Combat and ability architecture: custom action layer with GAS-ready contract

**Status:** Accepted
**Date:** 2026-11-10
**Deciders:** Technical Director, with Game Designer
**References:** Docs/SYSTEMS_REFERENCE.md (§6 Baubles, §8 PlayerVitalSystem), .opencode/rules/ue-cpp.md

## Context

The studio rule states that combat, buffs and stats go through GAS "once the
relevant system is GAS-based". Today nothing is GAS: `UPlayerVitalSystem` and
`AMobCharacter` manage plain floats, and `FToolInstance` stores hardcoded tier
multipliers (`GetBaseMiningForce`). The overhaul needs tools, weapons, combat
feedback and (later) mob status effects and Weeping Angel states. The question is
whether to adopt GAS now or build a custom action layer that can be migrated
later.

## Decision

1. **Build a custom action layer now (Option C).** `UPlayerActionComponent` plus
   `UToolDefinition` (DataAsset) drive montages, notifies, damage windows,
   cooldowns and durability. All damage flows through a single funnel
   (`UCombatSubsystem::ApplyDamage` → `AMobCharacter::ApplyMobDamage`), so a
   future GAS migration has one replacement point.
2. **Run a 3-day GAS spike before mob behaviors ship.** The spike evaluates
   `UAbilitySystemComponent` + `UAttributeSet` + `UGameplayEffect` + tags on a
   single mob. If status effects (sunlight burn, creeper fuse, Angel states)
   prove cheaper in GAS, migrate at that point. Criteria for switching are
   recorded in this ADR's consequences.
3. **Migrate `FToolInstance` to a DataTable.** `DT_Tools` holds tier multipliers,
   max durability, damage, mesh reference and icon. `FToolInstance` keeps only
   runtime state (type, tier, durability, stable `InstanceId` for save data).
   This corrects the existing violation of "no hardcoded balance values".
4. **Montages are per tool class, not per tier.** Sections Windup/Impact/Recovery,
   with `AnimNotify_ActionWindow`, `AnimNotify_PlayCue`, `AnimNotify_ActionComplete`
   and `AnimNotifyState_MiningLoop`. The tier changes mesh, material and values,
   not animation.

## Consequences

- **Positive:** weapons ship in 1-2 weeks; zero plugin dependency; one damage
  funnel makes migration mechanical.
- **Negative:** status effects must be hand-rolled until the spike decides.
- **Migration triggers (switch to GAS if any holds):** more than 3 concurrent
  status effects; need for gameplay prediction in multiplayer; ability
  cooldowns interacting with animation cancels.
- **Risk:** reinventing GAS poorly. Mitigated by keeping the funnel and the
  attribute struct behind an interface.

## Alternatives considered

- **GAS now:** rejected for now. Two weeks of migration before the first visible
  weapon, and a plugin dependency, for a sandbox where 90% of "abilities" are
  "mine a block". Re-evaluate at the spike.
- **Custom forever:** rejected. Status effects for Weeping Angels and mobs will
  need a real model; planning for migration costs little.