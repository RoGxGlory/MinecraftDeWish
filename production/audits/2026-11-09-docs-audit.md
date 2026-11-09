# Documentation Audit — 2026-11-09

**Scope:** `Docs/` (24 files), the four root documents (`README.md`, `PRE_RELEASE.md`,
`RELEASE_v0.2.0-alpha.md`, `AGENTS.md`), `Source/MinecraftDeWish` (18 classes,
~7,000 lines), `Config/`, `MinecraftDeWish.uproject`, and the `Content/` tree.
**Auditor:** studio session at commit `48fd854` plus the fixes recorded below.
**Status:** factual fixes applied and **uncommitted**. Design work is tracked separately
in `design/`.

---

## 1. Method

Three comparisons:

1. **Documentation ↔ code.** Every claim in a registry or technical reference was checked
   against the C++ implementation and the assets that actually exist.
2. **Documentation ↔ documentation.** Release notes, README, registries and the GDD were
   compared to each other for contradictions.
3. **Documentation ↔ game premise.** The project's actual design (post-apocalyptic
   Minecraft-like horror survival built around the Weeping Angels) was compared with what
   the documentation says the game is.

---

## 2. Drift table

Findings that were **factually wrong** (doc said X, code does Y).

| # | Subject | Documentation claimed | Code / assets actually do | Resolution |
|---|---|---|---|---|
| A | Controls | `PRE_RELEASE.md`: WASD, drop `Q`, inventory `E` working | `RELEASE_v0.2.0-alpha.md`: **ZQSD**, drop `A`, several actions `(WIP)` | Kept v0.2 (ZQSD/AZERTY) as truth; release notes frozen with archive banners |
| B | Block debris | `SYSTEMS_REFERENCE` §5: Chaos solver, 8–18 convex cells, gravity 1200, 2.2 s + 0.8 s | 32–44 fragments of 5–9 cm, gravity **1350**, **2.0 s** + 0.8 s, one `UProceduralMeshComponent`, per-particle sweeps, **no Chaos** | §5 rewritten from the code |
| C | Biomes | `GAME_DESIGN.md`: no mention of biomes | `EBiomeType` = Plains / Forest / Mountains / Desert / **HorrorForest** | GDD marked obsolete; biome design deferred to `design/gdd/` |
| D | Mob loot | `MOB_REGISTRY`: `BlockID 17 (String)`, `BlockID 1 (Bone)` | 17 = Leaves, 1 = Dirt. No item registry — pickups are block IDs | Registry now labels every drop a **block proxy** and records the real block |
| E | Non-solid blocks | `BLOCK_REGISTRY`: Fence / Gate / Door / Glass transparent and “operable” | Only `Torch` is non-solid. All four are full cubes; no door state | Registry corrected + “Implementation limits” section added |
| F | Containers | `SYSTEMS_REFERENCE`: chest / crate / barrel / backpack containers | No container class in `Source/`; only hover tracking exists | README corrected; container design deferred |
| G | Water | `BLOCK_REGISTRY`: `Water`, bucket tool, durability 100 | No bucket, no fluid simulation | Marked **not implemented** |
| H | Release status | v0.2: “Everything below is implemented and playable” | Same page marks four features `(WIP)` | Release notes frozen; banner points at `Docs/` for current state |
| I | Project metadata | Project named “MinecraftDeWish” | `DefaultGame.ini`: `ProjectName=First Person BP Game Template` | Renamed to `MinecraftDeWish` |
| J | Packaging | — | Cooks `/Game/Blueprints` (does not exist) and omitted `/Game/Variant_Horror` + `Lvl_Horror` | Dead cook path removed; horror assets cooked explicitly |
| K | Unused plugin | — | `ChaosVehiclesPlugin` enabled, never used | Disabled in `MinecraftDeWish.uproject` |

---

## 3. Documentation gaps

Not wrong, simply absent.

| Gap | Why it matters |
|---|---|
| `Docs/GAME_DESIGN.md` does not meet the project's own GDD standard | Missing Player Fantasy, pillars, non-goals, edge cases, tuning knobs, acceptance criteria and dependencies. It is a mechanics inventory. |
| Zero automation tests | `IMPLEMENT_SIMPLE_AUTOMATION_TEST` has 0 occurrences, while `.opencode/rules/test-standards.md` requires one per gameplay rule. |
| Zero ADRs | `Docs/architecture/` contains only a README, while `coding-standards.md` requires an ADR per system. |
| No level design documentation | `VoxelStructureGenerator` already places village roads, houses, crafting tables, furnaces and torches with no design rationale. |
| No Weeping Angel documentation | The core antagonist has no design document at all. |
| Horror assets undocumented | `BP_HorrorCharacter`, `BP_HorrorGameMode`, `BP_HorrorPlayerController`, `Lvl_Horror`, flickering light, dust motes, `IA_Sprint` — no design text. |
| No UI/UX, audio, save-format or performance documents | UI and audio exist in `Content/` but are undocumented. |
| No item system | Mobs drop blocks as proxies; this blocks loot, crafting of items, durability economy and narrative props. |

---

## 4. Design problems (beyond documentation)

1. **The heart of the game is 0% implemented.** `grep -i "angel|weeping|statue|observer|lineofsight"` over `Source/` returns nothing. `EMobType` is Zombie / Skeleton / Spider / Creeper. There is no observation test (FOV + line trace), no light gate, no day/night gate, and no statue/move state machine.
2. **No item system.** Every pickup is a block ID.
3. **Tool↔block affinity is hardcoded** in `FBlockHelpers::GetPreferredToolType()` (a C++ `switch`), although `Block_DataTable` already exists and project rules forbid gameplay values in code.
4. **`AChaosBlockDestructionActor` is misnamed** — it is a hand-written ballistic simulation; the name pollutes the docs. Rename tracked as tech debt.
5. **Two competing entry points** exist (`Lvl_FirstPersonTest` + `BP_FirstPersonGameMode` are the configured startup; the horror assets are the intended direction). This is documented, not changed.

---

## 5. Applied fixes

| File | Change |
|---|---|
| `Docs/SYSTEMS_REFERENCE.md` | §5 rewritten from the code; duplicate section number `6.` removed and subsequent sections renumbered to 13 |
| `Docs/BLOCK_REGISTRY.md` | Fence / Gate / Door / Glass marked cube placeholders; `Water` marked unimplemented; “Implementation limits” section added |
| `Docs/MOB_REGISTRY.md` | “Item drops are block proxies” table added; every loot line corrected; missing Spider Eye recorded |
| `Docs/GAME_DESIGN.md` | Obsolete banner pointing here and to `design/gdd/game-concept.md` |
| `PRE_RELEASE.md` | Archive banner (v0.1) |
| `RELEASE_v0.2.0-alpha.md` | Archive banner (v0.2) |
| `README.md` | Repositioned as post-apocalyptic horror survival; status callout; drop key corrected to `A`; container claim removed; documentation index rebuilt |
| `Config/DefaultGame.ini` | `ProjectName=MinecraftDeWish`; dead `/Game/Blueprints` cook path removed; `/Game/Variant_Horror` and `Lvl_Horror` added to cook |
| `MinecraftDeWish.uproject` | `ChaosVehiclesPlugin` disabled |

## 6. Decisions recorded during the audit

| Topic | Decision |
|---|---|
| Game identity | Minecraft-like sandbox **plus its own identity** (PBR, Baubles, horror/atmospheric direction). Not a 1:1 clone. |
| Antagonist | The **Weeping Angels** are the core threat. Classic mobs (zombie, skeleton, spider, creeper) remain as a secondary bestiary. |
| Controls | **ZQSD / AZERTY** per v0.2 is the source of truth. |
| Release notes | Frozen with archive banners, not rewritten. |
| Historical docs | Historical release documents stay as records. |
| Documentation strategy | Consolidate into `design/`; `Docs/` becomes the reference/archive layer. |
| Current build | `Lvl_FirstPersonTest` + `BP_FirstPersonGameMode` remain the configured entry point; the horror assets are the target to build toward. |
| Config | Metadata, cook paths and the unused plugin fixed in this pass. |

## 7. Angel rule (design note, not implemented)

An Angel may move when **both**:

- it is **night or the local light level is low**, and
- it is **not inside the player's field of view** (FOV cone + unobstructed line trace).

Daylight under open sky therefore turns them into statues. Nothing in the codebase
implements any part of this rule yet.
