# P1 Remaining Tasks — Execution Plan

**Date:** 2026-11-11
**Owner:** Technical Director
**Status:** Approved by player

## Tasks

### T1 — PIE test + benchmark (Gate, P0 priority)
- Launch editor, open Lvl_FirstPersonTest, PIE 60s, capture CSV
- Compare to baseline (avg ≤ 16.6ms, p99 ≤ 16.75ms)
- Profile with Unreal Insights if p99 > 16.6ms
- **Deliverable:** CSV + rapport before/after

### T2 — Re-import 16 refined textures
- Re-import N/S PNGs into existing .uasset (drag-and-drop, same path)
- Verify MI_* recompile, no missing refs
- **Deliverable:** textures up to date in editor

### T3 — Mob LODs (3 levels, dithered fade)
- On SK_Mob_Base: add LOD1 (~800 tris), LOD2 (~200 tris), dithered fade ON, Nanite OFF
- Verify in PIE at distance
- **Deliverable:** SK_Mob_Base with 3 LODs

### T4 — Assemble montages (sections, notifies, slots)
- On 5 M_Swing_*: sections (Windup/Impact/Recovery), slots (DefaultSlot), notifies (ActionWindow, PlayCue, ActionComplete, MiningLoop)
- Assemble ABP_FP_Arms (state machine + layered blend)
- Verify compilation + notifies match C++ classes
- **Deliverable:** 5 montages assembled, ABP_FP_Arms functional

### T5 — Weather verify/assign
- Assets already created (NS_Rain, NS_AshStorm, M_PP_Fog, MI_PP_Fog)
- Verify SpawnRate param (exact name "SpawnRate")
- Assign soft refs on DayNightCycleSystem
- PIE: test weather states, indoor suppression, particle count ≤ 2000
- **Deliverable:** weather system functional in PIE

### T6 — Mood board adjustments (A1-A6)
- A4 (fog tuning) first → A1 (Angel at 60b) → A3 (torch luminance) → A5 (cave void)
- A2 and A6 already closed
- **Deliverable:** Angel_60b_fog.png + final lock mood board

## Execution Order

1. Vague 0: Build Test verification + source sequences for montages
2. Vague 1: T2, T3, T4, T5, A3, A4, A5 (parallel, editor)
3. Vague 2: T1 (benchmark) + A4 (fog tuning) in parallel
4. Vague 3: A1 (Angel 60b) after A4 → final lock

## Success Criteria

- T1: PIE runs, avg ≤ 16.6ms, p99 ≤ 16.75ms, CSV captured
- T2: 16 .uasset textures up to date, MI recompiled
- T3: SK_Mob_Base has 3 LODs, dithered fade ON
- T4: 5 montages with sections + notifies + slots; ABP_FP_Arms compiles
- T5: soft refs assigned; weather states tested in PIE; SpawnRate scales; particles ≤ 2000
- T6: A1 captured and passes; A3 ratio ≥ 2x; A4 fog 60-90b; A5 cave void; final lock mood board

## Dependencies

- A4 (fog) → A1 (Angel 60b) [hard dependency]
- T4 (montages) → source sequences (Vague 0)
- T1 (benchmark) → build Test green (Vague 0)