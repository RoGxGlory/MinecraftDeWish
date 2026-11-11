# CI / Build Setup — MinecraftDeWish

This document describes the continuous integration, build pipeline and Git LFS
configuration for MinecraftDeWish (Unreal Engine 5.8, Windows).

## Overview

| Concern | Implementation |
|---|---|
| Source control | Git (trunk-based: `main`, `develop`, `feature/*`, `release/*`, `hotfix/*`) |
| Large binaries | Git LFS (see `.gitattributes`) |
| Build CI | `.github/workflows/build-ue.yml` — self-hosted Windows runner with UE 5.8 |
| Asset CI | `.github/workflows/asset-validation.yml` — Ubuntu runner, Python/Pillow |
| Automation tests | Unreal Automation Test Framework, filter `MinecraftDeWish.*` |

## Git LFS

Tracked extensions (`.gitattributes`):

```
*.uasset, *.umap, *.png, *.wav, *.fbx, *.blend
```

### First-time migration (one-off, run locally)

```powershell
# 1. Ensure Git LFS is installed
git lfs version

# 2. Track the patterns (already in .gitattributes)
git lfs track "*.uasset" "*.umap" "*.png" "*.wav" "*.fbx" "*.blend"

# 3. Migrate already-committed binaries into LFS (rewrites history)
git lfs migrate import --include="*.uasset,*.umap,*.png,*.wav,*.fbx,*.blend" --everything

# 4. Verify
git lfs ls-files | Select-Object -First 20
git lfs status
```

> Migrating history rewrites commits. Coordinate with the team before pushing
> rewritten history to `origin/main`.

### LFS storage / bandwidth

GitHub LFS free tier includes 1 GB storage and 1 GB bandwidth per month. With
~7k `.uasset` and ~6.7k `.png` files this project will exceed that quickly;
consider a self-hosted LFS server (e.g. `lfs-test-server`) or Git LFS on a
private mirror for large teams.

## Self-hosted runner setup

UE 5.8 is too large to install on a hosted runner within job time, so the build
job targets a self-hosted Windows runner.

### Runner labels

Register the runner with these labels:

```
self-hosted, windows, ue-5.8
```

### Required software on the runner

1. Windows 10/11 (x64).
2. Unreal Engine 5.8 installed (default `C:\Program Files\Epic Games\UE_5.8`).
   - Verify `Engine\Build\BatchFiles\Build.bat` and `RunUAT.bat` exist.
3. Visual Studio 2022 with the "Game development with C++" workload
   (MSVC v143, Windows 10/11 SDK).
4. Git + Git LFS on `PATH`.
5. GitHub Actions runner service running as a user that can launch UE.

### Runner environment variables

Set on the runner (machine or service env):

| Variable | Value | Purpose |
|---|---|---|
| `UE_ROOT_RUNNER` | `C:\Program Files\Epic Games\UE_5.8` | Override default UE path |
| `UE_ROOT` | (optional) | Per-job override via `workflow_dispatch` |

### Runner registration (summary)

```powershell
# From the runner directory
.\config.cmd --url https://github.com/RoGxGlory/MinecraftDeWish `
  --token <RUNNER_TOKEN> --labels "self-hosted,windows,ue-5.8" --name mcwish-ue58-win
.\run.cmd   # or install as a service
```

## Build workflow (`.github/workflows/build-ue.yml`)

### Jobs

1. **build-editor** — compiles `MinecraftDeWishEditor` (Win64, Development).
   - Restores the Derived Data Cache (DDC) from cache.
   - Uploads `Saved/Logs/*.log` on failure.
2. **automation-tests** — depends on `build-editor`; runs the automation suite
   filtered to `MinecraftDeWish.*` via `RunUAT RunAutomationTests`.
   - Uploads `TestResults/` (30-day retention).

### Triggers

- `push` to `main`, `develop`, `release/*`, `hotfix/*`
- `pull_request` to `main`, `develop`
- `workflow_dispatch` (manual, with optional `ue_root` input)

### Concurrency

One in-flight run per ref; newer pushes cancel older runs.

### Local reproduction

```powershell
# Build editor target
& "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" `
  MinecraftDeWishEditor Win64 Development `
  "-Project=$PWD\MinecraftDeWish.uproject" -WaitMutex -FromMsBuild

# Run automation tests
& "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\RunUAT.bat" RunAutomationTests `
  "-project=$PWD\MinecraftDeWish.uproject" -platform=Win64 -configuration=Development `
  -test=MinecraftDeWish -nullrhi -unattended -nopause `
  "-ReportOutputPath=$PWD\TestResults"
```

### Test filter

The 8 (and growing) automation tests live under
`Source/MinecraftDeWish/Private/Tests/` and are registered with names like
`MinecraftDeWish.Mining.*`, `MinecraftDeWish.Voxel.*`,
`MinecraftDeWish.WorldGenerator.*`. The `-test=MinecraftDeWish` filter runs all
of them. To run a single suite locally, pass `-test=MinecraftDeWish.Mining`.

## Asset validation workflow (`.github/workflows/asset-validation.yml`)

Runs on `ubuntu-latest`:

1. Installs Python 3.11 + Pillow.
2. Runs `.github/scripts/validate_assets.py`:
   - No spaces in asset paths under `Content/`.
   - No forbidden name suffixes (`_New`, `_Final`, `_Copy`, `_vN`, ...).
   - PNG dimensions power-of-two and ≤ 4096 px; per-file size ≤ 64 MB.
   - FBX per-file size ≤ 256 MB.
   - Prefix heuristics (warning only) for `Blueprints/`, `UI/`, `Materials/`,
     `Textures/`, `Import/`.
3. Validates every `*.json` is valid UTF-8 without trailing commas.
4. Warns about large (>100 MB) files not tracked by LFS.

Errors fail the job; warnings are reported but do not block.

## Branching strategy

- `main` — always shippable, protected (requires green CI + review).
- `develop` — integration branch, runs full CI.
- `feature/*` — branched from `develop`.
- `release/*` — release candidates.
- `hotfix/*` — emergency fixes from `main`.

## Artifacts & retention

| Artifact | Retention | Contents |
|---|---|---|
| `build-log` | 14 days | Build logs (on failure) |
| `automation-test-report` | 30 days | Automation test results |
| DDC cache | (cache) | Derived Data Cache |

## Troubleshooting

- **"Unreal Engine 5.8 not found"** — set `UE_ROOT_RUNNER` on the runner or
  pass `ue_root` via `workflow_dispatch`.
- **LFS checkout slow / missing assets** — ensure `lfs: true` on checkout and
  that the runner has LFS bandwidth quota; run `git lfs pull` manually.
- **Tests pass locally but fail in CI** — check `-NullRHI` (no GPU) and
  `EngineFilter` flags; tests requiring a rendering context may need
  `-RenderOffscreen` instead.
- **Build cache stale** — bump the DDC cache key by changing a `.uasset`/
  `.umap` (automatic) or clear the cache in the Actions UI.
- **Runner offline** — the job will queue; check the runner service and that
  the machine is powered on.

## Related documents

- `Docs/SYSTEMS_REFERENCE.md` — implemented gameplay systems.
- `.opencode/docs/technical-preferences.md` — project standards.
- `PRE_RELEASE.md`, `RELEASE_v0.2.0-alpha.md` — release context.
