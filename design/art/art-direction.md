# Art Direction — LAMPLIGHTER

**Status:** Accepted
**Date:** 2026-11-10
**Owner:** Creative Director
**References:** design/ui/ui-direction.md, Docs/SYSTEMS_REFERENCE.md, ADR-0001, ADR-0004

## 1. Thesis

> **Eroded Realism — the world is built, the Angel is sculpted.**

The world is made of worn blocks and sodium light. The Weeping Angel is the only
thing in the game that was not built from blocks — and therefore the only thing
that does not belong to the world. That contrast is the visual identity and the
mechanic. A screenshot must be identifiable without a logo: *worn voxel world,
sodium light, a white statue that should not be there.*

## 2. Palette

| Hex | Name | World use | UI use |
|---|---|---|---|
| `#0B0E11` | Void | night sky, unlit interiors, fog | modal scrim (85%) |
| `#171C21` | Concrete | cut stone, POI walls, floors | panel surface |
| `#2A3138` | Steel | metal, roofs, bars, machines | disabled state |
| `#E3DCCB` | Bone | plaster, overcast sky, **Angel stone** | primary text |
| `#E8A33D` | Sodium | **the only warm light**: torch, lamp, furnace, generator | accent, focus, selected |
| `#C8462F` | Rust/Blood | creeper, damage, corrosion, explosion | danger, error |
| `#4FA6A0` | Signal | electricity, cameras, CRT — the only "clean" light | success, active toggle |
| `#5C6268` | Ash | fog, tarnished metal, disabled | secondary text |
| `#6B7355` | Moss | lichen, residual vegetation | — |
| `#9AA36B` | Sick green | Horror Forest, sick leaves | — |
| `#8C8577` | Dust | sand, rubble, ash on ground | — |

**Rules (non-negotiable):**
- One warm source only: sodium. Everything else is cold, desaturated, ashen.
- No saturated blue sky. Day sky is bone-ash; night is cold teal-dark.
- XP orbs desaturated to `#B9C46A` — the only "game-y" element, kept discreet.
- Rust red is reserved for danger. It never decorates.

## 3. Light and grade

- Post-process + single LUT "Sodium Grade": high contrast in lows, soft rolloff
  in highs, 25-35% desaturation outside sodium.
- Ash fog: **measured** at `λ ≈ 16 blocks`, `p ≈ 1.37`. Silhouette readability
  ends at **25 blocks**, material/colour at ~8–12 blocks. *(The former "60-90
  blocks" target was withdrawn on 2026-11-12 — it is unreachable at this density,
  and the density is kept. See `Docs/art/art-bible.md` §5.)*
- Consequence: the Angel must read as a statue silhouette at **25 blocks**
  (A1a), as a statue at **12 blocks** (A1b), and as bone stone at **3 blocks**
  (A1c). It must be **lighter than the fog behind it at every distance**:
  `L_statue ≥ 1.4 × L_fog_background`.
- The torch is the brightest object on screen, always. That is the visual
  hierarchy of the fantasy. Quantified: `R_peak ≥ 2.0` (torch peak vs fog band),
  `R_pool ≥ 1.5` (ground within 3 blocks vs same material far from any torch),
  `R_bloom ≥ 8 px` (sodium reads as light, not a decal), and
  `TDR = A_sodium / A_uiwarm ≥ 3.0` (the world's warm light must out-area the
  UI's warm accent). Status: **contested** — see §14 and `art-bible.md` §4.2.
- Day/night cycle (20 min) is kept; the grade is **constant**. Only the player's
  light changes. Phase 3 never brightens the world — the player installs light.

## 4. Blocks and terrain

- Patrix is kept as the **material truth** (the player recognises their blocks).
  No mass texture rework.
- **12 "ruin" variants** (cracked/mossy/sooted/scorched) of the most-seen
  blocks → new slices in the existing Texture2DArray (validate 37→128 slice budget).
- **Deferred decals:** moss, soot, ash, blood, burn marks, torn posters. Best
  impact/cost ratio in the project. No new BlockIDs.
- Vegetation: leaves and grass desaturated to olive/sick; overgrowth props
  (ivy, roots) blocked until a non-solid mesh path exists (only torches have one).
- Erosion reads through material + decal + fog, never through silhouette.

## 5. Vanilla mobs — stylized low-poly

Not voxel-rigged, not photorealistic. **Low-poly stylized horror:** readable
silhouettes, simple but articulated geometry, matte desaturated materials.
Mobs are creatures of the world (flesh, bone, chitin); the Angel is stone.

| Mob | Visual read |
|---|---|
| Zombie | Torn work clothes, exposed ribs, dislocated jaw. Slow, hunched. |
| Skeleton | Stained, broken bones; half-dressed (police uniform?). |
| Spider | Low and wide, 8 articulated legs, chitin plates, tufts. Must read as a shape at night. |
| Creeper | The only organic form among voxels: swollen, moss-covered. Explosion tell = swelling + sodium cracks from within. No face. |

One shared humanoid rig (`SK_Mob_Base`, 6 bones, rigid weights) covers Zombie +
Skeleton. Spider gets its own skeleton.

## 6. The Angel — the contrast is the mechanic

- **Material:** sculpted stone/plaster, `#E3DCCB`, high roughness, no metal, no
  emissive. Classic drapery, folded wings, **no eyes**.
- **No VFX, ever.** No particles, no aura, no glow. The only animation is
  movement and the grating of stone.
- **No walk cycle.** Only **8 statue poses** (pointing, head turned, hands on
  face, mid-stride, crouched, wings half-open, etc.) that read as "caught mid-gesture".
- **Acceptance criterion (angels.md §8.5):** a frozen statue is distinguishable
  without HUD. Resolved by three signals: smooth silhouette, material contrast,
  pose that changes between two glances.
- At 25 blocks in fog (the measured visibility limit): a clear, smooth,
  immobile shape, backlit. Enough. At 12 blocks it reads as a statue; at 3
  blocks it must read as `#E3DCCB` bone. The 60-block criterion is cancelled.

## 7. Tools, weapons, hands

- 5 tools × 5 tiers = 25 meshes. **Minecraft silhouettes conserved** (muscle
  memory) + scavenged detailing: taped grip, wire-wrapped handle, repaired guard.
- Tier reads through material: raw wood → cracked stone → pitted iron →
  **diamond = the only "pure" material (cold white-blue)** → obsidian = black glass.
- **First-person hands:** rolled sleeves, dirty gloves, a broken watch on the
  wrist. Free narrative detail: time no longer belongs to you.
- 5 swing sets (pickaxe/shovel/axe/hoe/sword), not 25.

## 8. UI

Reference: `design/ui/ui-direction.md` (authoritative, approved 2026-11-10).
Three additions from art direction:
1. **Baubles panel** missing → `WBP_Baubles_Root`, priority 2.
2. The legacy textures `quick_slots`/`Item_BG_Panel`/`Highlight_Slot` (Minecraft
   vanilla style) are incompatible with enamel — they die with `WB_HUD` in phase 5.
3. **Icon treatment:** Patrix icon unchanged + inner shadow + 2px sodium ring on
   selection. Do not repaint 1,801 icons.

## 9. Weather and day/night

Base ambiance is oppressive (sodium/cold night, desaturated, ash fog). A dynamic
weather + day/night system modulates mood:
- **Day:** clearer but still desaturated, better visibility, mobs burn. The player
  breathes.
- **Night:** oppressive, reinforced fog, mobs active, sodium = only warm light.
- **Weather:** rain (audio wash, wet surfaces, more reflections), thick fog
  (reduced visibility, tension), ash storm (particles, wind, noise), clear sky
  (rare, almost unsettling by contrast).
- **Dynamic transition:** the global grade (LUT) stays constant; only light
  intensity/filtration changes. The player feels the weather without a colour shift.

## 10. Main menu

Improve the existing `Lvl_MainMenu` (concrete/void/floor materials, Angel
statue, pointing arm) to perfection: lighting grade, material polish, statue
readability, LAMPLIGHTER UI on top, fix the engine cached-lighting error message.
The menu becomes a **kept promise** — real game blocks at 100 UU scale.

## 11. Priority assets (order = screen time ÷ cost)

1. Grade + post-process + LUT + sky/fog + light presets (transforms 100% of the game, marginal cost)
2. `Lvl_MainMenu` rebuilt in voxels + unified menu materials
3. UI kit: `WBP_Common_*`, Title, Pause, Settings + HUD restyle
4. First-person hands + 5 tool classes (wood tier first) + 5 swings
5. Angel: 8 poses + statue readability pass + material
6. Vanilla mobs: shared rig → Zombie + Skeleton → Creeper → Spider
7. 12 ruin variants + decal kit + overgrowth props
8. 5 loot props (flesh, bone, string, eye, arrow)
9. VFX/SFX-visual pass (dust by material, creeper sodium flash)
10. POI prop kits (hospital, police station, village)

## 12. What art cannot fix (state now)

1. Fence, Fence Gate, Door, Glass are **opaque full cubes.** No asset fixes this:
   a non-cubic mesh path is required. Blocking for "no placeholder".
2. **Water:** no simulation, no fluid mesh. Structural placeholder.
3. **Loot:** without `items.md`, rotten flesh stays as dirt. The 5 props in
   priority 8 are a bandage, not the solution.
4. **Baubles UI:** the panel does not exist; the system is functional. Most
   embarrassing gap in the build.
5. **Angel:** mesh without rig. The 8 poses require either a rig or 8 static
   meshes. Technical decision (technical-director), not artistic.

## 13. Resolved decisions

| Decision | Choice |
|---|---|
| D2 | Art original (replace Patrix) |
| D8 | Low-poly stylized mobs, non-voxel, distinct from Angel by material |
| D9 | Improve existing main menu |
| D10 | Respect approved UI direction (vignette + audio) |
| D12 | Post-apo horror + dynamic weather/day-night |
| D13 | Nothing textured like Minecraft |

## 14. Design test

In six months, a screenshot of the game must be identifiable without a logo:
*worn voxel world, sodium light, a white statue that should not be there.* If a
player describes the Angel as "a mob", the contrast has failed.

**Validation status (2026-11-12, rev 2):** one of three signals is validated (worn
voxel world). The other two are now **measured**, and the picture is better than
the previous revision reported:

- *Sodium light* — **contested, not failed.** A second measurement pass on
  `105418`/`105502` measures `R_peak` **2.14 / 2.40**, `R_pool` **1.89 / 2.30**
  and bloom **40–200 px** — pass on all three targets. The earlier "≈ 1.0, no
  pool, no halo" verdict is not reproducible, and its stated cause (a bright
  cobblestone floor) was the torch's **own warm light pool**. The torch fix list
  is **frozen** pending a sampler-based measurement. The one certain defect is
  **colour**: the flame reads `#FFE08A` and the light is authored
  `RGB (1.0, 0.78, 0.50)` — pale and green against sodium `#E8A33D`. See
  `art-bible.md` §4.2.
- *A white statue that should not be there* — **failed, and now quantified.** The
  Angel reads `#6E7C74` against fog `#93A19A`: **`L_statue/L_fog = 0.56×`**
  against a required `1.4×`, a 2.5× shortfall. It is also **one albedo change**
  from passing: authoring the base colour to `#E3DCCB` projects `2.11×`. See
  `art-bible.md` §8.1.

The remaining certain defect in the *thesis* is the **UI accent**, which occupies
0.73 % of every frame in the same hue the torch needs (art-bible §3.1).

`Angel_60b_fog.png` is cancelled; its replacement is `Angel_25b_fog.png`.

---

## 15. Mood board & reference captures

**Statuses live in one place only:** `Docs/art/art-bible.md` §15 (A1–A6) and §16
(open validations), mirrored by `Docs/art/mood-board/manifest.csv`. This section
no longer restates them — it was a third copy and it drifted.

**Summary as of 2026-11-12 (rev 2):**

- 12 captures, 2 sets, `Docs/art/mood-board/`. **Set frozen** as the P2 target
  set — contract in `art-bible.md` **§19**, folder guide in
  `Docs/art/mood-board/README.md`.
- **Approved:** `001008` (palette only — no sky visible), `105207` (A1b target),
  `105250` (A1c **anti-target**), `105418` / `105502` (torch framing — verdict
  contested).
- **Excluded:** `Screenshot 2026-05-23 202820.png` (pre-overhaul, labelled
  BEFORE) and `000917` (labelled cave, actually an outdoor forest).
- **Withdrawn target:** "60-90 blocks of fog readability". Measured `λ ≈ 16
  blocks`; silhouette ends at 25 blocks. Fog is kept, A1 is re-based (§3, §6).
- **Sunset:** there is none. The day/night system authors no sun colour, so the
  palette rule has nothing to violate yet — and the dusk grade is now constrained
  to Bone → Ash → cold teal. The only warm light is the one the player installs.

**Additional observations to action in P2** (detail in `art-bible.md` §7, §11, §16):

- Block boundaries and erosion are not readable in `000850`; validate block edges
  + wear in PIE before expanding the 12 ruin variants (V5).
- Fog reads grey-green rather than pure Ash (`#5C6268`); verify fog colour stays
  within Ash across biomes (V6).
- The orange UI bar out-saturates every torch in the game. Sodium is now reserved
  for world light; the UI accent moves to `#C08A45` (V7).