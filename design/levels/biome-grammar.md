# Biome Grammar — MinecraftDeWish

| Field | Value |
|---|---|
| Status | Draft (2026-11-09) |
| Reference | Totten, *An Architectural Approach to Level Design*, 2nd ed., Ch. 4, 7, 11 |
| Related | `design/levels/LEVEL_DESIGN_FRAMEWORK.md`, `Source/MinecraftDeWish/Private/WorldGenerator.cpp` |
| Implementation | 5 biomes exist in code; transition biomes are **proposed, not implemented** |

---

## 1. The field model

Every column of the world is classified from **three noise fields**, sampled with the same
warped coordinate so that texture blending and block assignment land on the same contour:

| Field | Function | Range | Meaning |
|---|---|---|---|
| `Continental` | `Fractal2D(3, 0.005)` | ≈ −1 … +1 | Landmass shape and mountain uplift |
| `Temp` | `Fractal2D_01(3, 0.006)` | 0 … 1 | Temperature band |
| `Moisture` | `Fractal2D_01(3, 0.006)` | 0 … 1 | Humidity band |

Borders are warped by `BiomeBlendWarp` and jittered by `BiomeBlendNoise`, then cross-faded
over a distance measured in blocks (`BiomeBlendWidth`). **The grammar below is the
classification; the cross-fade is the rendering.** A biome border is therefore a *band*, not
a line, and the band is where transition content lives.

---

## 2. Base biomes (implemented)

Exact classification order from `AWorldGenerator::ClassifyBiomeFields()` — first match wins:

| Biome | Condition | Terrain signature | Surface | Sky access | Danger |
|---|---|---|---|---|---|
| **Mountains** | `Continental > 0.35` | Uplifted, peaks to `ChunkHeight − 8`, snow above `0.65 × ChunkHeight` | Stone, snow | Full | Low by day, lethal by night (exposed) |
| **Desert** | `Temp > 0.60 && Moisture < 0.40` | Flattened (`Height` lerped toward base) | Sand, stone | Full | Very high: no cover, no refuge |
| **Horror Forest** | `Temp < 0.40 && Moisture > 0.55` | Rolling, dense canopy | Grass under heavy leaf cover | **Blocked by canopy** | Maximum: Angels move at noon |
| **Forest** | `Moisture > 0.50` | Rolling hills, dense trees | Grass, dirt | Partial | Medium: short sightlines |
| **Plains** | otherwise | Gentle rolling | Grass, dirt | Full | Low: readable, exposed |

### 2.1 The horror forest is the design keystone

Because sky light is blocked by the canopy, the horror forest is the only surface biome
where **daylight does not freeze the Angels**. That single interaction gives the biome an
identity that costs no new mechanics:

- It is the only place where the surface rule ("day is safe") is false.
- It is where the player first learns that the base has to be lit, not just built.
- It is the natural early-game "district edge": a wall of trees the player is not ready for.

Grammar consequence: **never place a horror forest adjacent to the spawn biome.** It must
be reached through a forest, so the player has a readable "the trees got wrong" moment.

---

## 3. Transition biomes (proposed)

The user decision is to add transitions. They are **not new mechanics** — each is a band
between two existing biomes, with its own palette, landmark and POI affinity. Proposed as
new `EBiomeType` values; thresholds sit between the base conditions and must be classified
**before** the base biome they soften.

| Biome | Proposed condition | Between | Silhouette | Why it exists |
|---|---|---|---|---|
| **Foothills** | `Continental > 0.20 && <= 0.35` | Plains/Forest ↔ Mountains | Terraced slopes, boulder fields | The mountain is *announced* before it is climbed; also gives a defensible shelf |
| **Scrubland** | `Temp 0.50–0.60 && Moisture 0.30–0.40` | Plains ↔ Desert | Sparse dead grass, cracked earth | The desert's heat is a gradient, not a wall |
| **Marsh** | `Moisture > 0.65 && Temp 0.40–0.60` | Forest ↔ lowland | Standing water pools, reeds, drowned trees | Gives water a place without a fluid simulation (pools are block geometry) |
| **Blight** | `Temp 0.35–0.45 && Moisture 0.50–0.60` | Forest ↔ Horror Forest | Dead trunks, grey grass, no canopy | The horror forest is *foretold*: the player can turn back |

`Foothills` is nearly free: the mountain uplift already starts at `Continental > 0.20`, so
the band exists in the terrain and only lacks a classification and a palette.

---

## 4. Biome cards

Each card is a **pattern language entry**: what the biome promises the player and what it
demands in return.

### 4.1 Plains — *the reading room*

- **Promise:** long sightlines, easy navigation, villages.
- **Demand:** exposure. Nothing to hide behind when night falls.
- **Landmarks:** village towers, lone dead trees, road junctions.
- **POI affinity:** villages (high), hospitals (low), police stations (medium).
- **Scene patterns:** open field, road crossing, lone farmstead, stone circle.
- **Blind angles:** 0–1. This is where the player learns to look.

### 4.2 Forest — *the short corridor*

- **Promise:** cover, wood, concealment from the surface threat.
- **Demand:** every sightline is short; an Angel 20 blocks away is already hidden.
- **Landmarks:** clearings (visible sky), a ridge, a felled grove.
- **POI affinity:** outposts (medium), hospitals (low), horror transition (high).
- **Scene patterns:** game trail, clearing with a circle of stumps, hunter's blind,
  tree-house outpost, overgrown ruin.
- **Blind angles:** 2–3. Forest is where the player stops trusting the torch radius.

### 4.3 Desert — *the exposure*

- **Promise:** visibility. You can see everything, and everything can see you.
- **Demand:** no water, no wood, no shade; night in the desert is the worst place to be.
- **Landmarks:** mesas, wrecked convoys, a radio mast.
- **POI affinity:** police stations (medium — remote outposts), villages (low).
- **Scene patterns:** dune ridge, dry riverbed, sand-buried structure, wreck field.
- **Blind angles:** 0 at range, but the flatness removes *all* refuge. Difficulty comes from
  the absence of cover, not from occlusion.

### 4.4 Mountains — *the prospect*

- **Promise:** dominance. From a shoulder the player can see 100+ blocks.
- **Demand:** the climb is the level. Falling, exposure, and the fact that a light on a
  ridge is visible from very far away.
- **Landmarks:** peaks, a crashed aircraft, a monastery-like ruin.
- **POI affinity:** observatories, relay stations, cave mouths (high).
- **Scene patterns:** switchback trail, cliff shelf, cave entrance, summit plateau.
- **Blind angles:** high *below* the player, low above. Mountains are the one place where
  the player can be the one watching.

### 4.5 Horror Forest — *the false day*

- **Promise:** the game's best loot and the only tree material worth having.
- **Demand:** the day rule is suspended. Light must be carried, then installed.
- **Landmarks:** a single dead giant, a chapel, a clearing that should not be there.
- **POI affinity:** hospitals (high), chapels, research sites.
- **Scene patterns:** black corridor between trunks, dead clearing, hanging grove, shrine,
  the "one lit window" (a survivor's outpost, a trap).
- **Blind angles:** 4+. This is the first place where a single player cannot cover the room,
  which is the designed motivation for phase 2 lighting.

---

## 5. Scene pattern catalogue

Named, reusable scenes. The generator selects from these; it never invents geometry.

| Pattern | Biomes | Function | Phase |
|---|---|---|---|
| **The lit island** | All | A lit refuge in a dark field | 1–3 |
| **The blind gallery** | Horror Forest, interiors | Long space with no sightline to its end | 2–3 |
| **The dead-end vault** | All | Reward behind a single exit | 1–2 |
| **The one lit window** | Horror Forest, Forest | Bait: a light in the dark that is not a refuge | 1 |
| **The switchback** | Mountains, Foothills | Vertical traversal that reveals the map | 1 |
| **The drowned road** | Marsh | Path that becomes water and forces a detour | 1 |
| **The road crossing** | Plains, Scrubland | Node where POIs attach | 1 |
| **The canopy ceiling** | Horror Forest | Sky access disappears; the day rule breaks | 1–2 |
| **The generator room** | All interiors | Room the player must keep lit to keep the POI safe | 2–3 |
| **The camera mast** | Fortress | High point that sees the blind angles | 3 |

---

## 6. POI distribution

POIs are **sparse events** (user decision), not roadside furniture.

| POI | Frequency | Biomes | Contract |
|---|---|---|---|
| **Village / NPC outpost** | Rare (`~1 per 800–1200 blocks`) | Plains, Forest, Foothills | **Safe while its light holds.** Has a generator/lamp the player can repair or loot. |
| **Abandoned hospital** | Rare | Forest, Horror Forest, Blight | Dark interior, medical loot, dense Angels, 2–3 blind angles per ward |
| **Abandoned police station** | Rare | Plains, Scrubland, Desert | Armoury behind denial, evidence of the collapse, medium Angel density |
| **Relay / observatory** | Very rare | Mountains | High-value navigation landmark, long sightlines, exposed |
| **Chapel / shrine** | Rare | Horror Forest, Blight | Lore, a single safe room, a bell that can be rung (noise = risk) |

**Village safety rule (decided):** a village is safe **as long as its light holds**. The
player can therefore:

- **use** it as a refuge for the night,
- **repair** it (costly, keeps a permanent node),
- **loot** it (fast, and it stops being safe — which may be exactly what the player wants).

This single rule turns villages into an economic decision instead of a free hotel.

---

## 7. Failure modes to watch

| Failure | Symptom | Fix |
|---|---|---|
| **Mountains dominate** | `Continental > 0.35` produces too much uphill; traversal is exhausting | Raise the threshold or lower the uplift curve; measure actual world share |
| **Desert is empty** | Nothing to do, no reason to cross | Add desert-specific POIs (convoy wrecks, police outposts) |
| **Forest is noise** | Same trees everywhere, no landmarks | Enforce one landmark per ~200 blocks, one clearing per ~150 |
| **Horror forest unreadable** | Player cannot tell it started | The Blight transition exists precisely to announce it |
| **Village as free hotel** | Player camps forever, phase 1 collapses | Villages are rare, far apart, and their light has a cost |
| **Sky access everywhere** | Day is always safe, horror never lands | Canopy, caves, and interiors must be common enough to matter |

---

## 8. Tuning knobs

| Knob | Location | Purpose |
|---|---|---|
| `TerrainFrequency` | `WorldGenerator` | Size of landmasses |
| `BiomeBlendWidth` | `WorldGenerator` | Physical width of the cross-fade, in blocks |
| `BiomeBlendWarp` / `BiomeBlendNoise` | `WorldGenerator` | Organic border meander vs. lobe jitter |
| `BiomeHeightRamp` | `WorldGenerator` | Terrain-height ramp across a border |
| Classification thresholds (0.35 / 0.60 / 0.40 / 0.55 / 0.50) | `ClassifyBiomeFields` | Biome share of the world |
| Transition thresholds | §3 (proposed) | Width of each transition band |
| `bEnableCaves`, `bEnableTrees`, `bEnableOres` | `WorldGenerator` | Feature toggles per test map |

Thresholds are **gameplay values**: they must move into a `UBiomeDefinition` DataAsset (or a
DataTable) before shipping, per `.opencode/rules/ue-data.md`. Today they are literals in
`ClassifyBiomeFields()`.

---

## 9. Acceptance criteria

1. A player can identify the biome they are in from silhouette alone, without a HUD.
2. Every biome border is traversable on foot at walk speed — no unclimbable step.
3. The horror forest is reachable only through forest or blight.
4. Every biome contains at least one landmark visible from 150 blocks.
5. Villages are safe **only** while their light holds, and that is observable in play.
6. No biome has a POI frequency so high that the player stops treating POIs as events.
7. The transition biomes are visually distinct from both neighbours without a new texture
   pack pass.

---

## 10. Open questions

- Exact transition thresholds (§3) — needs a world-share measurement, not a guess.
- Whether `Marsh` can exist without a fluid simulation (pools as static block geometry is
  the proposed answer).
- Whether `Blight` is a real biome or a "decoration band" applied to Forest near a Horror
  Forest border — cheaper, and probably better.
- Where the spawn point sits relative to the first village and the first horror forest.
