# Mood board — folder guide (frozen 2026-11-12)

**Read this before using any file in this folder as a reference.**

This directory is the **frozen P1 reference set**. It is a *defect record* and a
*target set*, not a gallery of things the game should look like. Some files here
are deliberately **anti-targets** — frames that show exactly what P2 must
eliminate.

| Authority | Document |
|---|---|
| Machine index (file → status → purpose) | `manifest.csv` (this folder) |
| Readable contract (what each frame licenses) | `Docs/art/art-bible.md` §19 |
| Measurement detail | `Docs/art/mood-board.md` §9 |
| Adjustment statuses A1–A6 | `Docs/art/art-bible.md` §15 |

---

## The one file you must not mistake for a target

### `Screenshot 2026-05-23 202820.png` — **BEFORE / pre-overhaul — EXCLUDED**

- **Set:** BEFORE (predates both capture sets)
- **Status:** `excluded` — never a P2 target
- **Why:** flat vanilla Minecraft lighting, no sodium, no ash fog, no
  post-process, no grade. It is the visual state the project is *leaving*.
- **What it is good for:** exactly one thing — showing what the build looked like
  before the overhaul, when someone needs that contrast.
- **A6 note:** earlier documents cited this file as `000820`, which does not
  exist. The correct filename is `Screenshot 2026-05-23 202820.png`. If you see
  `000820` referenced anywhere, that reference is stale.

---

## Statuses

| Status | Meaning | How to use it |
|---|---|---|
| **approved** | A P2 target | Reproduce the property named in `art-bible.md` §19 — nothing more |
| **conditional** | One narrow use, named in the manifest | Only for that use. Do not generalise it |
| **excluded** | Never a target | Do not cite it as direction. Two files: the pre-overhaul capture and `000917` |

## The two anti-targets

- `Screenshot 2026-11-11 105250.png` — the Angel renders `#6E7C74` against fog
  `#93A19A`: **darker than its own background**, at 0.56×. This is the thesis
  failure. P2 must produce its inverse.
- `Screenshot 2026-11-11 105418.png`, `105502.png` — torch framing references
  whose verdict is **contested** (`art-bible.md` §4.2). Use the *framing and
  falloff shape*; do not tune against the old `FAIL` verdict, which a second
  measurement did not reproduce.

## Naming from here on

New captures use semantic names, not timestamps:

```
<tier>_<subject>_<distance>_<state>.png
A1a_Angel_25b_fog.png
A5_Cave_L0_nolight.png
A3_Torch_5b_HUD.png
```

The P1 timestamp filenames are kept for traceability. `Angel_60b_fog.png` was
cancelled before it was ever shot; its replacement is `Angel_25b_fog.png`.
