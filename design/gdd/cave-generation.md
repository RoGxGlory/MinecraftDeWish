# GDD — Cave Generation (Underground Network)

| Field | Value |
|---|---|
| Status | Draft (2026-11-11) — **supersedes `world-generation.md` §3.4** (cave shape only) |
| System owner | systems-designer / engine-programmer / gameplay-programmer |
| Related | `design/gdd/world-generation.md` §3.4, §3.5, §3.8, `design/gdd/mob-spawning.md`, `design/gdd/weeping-angels.md`, `design/gdd/light-and-power.md`, `design/gdd/poi-placement.md`, `design/gdd/save-load.md`, `design/gdd/player-vitals.md`, `Docs/SYSTEMS_REFERENCE.md` §1–2, `Docs/BLOCK_REGISTRY.md` |
| Source of truth for code | `AWorldGenerator::GenerateChunkData` (cave pass), `FVoxelNoise`, `VoxelDataTypes.h` (`CHUNK_SIZE_X/Y`, `ChunkHeight`, `EBlockType`, `EBiomeType`) |
| Contract | Carve pass runs **after terrain, before ores**. Deterministic pure function of `(WorldCoord, WorldSeed)`. No shared mutable state between chunks. |

---

## 1. Overview

Cave Generation produces the **underground network**: the tunnels, chambers, fissures, ruined
infrastructure and hidden ways that exist below `TerrainHeight` in every chunk of the world.
It is a sub-pass of `AWorldGenerator::GenerateChunkData`, evaluated per voxel from world
coordinates, and it is the **only** system allowed to turn Stone into Air after the terrain pass.

The pass produces five shapes and two procedural structures:

| # | Element | Role |
|---|---|---|
| A | **Spaghetti tunnels** | The primary walkable network. Connected, branching, 3–4 blocks wide. |
| B | **Noodle passages** | Thin connectors and squeezes between tunnels and chambers. |
| C | **Caverns** | Large open chambers. Vertical spectacle, big floors, mob arenas. |
| D | **Ravines** | Rare vertical fissures with stepped ledges. Vertical traversal. |
| E | **Cave entrances** | Surface sinkholes. The only feature allowed to break the surface cap. |
| F | **Abandoned mineshafts** | Procedural corridor lattice with timber frames, barrels, rare torches. |
| G | **Dungeons** | Small cobblestone rooms. Dark, unlooted, occupied. |

The underground is not decoration. It is **where the game's predator lives**, where the
resources are, and where the player learns that a torch is a border rather than a lamp.
Every rule below exists to serve one of those three jobs.

This document owns cave **shape and placement**. It does not own: light levels
(`light-and-power.md`), spawn rates (`mob-spawning.md`), Angel behaviour
(`weeping-angels.md`), ore *values* (`world-generation.md` §3.5, `BLOCK_REGISTRY.md`), or
authored POI content (`poi-placement.md`). Where it touches those systems it declares an
**interface** and defers the decision.

---

## 2. Player Fantasy

> *"The world has a second floor, and I am not the only thing on it."*

The cave system exists to make the player feel four things:

1. **Descent as commitment.** The surface is legible: sky, horizon, biome. Below it, the world
   stops having a map. Every step down costs a torch, a sightline and an exit. The player
   should feel the light behind them getting further away, and should feel that turning back
   is a *decision with a cost*, not a free undo.

2. **A world with a ceiling.** Caverns must produce the specific awe of standing in a space
   whose roof you cannot see. This is the one place the game can be *big* while being
   enclosed. It is the payoff for digging.

3. **The predator's house.** Caves are the Angel's habitat. The player must learn to read
   cave geometry the way they read biome silhouette above ground: a blind corner, a dead end,
   a chamber with two exits — each is a threat assessment. Cave shape *is* the horror pacing.

4. **Reward for reading the ground.** Ore is exposed on cave walls. A player who learns to
   follow the network finds iron without strip-mining; a player who ignores it grinds. Caving
   must be the *skilled* route to mid-tier resources, never the mandatory one.

The failure state to design against: caves that are a uniform sponge. If every tunnel looks
like every other tunnel, the underground becomes a corridor with resource nodes, and both the
awe and the fear collapse into a chore.

---

## 3. Detailed Rules

### 3.1 Position in the generation pipeline

The cave pass is step 2 of `GenerateChunkData`, and the order is load-bearing:

```
1. Terrain pass     → blocks[], ColumnTopZ[] (cached per column)
2. CAVE PASS        → carve Stone → Air; stamp procedural structures      [this document]
3. Ore pass         → writes ore into Stone only (cannot fill cave air)
4. Flora pass       → trees (above surface; unaffected)
5. Structure pass   → authored POI modules (stamp over cave air → POIs always win)
```

Consequences that the rest of this document depends on:

- **Ores can never float.** The ore pass only replaces Stone, so a voxel carved to Air stays Air.
- **Caves truncate ore veins** and expose them on cave walls (§3.10).
- **POIs overwrite caves.** A POI stamped in step 5 replaces any cave air inside its volume,
  so a hospital basement is never bisected by a tunnel. The cave pass therefore does **not**
  need to query POI bounds.
- **Caves may undermine a POI from below.** That is accepted and thematic (a village over a
  cavern), but `poi-placement.md` must reject a POI pad whose footprint intersects a cave void
  larger than `POIVoidTolerance` (propagation note in §6).

### 3.2 Coordinates, depth band and clearance invariants

- **Voxel size:** 100 UU = 1 block. Chunk = `16 × 16 × ChunkHeight` (default 192), full-height
  column, no third chunk axis. All cave math is therefore **chunk-local in X/Y, absolute in Z**.
- **Depth band.** Carving is restricted to:

  ```
  CaveFloorZ      ≤ Z ≤ TerrainHeight(X,Y) − CaveSurfaceCap
  CaveFloorZ      = 4        (4 stone blocks seal the bedrock floor)
  CaveSurfaceCap  = 6        (6 stone blocks seal the surface)
  ```

  If `TerrainHeight − CaveSurfaceCap < CaveFloorZ`, the column has no cave volume and the pass
  skips it entirely (no evaluation, no cost).
- **Carve target.** Only `Stone` (ID 4) may be carved. Never `Bedrock` (25), `Dirt` (1),
  `Grass` (2), `Sand` (18), `Water` (26), any ore, or any placed block. Carving is
  `if (Block == Stone) Block = Air;` — a single, auditable rule.
- **Clearance invariant.** Every archetype must guarantee a minimum interior clearance of
  **2 blocks vertically** on its centreline, because the player capsule is `PlayerHeight = 1.8`
  blocks and no crouch/crawl state exists today. This is enforced by the radius floors in §7,
  not by a post-process.
- **No accidental surface breach.** Every carved voxel satisfies `Z ≤ TerrainHeight − 6`,
  **except** voxels carved by a cave entrance shaft (§3.7), which is the single documented
  exception and is tagged for the test suite.
- **No fluid.** No lava, no underground water, no liquid simulation. `Water` (ID 26) is a
  reserved static block with no flow. Caves are **dry**. Underground aquifers are explicitly
  out of scope (see Appendix A).

### 3.3 Archetype A — Spaghetti tunnels (primary network)

The walkable backbone. A tube is the intersection of two independent 3D noise fields: the set
of points where *both* fields are near zero is a curve, and a thickness around that curve is a
tunnel.

```
f_t  = TunnelFrequency           (blocks⁻¹, default 1/32)
R_t  = TunnelRadius              (blocks,    default 1.9)
T_t  = (R_t · NoiseGradient · f_t)²          // derived, never authored directly

for each voxel in band:
    N1 = Perlin3D(WX·f_t + 317,  WY·f_t + 571,  WZ·f_t + 193)
    N2 = Perlin3D(WX·f_t + 1201, WY·f_t + 733,  WZ·f_t + 947)
    if (N1² + N2²) < T_t · TunnelDensity(biome):  carve
```

- The two offsets (`317/571/193`, `1201/733/947`) are **decorrelation constants**. They must
  differ on all three axes; if two axes share an offset the intersection degenerates into a
  planar sheet and the tunnels become walls.
- `TunnelDensity(biome)` is a per-biome multiplier from `DT_CaveProfiles` (§3.13). It scales
  the *threshold*, which is a smooth density control: `0` disables the archetype, `1.5` widens
  and lengthens tunnels without changing their radius.
- Tunnels are **not** given explicit endpoints or branches. Branching emerges from the noise
  topology, which is why the network reads as organic rather than as a graph. It also means
  connectivity is a *statistical* property, verified by probe (§8, AC 8) rather than
  guaranteed by construction.

### 3.4 Archetype B — Noodle passages (connectors)

Thin tubes formed by the intersection of two **slabs** (`|noise| < w`). Cheap, tight, and
mostly useful for stitching the big tunnels into one network so the player is rarely walled in.

```
f_n  = NoodleFrequency           (blocks⁻¹, default 1/24)
R_n  = NoodleRadius              (blocks,    default 1.1)
w_n  = R_n · NoiseGradient · f_n             // derived

for each voxel in band:
    N3 = Perlin3D(WX·f_n + 2411, WY·f_n + 3137, WZ·f_n + 4339)
    N4 = Perlin3D(WX·f_n + 5077, WY·f_n + 6037, WZ·f_n + 7019)
    if |N3| < w_n · NoodleDensity(biome) and |N4| < w_n · NoodleDensity(biome):  carve
```

- Cross-section is a **square** of side `2·R_n` (2.2 blocks at default), so the diagonal is
  3.1 blocks: the player fits, but the passage reads as a squeeze. This is deliberate — a
  noodle is where the player cannot see behind themselves.
- `R_n` must stay `≥ 0.9` blocks. Below that the square's side drops under 1.8 blocks and the
  passage becomes impassable while still looking like a passage. That is a trap, not a
  feature.
- Noodles are intentionally the **highest-density archetype by count and the lowest by volume**.
  They are what makes the underground feel continuous without making it hollow.

### 3.5 Archetype C — Caverns (chambers)

Large open volumes from a single low-frequency field with a low-frequency **mask bias** so
chambers appear in swarms rather than evenly.

```
f_c   = CavernFrequency          (blocks⁻¹, default 1/48)
f_m   = CavernMaskFrequency      (blocks⁻¹, default 1/160)
N_c   = Fractal3D(WX·f_c, WY·f_c, WZ·f_c·CavernVerticalSquash, 2 octaves)
M_c   = Fractal3D(WX·f_m, WY·f_m, WZ·f_m, 2 octaves)
if (N_c + CavernMaskBias · M_c) > CavernThreshold · CavernDensity(biome):  carve
```

- `CavernVerticalSquash = 1.5` multiplies the **Z** frequency only: noise varies faster
  vertically, so a chamber is ~1.5× wider than tall. Rooms read as rooms, not as bubbles.
- `CavernMaskBias = 0.30` is what turns an even sprinkle of small voids into **swarms of large
  chambers separated by solid rock**. Set it to `0` and the underground becomes a uniform
  sponge — this is the single most important knob in the document.
- Caverns are the **mob arenas** and the **Angel stages**: they are the only archetype with
  enough floor area and sightline length for a chase. Their spawn multiplier is the highest
  (§3.12).
- Caverns must never be carved within `CavernSurfaceCap = 10` blocks of the surface (a larger
  cap than tunnels, because a 20-block chamber with a 6-block roof would collapse visually).
  Effective band for caverns: `[CaveFloorZ, TerrainHeight − 10]`.

### 3.6 Archetype D — Ravines (fissures)

Rare vertical gashes. A 2D field selects a **centreline** on the horizontal plane; distance to
that line (in blocks) is compared to a depth-dependent half-width that tapers shut at both ends
and carries horizontal ledges.

```
f_r   = RavineFrequency              (blocks⁻¹, default 1/220)
R1    = Fractal2D(WX·f_r + 4000, WY·f_r + 4000, 3 octaves, persistence 0.5, lacunarity 2.0)
d     = |R1| / (NoiseGradient2D · f_r)          // distance to centreline, in BLOCKS
Region = Fractal2D(WX·f_region + 9000, WY·f_region + 9000, 2 octaves)   // f_region = 1/400

t          = (Z − RavineBottomZ) / (RavineTopZ − RavineBottomZ)      // ∈ [0,1]
HalfWidth(t) = RavineMaxHalfWidth · sin(π·t)^0.7 · (1 + 0.25·Wn)

carve if  Region > RavineRegionThreshold
      and d < HalfWidth(t)
      and NOT InLedgeBand(Z, d, HalfWidth(t))
```

- `RavineTopZ = TerrainHeight − 8`, `RavineBottomZ = RavineMinZ` (default 6). The `sin(π·t)`
  taper means the fissure closes at both ends: no surface breach, no flat floor slab.
- **Ledges.** `InLedgeBand` suppresses carving in a horizontal band near the wall:

  ```
  InLedgeBand = ((Z mod RavineLedgeSpacing) < RavineLedgeThickness) and (d > RavineLedgeInnerRatio · HalfWidth(t))
  ```

  With `RavineLedgeSpacing = 7`, `RavineLedgeThickness = 2`, `RavineLedgeInnerRatio = 0.55`,
  the player gets a 2-block shelf every 7 blocks up the wall — a ravine is **climbable by
  design**, not a pit. This is the difference between a landmark and a death sentence.
- Because `InLedgeBand` only *suppresses ravine carving*, a tunnel that already carved that
  voxel still wins (see §3.14, union rule). Ledges never plug other archetypes.
- `RavineRegionThreshold = 0.40` gates the whole archetype to ~9 % of the world so ravines stay
  events. Two ravines never share a centreline because the 220-block period separates them.

### 3.7 Archetype E — Cave entrances (surface access)

**The only feature permitted to break the surface cap.** Without entrances, the entire cave
system is discovered only by digging, which turns exploration into strip-mining. Entrances are
the interface between the two halves of the world.

**Placement (deterministic, one candidate per chunk):**

```
if (hash(ChunkX, ChunkY, Seed, 0x456E7472) & 0xFFFF) / 65535.0 < CaveEntranceChunkChance:
    choose the column (X,Y) in [2..13]² with the lowest hash(WorldX, WorldY, Seed, 0x456E7473)
```

- Candidate rate `CaveEntranceChunkChance = 0.08`; roughly 1 entrance per 14 chunks after the
  reach test below rejects candidates (§4.7). At `RenderDistance = 8` that is one entrance per
  ~64×64 blocks — findable by walking, never a field of holes.
- The `[2..13]²` apron keeps the 3×3 shaft (and its rim) fully inside the chunk, exactly like
  the tree pass — no cross-chunk stamping, no tearing at borders.

**Geometry:**

```
TopZ    = TerrainHeight(X,Y)
ReachZ  = the highest Z in [TopZ − EntranceMaxShaftDepth, TopZ − CaveSurfaceCap] that is Air
          (i.e. the first carved voxel below this column)
if no Air found within EntranceMaxShaftDepth:      → no entrance (no orphan hole)
else:
    carve a 3×3 shaft from TopZ down to ReachZ (only Stone, Dirt, Grass, Sand)
    replace the 12 surrounding surface blocks with Dirt  → the rim / the tell
```

- `EntranceMaxShaftDepth = 40`. With ~5.5 % total cave volume,
  `P(reach) ≈ 1 − 0.945⁴⁰ ≈ 90 %`.
- The shaft is a **sinkhole**, not a staircase. It is a decision point: the player looks down,
  cannot see the bottom, and chooses. That is the correct emotional beat for a first descent.
- **The tell.** A Dirt rim on the surface plus a visible dark pit is the entire discovery
  affordance, matching the 24-block tell rule of `poi-placement.md` §3.6. Any additional tell
  (draft particles, sound) is an audio/VFX dependency, not a generation rule.
- Hillside "cave mouths" (a horizontal arch in a slope) are **deferred**. They need slope-aware
  geometry and a different placement test; the sinkhole delivers the same gameplay function at
  a fraction of the cost.

### 3.8 Procedural structure F — Abandoned mineshafts

Mineshafts are **not stamped modules**. A chunk generates independently and asynchronously, so
a structure spanning 80 blocks cannot be authored once and copied — it must be a **pure function
of world position**, exactly like the ore noise. The mineshaft is therefore a *corridor lattice*:

```
RegionSize = 256 blocks
RX = floor(WX / RegionSize);  RY = floor(WY / RegionSize)
H  = hash(RX, RY, Seed, 0x4D696E65)
if (H & 0xFF) / 255.0 > MineshaftChance:        → no mineshaft in this region

OX, OY = per-region offsets derived from H      // decorrelates lattices across regions
AxisA = |frac((WX − OX) / MineshaftLatticeSpacing) − 0.5| < MineshaftHalfWidth / MineshaftLatticeSpacing
AxisB = |frac((WY − OY) / MineshaftLatticeSpacing) − 0.5| < MineshaftHalfWidth / MineshaftLatticeSpacing

carve if (AxisA or AxisB)
      and MineshaftMinZ ≤ Z ≤ min(MineshaftMaxZ, TerrainHeight − CaveSurfaceCap)
```

- Cross-section is `2 · MineshaftHalfWidth = 3` blocks wide and `MineshaftCorridorHeight = 3`
  blocks tall. `MineshaftLatticeSpacing = 32`, so corridors are 32 blocks apart — a walkable
  grid, not a sponge.
- **Timber frames.** At every `MineshaftFrameSpacing = 4` blocks along a corridor, write
  `Wood_Log` (5) posts on both walls and `Wood_Planks` (6) across the ceiling. Frames are
  written *after* carving, only into Air, and are `SetBlockGenerated`.
- **Torches.** `MineshaftTorchChance = 0.06` per frame, placed on a wall block adjacent to the
  corridor at head height. Mineshafts are **dim, not black**: enough light to navigate, never
  enough to make them safe. This is the archetype's whole identity.
- **Loot.** `Barrel` (20) at lattice intersections and in occasional side alcoves. Loot
  contents are derived from `hash(WorldX, WorldY, WorldZ, Seed, 0x4C6F6F74)` and are resolved
  lazily on first open; the chunk delta records `bLooted` so a reload cannot re-roll (§3.16).
- Mineshafts carve **only Stone**, so a mineshaft that runs above the local terrain simply
  stops existing there instead of cutting a trench through the sky.

### 3.9 Procedural structure G — Dungeons

A single small room per `DungeonRegionSize = 128`-block region, chance `DungeonChance = 0.08`.

```
Room: 9 × 5 × 9 interior, Cobblestone (3) shell, floor at Z, ceiling at Z+4
Band: DungeonMinZ (16) ≤ Z ≤ min(DungeonMaxZ (48), TerrainHeight − 10)
Contents: 1 Barrel (loot), 25 % chance of a Crafting_Table (15), 0 torches
```

- **Zero torches is a rule, not an omission.** A dungeon must be dark enough that its occupants
  spawned there before the player arrived (`LightLevel < 7`, `mob-spawning.md` §3.4).
- There is no mossy cobblestone and no spawner block in the registry, so a dungeon is
  *Cobblestone + Barrel + darkness*. Do not design around blocks that do not exist.
- The dungeon carves its own volume (it is a room, not a stamp over terrain) and writes its
  shell only where the surrounding block is Stone, so a dungeon near the surface does not build
  a floating box in the air.

### 3.10 Ore interaction — veins exposed by caves

Caves do not place ore. They **reveal** it, because the ore pass runs after carving and only
replaces Stone.

Rules this document owns:

1. **No floating ore.** Guaranteed by pass order (§3.1). An ore voxel always has a solid
   neighbourhood from the ore pass's own perspective.
2. **Exposed veins are intended and measurable.** An ore voxel with at least one Air
   face-neighbour is a *visible* ore. The model in §4.11 predicts a **5–10 % exposure rate** at
   the shipped cave volume. Caving is therefore a *supplement* to dedicated mining, not a
   replacement — which is the correct balance: the underground rewards exploration, but a player
   who wants diamond still digs for it.
   > **Do not use the naive per-voxel estimate.** `1 − (1−V)⁶ ≈ 25 %` is wrong, because ore
   > voxels cluster into veins: most ore voxels are vein *interior* and see no air at all. §4.11
   > carries the corrected model.
3. **`CaveAdjacencyOreBonus` ships at `0.0` (off).** It lowers the ore threshold by up to
   `0.06` for Coal and Iron only, for voxels adjacent to cave air, if playtests show caving is
   not rewarding enough. It is a documented lever with a real cost (it changes ore distribution
   and requires a `GeneratorVersion` bump). It must never apply to Diamond or Emerald: rare ores
   stay gated behind depth and dedicated mining, or the progression pillar breaks.
4. **Cavern floors are the reward surface.** A cavern floor is a large, exposed Stone plane at
   depth, so the ore gates apply to it with no special case — the player who reaches a deep
   chamber sees what is in the floor. No extra rule needed; do not add one.
5. **Entrances can expose ore at the surface.** A sinkhole shaft cutting a shallow vein is a
   legitimate discovery moment. Do not suppress it.

### 3.11 Lighting and darkness

Cave lighting is governed by `light-and-power.md` (levels) and `mob-spawning.md`
(`CalculateLightLevel`). This document specifies only what cave **geometry** must provide.

- **Light model.** `LightLevel = max(SkyLight, TorchLight)` on a `0–15` scale. Underground,
  `SkyLight = 0` (no column exposure), so an unlit cave is `LightLevel = 0`.
- **Torches are binary bubbles.** `TorchLightRadius = 5` blocks sets `LightLevel = 14` inside
  the radius and `0` outside it (`mob-spawning.md` §3.4). A cave is therefore a sequence of
  **safe islands** separated by darkness — the single most important spatial property of the
  underground.
- **Hard budget: `MaxTorchLights = 16`.** Torches use a global pool of 16 shadowless point
  lights. A cave route must be navigable with **≤ 4 torches per 32 blocks of tunnel**, or the
  player will out-run the light pool and see torches pop out behind them. Cave geometry must
  therefore keep sightlines short (§3.3 curvature) and must not require long straight halls.
- **Torch light does not respect walls** (shadowless). A torch on one side of a 1-block wall
  lights the chamber on the other side. Do not design a "secret dark room" that shares a thin
  wall with a lit corridor — it will not be dark.
- **Ambient floor.** Unlit cave interiors must remain *readable*: the player needs silhouettes,
  not a black screen. Proposed `CaveAmbientLuminance ∈ [0.02, 0.04]` of full exposure. The exact
  value is an **art-director decision** (it trades legibility against the darkness threat); the
  design requirement is only "silhouettes visible, threats not identified".
- **No torch is placed by the cave pass** except mineshaft frames (§3.8). Caves ship dark.
- Emissive ore glow would make depth legible and is thematically strong, but `M_Global` has no
  emissive mask today. Deferred to `technical-artist` / `art-director` (Appendix A).

### 3.12 Mob spawning in caves

Spawn **gating** belongs to `mob-spawning.md` (light < 7, 24–128 blocks from the player, solid
opaque floor, not within 5 blocks of a torch). Caves satisfy that gate automatically. This
document owns the **cave-specific modifiers** that the spawner must consume:

1. **Archetype weight.** `CaveSpawnWeightMultiplier[archetype]`: Cavern `1.4`, Mineshaft `1.2`,
   Ravine `1.0`, Tunnel `0.8`, Dungeon `1.5`. A spawner sampling a candidate should weight by
   the archetype that carved the voxel (tagged at carve time, §3.14). Rationale: chambers have
   floor area and sightlines; tunnels have neither, and a tunnel ambush with no escape reads as
   unfair rather than tense.
2. **Depth escalation.**

   ```
   DepthBelowSurface = TerrainHeight(X,Y) − Z
   ThreatFactor      = clamp(DepthBelowSurface / CaveDepthForMaxThreat, 0, 1)
   SpawnWeight       = 1 + CaveThreatScale · ThreatFactor
   ```

   `CaveDepthForMaxThreat = 48` blocks, `CaveThreatScale = 1.5` → ×1.0 at the surface cap,
   ×2.5 at 48 blocks down and below. **The deeper the player goes, the worse it gets** — the
   only escalating pressure in a game whose horror curve otherwise descends
   (`game-concept.md` P5). It is bounded by the world floor at Z = 4, so it saturates, it does
   not run away.
3. **Cave cap share.** At most `CaveMobCapShare = 0.5` of the global mob cap may be underground.
   Without this, a player mining at depth starves the surface of mobs and the night above becomes
   free. `mob-spawning.md` must implement the split; this document only requires it.
4. **Spider emphasis.** Spiders climb walls and are the archetypal cave threat
   (`mob-spawning.md` §3.8). Cave spawn tables should weight Spider above its surface share;
   exact numbers belong to `mob-spawning.md`.
5. **Angels.** Caves are the Angel habitat (`weeping-angels.md` §3.11). Cave geometry must
   supply the niches the Angel needs: **dead ends, alcoves and blind corners**, at a measurable
   rate. Design target: **≥ 1 pocket per 32 blocks of tunnel centreline**, where a *pocket* is
   an air region of at least 3×3×3 whose narrowest connection is ≤ 2 voxels wide. Verified by
   probe (§8, AC 9). This is a **shape** requirement, which is why it lives in this document and
   not in the Angel document.
6. **Pathfinding risk.** No navmesh is generated for procedural chunk meshes. Cave mobs
   therefore rely on direct steering (and climbing, for spiders), not on A*. If a cave network
   produces a chamber that a steering agent cannot exit, mobs will pile up in it. Mitigated by
   the connectivity and traversability probes (§8, AC 8/10) and tracked as a risk in Appendix A.

### 3.13 Biome cave profiles (data-driven)

Cave density is per biome, read from `DT_CaveProfiles` (row per `EBiomeType`), never hardcoded
(`.opencode/rules/ue-data.md`).

| Biome | `TunnelDensity` | `NoodleDensity` | `CavernDensity` | Ravine region bias | Intent |
|---|---|---|---|---|---|
| Plains | 1.00 | 1.00 | 1.00 | 0.90 | Baseline. The reference the others are read against. |
| Forest | 1.05 | 1.00 | 1.00 | 1.00 | Slightly more network under the canopy. |
| Desert | 0.75 | 0.85 | 0.90 | 1.15 | Less network, more erosion: deserts crack, they do not hollow. |
| Tundra | 1.00 | 1.00 | 1.10 | 1.00 | More chambers (ice voids read as caves). |
| Mountains | 1.25 | 1.10 | 1.15 | 1.00 | Most rock, most cave. Mountains are the deep-mine biome. |
| Horror Forest | 1.15 | 1.20 | 1.20 | 1.10 | The biome that is *supposed* to be riddled. Highest spawn pressure. |

Multipliers scale **thresholds** (density), never radii: a Desert tunnel and a Mountain tunnel
must be the same width, or the player cannot learn tunnel readability across biomes.

### 3.14 Evaluation order, determinism and the pass contract

- **Carving is a pure union.** `IsAir = Tunnel ∨ Noodle ∨ Cavern ∨ Ravine ∨ Mineshaft ∨ Dungeon`.
  Because it is a union, **evaluation order cannot change the result**, which makes determinism
  trivial to prove and lets the implementation reorder for profiling.
- The one apparently subtractive rule (ravine ledges) is implemented as **"do not carve"**, not
  as "re-solidify", so it cannot plug a tunnel (§3.6).
- **Determinism contract.** The pass is a pure function of `(WorldX, WorldY, WorldZ, WorldSeed,
  GeneratorVersion)`. It reads no other chunk, holds no mutable state, and calls only
  `FVoxelNoise`. Two chunks generated in either order, on any thread, on any machine, produce
  identical voxels at the shared border plane.
- **Carve tagging (diagnostic only, never persisted).** Each carved voxel records the archetype
  that carved it in a per-chunk debug mask, used by the spawner weighting (§3.12) and by the
  probes. It is not written to the save.
- **`GeneratorVersion` must be bumped** by any change to a frequency, threshold, radius, offset,
  lattice constant or biome profile. A world generated at a different version is not the same
  world; `save-load.md` §3.3 governs the rejection path.

### 3.15 Performance budget and sampling

The cave pass shares the `MaxGenerationTimeMs = 4.0` ms/frame budget with terrain, ores and
flora. Allocation:

| Pass | Budget share | Default |
|---|---|---|
| Terrain | 40 % | 1.6 ms |
| **Cave (this pass)** | **30 %** | **`CavePassBudgetMs = 1.2 ms`** |
| Ores | 20 % | 0.8 ms |
| Flora + structures | 10 % | 0.4 ms |

**Cost control, in order of impact:**

1. **Band early-out.** Only `[CaveFloorZ, TerrainHeight − CaveSurfaceCap]` is evaluated: at
   default `ChunkHeight = 192` with `TerrainHeight ≈ 77`, that is 73 of 192 layers → 16×16×73 =
   **18,688 voxels**, not 49,152.
2. **2D gates before 3D.** Ravine line test and mineshaft lattice test are 2D and per column.
   They reject ~95 % of columns with 1–3 noise calls before any 3D work.
3. **Trilinear interpolation on a world-anchored grid.** 3D noise is evaluated every
   `CaveNoiseStep = 2` blocks and interpolated. Because the tunnel field has a 32-block period,
   a 2-block step is 1/16 of a period — far inside Nyquist, so interpolation error is invisible.
   The grid **must be anchored at world multiples of `CaveNoiseStep`**, not at chunk-local
   origin, or the interpolated field will be discontinuous at chunk borders.
4. **Interpolate the fields, not the density.** Interpolate `N1` and `N2` separately, then
   combine with `N1² + N2²`. Interpolating the squared sum instead produces blobby,
   faceted cave walls.
5. **Tiered steps.** The cavern mask (160-block period) is sampled at `CavernMaskStep = 8`; the
   ravine region gate at `RavineRegionStep = 16`. They are the two lowest-frequency fields and
   the two cheapest to approximate.

Cost model at defaults: `9 × 9 × 38 ≈ 3,078` interpolated samples × 5 fields ≈ **15 k noise
calls per chunk** (vs ~112 k unoptimised, a ~7× reduction).

**Memory.** Zero extra allocation: the pass writes into the existing per-chunk `blocks[]` array
and a small per-chunk scratch grid for the interpolation lattice.

**Threading.** The pass runs inside `GenerateChunkData`, which already executes on
`FPendingAsyncChunkGen`. It must not touch `UObject`s, world queries or other chunks.

### 3.16 Streaming, persistence and save data

- **No separate streaming.** Caves are baked at chunk generation. `UpdateChunkStreaming`
  (`world-generation.md` §3.8) is unchanged: chunks within `RenderDistance` (2–64, default 8)
  are generated; chunks outside are delta-saved and destroyed.
- **Caves are not persisted.** They regenerate identically from the seed. Only *player edits*
  (mined blocks, placed blocks, opened barrels) live in the chunk delta, via the
  `SetBlock` / `SetBlockGenerated` split of ADR-0002.
- **Generated structure blocks must use `SetBlockGenerated`** (timber frames, dungeon shell,
  barrels, entrance rim) so a generated structure never marks a chunk as player-modified.
- **Loot determinism.** A barrel's contents are derived from its world coordinate and seed and
  resolved on first open. `bLooted` is stored in the chunk delta. Without this, a chunk
  unload/reload would re-roll loot — a trivially exploitable dupe loop. This is a **hard
  requirement**, not a nicety.
- **A cave entrance can be destroyed by the player** (fill the shaft). The hole is then closed,
  the tunnel still exists, and nothing regenerates. Correct and intended.
- **`bEnableCaves = false`** disables all five archetypes **and** both structures (they have
  nowhere to live). `bEnableCaveStructures` can additionally disable structures while keeping
  caves — useful for debugging shape.

---

## 4. Formulas

### 4.1 Noise primitives and the gradient constant

All archetypes use `FVoxelNoise`. This document states frequencies as **effective frequency**
`f` in `blocks⁻¹`:

```
Perlin3D(x, y, z)                        → [-1, 1]
Fractal3D(x, y, z, Octaves)              → [-1, 1]
Fractal2D(x, y, Octaves, Persistence, Lacunarity) → [-1, 1]
```

> **Code convention warning.** The shipped ore code calls
> `Fractal3D(WX·k, WY·k, WZ·k, 2, f_code)`, i.e. it scales coordinates by `k` *and* passes a
> separate small frequency, so `f_effective = k · f_code`. Implementations of this document must
> preserve the **effective** frequency, not the argument values. Getting this wrong silently
> changes every cave in the world.

Two calibration constants describe the noise implementation itself. They are **measured once**
by the volume probe (§8, AC 3) and then frozen:

| Constant | Value | Meaning |
|---|---|---|
| `NoiseGradient` | `1.0` | `∇(noise)` per unit of effective frequency. Converts a noise threshold into a radius in blocks. |
| `Sigma` | `0.30` | Standard deviation of the noise amplitude. Converts a threshold into a volume fraction. |

The shipped `NoiseGradient = 1.0` and `Sigma = 0.30` reproduce the observed behaviour of the
current implementation (a ~1.9-block tunnel radius at `f = 0.035`). They are mutually consistent:
the radius estimate and the volume estimate of the same tunnel agree to within a few percent.

### 4.2 Unified carve criterion

Every archetype reduces to the same shape: a **squared noise distance** compared to a **radius
in blocks**.

$$\text{carve} \iff D(p) < \left(R \cdot G \cdot f\right)^2$$

where `D` is the squared noise distance (`N1²+N2²` for a two-field tube, `N²` for a one-field
sheet), `R` the desired radius in blocks, `G = NoiseGradient`, `f` the effective frequency.
Designers author `R` (blocks) and `f` (period); the threshold is **derived**:

$$T = (R \cdot G \cdot f)^2$$

This inversion is the point of the document: no one authors `0.0035`. They author "2-block
radius at a 32-block period" and the code derives the threshold.

### 4.3 Tunnel

$$T_t = (R_t \cdot G \cdot f_t)^2, \qquad R_t = 1.9,\ f_t = \tfrac{1}{32}$$

$$T_t = \left(1.9 \times 1.0 \times 0.03125\right)^2 = 0.00352$$

(Matches the shipped `0.0035`.)

**Volume fraction** (two independent near-Gaussian fields):

$$V_{tunnel} = 1 - \exp\!\left(-\frac{T_t \cdot \mu}{2\sigma^2}\right), \qquad \mu = \text{biome density}$$

At `µ = 1`, `σ = 0.30`: `V = 1 − exp(−0.00352/0.18) = 1.94 %`.

**Inverting for a target volume** (the calibration direction):

$$T = -2\sigma^2 \ln(1 - V_{target})$$

Example: target `V = 2 %` → `T = −0.18 × ln(0.98) = 0.00364`. Confirms the default.

### 4.4 Noodle

$$w_n = R_n \cdot G \cdot f_n, \qquad R_n = 1.1,\ f_n = \tfrac{1}{24}$$

$$w_n = 1.1 \times 1.0 \times 0.041667 = 0.0458$$

**Volume fraction** (two independent slab tests):

$$V_{noodle} = \left[2\Phi\!\left(\frac{w_n}{\sigma}\right) - 1\right]^2 = \left[2\Phi(0.153) - 1\right]^2 = (0.1216)^2 = 1.48\%$$

Cross-section is `2R_n = 2.2` blocks per side; minimum clearance `2.2` blocks ≥ the 2-block
player requirement.

### 4.5 Cavern

$$V_{cavern} = P\left(N_c + B\,M_c > T_c\right),\qquad \sigma_{eff} = \sqrt{\sigma^2 + (B\sigma)^2}$$

With `B = CavernMaskBias = 0.30`, `σ = 0.30`:

$$\sigma_{eff} = \sqrt{0.09 + 0.0081} = 0.313$$

$$V_{cavern} = 1 - \Phi\!\left(\frac{T_c}{\sigma_{eff}}\right) = 1 - \Phi\!\left(\frac{0.72}{0.313}\right) = 1 - \Phi(2.30) = 1.07\%$$

**Inverting for a target volume:**

$$T_c = \sigma_{eff}\,\Phi^{-1}(1 - V_{target})$$

Example: target `1.0 %` → `Φ⁻¹(0.99) = 2.326` → `T_c = 0.728`. Matches the default `0.72`.

**Chamber size.** With `f_c = 1/48` and a 2-octave fractal, a chamber's characteristic diameter
is `≈ 1/f_c × 0.6 ≈ 29` blocks horizontally and `29 / 1.5 ≈ 19` blocks vertically. That is the
"cannot see the roof" scale. Lowering `f_c` to `1/64` yields ~38×26 blocks — a cathedral, and a
much bigger meshing and lighting problem.

### 4.6 Ravine

$$d(p) = \frac{|R_1(p)|}{G_2 \cdot f_r} \quad \text{(blocks)},\qquad G_2 = 1.0,\ f_r = \tfrac{1}{220}$$

$$d(p) = |R_1(p)| \times 220$$

$$\text{HalfWidth}(t) = W_{max}\sin(\pi t)^{0.7}\left(1 + 0.25\,W_n\right),\qquad W_{max} = 4.5$$

$$\text{carve} \iff \text{Region} > 0.40 \ \wedge\ d < \text{HalfWidth}(t) \ \wedge\ \neg\text{InLedgeBand}$$

**Column probability.** `P(d < 4.5) = P(|R₁| < 0.0205) = 2Φ(0.068) − 1 = 5.4 %` per column in
a ravine region; `P(Region > 0.40) = 1 − Φ(1.33) = 9.1 %`. Combined: `0.49 %` of columns.
In a 16×16 chunk that is **1.26 columns per chunk** — a ravine is visible in roughly one chunk
in five, and traversable in far fewer.

**Volume fraction.** Band ≈ 41 % of the cave band, so `V_ravine ≈ 0.0049 × 0.41 × 0.7 ≈ 0.14 %`.

### 4.7 Entrance

$$P(\text{reach}) = 1 - (1 - V_{cave})^{D_{max}},\qquad D_{max} = 40,\ V_{cave} \approx 0.055$$

$$P(\text{reach}) = 1 - 0.945^{40} = 0.895$$

Effective entrance rate: `CaveEntranceChunkChance × P(reach) = 0.08 × 0.895 ≈ 0.072` per chunk
→ **1 entrance per ~14 chunks**. The `MinecraftDeWish.Cave.Entrance` probe reports the real rate
(AC 13); adjust `CaveEntranceChunkChance` from that measurement, not from this estimate.

### 4.8 Mineshaft lattice

$$\text{AxisA} = \left|\text{frac}\!\left(\frac{WX - O_X}{S}\right) - 0.5\right| < \frac{h}{S},\qquad S = 32,\ h = 1.5$$

Footprint fraction inside a region (two axes, union-corrected):

$$A = 2\left(\frac{2h}{S}\right) - \left(\frac{2h}{S}\right)^2 = 2(0.09375) - 0.00879 = 0.1787$$

Vertical fraction of the cave band covered by the `[MineshaftMinZ, MineshaftMaxZ]` window:

$$F_z = \frac{60 - 20}{73} = 0.548$$

$$V_{mineshaft} = \text{Chance} \times A \times F_z = 0.10 \times 0.1787 \times 0.548 = 0.98\%$$

So inside a mineshaft region, ~9.8 % of the rock is corridor — that reads as a large mine
complex, which is the point. Weighted over the world it is **0.98 % of the cave band**.

### 4.9 Dungeon

$$V_{dungeon} = \text{DungeonChance} \times \frac{9 \times 5 \times 9}{128 \times 128} = 0.08 \times 0.00247 = 0.02\%$$

Negligible by volume, significant by content. One dungeon per `1 / 0.08 ≈ 12.5` regions of
128×128 blocks.

### 4.10 Total volume and calibration

| Component | Nominal `V` |
|---|---|
| Tunnels | 1.94 % |
| Noodles | 1.48 % |
| Caverns | 1.07 % |
| Ravines | 0.14 % |
| Mineshafts | 0.98 % |
| Dungeons | 0.02 % |
| **Sum** | **5.63 %** |
| **Union (overlap-corrected, ≈ 0.1 % double-carved)** | **≈ 5.5 %** |

Target band: **`CaveVolumeTarget = 5.0 %`, tolerance ± 1.5 %** (accepted range 3.5 % – 6.5 %).
Below 3.5 % the underground stops being a network and becomes isolated pockets; above 6.5 % it
becomes a sponge that no longer reads as rock.

**Calibration procedure (mandatory before shipping a change):** run `MinecraftDeWish.Cave.Volume`
over 64 chunks at three seeds, compare against the table, and adjust **thresholds** (via the
radius/volume inversions in §4.3–4.5), never radii, unless the change is intentionally a
readability change.

> The table is analytic and each component carries a ±25 % modelling error. It exists to tell a
> designer **which knob moves which share**, not to be trusted to a decimal. The probe is the
> contract; the table is the map.

### 4.11 Ore exposure

$$\text{OreExposureRate} = \frac{\left|\{\,o \in \text{Ores} : \exists\, n \in \text{Faces}(o),\ \text{Air}(n)\,\}\right|}{\left|\text{Ores}\right|}$$

Ore does not scatter — it forms veins, so most ore voxels are interior and see no air. The
model must account for that:

$$\text{Exposure} \approx s_{vein}\left[1 - (1 - V)^{f}\right]$$

| Symbol | Meaning | Nominal |
|---|---|---|
| `V` | Carved cave volume fraction | `0.055` (§4.10) |
| `s_vein` | Fraction of ore voxels lying on a vein **surface** | `0.35 – 0.45` |
| `f` | Mean outward faces per surface voxel | `2 – 3` |

$$\text{Exposure} \approx 0.40 \times \left[1 - 0.945^{2.5}\right] = 0.40 \times 0.132 = 5.3\%$$

Across the parameter ranges this gives **5 % – 10 %**.

Target band **5 % – 15 %**. Above 20 % ore becomes a surface find and depth stops mattering;
below 4 % caving stops being a resource strategy and the underground is only a threat. If the
probe (AC 19) measures under 5 %, enable `CaveAdjacencyOreBonus` rather than widening caves.

> **Correction to a tempting shortcut.** `1 − (1−V)⁶` (treating ore as scattered voxels) gives
> ~25 % and is wrong by a factor of ~4. It ignores both vein clustering (`s_vein`) and the fact
> that a surface voxel only has 2–3 outward faces. Do not tune against it.

### 4.12 Mob threat scaling

$$\text{ThreatFactor} = \text{clamp}\!\left(\frac{\text{TerrainHeight} - Z}{48},\, 0,\, 1\right)$$

$$\text{SpawnWeight} = 1 + 1.5 \cdot \text{ThreatFactor} \quad \in [1.0, 2.5]$$

### 4.13 Interpolation and cost

Sample grid over a chunk's cave band, anchored to world multiples of `CaveNoiseStep = 2`:

$$N_X = \left\lfloor \frac{16}{2} \right\rfloor + 1 = 9,\quad N_Y = 9,\quad N_Z = \left\lfloor \frac{H_{band}}{2} \right\rfloor + 1$$

At `H_band = 73`: `N_Z = 38` → `9 × 9 × 38 = 3,078` samples. Five fields (N1, N2, N3, N4, N_c)
= **15,390 noise calls/chunk**, plus ~768 for the 2D gates.

Interpolation (trilinear, per voxel):

$$N(x,y,z) = \sum_{i,j,k \in \{0,1\}} w_i^x w_j^y w_k^z \, N_{i,j,k},\qquad w^1 = \text{frac}\!\left(\frac{x}{\text{step}}\right)$$

### 4.14 Hash

Deterministic splitmix-style, matching the existing flora/village convention:

$$\text{hash}(x,y,z,s) = \text{splitmix}\left(x \cdot 73856093 \oplus y \cdot 19349663 \oplus z \cdot 83492791 \oplus s\right)$$

$$\text{roll} = \frac{\text{hash} \,\&\, \texttt{0xFFFF}}{65535.0} \in [0,1]$$

Salt constants (must never be reused across features):
`0x456E7472` entrance chunk · `0x456E7473` entrance column · `0x4D696E65` mineshaft region ·
`0x4C6F6F74` barrel loot · `0x44756E67` dungeon region.

---

## 5. Edge Cases

| Case | Resolution |
|---|---|
| Column too short (`TerrainHeight − 6 < 4`) | No cave volume in that column; the pass skips it. No evaluation cost, no carve. |
| Cave would breach the surface | Impossible by band construction: `Z ≤ TerrainHeight − 6`. Only an entrance shaft may exceed it, and it is tagged. |
| Entrance candidate finds no air within 40 blocks | No entrance is created. **No orphan hole is ever dug.** |
| Entrance shaft would cut a POI footprint | POIs stamp in step 5 and overwrite the shaft. Accepted; the POI wins. |
| Entrance shaft lands in a mine shaft corridor | Valid connection. Shaft joins the corridor; nothing special happens. |
| Ravine ledge voxel also carved by a tunnel | Tunnel wins (union). Ledges only suppress ravine carving, never re-solidify. |
| Ravine + cavern overlap | Creates a very tall void. Accepted (vertical spectacle). No fall damage exists today — see Appendix A risk R3. |
| Cavern ceiling closer than 10 blocks to the surface | Prevented: cavern band is capped at `TerrainHeight − 10`, not `− 6`. |
| Cave carving a voxel that is not Stone | Never happens: the carve rule tests `Block == Stone` first. Dirt, Sand, Bedrock, ores, Water and placed blocks are untouched. |
| Ore vein cut in half by a tunnel | Intended. The cut face is exposed ore on the cave wall (§3.10). |
| Mineshaft runs above local terrain | The corridor simply stops existing (no Stone to carve). No trench in the sky. |
| Mineshaft crosses a chunk border | Continuous by construction: the lattice is a function of world coordinates. |
| Two regions' lattices abut with mismatched offsets | Intended: per-region offsets decorrelate them. Corridors do not need to align across a region border. |
| Dungeon near the surface | Shell is written only where the neighbour is Stone; no floating box. |
| Barrel loot re-rolled after chunk reload | **Forbidden.** Loot is seed-derived and `bLooted` is persisted in the chunk delta (§3.16). |
| Player fills an entrance shaft | The shaft stays filled. The tunnel below still exists. Nothing regenerates. |
| Player mines into a cave from above | Normal mining; the cave is revealed. The `CaveSurfaceCap` is a generation rule, not a physical barrier. |
| Torch placed in a cave lights a room through a 1-block wall | Accepted limitation of shadowless point lights. Do not design thin-wall secrets. |
| 17th torch in view | The `MaxTorchLights = 16` pool drops the furthest light. Documented; cave routes must not need more than ~4 torches per 32 blocks. |
| `CaveNoiseStep` changed at runtime | Changes the interpolated field → different world. Requires a `GeneratorVersion` bump. |
| World coordinates beyond ~100 k blocks | `float` Perlin loses precision and the field degrades/discontinues. Known limitation — see Appendix A risk R1. |
| `bEnableCaves = false` | All archetypes and both structures are skipped. Terrain, ores and flora unaffected. |
| `bEnableCaveStructures = false` | Archetypes carve; no mineshafts, no dungeons. Debug/authoring mode. |
| `ChunkHeight` changed | Save is rejected (`world-generation.md` §5). Caves regenerate at the new height. |
| Two chunks generate concurrently | No shared state; the pass reads only its own `blocks[]` and `FVoxelNoise`. No lock needed. |
| A cave voxel is the floor a mob spawns on | Valid. Cavern floors are the intended arena surface (§3.12). |

---

## 6. Dependencies

| System | Why | Status |
|---|---|---|
| `AWorldGenerator` (`GenerateChunkData`) | Host of the cave pass; owns `TerrainHeight`, band, block writes, `SetBlockGenerated` | Implemented |
| `FVoxelNoise` | `Perlin3D`, `Fractal3D`, `Fractal2D` — all cave fields | Implemented |
| `VoxelDataTypes.h` | `CHUNK_SIZE_X/Y`, `ChunkHeight`, `EBlockType`, `EBiomeType` | Implemented |
| `AChunkActor` | Meshes carved voxels; cave walls rely on `IsBlockTransparent` face culling | Implemented |
| `M_Global` | Stone texture on cave walls; biome cross-fade is gated to the top 4 blocks, so cave walls correctly stay Stone | Implemented |
| `Block_DataTable` | `Stone` (4), `Cobblestone` (3), `Wood_Log` (5), `Wood_Planks` (6), `Barrel` (20), `Crafting_Table` (15), `Torch` (19), `Dirt` (1) | Implemented |
| `world-generation.md` §3.4 | **Superseded by this document** for cave shape; §3.4 must be reduced to a pointer + the shipped formula | Implemented (needs doc update) |
| `world-generation.md` §3.5 | Ore pass runs *after* this pass; ore thresholds, depth gates and biome gating are unchanged | Implemented |
| `world-generation.md` §3.8 | Streaming, `MaxGenerationTimeMs`, async generation — cave pass runs inside it | Implemented |
| `mob-spawning.md` | Consumes archetype weight, depth escalation, `CaveMobCapShare`; owns the light gate | Draft |
| `weeping-angels.md` | Caves are the Angel habitat; requires pocket rate and blind corners (§3.12.5) | Draft (0 % implemented) |
| `light-and-power.md` | `LightLevel` model, `TorchLightRadius`, `MaxTorchLights` pool, `AngelLightThreshold` | Draft (0 % implemented) |
| `poi-placement.md` | POIs stamp after caves and win; must reject pads over large cave voids | Draft (not implemented) |
| `save-load.md` | `SetBlockGenerated` split, chunk delta, `bLooted`, `GeneratorVersion` gating | Draft |
| `player-vitals.md` | Fall damage — **absent today**, which makes deep ravines non-lethal (risk R3) | Draft |
| `items-loot.md` | Barrel loot table contents | Draft |
| `design/gdd/angels.md` | **Duplicate of `weeping-angels.md`** — reconcile; this document references `weeping-angels.md` only | Stale duplicate |

**Bidirectional obligations.** This document is the source of truth for cave *shape*. Any change
to a frequency, threshold, radius, band or biome profile must be propagated to
`world-generation.md` §3.4/§4.4, `mob-spawning.md` (spawn surfaces), `weeping-angels.md` (habitat),
`poi-placement.md` (void tolerance) and `save-load.md` (via `GeneratorVersion`).

**Interfaces this document requires from others:**

- `AWorldGenerator::GetTerrainHeight(WorldX, WorldY)` — per column, already cached by the terrain pass.
- `AWorldGenerator::SetBlockGenerated(X, Y, Z, BlockID)` — for structure blocks (ADR-0002).
- A per-chunk archetype tag mask — for spawner weighting and probes (new, trivial to add).
- `GetLightLevelAt(Voxel)` — does not exist yet; required by the spawner and Angels, not by this pass.

---

## 7. Tuning Knobs

All values live in `FVoxelCaveParams` (`USTRUCT`, `UPROPERTY(EditAnywhere)` with `ClampMin`/
`ClampMax`) on `AWorldGenerator`, except the per-biome profile rows in `DT_CaveProfiles`.
**No cave value may be hardcoded in C++.**

### 7.1 Global and band

| Knob | Default | Range | Effect |
|---|---|---|---|
| `bEnableCaves` | `true` | bool | Master switch for all archetypes and structures. |
| `bEnableCaveStructures` | `true` | bool | Mineshafts + dungeons only. |
| `CaveFloorZ` | `4` | `[1, 16]` | Stone floor above bedrock. Lower = deeper pits, closer to the world edge. |
| `CaveSurfaceCap` | `6` | `[3, 12]` | Stone roof under the surface. Higher = harder to accidentally breach while mining. |
| `CaveVolumeTarget` | `0.050` | `[0.03, 0.07]` | Design target used by the calibration probe, not read at runtime. |

### 7.2 Tunnels

| Knob | Default | Range | Effect |
|---|---|---|---|
| `TunnelRadius` | `1.9` blocks | `[1.2, 4.0]` | Tunnel half-width. Below 1.2 the passage stops being comfortably walkable. |
| `TunnelFrequency` | `1/32` | `[1/64, 1/12]` | Tunnel period. Lower = longer, straighter, rarer tunnels. |
| `TunnelDensity(biome)` | `0.75–1.25` | `[0, 2]` | Threshold multiplier per biome (`DT_CaveProfiles`). |

### 7.3 Noodles

| Knob | Default | Range | Effect |
|---|---|---|---|
| `NoodleRadius` | `1.1` blocks | `[0.9, 2.0]` | Squeeze width. **Hard floor 0.9** — below it the passage is impassable. |
| `NoodleFrequency` | `1/24` | `[1/48, 1/12]` | Connector period. |
| `NoodleDensity(biome)` | `0.85–1.20` | `[0, 2]` | Threshold multiplier per biome. |

### 7.4 Caverns

| Knob | Default | Range | Effect |
|---|---|---|---|
| `CavernThreshold` | `0.72` | `[0.55, 0.90]` | Chamber volume. **Primary density knob.** |
| `CavernFrequency` | `1/48` | `[1/96, 1/24]` | Chamber scale. Lower = fewer, larger chambers. |
| `CavernVerticalSquash` | `1.5` | `[1.0, 3.0]` | Width : height ratio of chambers. |
| `CavernMaskBias` | `0.30` | `[0, 0.60]` | Swarm clustering. `0` = uniform sponge (do not ship). |
| `CavernMaskFrequency` | `1/160` | `[1/512, 1/64]` | Swarm size. |
| `CavernSurfaceCap` | `10` blocks | `[6, 24]` | Roof thickness above chambers. |
| `CavernDensity(biome)` | `0.90–1.20` | `[0, 2]` | Threshold multiplier per biome. |

### 7.5 Ravines

| Knob | Default | Range | Effect |
|---|---|---|---|
| `RavineMaxHalfWidth` | `4.5` blocks | `[2, 12]` | Maximum fissure half-width at mid-height. |
| `RavineFrequency` | `1/220` | `[1/512, 1/96]` | Centreline period. Lower = fewer, longer fissures. |
| `RavineRegionThreshold` | `0.40` | `[0.2, 0.8]` | Fraction of the world where ravines may exist. |
| `RavineMinZ` | `6` | `[2, 24]` | Fissure floor. |
| `RavineLedgeSpacing` | `7` blocks | `[4, 12]` | Vertical distance between ledges. |
| `RavineLedgeThickness` | `2` blocks | `[1, 4]` | Ledge height. |
| `RavineLedgeInnerRatio` | `0.55` | `[0.3, 0.9]` | How far out from the centreline the ledge starts. |

### 7.6 Entrances

| Knob | Default | Range | Effect |
|---|---|---|---|
| `CaveEntranceChunkChance` | `0.08` | `[0, 0.30]` | Candidate rate per chunk. **Primary discoverability knob.** |
| `EntranceMaxShaftDepth` | `40` blocks | `[16, 80]` | How far a shaft may dig to reach air. |
| `EntranceShaftWidth` | `3` blocks | `[3, 5]` | Shaft cross-section (never below 3: 2 is impassable with the rim). |
| `EntranceRimRadius` | `2` blocks | `[0, 4]` | Dirt rim width — the discovery tell. |

### 7.7 Structures

| Knob | Default | Range | Effect |
|---|---|---|---|
| `MineshaftChance` | `0.10` | `[0, 0.5]` | Fraction of 256-block regions with a lattice. |
| `MineshaftLatticeSpacing` | `32` blocks | `[12, 48]` | Corridor spacing. |
| `MineshaftHalfWidth` | `1.5` blocks | `[1.0, 2.5]` | Corridor half-width (3 blocks wide at default). |
| `MineshaftCorridorHeight` | `3` blocks | `[3, 5]` | Corridor height. |
| `MineshaftMinZ` / `MineshaftMaxZ` | `20` / `60` | `[4, 120]` | Depth band. |
| `MineshaftFrameSpacing` | `4` blocks | `[2, 8]` | Timber frame cadence. |
| `MineshaftTorchChance` | `0.06` | `[0, 0.4]` | Torches per frame. Above ~0.2 the mineshaft stops being scary. |
| `DungeonChance` | `0.08` | `[0, 0.4]` | Fraction of 128-block regions with a dungeon. |
| `DungeonMinZ` / `DungeonMaxZ` | `16` / `48` | `[4, 120]` | Depth band. |

### 7.8 Performance and interaction

| Knob | Default | Range | Effect |
|---|---|---|---|
| `CavePassBudgetMs` | `1.2` ms | `[0.2, 4.0]` | Cave share of `MaxGenerationTimeMs`. |
| `CaveNoiseStep` | `2` blocks | `[1, 4]` | Interpolation step. `1` = exact and slow; `4` = fast and blobby. |
| `CavernMaskStep` | `8` blocks | `[2, 16]` | Mask sampling step. |
| `RavineRegionStep` | `16` blocks | `[4, 32]` | Region-gate sampling step. |
| `CaveAdjacencyOreBonus` | `0.0` (off) | `[0, 0.06]` | Ore threshold reduction next to cave air (Coal/Iron only). **Requires `GeneratorVersion` bump when enabled.** |
| `CaveSpawnWeightMultiplier` | `0.8–1.5` | `[0.5, 2.0]` | Per-archetype spawn weight (§3.12). |
| `CaveMobCapShare` | `0.5` | `[0.2, 0.8]` | Maximum share of the global mob cap underground. |
| `CaveDepthForMaxThreat` | `48` blocks | `[16, 96]` | Depth at which threat scaling saturates. |
| `CaveThreatScale` | `1.5` | `[0, 3]` | Depth escalation strength. |
| `CaveAmbientLuminance` | `0.03` | `[0.0, 0.08]` | Unlit cave legibility floor. **Art-director owned.** |

---

## 8. Acceptance Criteria

Each criterion is verifiable by an Unreal Automation Test (`MinecraftDeWish.Cave.*`) or by the
named probe, and is pass/fail — no "should feel".

1. **Determinism.** Generating the same `(WorldSeed, GeneratorVersion)` twice, in two different
   streaming orders and on two threads, produces bit-identical voxel arrays for 64 reference
   chunks.
2. **Border continuity.** For any pair of horizontally adjacent chunks generated in either
   order, the 16×`ChunkHeight` voxel column on the shared plane is identical in both. Verified
   for tunnels, noodles, caverns, ravines, mineshafts and dungeons.
3. **Calibration constants.** The `MinecraftDeWish.Cave.Volume` probe reports a measured
   `NoiseGradient` and `Sigma` within 10 % of `1.0` and `0.30` on the shipped `FVoxelNoise`. A
   deviation fails the test and requires re-deriving the shipped thresholds.
4. **Surface integrity.** No carved voxel satisfies `Z > TerrainHeight − CaveSurfaceCap`, with
   the sole exception of voxels tagged `EntranceShaft`. Test asserts zero violations over 64
   chunks.
5. **Floor integrity.** No carved voxel satisfies `Z < CaveFloorZ`; `Z = 0` is Bedrock in every
   column of every sampled chunk.
6. **Carve target.** Every carved voxel was `Stone` (ID 4) before the pass. No `Bedrock`, `Dirt`,
   `Grass`, `Sand`, `Water`, ore or placed block is ever removed by the cave pass.
7. **Tunnel radius.** Measuring the maximum inscribed width of 32 sampled tunnel segments gives
   `1.9 ± 0.4` blocks radius at default `TunnelRadius = 1.9` and `TunnelFrequency = 1/32`.
8. **Connectivity.** A flood fill over the carved volume of 64 chunks finds that ≥ 60 % of carved
   voxels belong to a connected component spanning ≥ 48 blocks on its longest axis.
9. **Angel pockets.** The `MinecraftDeWish.Cave.Pocket` probe reports ≥ 1 pocket (≥ 3×3×3 air,
   narrowest connection ≤ 2 voxels) per 32 blocks of tunnel centreline, over 64 chunks.
10. **Traversability.** A 1×2×1 agent flood-filling from a sample entrance reaches ≥ 55 % of
    carved voxels. Passages narrower than 2 blocks of clearance are permitted but must not
    dominate the network.
11. **Volume fraction.** `MinecraftDeWish.Cave.Volume` over 64 chunks × 3 seeds reports total
    carved volume in `[3.5 %, 6.5 %]`, with `CaveVolumeTarget = 5.0 %` as the centre.
12. **Entrance validity.** Every generated entrance shaft (a) reaches a carved voxel within
    `EntranceMaxShaftDepth`, (b) has a Dirt rim of `EntranceRimRadius`, (c) is fully contained in
    its chunk, and (d) is the only surface breach in the chunk. No entrance exists without a
    connection.
13. **Entrance rate.** The measured entrance rate over 256 chunks is within ±50 % of
    `CaveEntranceChunkChance × P(reach)`, and is logged by the probe.
14. **Ravine shape.** Every ravine voxel satisfies `d < HalfWidth(t)`; the fissure closes at both
    ends (`HalfWidth(t) → 0` as `t → 0` and `t → 1`); at least one 2-block-thick ledge exists
    every `RavineLedgeSpacing` blocks on the wall.
15. **Ravine ledges do not plug tunnels.** A voxel carved by a tunnel inside a ravine's ledge band
    remains Air.
16. **Mineshaft geometry.** Corridors are exactly `2 × MineshaftHalfWidth` blocks wide and
    `MineshaftCorridorHeight` tall; timber frames appear every `MineshaftFrameSpacing` blocks;
    no mineshaft voxel is above `TerrainHeight − CaveSurfaceCap`.
17. **Mineshaft continuity.** A corridor crossing a chunk border is continuous across it.
18. **Loot determinism.** A barrel's contents are identical when the chunk is unloaded and
    reloaded without being opened, and are **not** re-rolled after being looted and reloaded.
19. **Ore exposure.** `OreExposureRate` over 64 chunks lies in `[0.05, 0.15]` (§4.11). If it is
    below `0.05`, `CaveAdjacencyOreBonus` is the sanctioned lever — not a wider cave volume.
20. **No floating ore.** Every ore voxel has at least one solid neighbour from the ore pass's
    perspective; no ore voxel is fully surrounded by Air.
21. **Pass order.** An injected ore voxel in a cave volume cannot exist: the ore pass, run after
    the cave pass, writes nothing into Air. Verified by a targeted unit test on the pass order.
22. **Performance.** `FVoxelFrameStats` cave-pass time (`p95`) is ≤ `CavePassBudgetMs` (1.2 ms)
    over a 512-chunk streaming benchmark with `RenderDistance = 8`. `MaxGenerationTimeMs` is never
    exceeded.
23. **Memory.** The cave pass performs zero heap allocations per chunk beyond the pre-allocated
    interpolation scratch grid.
24. **Thread safety.** Running generation on the async path produces the same result as the
    synchronous path; no data race is reported by the automation test running 4 concurrent chunk
    generations.
25. **Biome profiles.** Changing a row in `DT_CaveProfiles` changes the measured volume fraction
    in that biome only, in the direction implied by the multiplier, with all radii unchanged.
26. **Toggles.** `bEnableCaves = false` produces zero carved voxels and zero structure blocks;
    `bEnableCaveStructures = false` produces carved voxels but zero mineshaft/dungeon blocks.
27. **Tuning surface.** Every knob in §7 is `EditAnywhere` with a declared `ClampMin`/`ClampMax`;
    no cave value is a literal in C++ (verified by review against `ue-data.md`).
28. **`GeneratorVersion` discipline.** Any change to a frequency, threshold, radius, offset,
    lattice constant or biome profile is accompanied by a `GeneratorVersion` bump and a
    `save-load.md` migration note in the same commit.

---

## Appendix A — Open Questions, Risks and Deferred Work

**R1 — `float` noise precision at large world coordinates.** `Perlin3D(WX · f)` with
`WX ≈ 100,000` and `f = 0.03` evaluates noise at `≈ 3,000`, where `float` has ~0.0002 resolution.
The field degrades and can visibly discontinuity. Mitigation options: (a) compute the coordinate
product in `double` before the `float` noise call, (b) wrap world coordinates at a `2²⁰`-block
period, (c) accept a practical world limit. **This affects the existing implementation today**,
not just caves. Needs a `technical-director` decision.

**R2 — No navmesh for procedural chunks.** Cave mobs use direct steering. A chamber with a
single narrow exit can trap mobs. Mitigated by AC 8/10 but not solved. Needs an
`ai-programmer` / `unreal-specialist` assessment.

**R3 — No fall damage.** `player-vitals.md` implements health and regen but no fall damage. Deep
ravines and caverns are therefore not lethal, which removes most of the risk from vertical
cave features. Either fall damage ships, or ravine design must be re-evaluated. **Blocking for
the ravine archetype's stated fantasy.**

**R4 — Cave lighting legibility.** `CaveAmbientLuminance` and the "silhouettes visible, threats
not identified" requirement are an art decision. Also open: emissive ore glow (needs an emissive
mask in `M_Global`) — strong thematic and legibility value, currently unimplemented.

**R5 — Underground fluid.** No lava, no underground water, no aquifers. Caves are dry. If lava
ships later, `CaveLavaFillZ` below a depth threshold becomes a new archetype modifier and a new
hazard layer. Out of scope for v1.

**R6 — Hillside cave mouths.** Deferred. The sinkhole covers the gameplay function. A horizontal
arch entrance needs slope-aware geometry.

**R7 — `angels.md` is a stale duplicate** of `weeping-angels.md`. Reconcile before either is
implemented; this document references `weeping-angels.md` only.

**R8 — POI / cave conflict.** `poi-placement.md` must define `POIVoidTolerance` and reject a POI
pad over a large cave void. Until it does, a village can be generated over a cavern roof.

**Open design question for the `game-designer`:** should the deepest caves contain a
*non-random* landmark (a single authored "deepest chamber" per world) to give descent a
destination? The current system is uniform at all depths, and a purely statistical underground
may not reward a 150-block descent enough. This is a scope decision, not a formula.
