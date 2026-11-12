# Art Bible — LAMPLIGHTER (MinecraftDeWish)

**Status:** Accepted — **P1 CLOSED (rev 2)**, mood board frozen, P2.0 ready.
**Owner:** Creative Director
**Supersedes:** the previous §13 / §17.3 / §18.3 triplication of the A1–A6 table.

> **Rev 2 (2026-11-12) — what changed and why.** A second independent measurement
> pass on the torch captures **contradicts the recorded A3 verdict on all three
> metrics** (§4.2). A3 moves from `FAIL` to **`CONTESTED`**, and its ordered fix
> list is **frozen** until a tool-based measurement settles it (V2). Everything
> else in A1–A6 is confirmed and now carries numbers rather than adjectives.
> The approved captures are frozen as the P2 target set in **§19**.
**Sources of truth:**
- `design/art/art-direction.md` — creative direction (Accepted)
- `Docs/art/style-guide.md` — technical visual guide (Accepted)
- `Docs/art/mood-board.md` — in-engine captures (reference set)
- `Docs/art/mood-board/manifest.csv` — machine-readable capture manifest
- `Docs/art/material-parameters.md` — material parameters
- `Docs/technical-contract.md` — frozen technical contract

> Single reference for the art team. Any change to a frozen rule needs an ADR
> approved by the Technical Director.

---

## 1. Purpose & Scope

Defines LAMPLIGHTER's visual identity and the constraints every art asset must
respect. Read before creating or modifying an asset.

**Covers:** thesis, palette, light/grade, fog & visibility, materials, blocks,
mobs, the Angel, tools, UI, weather, main menu, reference captures, A1–A6
status, open validations, P2 plan.

**Does not cover:** material parameter detail (`material-parameters.md`), frozen
technical contract (`technical-contract.md`), detailed UI direction
(`design/ui/ui-direction.md`).

---

## 2. Thesis — Eroded Realism

> **"The world is built, the Angel is sculpted."**

The world is worn blocks and sodium light. The Weeping Angel is the only thing in
the game not built from blocks — and therefore the only thing that does not
belong to the world. That contrast is the visual identity **and** the mechanic.

**Design test (art-direction §14):** a screenshot must be identifiable without a
logo as *worn voxel world, sodium light, a white statue that should not be
there.* If a player describes the Angel as "a mob", the contrast has failed.

**Validation state (2026-11-12, rev 2):**

| Signal | Status | Evidence |
|---|---|---|
| Worn voxel world | **VALIDATED** | `000733`, `000817`, `000850`, `001008` |
| Sodium light | **CONTESTED — measurement dispute** | Two passes disagree: pass 1 measured `R_peak ≈ 1.0` / no pool / no halo; pass 2 measures `R_peak 2.14–2.40`, `R_pool 1.89–2.30`, bloom 40–200 px — i.e. **pass** on all three. The in-engine torch light (2500 cd, 900 UU, `RGB 1.0/0.78/0.50`) and the captures' warm floor pools both support pass 2. Fix list frozen until V2. See §4.2 / A3 |
| A white statue that should not be there | **FAILED (measured, corroborated)** | The Angel reads `#6E7C74` against fog `#93A19A` → `L_statue/L_fog = 0.56×` in linear luminance, against a required **≥ 1.4×**. A **2.5× shortfall**. See §8.1 / A1 |

> Rev 2 note: the previous revision asserted "sodium light FAILED (measured)" as
> fact. It is now clear that verdict rested on a single pass whose stated cause —
> "the cobblestone floor at RGB 190–205 is brighter and larger" — describes the
> torch's **own warm light pool** being counted as a competitor. A bright warm
> floor next to a torch is *evidence for* sodium dominance, not against it. The
> defect claim is withdrawn pending V2; the sodium pillar is not known to be
> broken, and must not be "fixed" as if it were.

---

## 3. Palette

Source of truth: `design/art/art-direction.md` §2. No new colour without
art-director approval.

| Hex | Name | World use | UI use |
|---|---|---|---|
| `#0B0E11` | Void | night sky, unlit interiors, cave fog | modal scrim (85%) |
| `#171C21` | Concrete | cut stone, POI walls, floors | panel surface |
| `#2A3138` | Steel | metal, roofs, bars, machines | disabled state |
| `#E3DCCB` | Bone | plaster, overcast sky, **Angel stone** | primary text |
| `#E8A33D` | Sodium | **the only warm light**: torch, lamp, furnace, generator | *(reserved — see below)* |
| `#C8462F` | Rust/Blood | creeper, damage, corrosion, explosion | danger, error |
| `#4FA6A0` | Signal | electricity, cameras, CRT — the only "clean" light | success, active toggle |
| `#5C6268` | Ash | fog, tarnished metal, disabled | secondary text |
| `#6B7355` | Moss | lichen, residual vegetation | — |
| `#9AA36B` | Sick green | Horror Forest, sick leaves | — |
| `#8C8577` | Dust | sand, rubble, ash on ground | — |

**Non-negotiable rules:**
- One warm source only: sodium. Everything else is cold, desaturated, ashen.
- No saturated blue sky. Day sky is bone-ash; night is cold teal-dark.
- XP orbs desaturated to `#B9C46A` — the only "game-y" element, kept discreet.
- Rust red is reserved for danger. It never decorates.
- The sodium grade (LUT) is constant; only light intensity/filtration changes
  with weather and day/night. The global grade never brightens the world.

### 3.1 UI accent is a separate token (decision, 2026-11-12)

`#E8A33D` is **reserved for world light**. UI accent must not use it.

Reason: measured, the HUD bar (`~#F2A01C`, top-left, constant across all 12
captures) is **more saturated and more visually dominant than any torch in the
game**. The UI is currently stealing the "only warm light" role from the world.
That is a pillar violation, not a taste issue.

- **UI accent token:** `#C08A45` at ≤70% saturation, ≤80% of the sodium value.
- Selection ring on icons stays a 2px sodium-tinted ring (§9) — it is a
  *selection* signal, not ambient warmth, and it is small-area.
- Owner of the change: Art Director + UI Designer (V7).

---

## 4. Light & Grade

- Post-process + single LUT "Sodium Grade": high contrast in lows, soft rolloff
  in highs, 25–35% desaturation outside sodium.
- The torch is the brightest object on screen, always. That is the visual
  hierarchy of the fantasy.
- Day/night cycle (20 min) kept; the grade is **constant**. Only the player's
  light changes. Phase 3 never brightens the world — the player installs light.
- Weather modulates mood: rain (wet surfaces, more reflections), thick fog
  (reduced visibility), ash storm (particles, wind, noise), clear sky (rare,
  unsettling by contrast). Only light intensity/filtration changes; the LUT stays.

### 4.1 Measurement definitions (used by A3 and §8)

All luminance values are **relative, measured on a linearised capture** (sRGB
decoded, no tonemap inversion needed since we compare in-frame ratios).

| Symbol | Definition |
|---|---|
| `L_peak(flame)` | max luminance in a 32px box around the flame centroid |
| `L_sky` | median luminance of the sky/fog band (top 20% of frame) |
| `R_peak` | `L_peak(flame) / L_sky` |
| `L_ground(near)` | mean luminance of ground within 3 blocks of the torch |
| `L_ground(far)` | mean luminance of the same material ≥12 blocks from any torch |
| `R_pool` | `L_ground(near) / L_ground(far)` |
| `R_bloom` | visible bloom radius around the flame, in px |
| `A_sodium` | **area** of pixels lit warm by a world light source (hue 20–50°, sat ≥ 0.20, `L ≥ L_ambient`), as % of frame |
| `A_uiwarm` | **area** of warm UI pixels (HUD bar, warm accents, warm labels), as % of frame |
| `TDR` | **Torch Dominance Ratio** — `A_sodium / A_uiwarm`. Area dominance of world light over UI warmth |

### 4.2 A3 — torch dominance: **CONTESTED** (rev 2)

Two independent passes over the same two captures disagree on every metric. Both
are recorded, because the disagreement is itself the finding.

| Metric | Target | Pass 1 (recorded 11-12) | Pass 2 (rev 2) | Rev 2 verdict |
|---|---|---|---|---|
| `R_peak` | **≥ 2.0** | ≈ 1.0 — "flame not brighter than the fog band" | **2.14** (`105418`), **2.40** (`105502`) | **PASS** |
| `R_pool` | **≥ 1.5** | ≈ 1.0 — "no warm pool, no gradient" | **1.89** (`105418`), **2.30** (`105502`), with visible falloff | **PASS** |
| `R_bloom` | **≥ 8 px** | 0 px — "no halo, no glow" | **40–70 px** (`105418`), **120–200 px** (`105502`) | **PASS** |
| `TDR` | **≥ 3.0** | not measured | **≈ 2.7** (`105418`), **≈ 10.3** (`105502`) | **framing-dependent** |
| Flame hue | sodium `#E8A33D` ±10° | "orange-yellow, unmeasurable" | core `#FFE08A` / fringe `#E07820` → **too pale, too yellow** | **UNVERIFIED / suspect** |

**Why pass 1 was probably wrong.** Its stated cause was "the cobblestone floor at
RGB 190–205 is brighter and larger in area". Pass 2 measures that same floor at
`#F0E4C4` **near** the torch and `#9E9E96` **far** from it — the bright floor *is*
the torch's warm pool, falling off with distance. Pass 1 counted the light the
torch casts as a rival to the torch. That is a misattribution, and it is the only
thing that made the sodium pillar look broken.

**Why pass 2 is not authoritative either.** Its hex values are perceptual
estimates (±10–15 per channel), it cannot separate post-process bloom from direct
lighting in a still, and it has no pixel sampler. Neither pass is a measurement
instrument. **`R_peak`/`R_pool`/`TDR` are only settled by a sampler (V2).**

#### Decision (rev 2, binding)

1. **A3 status = `CONTESTED`.** Not `PASS`, not `FAIL`.
2. **The ordered fix list below is FROZEN.** Do not raise the torch emissive, do
   not lower fog inscattering, do not add bloom. Applying those fixes to values
   that already pass would over-brighten the torch, blow out the fog relationship
   settled in §5, and break A1 in the opposite direction (statue darker than a
   now-blazing foreground).
3. **V2 is re-specified as an instrument-based test**, not an eyeball test.
4. **What survives as a real defect is the UI, not the world** (§3.1): the HUD bar
   is `~#F0A020` at **0.73 % of frame** in *every* capture. In `105418` the world's
   entire sodium-lit area only out-areas it **2.7:1** — below the 3:1 target, i.e.
   a single UI element is within a factor of three of all the warm light in the
   scene. The torch is not too dim; the UI is too loud, and it is loud in the same
   hue the torch needs.

#### Torch dominance — quantified targets (unchanged numbers, new framing)

| Metric | Target | Rationale |
|---|---|---|
| `R_peak` | **≥ 2.0** (stretch 3.0) | flame core must beat the fog band |
| `R_pool` | **≥ 1.5** | the torch must visibly change its surroundings |
| `R_bloom` | **≥ 8 px** | sodium reads as light, not as a decal |
| `TDR` | **≥ 3.0** | the world's warm light must out-area the UI's warm accent 3:1 |
| `A_sodium` | **≥ 3 % of frame** | a torch in frame must actually light the frame |
| Flame hue | `#E8A33D` ±10° hue | sodium, not amber, not yellow-white |

`TDR` and `A_sodium` are **framing-dependent by nature** — a torch 20 blocks away
lights almost nothing. The acceptance capture protocol is therefore fixed:

> Camera 4–6 blocks from the torch, torch in the upper third, at least one adjacent
> surface within 3 blocks visible, **no HUD** for the light measurement and a
> second frame **with HUD** for the `TDR` measurement.

#### In-engine state (audited 2026-11-12, for V2)

| Item | Value in code | Note |
|---|---|---|
| Torch point light | `2500` intensity, `900 UU` (9 m) radius, `CastShadows=false` | `WorldGenerator.cpp` torch light pool |
| Torch light colour | `RGB (1.0, 0.78, 0.50)` | **Suspect.** Sodium `#E8A33D` = `(0.910, 0.639, 0.239)`. The authored colour is paler and greener — it will never render as sodium |
| Torch light pool | `MaxTorchLights = 16`, pooled | Hard cap: **a 17th torch is unlit**. A torch-dense room silently loses its warm light. Design constraint, not a bug |
| Sodium bloom | none authored | §4.2 fix #4 stays deferred |
| Sodium LUT / grade | **does not exist** in `Content/` | No post-process material or LUT asset found. P2.1 is the whole grade, from zero |

**Ordered fix list — FROZEN until V2 reports:**

1. **Torch light colour → sodium.** `(1.0, 0.78, 0.50)` → `(0.910, 0.639, 0.239)`.
   This one is not contingent on V2: the current colour is measurably not sodium,
   and no measurement dispute changes a hex value.
2. **Torch emissive multiplier** — *only if* V2 shows `R_peak < 2.0`.
3. **Torch point light intensity/radius** — *only if* V2 shows `R_pool < 1.5`.
4. **Fog inscattering ceiling** — *only if* V2 shows the fog band out-glowing the
   flame. Do not touch fog density (§5 is settled).
5. **Sodium-only bloom** — *only if* V2 shows `R_bloom < 8 px`.
6. **UI accent desaturation** (§3.1) — **unconditional**, see V7. This is the one
   fix that is certainly needed.

**Design test:** a capture with a torch in frame where the torch is unambiguously
the brightest thing on screen, measured with a sampler. If the flame is not the
brightest object, the sodium pillar is not delivered and the thesis fails at
signal 2.

---

## 5. Fog & Visibility (A4 — settled, measured)

**Decision (2026-11-12): the fog density is kept. A1 is re-based on the actual
visibility the fog gives us.** The previous "60–90 blocks" target in
`art-direction.md` §3 and `style-guide.md` §3 is **withdrawn** and replaced by the
measured curve below.

### 5.1 Measured fog curve

Contrast retention against the fog, `C(d) = exp(-(d/λ)^p)`, fitted on the
captures (anchors: `C(12) ≈ 0.50` from `105207`, `C(25) ≈ 0.15` from `105123`):

| Parameter | Value |
|---|---|
| `λ` (e-folding distance) | **≈ 16 blocks** |
| `p` (exponent) | **≈ 1.37** |

Predicted curve:

| Distance | `C(d)` | Reading |
|---|---|---|
| 3 blocs | 0.90 | silhouette nette (`105250` confirms) |
| 12 blocs | 0.50 | statue lisible, couleur déjà mangée (`105207` confirms) |
| 20 blocs | 0.25 | silhouette seulement |
| **25 blocs** | **0.15** | **limite de lisibilité — la cible A1** |
| 40 blocs | 0.03 | disparu |
| 60 blocs | 0.002 | invisible |

The model reproduces every observation in the captures. It is the reference:
any fog tuning is checked against this table.

### 5.2 Why the previous estimate was wrong

The earlier docs recorded "20–50 blocks of visibility" (and, in one revision,
"10–15 blocks"). The measured value is **12–25 blocks for silhouette, ~8–12 for
material/colour**. Both earlier numbers were eyeballed, not measured.

### 5.3 Parked option (not scheduled)

If the 60-block silhouette ever becomes desirable again (e.g. to restore the
"statue at the edge of visibility" icon), the fog must go from `λ = 16` to
`λ ≈ 58` — a **÷3.7 density reduction** — with `p` raised to ~1.46 so `C(90)`
stays at 0.15. This is documented so nobody re-derives it. It is **not** in P2.

### 5.4 Non-negotiables that survive the decision

- Fog **colour** stays inside Ash `#5C6268`. The green-grey bias observed in
  `105016`–`105250` is a biome tint leaking into the base fog; it must not
  dominate. Verify per biome.
- Fog must have **height falloff**. Current fog is flat and uniform with no
  vertical gradient and no god rays — that is why it reads as a "wall" rather
  than as atmosphere.
- Caves must not use the sky's pale fog (§6). Cave fog is Void `#0B0E11`.

### 5.5 Authoring the curve in engine (A4, rev 2)

`p = 1.37` is the awkward part: **Unreal's `ExponentialHeightFog` is `p = 1` by
construction.** A single height-fog actor therefore cannot reproduce the measured
curve. Fitting `p = 1` to one anchor always misses the other:

| Fit | `λ` | `C(12)` | `C(25)` | Verdict |
|---|---|---|---|---|
| Fit near anchor `C(12)=0.50` | 17.3 | 0.50 | **0.236** | 25-block tier over-readable by 57 % — A1a gets easier than reality, statue still invisible past 30 |
| Fit far anchor `C(25)=0.15` | 13.9 | **0.42** | 0.15 | 12-block tier loses contrast the captures actually show — A1b evidence would not reproduce |
| Measured `p = 1.37` | 16 | 0.50 | 0.15 | reproduces both |

**Recommended authoring:** base `ExponentialHeightFog` (`p = 1`, `λ ≈ 17.3`,
Ash colour, height falloff ON) **plus** a low-altitude volumetric layer or a
density curve that pulls `C(25)` from 0.236 down to 0.15. That second component
is what makes the fog read as *a floor of ash* rather than a uniform wall, and it
is the same component that gives §5.4's height falloff. One actor cannot do it;
two can, and they are cheaper than raising global density.

**Calibration procedure (do this, do not compute it):**

1. Build a test corridor: a `#E3DCCB` 1×1 m reference pillar at 3, 12, 20, 25,
   40 and 60 m from a fixed camera, flat ground, no HUD.
2. For each distance, measure the pillar's luminance against the fog field
   beside it, on a linearised capture.
3. Tune `FogDensity` and the second layer until measured `C(d)` matches §5.1
   within ±0.03 at 12 and 25 m.
4. Record the final parameter values here. Until step 4 is done, §5.1 is a
   *target*, not a description of the build.

> **Status:** no `ExponentialHeightFog` actor or fog parameter is authored in
> `Config/` today. `M_PP_Fog` (weather) exists but is a post-process material,
> not the atmospheric fog. The measured curve comes from the captures; the build
> does not yet contain the fog that produced it. **P2.1 must author it from
> §5.1, then re-fit.**

---

## 6. Cave & Void Levels (A5 — quantified ladder, rev 2)

**Target:** unlit cave volumes read as Void `#0B0E11` (RGB 11/14/17).

**Measured 2026-11-12:** no valid cave capture exists. The darkest regions
available are RGB **30–80** (`105418` interior behind a door, `105502` shadowed
corners) — **3 to 5× too bright**. `000917`, previously labelled "cave", is an
outdoor forest and is excluded.

**What changed in rev 2:** cave generation now exists in the build (tunnels,
noodles, caverns — `VoxelCaveRules`). A5 is no longer blocked on *being able to
reach* a cave. It is blocked only on capturing one and on the GI clamp below.

### 6.1 The void ladder

"Void" is not one value. It is four levels, and the acceptance test must name
which one it is testing — otherwise "≥70 % ≤ RGB 25" is unfalsifiable.

| Level | Where | Luminance rule | Rationale |
|---|---|---|---|
| **L0 — Absolute void** | ≥ 24 blocks from any opening *and* ≥ 24 blocks from any light source | **≥ 70 % of pixels at RGB ≤ 25; 0 % above RGB 40** | The dark the player installs light into. This is the A5 target |
| **L1 — Torch pool** | Within 8 blocks of a placed torch | Peak at the flame, falling to **≤ RGB 40 by 8 blocks** | The pool is the whole point: light must be *local*, not a room-wide fill |
| **L2 — Mouth zone** | Within 12 blocks of a cave opening / skylight | May use surface fog, but surface fog must not exceed **25 % of the cave volume** and must decay to L0 by 12 blocks | Skylight leaking 20 blocks deep is the observed defect |
| **L3 — Structure** | Mineshafts, POI basements | Sodium only. No Signal, no ambient | Structures are lit by the people who built them, and they are gone |

**Accessibility floor (non-negotiable):** in L0 the world may be black, but the
**crosshair, HUD and interaction prompts stay legible**. "Dark" must never mean
"the interface disappeared". Test on a display at 40 % brightness.

### 6.2 Actions

1. **Capture a real cave interior** at L0 (ceiling, no skylight, ≥20 blocks from
   any opening) — now reachable, see V4.
2. **Kill skylight leakage into cave volumes** — the likely source of L2 bleed.
3. **Clamp Lumen GI contribution inside cave volumes** — bounce light is the
   likely culprit for the RGB 30–80 floor. Terrain is already excluded from the
   Lumen surface cache (ADR-0001), so the leak is the coarse GI proxy.
4. **Cave-specific fog:** Void colour, shorter `λ` than surface fog.
5. **Post-process underground blend by depth:** an exposure floor, **not** a
   colour shift — the LUT stays constant (§3).

**Acceptance:** ≥ 70 % of pixels in an unlit L0 cave volume at RGB ≤ 25, and 0 %
above RGB 40, measured on a linearised capture with no torch in frame.

---

## 7. Blocks & Terrain

- Patrix kept as the **material truth** (the player recognises their blocks). No
  mass texture rework.
- **12 "ruin" variants** (cracked/mossy/sooted/scorched) of the most-seen blocks
  → new slices in the existing Texture2DArray (validate the 37→128 slice budget).
- **Deferred decals:** moss, soot, ash, blood, burn marks, torn posters. Best
  impact/cost ratio. No new BlockIDs.
- Vegetation: leaves and grass desaturated to olive/sick; overgrowth props (ivy,
  roots) blocked until a non-solid mesh path exists (only torches have one).
- Erosion reads through material + decal + fog, never through silhouette.

**Open defect (from `000850`):** block boundaries are not readable at 1–2 blocks
— the floor reads as a continuous mottled texture. No PBR response
(specular/roughness/normal) and no wear/erosion is visible. The texture pass must
validate block edges + wear in PIE before the 12 ruin variants are expanded (V5).

---

## 8. The Angel — the contrast is the mechanic

- **Material:** sculpted stone/plaster, `#E3DCCB`, high roughness, no metal, no
  emissive. Classic drapery, folded wings, **no eyes**.
- **No VFX, ever.** No particles, no aura, no glow. The only animation is
  movement and the grating of stone.
- **No walk cycle.** Only **8 statue poses** (pointing, head turned, hands on
  face, mid-stride, crouched, wings half-open, etc.) that read as "caught
  mid-gesture".
- **Acceptance criterion (angels.md §8.5):** a frozen statue is distinguishable
  without HUD, via three signals: smooth silhouette, material contrast, pose that
  changes between two glances.

### 8.1 A1 — re-based on actual visibility (decision 2026-11-12)

The old criterion "a clear, smooth, immobile shape at 60 blocks" is
**unreachable**: at 60 blocks the fog leaves `C = 0.002` (§5.1). Rather than
change the fog (settled), **A1 is re-based on the visibility the fog gives us.**

| Tier | Distance | Requirement | Measured 2026-11-12 (rev 2, corroborated) |
|---|---|---|---|
| **A1a** | **25 blocks** | reads as a **statue silhouette**: smooth, immobile, distinguishable from a mob | **NOT TESTED** — closest evidence is `105123` at 20–30 blocks, where the shape is ambiguous |
| **A1b** | **12 blocks** | reads as a **statue** (wings, drapery, pose) | **PASS** — `105207` at 10–14 blocks |
| **A1c** | **3 blocks** | reads as **bone stone** `#E3DCCB` | **FAIL** — body `#6E7C74` vs fog `#93A19A` → **`L_statue/L_fog = 0.56×`** (required ≥ 1.4×) |

**A1c is now measured, not described.** Two passes agree, and the number is worse
than the prose suggested: in linear luminance the statue sits at **0.56× the fog
behind it**, against a required **1.4×**. That is a **2.5× shortfall** — the
statue is not "slightly dark", it is *less than half* the background luminance
where it should be 40 % above it.

**The real failure is A1c, and it is the thesis failure.** The Angel must be the
brightest, whitest thing in the frame. It is currently a grey-green object that
is *darker* than the fog. "A white statue that should not be there" requires the
statue to be **lighter than its background at every distance**.

**New quantified criterion (non-negotiable):**

> `L_statue ≥ 1.4 × L_fog_background` at every distance from 3 to 25 blocks.

#### A1 decision (rev 2, binding) — the 60-block criterion is WITHDRAWN

`C(60) = 0.002`. At 60 blocks the fog transmits **0.2 % of contrast**: the statue
would need to be ~500× brighter than the fog to register. No material, lighting
or exposure choice achieves that inside the frozen contract, and the fog density
is settled (§5, and a standing user override). Keeping the criterion would mean
either lying about the build or destroying the fog — both worse than re-basing it.

| Was | Now |
|---|---|
| "a clear, smooth, immobile shape at **60 blocks**" | 3 tiers: **25 b** silhouette (A1a) / **12 b** statue (A1b) / **3 b** bone colour (A1c) |
| Fog tuned to reach 60 blocks | Fog kept at `λ ≈ 16`, `p ≈ 1.37`. Visibility **is** 25 blocks |
| `Angel_60b_fog.png` | **cancelled** → `Angel_25b_fog.png` (V1) |

**This is a deliberate scope *increase* in strictness, not a relaxation.** The old
criterion was one soft sentence at a distance nobody could check. The new one is
three hard thresholds at distances that exist, with a luminance ratio that a
script can measure.

#### Cascade required (owner: game design) — `angels.md`

`angels.md` carried `PlayerObserveRange = 60 blocks`, justified as *"below
the 96-block render distance, so observation never exceeds visibility."* **That
justification is now false**: visibility is 25 blocks, not 96. The build has a
**25–60 block band where the Angel freezes for an observer who cannot possibly
see it** — the freeze fires on a line-of-sight test that no longer corresponds to
what is on screen.

Three options were presented (a mechanics decision, not an art one):

| Option | `PlayerObserveRange` | Consequence | Status |
|---|---|---|---|
| **A** | **25 blocks** | Freeze == what the player sees. Honest, testable, and it makes the fog a *gameplay* rule rather than decoration. Cost: Angels feel less "long-range hunted" | **DECIDED 2026-11-12** |
| B | 60 kept, as intended horror | You freeze what you cannot see — defensible, but the player will read it as a bug on first contact unless the audio sells it | Rejected |
| C | 60 kept + audio tell | B plus a stone-grind cue when an unseen Angel freezes. Costs a sound and a HUD-free feedback channel, buys the horror back | Rejected |

> **Decision (2026-11-12, user-approved): Option A — `PlayerObserveRange` 60 → 25.**
> Written to `design/gdd/angels.md` (§2.2 observers table, §4 formulas, §10 open
> questions). Freeze now equals what is on screen. Accepted trade-off: Angels feel
> less "long-range hunted". Consequence for P2.5: **the pose set must read at 25
> blocks, not 60.** `CameraObserveRange` is unchanged at 120 — a camera has no eyes
> and is not subject to the same fog argument; that is a separate question.

**Ordered fix list (rev 2 — re-prioritised, one fix does almost all the work):**

0. **Audit result:** `M_WeepingAngel_Body` / `_Face` / `_Wings` exist, with
   `T_WeepingAngel_*_D` and `_N` — but **no `_S` specular map**, and no evidence
   the base colour is authored as `#E3DCCB`. Start here; do not start with lighting.
1. **Material base colour → exact `#E3DCCB`.** This alone is projected to clear
   the criterion: the measured body is `#6E7C74`, whose linear luminance is
   `0.190` vs fog `0.340`. Swapping the albedo to bone (`L = 0.718`) is a
   **×3.78 luminance scale** and projects `L_statue/L_fog = 2.11×` — comfortably
   above the `1.4×` target, with margin to spare. **One material fix, no lighting
   tricks, no VFX.** Verify before touching anything else.
2. **Kill the green bias.** Measured body has `G − R = +14`; bone has `G − R = −7`.
   A **21-point green excess** — either the albedo is green, or foliage GI is
   tinting it. Fix in the albedo first (fix 1 already implies it), then check GI.
3. **Statue lighting priority** — *only if* fix 1 lands below `1.4×`. A rim or
   backlight raising the statue above the fog's inscattered luminance. Still no
   VFX: this is lighting, not glow.
4. **Re-capture A1a at exactly 25 blocks**, no HUD (V1).
5. **Validate the cheap way first:** the main menu has a fixed camera and a hero
   statue (§13). Prove `L_statue ≥ 1.4 × L_fog_background` there before touching
   the world. It is a ten-minute test versus a PIE capture session.

### 8.2 Consequence to resolve in `angels.md` (owner: game design)

`PlayerObserveRange = 60 blocks` (angels.md) but visual range is **25 blocks**.
The 25–60 block band is **blind observation**: the Angel freezes while the player
cannot see it. Either that is the intended horror (you freeze something you cannot
see), or `PlayerObserveRange` should drop to ~25 to match.

**Resolved 2026-11-12:** Option A — `PlayerObserveRange` is now **25 blocks** in
`angels.md`. The art side has no further input; the cascade is written and the
mechanics document is the source of truth for the value. This had to be decided
before P2.5 (Angel poses), because the pose set is designed to read at a distance,
and that distance is now 25 blocks.

---

## 9. Mobs — stylized low-poly

Not voxel-rigged, not photorealistic. **Low-poly stylized horror:** readable
silhouettes, simple but articulated geometry, matte desaturated materials. Mobs
are creatures of the world (flesh, bone, chitin); the Angel is stone.

| Mob | Visual read |
|---|---|
| Zombie | Torn work clothes, exposed ribs, dislocated jaw. Slow, hunched. |
| Skeleton | Stained, broken bones; half-dressed (police uniform?). |
| Spider | Low and wide, 8 articulated legs, chitin plates, tufts. Must read as a shape at night. |
| Creeper | The only organic form among voxels: swollen, moss-covered. Explosion tell = swelling + sodium cracks from within. No face. |

One shared humanoid rig (`SK_Mob_Base`, 6 bones, rigid weights) covers Zombie +
Skeleton. Spider gets its own skeleton (8 legs).

**No mob appears in any of the 12 captures.** Mob visual identity is unvalidated.

---

## 10. Tools, Weapons, Hands

- 5 tools × 5 tiers = 25 meshes. Minecraft silhouettes conserved (muscle memory)
  + scavenged detailing: taped grip, wire-wrapped handle, repaired guard.
- Tier reads through material: raw wood → cracked stone → pitted iron →
  **diamond = the only "pure" material (cold white-blue)** → obsidian = black glass.
- **First-person hands:** rolled sleeves, dirty gloves, a broken watch on the
  wrist. Free narrative detail: time no longer belongs to you.
- 5 swing sets (pickaxe/shovel/axe/hoe/sword), not 25.

---

## 11. UI

Reference: `design/ui/ui-direction.md` (authoritative, approved 2026-11-10).
Three additions from art direction:

1. **Baubles panel** missing → `WBP_Baubles_Root`, priority 2.
2. Legacy textures `quick_slots` / `Item_BG_Panel` / `Highlight_Slot` (Minecraft
   vanilla style) are incompatible with enamel — they die with `WB_HUD` in phase 5.
3. **Icon treatment:** Patrix icon unchanged + inner shadow + 2px sodium ring on
   selection. Do not repaint 1,801 icons.

**UI hygiene defects visible in the captures (fix in P2):**
- The orange HUD bar steals the warm-light role → §3.1.
- `"Default Block"` / `"Grass"` item labels render above the hotbar.
- Hotbar selection state is inconsistent across captures (slot 9 in `105016`,
  slot 1 elsewhere).
- `"Cobblestone / Durability: 2.5"` tooltip in `105502` is a debug element.

---

## 12. Weather & Day/Night

Base ambiance is oppressive (sodium/cold night, desaturated, ash fog). A dynamic
weather + day/night system modulates mood:

- **Day:** clearer but still desaturated, better visibility, mobs burn. The
  player breathes.
- **Night:** oppressive, reinforced fog, mobs active, sodium = only warm light.
- **Weather:** rain (audio wash, wet surfaces, more reflections), thick fog
  (reduced visibility, tension), ash storm (particles, wind, noise), clear sky
  (rare, almost unsettling by contrast).
- **Dynamic transition:** the global grade (LUT) stays constant; only light
  intensity/filtration changes. The player feels the weather without a colour shift.

**Constraint (new):** weather may modulate fog `λ` by at most ±40% around the
base 16 blocks, and must never push `C(12)` below 0.35 — otherwise the Angel
becomes unreadable even at A1b range.

---

## 13. Main Menu

Improve the existing `Lvl_MainMenu` (concrete/void/floor materials, Angel statue,
pointing arm) to perfection: lighting grade, material polish, statue readability,
LAMPLIGHTER UI on top, fix the engine cached-lighting error message. The menu
becomes a **kept promise** — real game blocks at 100 UU scale.

Note: the menu is the cheapest place to prove A1c (`L_statue ≥ 1.4 ×
L_fog_background`), because it has a fixed camera and a hero statue.

---

## 14. Reference Captures (12 files, `Docs/art/mood-board/`)

Machine-readable version: `Docs/art/mood-board/manifest.csv`. That manifest is
the single index; this table is its readable mirror.

| # | File | Set | Content | Status |
|---|---|---|---|---|
| 1 | `Screenshot 2026-05-23 202820.png` | BEFORE | Pre-overhaul build | **EXCLUDED — BEFORE (A6)** |
| 2 | `Screenshot 2026-11-11 000733.png` | 1 | Terrain at distance, dense fog, no torch | Conditional |
| 3 | `Screenshot 2026-11-11 000817.png` | 1 | Mid-range terrain, no biome boundary visible | Conditional |
| 4 | `Screenshot 2026-11-11 000850.png` | 1 | Close-up blocks; **boundaries not readable** | Conditional — V5 |
| 5 | `Screenshot 2026-11-11 000917.png` | 1 | Labelled "cave"; actually outdoor forest | **EXCLUDED — invalid capture** |
| 6 | `Screenshot 2026-11-11 001008.png` | 1 | Pale ground, no saturated orange | Approved (palette only) — V8 |
| 7 | `Screenshot 2026-11-11 105016.png` | 2 | Open terrain, very dense fog, no Angel | Conditional |
| 8 | `Screenshot 2026-11-11 105123.png` | 2 | Open terrain, whiteout fog; **ambiguous shape** at 20–30 b | Conditional — A1a |
| 9 | `Screenshot 2026-11-11 105207.png` | 2 | **Angel at 10–14 blocks**, reads as statue, decoloured | **Approved — A1b evidence** |
| 10 | `Screenshot 2026-11-11 105250.png` | 2 | **Angel at ~3 blocks**, sharp silhouette, grey-green | **Approved — A1c evidence (defect)** |
| 11 | `Screenshot 2026-11-11 105418.png` | 2 | Wooden structure exterior, torch, warm pool + falloff | **Approved — A3 framing (verdict contested)** |
| 12 | `Screenshot 2026-11-11 105502.png` | 2 | Interior, torch, debug tooltip | Conditional |

### 14.1 What the set proves, and what it does not

**Proves:** worn voxel world; desaturated palette; dense ash fog; block scale;
the Angel's silhouette at close range; the existence of torches; and (rev 2) that
torches *do* cast a warm, falling-off pool at close range — `R_pool` 1.89 / 2.30.

**Does not prove:** the sodium hierarchy (contested — see §4.2, not settled either
way); the Angel as white stone (it renders grey-green, `0.56×` its background);
any cave; any mob; any sky (see below); any biome transition (no boundary is
visible in `000817`).

### 14.2 A2 — the sunset question, answered (rev 2)

**A2 asks: does the sunset violate the sodium palette?** Three separate answers,
and they are all needed:

**1. There is no sunset in the game.** `UDayNightCycleSystem` rotates the sun
(`UpdateSunRotation`) and drives sky-light intensity, but **authors no colour
keyframes** — the sun's colour is whatever the level's directional light happens
to be. There is no dawn/dusk grade, no warm horizon, no orange disc. So the rule
is not violated: **there is nothing yet to violate it.** A2 is a PASS on the rule
and a FAIL on having a sky at all.

**2. The captures confirm the rule holds today.** A warm-colour census of
`105418`, `105502`, `105250` finds the only *saturated* warm element in any frame
is the **UI bar** `~#F0A020` (0.73 % of frame, every capture). The flame is sodium
(permitted). The terrain's warm tones are muted browns — `#8A6A42`, `#6B5A3A`,
`#B0A078` — which sit in the **Dust `#8C8577`** family and are legitimately warm-
neutral, not saturated. **No sunset violation.**

**3. The rule must become a test before the sky is authored.** "No saturated warm
colour" is currently an adjective. It becomes falsifiable as:

> **A2 acceptance:** in any frame, **no pixel** with hue 20–50°, saturation
> ≥ 0.45 and value ≥ 0.55 — except (a) emissive sodium sources (torch, lamp,
> furnace, generator), and (b) Rust/Blood `#C8462F` danger elements.

**Sunset authoring constraint (for P2.1):** the dusk transition is **Bone
`#E3DCCB` → Ash `#5C6268` → cold teal-dark**. The sun disc may desaturate and
warm *slightly* toward Dust `#8C8577` at the horizon, but must never enter the
sodium hue. **The only warm light in the game is the one the player installs.**
A sunset that lights the world orange is a pillar violation even if it is
beautiful — it hands the player a free light source and makes their torch
redundant.

**`001008` status:** remains approved as **palette evidence only**. Two analyst
passes still disagree on whether a sky band exists in it; that is unresolvable
from a compressed PNG and is not worth a third pass. A true sky capture is
required (V8).

---

## 15. Adjustments A1–A6 — single authoritative status (2026-11-12, rev 2)

| # | Domain | Status | Decision |
|---|---|---|---|
| **A1** | Angel silhouette | **RE-BASED + measured** | 60 blocks is unreachable (`C(60) = 0.002`). **Three tiers: 25 b silhouette (A1a, untested) / 12 b statue (A1b, PASS) / 3 b bone colour (A1c, FAIL at `0.56×` vs the required `1.4×`).** Rule: `L_statue ≥ 1.4 × L_fog_background`, 3→25 b. **The fix is one albedo change** (projected `2.11×`). **Cascade WRITTEN 2026-11-12:** `angels.md` `PlayerObserveRange` 60→25. |
| **A2** | Sunset / sky palette | **PASS on the rule — nothing to violate** | **No sunset exists**: the day/night system authors no sun colour. Warm census on 3 captures: only saturated warm element is the UI bar `~#F0A020` (0.73 %). Terrain browns are Dust-family, legitimate. Rule is now falsifiable (hue/sat/value thresholds, §14.2). Sunset authoring constrained to Bone→Ash→teal. Sky still unverified → V8. |
| **A3** | Torch dominance | **CONTESTED — was FAIL, not reproducible** | Pass 2 measures `R_peak 2.14/2.40`, `R_pool 1.89/2.30`, bloom 40–200 px — **pass on all three**. Pass 1's stated cause ("the floor is brighter") describes the torch's own pool. **Fix list frozen** pending a sampler-based V2. One fix is unconditional: torch light colour `(1.0,0.78,0.50)` is not sodium. Real defect: **UI area dominance**, `TDR ≈ 2.7` vs target 3.0. |
| **A4** | Fog curve | **CLOSED — curve settled, authoring not done** | `λ ≈ 16 b`, `p ≈ 1.37`, anchors `C(12)=0.50`, `C(25)=0.15`. **`p > 1` cannot be produced by `ExponentialHeightFog` alone** (`p = 1`); needs a base actor + a low-altitude layer. Calibration procedure in §5.5. **No fog actor is authored in the build yet** — P2.1 creates it, then re-fits. |
| **A5** | Cave void levels | **OPEN — reachable, not captured** | Caves now generate (`VoxelCaveRules`), so the capture is possible. Darkest available RGB 30–80 vs Void 11/14/17 → 3–5× too bright. **Void is now a 4-level ladder** (L0 absolute / L1 torch pool / L2 mouth / L3 structure, §6.1). Acceptance: L0 ≥70 % ≤ RGB 25 and 0 % > RGB 40. |
| **A6** | Early-reference hygiene | **CLOSED — labelled** | File is `Screenshot 2026-05-23 202820.png` (docs previously cited a non-existent `000820`). Confirmed pre-overhaul: flat vanilla lighting, no sodium, no ash fog, no post-process. Excluded; `status=excluded` in the manifest; **folder-level `README.md` added** so nobody browsing the directory mistakes it for a target. |

**Net (rev 2):** 2 closed (A4, A6), 1 re-based and now measured (A1), 1 closed on
the rule (A2), **1 re-opened as contested (A3)**, 1 open but now reachable (A5).

**What rev 2 actually bought:** the art bible previously reported *two* failed
pillars. One of them (sodium) was a measurement error, and the other (the Angel)
turns out to be a single material albedo away from passing. The project is in
better shape than the document said — and the document now says so.

---

## 16. Open Validations

| # | Validation | Blocks | Owner | Status |
|---|---|---|---|---|
| V1 | `Angel_25b_fog.png` — statue silhouette at exactly 25 blocks, no HUD | A1a | Art Director + Technical Artist | OPEN |
| V2 | **Sampler-based** torch measurement (`R_peak`/`R_pool`/`R_bloom`/`TDR`/`A_sodium`) on the §4.2 capture protocol | A3 | Technical Artist | **OPEN — re-specified, blocks the A3 fix list** |
| V3 | `L_statue ≥ 1.4 × L_fog_background` at 3/12/25 blocks | A1c | Technical Artist | OPEN |
| V4 | Real L0 cave capture + GI clamp: ≥70 % pixels ≤ RGB 25, 0 % > RGB 40 | A5 | Technical Artist | OPEN — **now reachable** |
| V5 | Block boundaries + wear readable in PIE | §7 texture pass | Technical Artist | OPEN |
| V6 | Fog colour stays inside Ash across biomes (green bias) | §5.4 | Art Director | OPEN |
| V7 | UI accent off sodium (`#C08A45`), HUD stops competing with world light | §3.1 | Art Director + UI Designer | OPEN |
| V8 | True sky capture (horizon + sun visible) for grade/sky reference | A2 caveat | Art Director | OPEN |
| **V9** | **`M_WeepingAngel_*` base colour == `#E3DCCB`** (audit + fix). Cheapest possible thesis test — do it in the main menu first | A1c | Technical Artist | **OPEN — new, do first** |
| **V10** | **Author the fog** (§5.5: base actor + low-altitude layer), then re-fit `C(12)=0.50` / `C(25)=0.15` | A4 authoring | Technical Artist | **OPEN — new, P2.1** |
| **V11** | **Torch light colour → sodium `(0.910, 0.639, 0.239)`**; verify `MaxTorchLights=16` cap is acceptable in a torch-dense room | A3 fix 1 | Technical Artist | **OPEN — new, unconditional** |

`Angel_60b_fog.png` is **cancelled**. Its replacement is `Angel_25b_fog.png` (V1).

**Order of work:** V9 → V11 → V3 → V2 → V1 → V10 → V4 → V6 → V8 → V7 → V5.
Rationale: V9 and V11 are single-value fixes with a measured pass criterion, so
they buy the two thesis signals before any capture session. V2 is deliberately
*after* V9/V3, because the statue measurement and the torch measurement compete
for the same exposure budget — fix the statue first, then judge the torch.

---

## 17. P2 — Visual overhaul, 1 biome vertical slice

### 17.1 Objective

Ship **one biome vertical slice**: a complete biome (surface + underground + POI
+ transition) textured, lit and playable under the LAMPLIGHTER grade, validated
in PIE and by mood-board capture. The slice must prove "Eroded Realism" end to
end on a narrow perimeter before expanding to every biome.

### 17.2 Biome

**Forest** — most representative of the "worn voxel world"; adjacent to Horror
Forest via the Blight transition (validates biome transition); contains POIs
(outposts, low hospitals) that give interior lighting and A5 void levels a test
bed; reachable from Plains (validates Plains↔Forest).

Fallback: **Plains** if long-range readability and sightlines must be validated
before forest transitions.

### 17.3 Lots

| Lot | Deliverable | Depends on | Priority |
|---|---|---|---|
| **P2.0a** | **V9 + V11: `M_WeepingAngel_*` albedo → `#E3DCCB`; torch light colour → sodium.** Two single-value fixes, both with a numeric pass criterion | — | **Blocking — do first, half a day** |
| **P2.0b** | **V2 sampler measurement + V3 statue ratio + V1 `Angel_25b_fog.png`.** Settles A3 and closes A1 | P2.0a | **Blocking** |
| **P2.0c** | **V10: author the fog** (§5.5 two-component), re-fit `C(12)`/`C(25)`. No grade work can be judged before the fog exists | P2.0a | **Blocking** |
| P2.1 | Grade + post-process + LUT + sky/fog presets + light presets | P2.0c | **Critical** — transforms 100% of the game |
| P2.2 | Texture pass: block boundaries + wear in PIE | P2.1 | High |
| P2.3 | Ruin variants (Texture2DArray slices, 37→128 budget) — **4 of 12 exist as standalone meshes, see gap G2** | P2.2 | High |
| P2.4 | Deferred decals: moss, soot, ash, blood, burn marks, posters | P2.3 | High |
| P2.5 | Angel: 8 poses + material fix + readability pass (§8.1) | P2.0b, P2.1 | **High** — thesis |
| P2.6 | `Lvl_MainMenu` rebuilt in voxels + unified materials | P2.1 | Medium |
| P2.7 | UI kit: `WBP_Common_*`, Title, Pause, Settings + HUD restyle | P2.1 | Medium |
| P2.8 | First-person hands + 5 tool classes (wood tier first) + 5 swings | P2.1 | Medium |
| P2.9 | Vanilla mobs: Zombie + Skeleton → Creeper → Spider (`SK_Mob_Base` exists) | P2.1 | Medium |
| P2.10 | Loot props (5) + POI prop kits | P2.1 | Low |
| P2.11 | VFX: dust by material, creeper sodium flash | P2.1 | Low |

### 17.4 Dependency gaps found in the P1 → P2 handover (rev 2)

These block P2 lots and were not visible in the previous plan. Each needs an
owner before the lot starts.

| # | Gap | Blocks | Consequence |
|---|---|---|---|
| **G1** | **No grade exists at all.** No post-process material, no LUT, no colour grading in `Content/`. `M_PP_Fog` is a weather material, not the Sodium Grade | P2.1, and every acceptance criterion downstream | The "Sodium Grade" has never been built. P2.1 is not a polish pass; it is the first implementation |
| **G2** | **The ruin variants are meshes, not array slices.** `SM_Block_ConcreteCracked`, `_MossyStone`, `_SootedBrick`, `_ScorchedWood` exist as standalone StaticMeshes with their own `MI_` — the plan calls for Texture2DArray slices on `M_Terrain_Opaque` | P2.3 | 8 variants × 3 maps = 24 new slices (37→61, inside the 128 budget), but the *mechanism* diverges: mesh-per-variant costs a draw call each. Decide: continue mesh-per-variant, or migrate to slices |
| **G3** | **`M_Terrain_Opaque` does not exist.** Terrain still renders through `M_Global` / `M_Item_PBR2` | P2.2 | The texture pass has no master material to pass through. Creating it is a prerequisite, not a side effect |
| **G4** | **`MaxTorchLights = 16` is a hard cap** in the light pool | P2.4, P2.10, POI interiors | A torch-dense room silently loses warm light after the 16th torch. Either raise the cap, or make the cap a *design* rule (only 16 torches light a scene, the rest are decorative) |
| **G5** | **`M_VFX_AngelDistort` exists** with a "refraction distortion on Angel move" parameter | Violates §8 "no VFX, ever" | Either delete the material or re-scope it. As written it is a direct contradiction of the thesis rule |

### 17.5 Slice acceptance criteria

1. A screenshot is identifiable without a logo as *worn voxel world, sodium light,
   a white statue that should not be there.*
2. Block boundaries readable at 1–2 blocks.
3. **`R_peak ≥ 2.0`, `R_pool ≥ 1.5`, `R_bloom ≥ 8 px`, `TDR ≥ 3.0`**, measured
   with a sampler on the §4.2 capture protocol — not eyeballed.
4. **`L_statue ≥ 1.4 × L_fog_background`** at 3/12/25 blocks; statue silhouette
   readable at 25 blocks.
5. Unlit **L0** cave volumes: ≥70 % at RGB ≤ 25, 0 % above RGB 40 (§6.1).
6. No pixel at hue 20–50°, sat ≥ 0.45, value ≥ 0.55 outside sodium and rust (§14.2).
7. Biome transition (Forest↔Blight or Forest↔Plains) visible but not jarring.
8. 60 FPS on target (GTX 1060, 1080p60, Lumen) inside the existing budgets.

### 17.6 Risks

| Risk | Impact | Mitigation |
|---|---|---|
| Angel material stays grey-green | Thesis fails at signal 3 | **V9 first, in the main menu** — ten minutes, fixed camera, hero statue. Not a PIE session |
| Torch "fixed" when it was never broken | Over-bright torch, blown fog, statue lost against a blazing foreground | **A3 fix list frozen.** Only the colour fix (V11) is unconditional. Measure before tuning |
| Grade built on top of unauthored fog | Every acceptance criterion judged against the wrong atmosphere, then re-judged | **V10 before P2.1.** The fog is the measuring stick |
| Volumetric/height fog cost on GTX 1060 | Frame budget | Profile with Unreal Insights at P2.1, not after. §5.5's second layer is the expensive part |
| Texture2DArray budget 37→128 | Slice overflow | 24 new slices planned → 61 used. Safe, *if* G2 is resolved toward slices |
| Angel has no rig (art-direction §12) | 8 poses need a rig or 8 static meshes | 8 static meshes already imported — settled unless animation is needed |
| `PlayerObserveRange` 60 vs visual 25 | Blind observation band | **RESOLVED 2026-11-12 — Option A, 25 blocks** (`angels.md` §2.2/§4). Pose set must now read at 25 b, not 60 |

---

## 18. Design Test

In six months, a screenshot of the game must be identifiable without a logo:
*worn voxel world, sodium light, a white statue that should not be there.* If a
player describes the Angel as "a mob", the contrast has failed.

**Current verdict (rev 2):** 1 of 3 signals validated (worn voxel world). The
statue signal is **measurably failed and one albedo change away from passing**
(`0.56×` → projected `2.11×` against a `1.4×` target). The sodium signal is
**contested, not failed** — the only failing measurement was not reproducible, and
its stated cause was a misattribution. The remaining certain defect is the **UI
accent**, which is stealing the warm-light role in the same hue the torch needs.

That is a materially better position than the previous revision reported, and it
changes the order of work: **fix two values, measure once, then build the grade.**

---

## 19. Frozen mood board — the approved P2 target set

This section is the **finalisation** of the P1 mood board. It states, for each
approved capture, what it is a target *for* and what it does **not** license. The
index remains `Docs/art/mood-board/manifest.csv`; this is the readable contract.

**Approved = a P2 target. Conditional = one narrow use, named. Excluded = never a
target.** No capture joins this set without closing its V-item (§16).

| Capture | Approved as | Target for | Does **not** license |
|---|---|---|---|
| `001008` | **Palette reference** | Desaturated world palette; no saturated warm outside sodium/rust; Dust-family terrain | Sky grade, horizon, sun, fog colour (sky is occluded) |
| `105207` | **A1b target — the one that must be reproduced** | Angel reading as a *statue* at 10–14 blocks: wings, drapery, hands-on-face, decoloured by fog | Statue luminance — the fog has already eaten the colour at this range; brightness must be proven at 3 b (A1c) |
| `105250` | **A1c negative reference** | The exact defect to eliminate: `#6E7C74` body vs `#93A19A` fog = `0.56×`. P2 must produce its inverse | Any part of its look. This frame is the *anti-target* |
| `105418` | **A3 reference (contested)** | Torch framing and the warm-pool *falloff* shape (`R_pool 1.89`, `TDR ≈ 2.7`) | The A3 verdict — it is disputed. Do not tune against it |
| `105502` | **A3 reference (contested)** | Interior torch dominance at close range (`R_pool 2.30`, `TDR ≈ 10.3`) — the framing that *works* | The debug tooltip, the HUD, or the floor brightness as an absolute |

**Conditional set** (`000733`, `000817`, `000850`, `105016`, `105123`) is usable
for fog, terrain and block-scale reference only, and each is subject to V5
(block readability). `105123` remains the only near-A1a evidence and is
**ambiguous** — it cannot be promoted without V1.

**Excluded, permanently:** `Screenshot 2026-05-23 202820.png` (pre-overhaul, A6)
and `000917` (labelled "cave", actually an outdoor forest).

### 19.1 What the frozen set is missing (P2 must supply)

The board is complete as a *defect record* and incomplete as a *target set*. Five
frames are needed before P2 can be judged:

| Missing capture | Why it matters | V-item |
|---|---|---|
| Angel at exactly **25 blocks**, no HUD | A1a has never been tested. It is the tier the whole fog decision rests on | V1 |
| **True sky** with horizon + sun | A2 has never seen a sky. The grade's most visible surface is unauthored and unphotographed | V8 |
| **L0 cave** interior | The darkest state in the game, never captured. A5 is unmeasured | V4 |
| **Torch with HUD** and **torch without HUD**, same framing | `TDR` requires both; no existing pair exists | V2 |
| **Biome transition** (Forest↔Blight) | No capture shows a boundary; the vertical slice's headline claim is untested | §17.5 #7 |

**Capture naming from here on:** `<tier>_<subject>_<distance>_<state>.png`, e.g.
`A1a_Angel_25b_fog.png`, `A5_Cave_L0_nolight.png`, `A3_Torch_5b_HUD.png`. The
P1 filenames are timestamp-based and are kept for traceability, but no new
capture may use a timestamp name — the manifest maps names to purposes, and
`Angel_60b_fog.png` → `Angel_25b_fog.png` already showed how easily a timestamp
hides what a frame was supposed to prove.

---

## Cross-references

- Creative direction: `design/art/art-direction.md`
- Technical visual guide: `Docs/art/style-guide.md`
- Mood board: `Docs/art/mood-board.md` · manifest: `Docs/art/mood-board/manifest.csv`
- Material parameters: `Docs/art/material-parameters.md`
- Technical contract: `Docs/technical-contract.md`
- UI direction: `design/ui/ui-direction.md`
- Biome grammar: `design/levels/biome-grammar.md`
- Game concept: `design/gdd/game-concept.md`
- Angels GDD: `design/gdd/angels.md`

---

*Last updated: 2026-11-12 — **rev 2, P1 closed**. A3 re-opened as contested after
a contradicting measurement; A1 measured (`0.56×`); A2 answered (no sunset
exists); A4 curve authoring procedure added; A5 void ladder; A6 labelled;
mood board frozen as the P2 target set (§19); P2 dependency gaps G1–G5 recorded.
**Post-rev-2 decision (same day):** `PlayerObserveRange` 60 → 25 approved and
cascaded into `angels.md`; P2.0 order (V9 → V11 → V3 → V2 → V1 → V10) approved;
rev 2 left uncommitted in the working tree by user choice.
Owner: Creative Director.*
