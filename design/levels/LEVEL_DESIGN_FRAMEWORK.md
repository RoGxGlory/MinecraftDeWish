# Level Design Framework — MinecraftDeWish

| Field | Value |
|---|---|
| Status | Draft (2026-11-09) |
| Reference | *An Architectural Approach to Level Design*, 2nd ed. — Christopher W. Totten |
| Related | `design/gdd/game-concept.md`, `design/levels/biome-grammar.md` |
| Implementation | Villages only (`FVoxelStructureGenerator`); everything else is design intent |

---

## 1. Scope

The world is **procedurally generated, with authored POIs seeded into it**. Level design
therefore works at two levels at once:

1. **The pattern level** — the rules that decide what a biome looks like, what a scene
   contains, and how spaces connect. This is the part that runs on every chunk.
2. **The authored level** — individual POIs (villages, hospitals, police stations) built as
   modular kits and placed by the generator.

The framework below is the contract between those two levels. `biome-grammar.md` is the
pattern language; this document is the method, the metrics and the review criteria.

---

## 2. Method: Totten's workflow applied (Ch. 3)

### 2.1 Form follows the core mechanic

The core mechanic is **observation gating motion** (`design/gdd/game-concept.md` §5.1). Every
space must be judged by one question:

> *Can the player see enough of this space from a safe position to act in it?*

Consequences that are not negotiable:

- **No dark corridor without a light source the player can reach.** A blind corridor is not
  tense, it is a coin flip.
- **No room with a single entrance and no sightline to it.** The player must be able to
  watch the door while doing something else.
- **Every interior needs a "look-back" position**: a spot from which the player can see the
  room they are about to leave.

### 2.2 Parti

The **parti** of the game is *the lit island in a hostile field*. Every level, from a cave
mouth to a fortress, is a variation of that diagram: a bounded, illuminated volume, an
approach that is exposed, and at least one angle the player cannot cover alone.

### 2.3 Graybox first

No biome is authored directly. The workflow is:

1. Graybox the scene in `Lvl_FirstPersonTest` with the engine's blockout shapes.
2. Walk it with the Angel rule in mind (drawn, not simulated — Angels are not implemented).
3. Only then express it in voxels, and only then skin it with Patrix materials.

The test level stays a permanent graybox lab; it is not a shipping level.

### 2.4 Modular level design

POIs are built from **modules**, not as monoliths:

- **Structural modules**: room, corridor, stair, threshold, courtyard.
- **Function modules**: entry, loot vault, generator room, observation post, dead end.
- **Horror modules**: blind corner, one-way drop, long dark gallery, statue plinth.

The generator assembles POIs from modules, so a hospital can be 3 rooms or 9 without a new
asset. This is what makes "authored but procedural" affordable.

---

## 3. Space grammar: Lynch applied (Ch. 4)

An infinite voxel world is unnavigable unless the player can build a mental map. Kevin
Lynch's *Image of the City* gives the only vocabulary that survives procedural generation.

| Lynch element | In MinecraftDeWish | Generation rule |
|---|---|---|
| **Landmark** | A POI, a mountain peak, a dead tree taller than the canopy, a radio tower | One landmark visible per ~200 blocks; must be visible **above** the canopy |
| **Path** | A road, a river of sand, a cleared trail, a cut through a forest | Villages generate roads; other paths are terrain-made, deliberately weak |
| **Node** | A village square, a cave mouth, a crossroads | Nodes are where POIs attach; never generate a POI without a node |
| **Edge** | A biome border, a cliff line, a shoreline | Borders are **readable** because of the cross-fade and the transition biomes |
| **District** | A biome, a horror forest sector, a hospital complex | Districts own a material palette and a danger level |

### 3.1 Architectural weenies

Totten's *weenie* (after Disney) is a distant object that pulls the player forward. In this
game the weenie is also a **safety decision**: the player walks toward the thing they can
see, and the thing they can see is also the thing that can see them.

Rules:

- Every biome needs **one** silhouette that reads at 150+ blocks.
- The silhouette must be reachable, and the route to it must be legible without a map.
- Landmarks must be **lit or lightable**, or the approach becomes a blind walk.

---

## 4. The Angel constraint: turning a rule into geometry

The rule: `CanMove = (IsDark OR IsNight) AND NOT IsObserved`.

This produces three spatial variables that level design must control.

### 4.1 Sky access

Sky light is free and permanent. **A space with open sky is safe during the day by
construction.** Therefore:

- Interiors are dangerously expensive to the player (they need artificial light).
- Underground is *always* dark, so caves are hostile 24/7.
- The horror forest is the trap: its canopy blocks sky light, so **daylight does not save
  you there**. This is the biome's whole identity.

### 4.2 Sightline length

| Zone | Typical sightline | Player feeling |
|---|---|---|
| Dense forest | 5–15 blocks | Claustrophobic, ambush |
| Plains / desert | 40–90 blocks | Exposed, watched |
| Cave tunnel | 3–10 blocks | Blind, must carry light |
| Village interior | 8–20 blocks | Ordered, safe |
| Mountain shoulder | 60–120 blocks | Prospect, dominance |

Design rule: **never give a player a 60-block sightline into a space they cannot retreat
from.** Long sightlines are a reward (prospect), not a default.

### 4.3 Blind angles

The number of blind angles in a room is the real difficulty dial:

- **0 blind angles**: a lit open plaza. Safe but boring — use as refuge.
- **1 blind angle**: a normal room. The player can cover it by turning.
- **2–3 blind angles**: a hospital ward, a police station bullpen. Requires light
  infrastructure, not just a torch.
- **4+ blind angles**: only solvable by cameras → phase-3 content.

This gives level design a numeric axis that matches the progression: **phase 1 spaces have
≤1 blind angle, phase 2 spaces have 2–3, phase 3 spaces have 4+**.

---

## 5. Prospect and refuge by phase (Ch. 6)

| Phase | Prospect (exposed, high value) | Refuge (safe, low value) | Design job |
|---|---|---|---|
| **1 — Nomadic** | Plains, deserts, mountain shoulders, village outskirts | Caves with one entrance, village interiors, lit clearings | Teach the player to trade exposure for information |
| **2 — Precarious base** | The base's outer wall, watchtowers | The base core, the stockpile | Make the base a refuge that keeps growing blind angles |
| **3 — Fortress** | Camera masts, generator roofs | Interior corridors, machine halls | Make the refuge depend on infrastructure, not geometry |

**Degenerative design (Ch. 10) in reverse:** the world starts at maximum hostility and the
player *reduces* the possibility space — sealing routes, lighting angles, automating
observation. Level design must leave the seams visible so the player can see what they
changed.

---

## 6. POI design pattern (Ch. 7 + Ch. 8)

Every POI follows the same seven-beat structure. This is the template the modular kit
enforces.

| Beat | Function | Totten concept |
|---|---|---|
| 1. **Landmark** | Visible from 150+ blocks, unique silhouette | Weenie / landmark |
| 2. **Approach** | Exposed traversal; the player is seen before they see | Controlled approach |
| 3. **Threshold** | A door, a gate, a drop — the point of no return | Portals & thresholds |
| 4. **Orientation** | First room shows the POI's logic (a hospital looks like a hospital) | Advertising, associative teaching |
| 5. **Reward** | The thing the player came for, behind a denial | Reward vault |
| 6. **Cost** | The dark part: loot is in the unlit wing | Prospect & refuge inversion |
| 7. **Exit** | A second way out, or a reason to go back through the dark | Layered escape |

### 6.1 Reward denial (Ch. 7)

The reward is never next to the entrance. Denial tools, in order of preference:

- **Layered walls**: the vault is visible through a window before it is reachable.
- **Zen view**: the reward is visible from a safe place, so the player knows what the risk
  is for.
- **Oku**: an empty room before the reward, to make the player hesitate.
- **Vertical denial**: the reward is below or above the visible path.

### 6.2 No tutorial: the spawn region teaches (Ch. 8 inverted)

**Decision (user): there is no tutorial.** No tutorial level, no scripted opening, no text
prompts, no tooltips, and the player starts with **nothing at all**.

Totten's Chapter 8 asks how a first level teaches without words. With the tutorial removed,
that job does not disappear — it moves to the **spawn region and the first POI**, and it is
enforced by spatial rules rather than by a level:

1. **The first Angel must be encountered frozen, not moving.** Statues are readable; a
   moving Angel at night is a death the player cannot learn from. This is why the first
   module of every POI is Angel-free (`design/gdd/poi-placement.md` §3.10) and why the
   Horror Forest is never adjacent to spawn (`design/levels/biome-grammar.md` §2.1).
2. **A frozen Angel must be visible before it is dangerous.** Its pose is the tell: wrong
   angle, head turned, hands raised. The player who looks at it twice sees it has moved.
3. **Light must be discoverable as a tool, not granted.** The player starts with nothing, so
   the first torch is a found object with a visible effect on a visible room.
4. **Death is the teacher of last resort.** It costs a corpse run, not a save
   (`design/gdd/save-load.md` §3.7).

**Accepted risk.** Some players will die to an Angel before understanding the rule. This is
deliberate and it is the price of the decision. The mitigations are all spatial (readable
statues, Angel-free entry modules, spawn biome away from the Horror Forest) and none of them
is a tutorial. If playtesting shows the rule is not learned without text, the fix is a
*better tell*, not an added prompt.

See `design/levels/onboarding.md` for the first-hours contract that replaces the tutorial.

---

## 7. Procedural blending (Ch. 11)

Totten's Chapter 11 method: **pattern language first, generator second, hand-made scenes as
patterns**.

1. **Pattern** — a named, reusable spatial idea ("the lit island", "the blind gallery",
   "the dead-end vault").
2. **Scene** — a concrete assembly of modules that expresses a pattern.
3. **Grammar** — rules for which scenes may follow which, per biome.
4. **Seeder** — the generator picks scenes and places them at nodes.

The generator never invents geometry. It **selects and assembles authored scenes**. This is
the only way to keep an infinite world legible.

### 7.1 Continuity rules

- A POI must sit on a **node** (road junction, valley floor, cliff shelf), never on a slope
  too steep to walk.
- A POI must be **visible from the path that leads to it** — the approach is part of the
  design.
- A POI never generates inside another POI's landmark radius.
- Every POI has a **terrain reconciliation** step (flatten the pad, keep the silhouette).

---

## 8. Metrics

Single source of truth for spatial constants. `[VERIFY]` marks values taken from engine
defaults rather than measured in this build.

| Metric | Value | Notes |
|---|---|---|
| Block size | `100 cm` | `DEFAULT_BLOCK_SCALE` |
| Chunk footprint | `16 × 16` blocks | `CHUNK_SIZE_X/Y` |
| Chunk height | **`64` today → `192` target** | `ChunkHeight` is `EditAnywhere` 32–256; tripling requires the depth-relative fixes in §9 |
| Render distance | `6` chunks = `96` blocks | `RenderDistance`, `EditAnywhere` 2–16 |
| Player collision capsule | `88 cm` half-height, `34 cm` radius | `[VERIFY]` `BP_FirstPersonCharacter` |
| Player eye height | `~165 cm` | `[VERIFY]` |
| Jump height | `~1.25 blocks` | `[VERIFY]` — keep it at one block so 1-block steps are climbable |
| Auto-step height | `~0.5 blocks` | `[VERIFY]` |
| Minimum corridor | `1` block | Passable, uncomfortable, deliberately rare |
| Comfortable corridor | `3` blocks | Standard interior |
| Corridor / arch height | `2` blocks | Standard; `3` for halls |
| Minimum cave ceiling | `2` blocks | Below this the player cannot see an Angel's head |
| Comfortable cave ceiling | `4–6` blocks | Allows a raised ledge and a blind angle below |
| Comfortable interior room | `8 × 8` to `16 × 16` blocks | One to three blind angles |
| Long sightline | `40+` blocks | Only on prospect terrain |
| Max Angel observation | `60` blocks (proposed) | Must stay below render distance |
| Camera observation | `120` blocks (proposed) | Longer than player; narrower cone |
| POI spacing | `~600–1200` blocks | Sparse, landmark-driven |
| Landmark visibility | `150+` blocks | Above canopy |
| Night crossing (60 blocks) | `10–15 s` at walk speed | The window in which an unobserved Angel closes |

---

## 9. Vertical extent: from 64 to 192

The user decision is to **increase world height**. Target: **192 blocks**.

Rationale:

- `BaseHeight = ChunkHeight * 0.4` → sea level rises from 25 to **~77**, which gives
  115 blocks of air above the surface and 77 below. That is enough for a real mountain
  shoulder, a deep cave network and buried POIs.
- Terrain clamps to `[4, ChunkHeight - 8]`, so peaks become **184** instead of `56`.
- Mountain snow starts at `0.65 × ChunkHeight` = **125**, which puts a permanent snow line
  well above the playable valley floor.
- Chunk memory triples from 16 384 to **49 152 voxels** — the main cost, and the reason the
  streaming budget has to be re-measured before the change ships.

**Consequences that must be fixed in code before the change is safe** (see
`Docs/SYSTEMS_REFERENCE.md` and `WorldGenerator.cpp`):

1. **Ore depth gates are absolute** (`Z <= 32`, `Z <= 60`). At 192 they would both sit in the
   top third of the world, so every ore would surface. They must become fractions of
   `ChunkHeight`: `Z <= 0.167 × ChunkHeight` (shallow band) and `Z <= 0.313 × ChunkHeight`
   (deep band), or better, named depth bands in a data asset.
2. **Streaming cost ×3.** `MaxChunkGenerationsPerFrame = 2` was tuned for 16 384 voxels per
   chunk. Either halve it, add a distance-based LOD for far chunks, or both. Measure with
   Unreal Insights before accepting the change.
3. Mountain snow uses `ChunkHeight * 0.65` — already relative, no change.
4. Cave extents are `4 .. TerrainHeight - 6` — already relative, but the cave network gets
   77 blocks of vertical room, so cave density needs re-tuning or the underground becomes
   hollow.
5. **Chunk save format stores the full voxel array.** 64 → 192 changes the array size, so
   old saves are not compatible. This needs a version field and a height field in the save
   header before the change lands (`design/gdd/save-load.md`).
6. **Collision baking** (`UProceduralMeshComponent`, async) triples its workload per chunk.
   Verify the async path does not stall the game thread.

---

## 10. Review checklist

Before a POI or biome is accepted:

- [ ] The approach is visible from the path, and the POI's landmark reads at 150+ blocks.
- [ ] The player can retreat without entering a blind space.
- [ ] Every interior has at least one "look-back" position.
- [ ] The number of blind angles matches the intended phase.
- [ ] A light source is reachable within the space, or the space is deliberately unlit and
      labelled as such by the biome.
- [ ] The reward is denied by geometry, not by a key.
- [ ] A second exit exists, or the single exit is a deliberate trap.
- [ ] A player who has never seen an Angel can read its frozen pose without any prompt.
- [ ] The biome palette and materials match `biome-grammar.md`.
- [ ] The graybox was walked before the scene was dressed.

---

## 11. Open questions

- Final `ChunkHeight` is **192** (decided). Remaining work is the depth-band conversion, the
  streaming re-measurement and the save-format version field.
- Whether the player may **build** lights during a night crossing (affects every open-air
  encounter).
- Whether villages can be **entered at night** (their light holds, but do the NPCs let you
  in?), which decides whether they are true refuges.
- Save-format versioning before the height change.
