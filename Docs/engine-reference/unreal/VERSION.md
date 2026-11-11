# Unreal Engine — Version Reference

| Field | Value |
|-------|-------|
| **Project Engine Version** | Unreal Engine 5.8 (`EngineAssociation` in `MinecraftDeWish.uproject`) |
| **Project Pinned** | 2026-11-09 |
| **Bundled Reference Snapshot** | Unreal Engine 5.7 (2026-02-13) |
| **LLM Knowledge Cutoff** | May 2025 |

## ⚠️ 5.8 Knowledge Gap

The bundled notes in this directory were written for **UE 5.7**, and the model's
training data stops around **UE 5.3**. **UE 5.8 is newer than both.**

Before writing engine API code for this project:

1. Read the relevant note in this directory (`breaking-changes.md`,
   `current-best-practices.md`, `deprecated-apis.md`, `modules/`, `plugins/`).
2. Verify the API against the official 5.8 documentation or the installed engine source
   (`.uproject` → engine install / `Engine/Source`).
3. If the 5.7 note and the 5.8 docs disagree, **the 5.8 docs win**.
4. Record any verified 5.8-specific finding in the relevant note so the next session
   does not repeat the research.

## Installed Engine Location (this machine)

- **Install root:** `F:\Developpement\UE_5.8`
- **Binaries (Win64):** `F:\Developpement\UE_5.8\Engine\Binaries\Win64`
- **Editor (GUI):** `F:\Developpement\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe`
- **Editor (CLI):** `F:\Developpement\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe`
- **Unreal Build Tool:** `F:\Developpement\UE_5.8\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe`
- **Build script:** `F:\Developpement\UE_5.8\Engine\Build\BatchFiles\Build.bat`
- **Engine source:** `F:\Developpement\UE_5.8\Engine\Source`
- **Build.version:** `F:\Developpement\UE_5.8\Engine\Build\Build.version`

UE is **not** under `C:\Program Files\Epic Games\` on this machine (3 drives). If these paths
fail, follow the (re)location procedure in `.opencode/docs/technical-preferences.md`
→ "Unreal Engine Location". Never assume the default install location.

## Verified Sources (check the 5.8 variants)

- Official docs: https://dev.epicgames.com/documentation/en-us/unreal-engine/
- UE 5.8 release notes: https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-engine-5-8-release-notes
- What's new: https://dev.epicgames.com/documentation/en-us/unreal-engine/whats-new
- Upgrade/migration: https://dev.epicgames.com/documentation/en-us/unreal-engine/upgrading-projects

## Previous Version Timeline (context only)

| Version | Release | Risk Level | Key Theme |
|---------|---------|------------|-----------|
| 5.4 | ~Mid 2025 | HIGH | Motion Design tools, animation improvements, PCG enhancements |
| 5.5 | ~Sep 2025 | HIGH | Megalights (millions of lights), animation authoring, MegaCity demo |
| 5.6 | ~Oct 2025 | MEDIUM | Performance optimizations, bug fixes |
| 5.7 | Nov 2025 | HIGH | PCG production-ready, Substrate production-ready, AI assistant |
| 5.8 | 2026 | HIGH | **Project target — verify before use** |

## Known Post-Cutoff Themes (verify per version)

- **Substrate Material System** — new material framework; verify migration state before adding materials.
- **PCG (Procedural Content Generation)** — production-ready in 5.7+; relevant to this voxel/world project.
- **Megalights** — large-scale dynamic lighting; check performance budget before enabling.
- **Animation Authoring** — new rigging/animation tools; verify before replacing existing pipelines.
- **AI Assistant** — in-editor guidance; experimental, do not depend on it for production decisions.
