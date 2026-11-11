# Performance Baseline Plan

**Project:** MinecraftDeWish (Unreal Engine 5.8, Windows PC, single-player)
**Status:** Draft — P0 deliverable (sprint-00 T3)
**Owner:** Performance Analyst (with Technical Director)
**Date:** 2026-11-10
**References:**
- `Docs/architecture/voxel-performance-plan.md` (lots, budgets, measurement protocol)
- `Docs/architecture/adr-0001-lumen-mandatory.md` (hardware floor, Lumen policy)
- `Source/MinecraftDeWish/Public/VoxelBenchSubsystem.h` (per-frame CSV recorder)
- `production/sprints/sprint-00.md` (T3)

This document defines the **reproducible baseline** that every later optimisation claim
(lots 0–5 of the voxel performance plan) is measured against. No lot's gain is accepted
without two CSV captures (before / after) taken on the same machine with the same label,
per `voxel-performance-plan.md` §4.

---

## 1. Purpose and scope

- Establish a **chiffrée, repeatable baseline** of frame time, draw calls, memory and VRAM
  for the eight scenarios below.
- Provide a **CI gate** that fails a build when a scenario's p95 frame time (or other
  tracked metric) regresses past its threshold.
- Cover both **voxel streaming** (the known bottleneck) and the **gameplay systems** added
  by the visual overhaul (combat, destruction, torch lighting).
- Out of scope: network (single player), load-time profiling of individual assets (covered
  by the asset audit), and non-PC platforms.

---

## 2. Reference environment

### 2.1 Hardware (minimum supported tier, ADR-0001)

| Component | Specification |
| --- | --- |
| GPU | NVIDIA GTX 1060 6 GB (reference). Secondary check: RX 580 8 GB. |
| CPU | 4 cores / 8 threads (Intel i5-8400 / Ryzen 5 1600 class or better) |
| RAM | 8 GB (close background apps; target < 6 GB in use before launch) |
| Storage | SSD (NVMe preferred); no HDD baseline accepted |
| Display | 1920×1080, 60 Hz, fullscreen (not windowed) |
| OS | Windows 10 22H2 or Windows 11, fully updated, no overlays |

### 2.2 Software / build

| Item | Value |
| --- | --- |
| Engine | Unreal Engine 5.8 (pinned in `MinecraftDeWish.uproject`) |
| Build config | **Test** (keeps stats and trace; Shipping is for final validation only) |
| Scalability preset | `@1` (Low) for the reference run; `@0` (Potato) is a separate floor check |
| Render distance | 8 chunks (default in code; `GetRenderDistance()`) |
| World height | 64 (current); 192 only after lots 1–3 |
| VSync | Off (`r.VSync 0`) |
| Frame rate cap | Off (`t.MaxFPS 0`) |
| Resolution scale | 100 % (`r.ScreenPercentage 100`) |
| Lumen | On (mandatory, ADR-0001) — verify with `stat lumen` |
| Hardware RT | Off (`r.RayTracing 0`, recommended default) |
| Seed | Fixed benchmark seed (document the seed value in the run log) |
| Save state | Fresh profile, no existing save, no mods |

### 2.3 Absolute floor check (separate, non-gating)

Run the same scenarios on a GTX 1050 Ti / RX 570 at preset `@0`, 50–60 % resolution
scale, render distance 4, targeting 30 fps. This is a **smoke** check, not a gate.

---

## 3. Measurement protocol

### 3.1 Build and launch

1. Build the **Test** target (`MinecraftDeWishEditor` is not used; use the game target).
2. Launch with the profiling flags in §7.1 (at minimum `-trace=cpu,gpu,counters,memory,bookmark`).
3. Confirm the GPU is the GTX 1060 in `stat unit` / Task Manager before recording.
4. Disable overlays (Discord, Steam, GeForce Experience) and background recording.

### 3.2 Warm-up

- **30 seconds** of free play in the benchmark map before any recording.
- Purpose: PSO cache warm-up, shader compilation, texture streaming, Lumen surface cache
  population, chunk streaming settle.
- Discard the warm-up frames entirely; they are never part of a sample set.

### 3.3 Recording

- Start recording with `mdw.Bench.Start <label>` (or `-VoxelBench` for auto-start).
- Insert phase markers with `mdw.Bench.Marker <text>` at each transition (see per-scenario
  setup). Markers are written into the CSV `Marker` column and allow slicing a capture.
- **Duration: 60 seconds** of steady-state recording per scenario (≈ 3600 frames at 60 fps).
- Stop with `mdw.Bench.Stop`; the CSV is written to `Saved/Profiling/`.

### 3.4 Repeats and aggregation

- **3 independent runs** per scenario, restarting the process between runs.
- Report the **median** of the three runs for p50/p95/p99/max and for draw calls.
- Report memory/VRAM as the **peak** observed across the three runs.
- A run is invalid if its frame count is < 3000 or its p95 differs by > 25 % from the
  other two runs (re-run).

### 3.5 Unreal Insights capture (parallel)

- Record an Insights trace for **at least one** of the three runs per scenario, with the
  channels in §7.1.
- Capture `stat` snapshots at the 30 s mark of each run (see §7.2).

### 3.6 Voxel-specific sampling

The CSV produced by `UVoxelBenchSubsystem` already records per frame:

`FrameMs, ActiveChunks, PendingGen, PendingRemesh, ChunksGenerated, ChunksRemeshed,
StreamingMs, GenerationMs, MeshMs, RemeshMs, PlayerX/Y/Z, RenderDistance, FastLeaves`.

Derive from these:
- p50/p95/p99 of `FrameMs`, `StreamingMs`, `GenerationMs`, `MeshMs`, `RemeshMs`.
- Max `PendingGen` / `PendingRemesh` (queue depth) — proxy for back-pressure.
- `ChunksGenerated` and `ChunksRemeshed` totals over the run (throughput).

---

## 4. Metrics captured

### 4.1 Frame time (from CSV + Insights)

| Metric | Definition | Source |
| --- | --- | --- |
| p50 frame time | Median frame time, ms | CSV `FrameMs` |
| p95 frame time | 95th percentile, ms | CSV `FrameMs` |
| p99 frame time | 99th percentile, ms | CSV `FrameMs` |
| Max frame time | Worst single frame, ms | CSV `FrameMs` |
| Frames over budget | Count and % of frames > 16.67 ms | CSV `FrameMs` |
| Frames over 2× budget | Count and % of frames > 33.33 ms | CSV `FrameMs` |

### 4.2 GPU / rendering

| Metric | Source |
| --- | --- |
| Draw calls (visible) | `stat scenerendering` → `DrawPrimitive calls` |
| Triangles | `stat scenerendering` |
| Base pass time | `stat gpu` / Insights GPU track |
| Lumen gather / reflection time | `stat lumen` |
| Virtual shadow map cost | `stat virtualshadowmaps` |
| VRAM (used) | `stat rhi` / `stat memory` / Insights memory track |

### 4.3 CPU / memory

| Metric | Source |
| --- | --- |
| Game thread time | `stat game` → `FrameTime` / Insights CPU track |
| Render thread time | `stat unit` → `Render` |
| Voxel stage times | CSV `StreamingMs/GenerationMs/MeshMs/RemeshMs` |
| Working set (RAM) | `stat memory` → `Physical Memory Used` / Task Manager |
| GC pauses | `stat gc` |

### 4.4 Voxel streaming

| Metric | Source |
| --- | --- |
| Active chunks | CSV `ActiveChunks` |
| Pending generation queue | CSV `PendingGen` (max) |
| Pending remesh queue | CSV `PendingRemesh` (max) |
| Chunks generated / run | CSV `ChunksGenerated` (sum) |
| Chunks remeshed / run | CSV `ChunksRemeshed` (sum) |

---

## 5. Scenario catalog

All scenarios run on the **benchmark map** (fixed seed) with the environment in §2.
Each scenario lists: setup, markers, metrics of interest, and provisional gate thresholds.
Thresholds are **provisional** (current code, pre-optimisation) and will be replaced by
`measured baseline + 10 %` once the first captures land; they exist so CI can run now.

### S1 — Cold spawn (initial streaming fill)

- **Setup:** Launch into the benchmark map at the spawn point; world streams from zero.
  No prior chunk cache. Player stands still for the whole run.
- **Markers:** `spawn` at t=0, `settled` when `PendingGen == 0` for the first time.
- **Focus metrics:** max frame time, frames over 2× budget, PendingGen peak, memory ramp.
- **Notes:** This is the worst-case hitch scenario (plan §1: 40–50 frames at 25–45 ms).
- **Thresholds:**

  | Metric | Gate |
  | --- | --- |
  | p50 frame time | ≤ 20.0 ms |
  | p95 frame time | ≤ 45.0 ms |
  | p99 frame time | ≤ 70.0 ms |
  | Max frame time | ≤ 120.0 ms |
  | Frames > 33.33 ms | ≤ 5 % |
  | Draw calls | ≤ 2500 |
  | Peak RAM | ≤ 1600 MB |
  | Peak VRAM | ≤ 2000 MB |

### S2 — Straight walk, 60 s

- **Setup:** Player walks forward at normal speed along a straight line for 60 s.
  No turns, no mining. Continuous forward streaming.
- **Markers:** `walk-start`, `walk-end`.
- **Focus metrics:** steady-state p95, StreamingMs, GenerationMs, MeshMs, queue depth.
- **Thresholds:**

  | Metric | Gate |
  | --- | --- |
  | p50 frame time | ≤ 16.67 ms |
  | p95 frame time | ≤ 25.0 ms |
  | p99 frame time | ≤ 40.0 ms |
  | Max frame time | ≤ 60.0 ms |
  | StreamingMs p95 | ≤ 8.0 ms |
  | GenerationMs p95 | ≤ 6.0 ms |
  | MeshMs p95 | ≤ 10.0 ms |
  | Draw calls | ≤ 1800 |
  | Peak RAM | ≤ 1200 MB |
  | Peak VRAM | ≤ 1600 MB |

### S3 — 180° turn

- **Setup:** Player walks 10 s forward, stops, performs a 180° turn in place, then
  walks 10 s back along the same line. Repeats until 60 s elapsed.
- **Markers:** `turn-1`, `turn-2`, `turn-3` at each reversal.
- **Focus metrics:** burst p95/p99 on reversal, PendingGen peak, RemeshMs.
- **Thresholds:**

  | Metric | Gate |
  | --- | --- |
  | p50 frame time | ≤ 16.67 ms |
  | p95 frame time | ≤ 35.0 ms |
  | p99 frame time | ≤ 55.0 ms |
  | Max frame time | ≤ 80.0 ms |
  | PendingGen peak | ≤ 40 chunks |
  | Draw calls | ≤ 2000 |
  | Peak RAM | ≤ 1200 MB |
  | Peak VRAM | ≤ 1600 MB |

### S4 — Fall into cave

- **Setup:** Player walks to a cave entrance, drops vertically into an unlit cave,
  lands, and stands still for 30 s, then walks 30 s inside the cave.
- **Markers:** `cave-enter`, `cave-land`, `cave-walk-end`.
- **Focus metrics:** vertical streaming, Lumen in enclosed dark space, torch lighting.
- **Thresholds:**

  | Metric | Gate |
  | --- | --- |
  | p50 frame time | ≤ 16.67 ms |
  | p95 frame time | ≤ 30.0 ms |
  | p99 frame time | ≤ 50.0 ms |
  | Max frame time | ≤ 70.0 ms |
  | Lumen gather time p95 | ≤ 4.0 ms |
  | Draw calls | ≤ 1500 |
  | Peak RAM | ≤ 1200 MB |
  | Peak VRAM | ≤ 1600 MB |

### S5 — Streaming demolition (50 blocks while walking)

- **Setup:** Player walks along a line and mines 50 blocks in a row (≈ 1 block/s)
  while streaming continues. Stresses mesh rebuilds interleaved with streaming.
- **Markers:** `demo-start`, `demo-end`.
- **Focus metrics:** MeshMs, RemeshMs, neighbour remesh cost, hitch count.
- **Thresholds:**

  | Metric | Gate |
  | --- | --- |
  | p50 frame time | ≤ 16.67 ms |
  | p95 frame time | ≤ 30.0 ms |
  | p99 frame time | ≤ 50.0 ms |
  | Max frame time | ≤ 70.0 ms |
  | MeshMs p95 | ≤ 8.0 ms |
  | RemeshMs p95 | ≤ 4.0 ms |
  | Draw calls | ≤ 1800 |
  | Peak RAM | ≤ 1200 MB |
  | Peak VRAM | ≤ 1600 MB |

### S6 — Combat: 20 mobs (new)

- **Setup:** Player stands in a flat arena with 20 active mobs (low-poly stylised,
  non-voxel). Mobs aggro and attack; player retaliates with melee/ranged. No chunk
  streaming (arena fits in one region). 60 s of continuous combat.
- **Markers:** `combat-start`, `wave-peak`, `combat-end`.
- **Focus metrics:** AI tick, combat logic, particles, animation, draw calls from mobs.
- **Thresholds:**

  | Metric | Gate |
  | --- | --- |
  | p50 frame time | ≤ 16.67 ms |
  | p95 frame time | ≤ 25.0 ms |
  | p99 frame time | ≤ 40.0 ms |
  | Max frame time | ≤ 60.0 ms |
  | Game thread (non-voxel) p95 | ≤ 6.0 ms |
  | Draw calls | ≤ 2500 |
  | Peak RAM | ≤ 1200 MB |
  | Peak VRAM | ≤ 1600 MB |

### S7 — Demolition: 50 blocks, stationary (new)

- **Setup:** Player stands still in front of a 50-block wall and destroys it block by
  block (≈ 1 block/s), including debris/physics and crack decals. No streaming.
- **Markers:** `demo-start`, `demo-end`.
- **Focus metrics:** mesh rebuild per destruction, physics debris cost, decal count.
- **Thresholds:**

  | Metric | Gate |
  | --- | --- |
  | p50 frame time | ≤ 16.67 ms |
  | p95 frame time | ≤ 30.0 ms |
  | p99 frame time | ≤ 50.0 ms |
  | Max frame time | ≤ 70.0 ms |
  | MeshMs p95 | ≤ 10.0 ms |
  | Draw calls | ≤ 2000 |
  | Peak RAM | ≤ 1200 MB |
  | Peak VRAM | ≤ 1600 MB |

### S8 — Cave: 40 torches (new)

- **Setup:** Player stands in a cave containing 40 placed torches (point lights),
  no streaming, 60 s. Stresses dynamic lighting + Lumen surface cache + shadow maps.
- **Markers:** `torches-on`, `torches-off` (toggle to measure delta).
- **Focus metrics:** light cost, Lumen, shadow map cost, VRAM.
- **Thresholds:**

  | Metric | Gate |
  | --- | --- |
  | p50 frame time | ≤ 16.67 ms |
  | p95 frame time | ≤ 30.0 ms |
  | p99 frame time | ≤ 50.0 ms |
  | Max frame time | ≤ 70.0 ms |
  | Lumen gather time p95 | ≤ 5.0 ms |
  | Virtual shadow map cost p95 | ≤ 3.0 ms |
  | Draw calls | ≤ 1800 |
  | Peak RAM | ≤ 1200 MB |
  | Peak VRAM | ≤ 2000 MB |

---

## 6. Voxel stage budgets (game-thread)

These are the sub-budgets the voxel streaming system must meet; they feed the scenario
gates in §5 and the lot targets in `voxel-performance-plan.md` §4.

| Stage | Baseline (to measure) | After lots 0–2 | After lots 3–4 |
| --- | --- | --- | --- |
| UpdateChunkStreaming | to measure | < 2 ms | < 2 ms |
| GenerateChunkData | to measure | < 3 ms | < 2 ms |
| Mesh build (incl. neighbour remesh) | to measure | < 6 ms | < 5 ms |
| Deferred remesh drain | to measure | < 2 ms | < 2 ms |
| **Total game-thread streaming** | to measure | **< 6 ms** | **< 5 ms** |
| Worst single frame (any scenario) | to measure | < 12 ms | < 10 ms |

A scenario fails its gate if **any** stage p95 exceeds its "after lots 0–2" target once
that lot is shipped (stricter than the provisional scenario gates).

---

## 7. Console commands and profiling flags

### 7.1 Command line (launch)

```
-trace=cpu,gpu,counters,memory,bookmark,frame
-statnamedevents
-VoxelBench
-ExecCmds="mdw.Bench.AutoStart 1"
-log
-NoVerifyGC
```

Notes:
- `-trace=...` enables the Unreal Insights channels; `frame` adds frame markers.
- `-statnamedevents` exposes named stat scopes to Insights.
- `-VoxelBench` auto-starts a recording labelled `cmdline` (see `VoxelBenchSubsystem::OnWorldBeginPlay`).
- Drop `-NoVerifyGC` only if investigating GC; it can mask collection pauses.

### 7.2 Console commands (in-game)

Voxel benchmark:
- `mdw.Bench.Start <label>` — start recording (e.g. `mdw.Bench.Start s2-walk`).
- `mdw.Bench.Stop` — stop and write the CSV + log the p50/p95/p99/max summary.
- `mdw.Bench.Marker <text>` — stamp a phase change into the CSV `Marker` column.
- `mdw.Bench.AutoStart 1` — auto-start on world begin play (cvar, default 0).

Stat panels (capture at the 30 s mark of each run):
- `stat unit` — Frame / Game / Draw / GPU / RHIT
- `stat game` — game thread breakdown
- `stat scenerendering` — draw calls, triangles, mesh draw calls
- `stat rhi` — RHI / VRAM
- `stat streaming` — texture/streaming
- `stat physics` — physics cost
- `stat gc` — garbage collection
- `stat lumen` — Lumen surface cache / gather / reflections
- `stat virtualshadowmaps` — VSM cost
- `stat fps` — live FPS (sanity only, not a metric)

GPU profiling:
- `ProfileGPU` — per-pass GPU timings (run once per scenario, not every frame).
- `r.ProfileGPU 1` — enable the GPU profiler.

Engine / quality (verify, do not change mid-run):
- `r.Lumen.DiffuseIndirect.Allow`, `r.Lumen.Reflections.Allow` — must read 1.
- `r.RayTracing` — must read 0 on the reference run.
- `r.ScreenPercentage` — must read 100.
- `sg.GlobalIlluminationQuality`, `sg.ReflectionQuality` — confirm preset `@1`.

Unreal Insights:
- Launch `UnrealInsights.exe`, open the `.utrace` file, inspect CPU (Game thread,
  Render thread), GPU, Memory and Bookmark tracks.

### 7.3 Markers convention

Marker names must be short, ASCII, no commas (the CSV sanitizer replaces `,` with `;`,
but keeping them clean avoids confusion): `spawn`, `settled`, `walk-start`, `turn-1`,
`cave-enter`, `demo-start`, `combat-start`, `torches-on`, etc.

---

## 8. Gate mode (CI)

### 8.1 Principle

A scenario **fails** the gate if any of its tracked metrics exceeds the threshold in §5.
The gate runs headless: launch the Test build with `-VoxelBench` and the scenario's
label, wait for the configured duration, stop, parse the CSV, compare, exit non-zero on
any failure. The process exit code is the gate result.

### 8.2 Exit codes

| Code | Meaning |
| --- | --- |
| 0 | All scenarios / metrics within threshold — PASS |
| 1 | One or more metric thresholds exceeded — FAIL (regression) |
| 2 | Missing or malformed CSV artifact — FAIL (infra) |
| 3 | Environment mismatch (wrong GPU, wrong preset, wrong build config) — FAIL (infra) |
| 4 | Insufficient samples (< 3000 frames) or invalid run (> 25 % spread) — FAIL (infra) |

### 8.3 Gate rules

- Run **all 8 scenarios**; a single failure fails the gate.
- Compare the **median of 3 runs** against the threshold (not a single run).
- A metric is a failure only if it exceeds the threshold by more than the **noise margin**
  (5 %) to avoid flaky gates; record the raw value regardless.
- Thresholds are stored as data (one row per scenario × metric) and versioned with the
  repo; changing a threshold requires a Technical Director sign-off and a note in the
  commit message referencing the design doc.
- On failure, the gate must print: scenario, metric, measured value, threshold, and the
  CSV path, so the regression is traceable.
- Baseline captures (before/after a lot) are **not** gated against absolute thresholds;
  they are compared against each other (delta report).

### 8.4 Delta report (lot validation)

For each lot, produce a before/after table using the §4 metric set, on the same machine
and label, and attach both CSVs + the Insights trace. A lot is "done" only when its
target column in `voxel-performance-plan.md` §4 (Budgets) is met or the deviation is
documented and accepted by the Technical Director.

---

## 9. Artifacts and reporting

Per run, store under `Saved/Profiling/` (git-ignored) and archive to the perf record:

| Artifact | Naming |
| --- | --- |
| CSV (per scenario) | `VoxelBench-<scenario>-<timestamp>.csv` |
| Insights trace | `<scenario>-<timestamp>.utrace` |
| Stat snapshots | `<scenario>-<timestamp>-stats.txt` |
| Summary report | `Docs/performance/reports/<date>-baseline.md` |

The summary report follows the Performance Report Format (frame time budget table,
memory budget table, top 5 bottlenecks, regressions) and must include the scenario
table with measured p50/p95/p99/max, draw calls, RAM and VRAM.

---

## 10. Ownership and cadence

- **Performance Analyst:** runs the baseline, maintains this plan and the thresholds.
- **Technical Director:** approves threshold changes and lot gate criteria.
- **DevOps Engineer:** wires the gate into CI (T5 of sprint-00).
- **QA Lead:** owns the test scenarios and validates reproducibility.
- Cadence: re-run the full baseline on every merge to `main` that touches voxel,
  rendering, or gameplay systems; full re-baseline at each milestone.

---

## 11. Open items

- Confirm the benchmark map and fixed seed exist (create if missing).
- Confirm render distance default and height (64 vs 192) for the baseline run.
- Decide whether the combat arena (S6) and torch cave (S8) are part of the benchmark
  map or separate maps (affects streaming isolation).
- Replace provisional thresholds with measured values + 10 % once first captures land.
- Add a data file for the gate thresholds once the CI tooling is chosen (T5).