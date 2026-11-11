# Technical Preferences

<!-- Project-specific standards. All studio agents reference this file. -->
<!-- Engine pinned from MinecraftDeWish.uproject on 2026-11-09. -->

## Engine & Language

- **Engine:** Unreal Engine 5.8 (`EngineAssociation: "5.8"` in `MinecraftDeWish.uproject`)
- **Language:** C++ (primary) and Blueprint (content, prototyping, designer-facing tuning)
- **Rendering:** Lumen / Nanite per project settings — verify `Config/DefaultEngine.ini` before changing
- **Physics:** Chaos
- **Runtime module:** `MinecraftDeWish` (`Source/MinecraftDeWish`)
- **Build:** Unreal Build Tool + Visual Studio solution (`MinecraftDeWish.sln`)

## Unreal Engine Location (this machine — IMPORTANT, read before any engine work)

The user has 3 drives; UE is **not** in the default `C:\Program Files\Epic Games\`.
Always use these paths; never guess or re-scan unless they fail.

- **Version:** Unreal Engine 5.8
- **Install root:** `F:\Developpement\UE_5.8`
- **Binaries (Win64):** `F:\Developpement\UE_5.8\Engine\Binaries\Win64`
- **Editor (GUI):** `F:\Developpement\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe`
- **Editor (CLI/headless):** `F:\Developpement\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe`
- **Unreal Build Tool:** `F:\Developpement\UE_5.8\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe`
- **Build script:** `F:\Developpement\UE_5.8\Engine\Build\BatchFiles\Build.bat`
- **Engine source:** `F:\Developpement\UE_5.8\Engine\Source`
- **Build version file:** `F:\Developpement\UE_5.8\Engine\Build\Build.version`

### How to (re)locate the engine if these paths ever fail

1. Read `MinecraftDeWish.uproject` and note `EngineAssociation` (e.g. `"5.8"`).
2. Enumerate all filesystem drives and search for `UnrealEditor.exe`:
   ```powershell
   $drives = (Get-PSDrive -PSProvider FileSystem).Root
   Get-ChildItem -Path $drives -Filter "UnrealEditor.exe" -Recurse -ErrorAction SilentlyContinue -Depth 6 |
     Select-Object -First 5 FullName
   ```
   Or search for the folder `UE_5.8`:
   ```powershell
   Get-ChildItem -Path $drives -Filter "UE_5.8" -Directory -Recurse -ErrorAction SilentlyContinue -Depth 4 |
     Select-Object -First 5 FullName
   ```
3. Verify a candidate by checking that `Engine\Build\Build.version` reports the expected
   `MajorVersion`/`MinorVersion` and that `Engine\Binaries\Win64\UnrealEditor.exe` exists.
4. Also check Epic Games Launcher installs: `C:\ProgramData\Epic\UnrealEngineLauncher\LauncherInstalled.dat`.
5. Never assume `C:\Program Files\Epic Games\` — on this machine the engine lives on `F:`.
6. After resolving, update this document so future sessions do not re-scan.

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
