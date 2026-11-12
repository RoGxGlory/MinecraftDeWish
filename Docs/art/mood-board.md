# Mood Board — LAMPLIGHTER

**Date:** 2026-11-12 (consolidated)
**Owner:** Art Director
**Authority:** statuses below are mirrored in `Docs/art/art-bible.md` §15.
The machine-readable index is `Docs/art/mood-board/manifest.csv` — it is the
single source for file → status mapping.

> **Why this file was rewritten:** the previous version and `art-bible.md`
> disagreed on A1, A3 and A4, and neither contained the 6 Set 2 captures. Every
> status now traces to a measured observation, and the manifest prevents the two
> documents from drifting apart again.

---

## 1. The 12 captures

Two sets, same build lineage, `Docs/art/mood-board/`.

### Set 1 — 2026-11-11 00:xx (6 files)

| File | Content | Status |
|---|---|---|
| `Screenshot 2026-05-23 202820.png` | Pre-overhaul build | **EXCLUDED** — BEFORE (A6) |
| `Screenshot 2026-11-11 000733.png` | Terrain at distance, dense fog, no torch | Conditional |
| `Screenshot 2026-11-11 000817.png` | Mid-range terrain, no biome boundary visible | Conditional |
| `Screenshot 2026-11-11 000850.png` | Close-up blocks; boundaries not readable | Conditional (V5) |
| `Screenshot 2026-11-11 000917.png` | Labelled "cave"; actually outdoor forest | **EXCLUDED** — invalid capture |
| `Screenshot 2026-11-11 001008.png` | Pale ground, no saturated orange | Approved — palette only (V8) |

### Set 2 — 2026-11-11 10:xx (6 files)

| File | Content | Status |
|---|---|---|
| `Screenshot 2026-11-11 105016.png` | Open terrain, very dense fog, no Angel | Conditional |
| `Screenshot 2026-11-11 105123.png` | Open terrain, whiteout fog; ambiguous shape at 20–30 b | Conditional (A1a) |
| `Screenshot 2026-11-11 105207.png` | **Angel at 10–14 blocks**, reads as statue, decoloured | **Approved** — A1b evidence |
| `Screenshot 2026-11-11 105250.png` | **Angel at ~3 blocks**, sharp silhouette, grey-green `#6E7C74` | **Approved** — A1c evidence (anti-target) |
| `Screenshot 2026-11-11 105418.png` | Wooden structure exterior, torch, **warm pool + falloff** | **Approved** — A3 framing (verdict **contested**) |
| `Screenshot 2026-11-11 105502.png` | Interior, torch, warm floor pool, debug tooltip | **Approved** — A3 framing (verdict **contested**) |

**Approved** = usable as a P2 target. **Conditional** = usable for a narrow
purpose, named in the note. **Excluded** = never a target.

---

## 2. What the set proves

1. **Worn voxel world** — validated. `000733`, `000817`, `000850`, `001008`.
2. **Desaturated palette** — validated. The only saturated element in any frame
   is the HUD bar (`~#F2A01C`), which is interface chrome, not scene light.
3. **Dense ash fog** — validated, and now measured: `λ ≈ 16 blocks`, `p ≈ 1.37`.
   Silhouette readability ends at 25 blocks; material/colour at ~8–12.
4. **Block scale** — 1 block = 1 m is consistent across frames.
5. **Angel silhouette at close range** — validated at 10–14 blocks (`105207`) and
   at ~3 blocks (`105250`).
6. **Torches exist and cast a local warm pool** — `105418`, `105502`; `R_pool`
   1.89 / 2.30 with visible falloff (rev 2).

## 3. What the set does NOT prove

| Gap | Consequence |
|---|---|
| Torch dominance is **contested**, not established | A3 re-opened; a sampler-based measurement is required (V2). Pass 2 measures pass on `R_peak`/`R_pool`/`R_bloom` |
| Angel renders grey-green `≈#6E7C74`, **`0.56×` the fog luminance** | "White statue that should not be there" fails → A1c |
| No cave interior anywhere | A5 unaddressed; darkest available RGB 30–80 vs Void 11/14/17 |
| No sky or horizon visible in `001008` | Sky grade unverified → V8 |
| No mob in any frame | Mob visual identity entirely unvalidated |
| No biome boundary visible in `000817` | Biome transition unvalidated |
| Block boundaries not readable in `000850` | Texture pass unproven → V5 |

---

## 4. Measured fog curve (A4)

Contrast retention against fog, `C(d) = exp(-(d/λ)^p)`, fitted on the captures:

- `λ ≈ 16 blocks`, `p ≈ 1.37`
- Anchors: `C(12) ≈ 0.50` (`105207`), `C(25) ≈ 0.15` (`105123`)

| Distance | `C(d)` | Reading |
|---|---|---|
| 3 | 0.90 | silhouette nette |
| 12 | 0.50 | statue lisible, couleur mangée |
| 20 | 0.25 | silhouette seulement |
| **25** | **0.15** | **limite — cible A1a** |
| 40 | 0.03 | disparu |
| 60 | 0.002 | invisible |

**Decision:** the fog is kept; A1 is re-based on this curve (art-bible §5, §8.1).
The 60–90 block target is withdrawn. If it is ever restored: `λ` must go 16 → 58
(÷3.7 density) with `p ≈ 1.46`. Parked, not scheduled.

**Authoring caveat (rev 2):** `p = 1.37` is **not producible by Unreal's
`ExponentialHeightFog`**, which is `p = 1` by construction. Fitting `p = 1` to
`C(12)` gives `C(25) = 0.236` (too readable); fitting it to `C(25)` gives
`C(12) = 0.42` (too flat). The curve needs a base height-fog **plus** a
low-altitude layer. Procedure in art-bible §5.5 (V10). No fog actor is authored
in the build yet — this curve describes the captures, not the current build.

---

## 5. Adjustment results (A1–A6)

| # | Domain | Status | Evidence / decision |
|---|---|---|---|
| **A1** | Angel silhouette | **RE-BASED** (was BLOCKED) | 60 blocks is unreachable (`C(60)=0.002`). Now 3 tiers: 25 b silhouette (A1a, untested), 12 b statue (A1b, **PASS** via `105207`), 3 b bone colour (A1c, **FAIL** via `105250` — grey-green, darker than fog). New rule: `L_statue ≥ 1.4 × L_fog_background` from 3 to 25 b. |
| **A2** | Sunset / sky palette | **PASS on rule; sky UNVERIFIED** | `001008` contains no saturated warm colour → the palette rule holds. But the sky is occluded, so the capture does not demonstrate sky grade. Two analyst passes disagree on whether a sky band is visible; unresolvable from a compressed PNG. Downgraded to palette reference; V8 opened. |
| **A3** | Torch dominance | **CONTESTED (was FAIL)** | Pass 1 measured `R_peak ≈ 1.0`, `R_pool ≈ 1.0`, `R_bloom = 0`. Pass 2 measures **`R_peak` 2.14/2.40, `R_pool` 1.89/2.30, bloom 40–200 px** — pass on all three. Pass 1's stated cause ("the cobblestone floor at RGB 190–205 is brighter and larger") is the torch's **own warm pool** (`#F0E4C4` near vs `#9E9E96` far). Fix list frozen; only the light colour is unconditionally wrong. New metric `TDR` (`A_sodium / A_uiwarm`) = 2.7 / 10.3. Detail in art-bible §4.2, measurements in §9 below. |
| **A4** | Fog knee | **CLOSED — fog kept, A1 re-based** | Measured `λ ≈ 16 b`, `p ≈ 1.37`. Kept as-is. Surviving rules: Ash colour, height falloff, Void fog in caves. |
| **A5** | Cave void levels | **OPEN — no valid capture** | `000917` is an outdoor forest. Darkest available RGB 30–80 vs Void 11/14/17 → 3–5× too bright. Acceptance: ≥70% of unlit cave pixels at RGB ≤ 25. |
| **A6** | Early-reference hygiene | **PASS (closed)** | File is `Screenshot 2026-05-23 202820.png` — the docs previously cited a non-existent `000820`. Confirmed pre-overhaul; excluded; indexed in the manifest with `status=excluded`. |

**Net (rev 2): 2 closed (A4, A6), 1 re-based and measured (A1), 1 closed on the
rule (A2), 1 re-opened as CONTESTED (A3), 1 open but now reachable (A5).**

---

## 6. Defects and anomalies

- `105016`: dark slab cut off at the top edge; hotbar on slot 9 while every other
  capture shows slot 1 (UI state inconsistency).
- `105123`: the central vertical shape cannot be identified as Angel or tree
  trunk. It is the closest thing to A1a evidence and it is ambiguous.
- `105250`: Angel material reads `#6E7C74` grey-green, not `#E3DCCB` bone, and at
  `0.56×` the luminance of the fog behind it (`#93A19A`). **The anti-target.**
- `105418`: torch flame ≈27×36 px (`0.05 %` of frame), core `#FFE08A` — **pale
  yellow, not sodium**. Warm pool and 40–70 px bloom **are** present (pass 2);
  the pass-1 claim of "no halo, no pool" is not reproducible.
- `105502`: torch ≈35×45 px, 120–200 px bloom, warm floor pool `#F0E4C4` near vs
  `#9E9E96` far. Debug tooltip `"Cobblestone / Durability: 2.5"` — a debug
  element that must not ship.
- All 12: orange HUD bar (`~#F0A020`) is the most saturated element in frame,
  occupies 0.73 % of every frame, and out-competes every torch → art-bible §3.1.
- Item labels `"Default Block"` / `"Grass"` render above the hotbar.

---

## 7. Non-determinable

- Exact distances (FOV unknown; estimates assume a ~2.5 m statue).
- True material colour of the Angel (grey-green could be base colour, green GI
  bounce, or grade — all three are candidates).
- Identity of the red-brown surface and the angular shape in `105502`.
- Nature of the cream terrain in `001008`.
- Whether a sky band exists in `001008`.
- Whether the torch flame hue is exactly `#E8A33D` (compressed capture).

---

## 8. Reference set for P2 — **FROZEN (finalised 2026-11-12, rev 2)**

The board is now frozen as the P2 target set. The **contract** — what each
approved frame is a target *for*, and what it does **not** license — is
`art-bible.md` **§19**. This table is the index; §19 is the authority.

| Status | Captures | Use |
|---|---|---|
| **Approved** | `001008` | Desaturated palette reference (not sky grade) |
| **Approved** | `105207`, `105250` | Angel silhouette at 12 b (target) and 3 b (anti-target); material defect evidence |
| **Approved** | `105418`, `105502` | Torch framing + warm-pool falloff shape — **not** the disputed verdict |
| **Conditional** | `000733`, `000817`, `000850`, `105016`, `105123` | Fog / terrain / block scale — subject to V5 |
| **Excluded** | `Screenshot 2026-05-23 202820.png`, `000917` | Never a target |

No new capture joins this set without closing its corresponding V-item
(art-bible §16). New captures use semantic names, not timestamps (§19.1).

---

## 9. Second measurement pass (rev 2) — the A3 contradiction

A second independent pass was run on `105418`, `105502` and `105250` after the
first review. It disagrees with the recorded A3 verdict on every metric. Both are
kept here because the disagreement is the finding.

### 9.1 Torch frames

| Quantity | `105418` (exterior) | `105502` (interior) |
|---|---|---|
| Flame bbox | ≈27×36 px (**0.05 %** of frame) | ≈35×45 px (**0.08 %**) |
| Flame core hex | `#FFE08A` | `#FFE89A` |
| Flame fringe hex | `#E07820` → `#A8501A` | `#E07820` → `#B84A18` |
| Visible bloom radius | **40–70 px** | **120–200 px** |
| Ground **near** torch | `#D0B47E` (warm tan) | `#F0E4C4` (warm cream) |
| Same material **far** from torch | `#8A8A80` (cool grey) | `#9E9E96` (cool grey) |
| Falloff visible? | yes, ≈150 px reach | yes, strong |
| `R_peak` (linearised) | **2.14** | **2.40** |
| `R_pool` (linearised) | **1.89** | **2.30** |
| `A_sodium` | ≈1.5–2.5 % | ≈7–8 % |
| `TDR` vs UI bar (0.73 %) | **≈2.7** | **≈10.3** |

**Verdict change:** `R_peak`, `R_pool` and `R_bloom` all **pass**. The recorded
"no halo, no warm pool, R ≈ 1.0" is not reproducible.

**The one real finding here:** the flame core is `#FFE08A` — **pale yellow-white,
not sodium `#E8A33D`**. Combined with the in-engine light colour
`RGB (1.0, 0.78, 0.50)` (paler and greener than sodium `(0.910, 0.639, 0.239)`),
the torch is bright but **not sodium**. That is a colour defect, not an intensity
defect, and it is the only A3 item that survives.

### 9.2 Angel frame (`105250`)

| Quantity | Value |
|---|---|
| Angel body | `#6E7C74`, `L = 0.190` |
| Fog immediately behind | `#93A19A`, `L = 0.340` |
| **`L_statue / L_fog`** | **0.56×** (required ≥ 1.4×) → **2.5× shortfall** |
| Green bias | body `G − R = +14`; bone `G − R = −7` → **21-point green excess** |

A1c **FAIL is corroborated**, and now quantified. Projected effect of the single
albedo fix: `L = 0.718 / 0.340 = 2.11×` — **passes with margin** (art-bible §8.1).

### 9.3 Warm-colour census (all three frames)

The only **saturated** warm element in any frame is the **HUD bar `~#F0A020` at
0.73 % of frame**. Terrain warm tones (`#8A6A42`, `#6B5A3A`, `#B0A078`) are muted
browns in the Dust `#8C8577` family — legitimate. **No A2 violation.**

### 9.4 Limits of this pass

The pass has **no pixel sampler**: all hex values are perceptual estimates
(±10–15 per channel), frame dimensions were assumed at 1920×1080, and bloom
cannot be separated from direct lighting in a still. **Neither pass is an
instrument.** That is exactly why V2 is re-specified as sampler-based, and why
the A3 fix list is frozen rather than executed.

---

*Last updated: 2026-11-12 (rev 2) — A3 re-opened as CONTESTED after a second
measurement pass (§9); set frozen as the P2 target set (§8, contract in
`art-bible.md` §19). Owner: Art Director. Statuses mirrored in
`Docs/art/art-bible.md` §15 and `Docs/art/mood-board/manifest.csv`.*
