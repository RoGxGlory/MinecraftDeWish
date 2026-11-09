# Technical Preferences

<!-- Project-specific standards. All studio agents reference this file. -->
<!-- Engine pinned from MinecraftDeWish.uproject on 2026-11-09. -->

## Engine & Language

- **Engine:** Unreal Engine 5.8 (`EngineAssociation: "5.8"`)
- **Language:** C++ (primary) and Blueprint (content, prototyping, designer-facing tuning)
- **Rendering:** Lumen / Nanite per project settings — verify `Config/DefaultEngine.ini` before changing
- **Physics:** Chaos
- **Runtime module:** `MinecraftDeWish` (`Source/MinecraftDeWish`)
- **Build:** Unreal Build Tool + Visual Studio solution (`MinecraftDeWish.sln`)

## Naming Conventions

- **Classes:** PascalCase with the Unreal prefix — `A` (Actor), `U` (UObject/Component), `F` (struct), `E` (enum), `I` (interface)
- **Files:** match the class name without the prefix, e.g. `AChunkActor` → `ChunkActor.h` / `.cpp`
- **Variables:** PascalCase; booleans start with `b` (`bIsMining`)
- **Functions:** PascalCase, verb-first (`StartMining`, `GetBlockAt`)
- **Constants:** `static constexpr` in PascalCase, or `UPPER_SNAKE_CASE` for macros
- **Blueprints:** `BP_<Name>`, `WBP_<Name>`, `DA_<Name>`, `DT_<Name>`, `M_<Name>`, `MI_<Name>`, `T_<Name>`, `S_<Name>`
- **Assets:** no spaces, no version suffixes, no `_New` / `_Final`
- **Log categories:** `DEFINE_LOG_CATEGORY_STATIC(LogMinecraftDeWish, Log, All)`

## Performance Budgets

- **Target framerate:** 60 FPS on the target hardware (confirm with `technical-director`)
- **Frame budget:** 16.6 ms; gameplay systems should stay under 4 ms per frame
- **Memory ceiling:** TBD — measure with Unreal Insights and `stat memory`
- **Profiling:** Unreal Insights, `stat unit`, `stat game`, `stat gpu`; never rely on an FPS counter alone

## Testing

- **Framework:** Unreal Automation Test Framework (`IMPLEMENT_SIMPLE_AUTOMATION_TEST`) and functional tests in `Content/Tests`
- **Minimum coverage:** every new gameplay system must have at least one automation test for its core rule
- **Required tests:** voxel generation/chunk boundaries, crafting/smelting recipes, player vitals, inventory/slots, mob spawning rules, save/load data
- **Verification:** build the editor target and run the automation suite before claiming a feature is done

## Forbidden Patterns

- `new` / `delete` for `UObject` instances — use `NewObject<>`, `CreateDefaultSubobject<>`, `TObjectPtr<>`
- Raw `UObject*` members without `UPROPERTY()` (GC will collect them)
- Hardcoded gameplay balance values (damage, health, drop rates, recipes) — use DataTables/DataAssets
- Direct UI references from gameplay code — use delegates/events
- `Tick` on actors that do not need per-frame updates
- String comparisons in hot paths — use `FName`
- Direct attribute mutation outside GAS (once the relevant system is GAS-based)
- New plugins or third-party libraries without `technical-director` approval

## Allowed Libraries / Addons

- Unreal Engine built-in modules
- Plugins already enabled in `MinecraftDeWish.uproject`: `ModelingToolsEditorMode`, `GameplayStateTree`
- New dependencies require an ADR in `Docs/architecture/` and explicit approval

## Architecture Decisions Log

- ADRs live in `Docs/architecture/` (use `/architecture-decision`)
- No ADR recorded yet — create one for the next cross-system technical decision
