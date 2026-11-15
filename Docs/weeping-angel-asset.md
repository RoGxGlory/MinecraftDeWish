# Weeping Angel — Asset Pipeline & Model Complexification

> Companion document to `design/gdd/weeping-angels.md` (behaviour) and
> `Docs/art/art-bible.md` §8 (contrast/readability). This page explains **how the
> Angel was built as an asset**, and how its model was deliberately complexified by
> splitting out the right arm so it could be reused as a UI visual option selector
> in the main menu.
>
> Screenshots: `Docs/images/weeping-angel/`.

---

## 1. Overview

The Weeping Angel is the only **non-voxel, smooth, organic** form in a voxel
world. It is sculpted stone/plaster, bone-white (`#E3DCCB`), high roughness, no
metal, no emissive, no VFX — ever. The contrast between the blocky world and the
smooth statue *is* the mechanic made visible: a player who sees a smooth shape in
a blocky world knows, without a HUD, that they are looking at an Angel.

To sell that, the Angel is delivered as a set of **static statue poses** rather
than a rigged character with a walk cycle. Each pose is a separate static mesh
authored in Blender, exported as FBX, and imported into Unreal as its own asset.

---

## 2. Source asset

| Item | Value |
|---|---|
| Blender source | `Blender/WeepingAngel_Separated.blend` (plus `.blend1` backup) |
| Exported meshes | `Content/Import/SM_WeepingAngel_Point_Body.fbx`, `…_RightArm.fbx` |
| In-engine meshes | `Content/Characters/WeepingAngel/Meshes/SM_WeepingAngel_*.uasset` |
| Blueprints | `Content/Blueprints/SM_WeepingAngel_Point_RightArm.uasset` |
| Materials | `M_WeepingAngel_Body`, `M_WeepingAngel_Face`, `M_WeepingAngel_Wings` |
| Textures | `T_WeepingAngel_Body_D/_N`, `…_Face_D/_N`, `…_Wings_D/_N` |
| Material colour | `#E3DCCB` bone white, high roughness, no specular map (outstanding) |

Export convention follows `Docs/technical-contract.md` §6: Forward `-Z`, Up `Y`,
`Apply Transform` ON, 1 unit = 1 m, FBX import scale 1.0. Auto-generate collision
is OFF; collision is authored explicitly.

---

## 3. The 8 statue poses

No walk cycle. The Angel uses 8 static poses that read as "caught mid-gesture".
Each is its own static mesh so a frozen statue is a single draw with no animation
state:

| # | Mesh | Pose |
|---|---|---|
| 1 | `SM_WeepingAngel_HandsOnFace` | Both hands covering the face, head tilted down (default spawn pose) |
| 2 | `SM_WeepingAngel_Point` / `…_Point_Body` + `…_RightArm` | One arm extended forward, finger pointing |
| 3 | `SM_WeepingAngel_HeadTurned` | Torso one way, head turned sharply the other |
| 4 | `SM_WeepingAngel_MidStride` | One leg forward, arms at sides, leaning into motion |
| 5 | `SM_WeepingAngel_Crouched` | Low crouch, hands on ground, wings folded tight |
| 6 | `SM_WeepingAngel_WingsHalfOpen` | Upright, wings spread to ~45° |
| 7 | `SM_WeepingAngel_ArmsCrossed` | Arms folded, wings folded, head level |
| 8 | `SM_WeepingAngel_Reaching` | Both arms extended forward, fingers spread |
| — | `SM_WeepingAngel_Attack` | Attack/contact pose |
| — | `SM_WeepingAngel_Idle` | Idle/standing variant |

A pose is selected when the Angel **freezes** (enters the `Statue` state) and
persists until it moves again. The pose change between two glances is the proof
that it moved while unobserved. Pose selection is a deterministic roll keyed on
`(AngelID, Position, TimeOfDay)` — it is an instant snap, accompanied by a
stone-grind sound, never an animation blend.

---

## 4. How the model was complexified: splitting the right arm

Originally the pointing pose shipped as a single combined mesh,
`SM_WeepingAngel_Point_Arm` (body + arm in one piece). That was simple, but it
locked the arm to the body: the arm could not be reused, re-posed, or driven
independently, and it could not be placed on its own in a level.

The user asked to **complexify** the model by separating the right arm into its
own asset, so the arm could be used independently — specifically as a **UI
visual option selector** in the main menu. This is recorded in commit
`6f8713a — "Separate Weeping Angel right arm asset"`.

### 4.1 What changed

- Added `Blender/WeepingAngel_Separated.blend` containing the Angel with the right
  arm isolated as a separate object/mesh.
- Removed the previous combined `Meshes/SM_WeepingAngel_Point_Arm.uasset`.
- Added `Meshes/SM_WeepingAngel_Point_RightArm.uasset` plus its FBX
  (`Content/Import/SM_WeepingAngel_Point_RightArm.fbx`) and Blueprint
  (`Content/Blueprints/SM_WeepingAngel_Point_RightArm.uasset`).
- Updated `Meshes/SM_WeepingAngel_Point_Body.uasset` to the arm-less body, and
  re-authored `Materials/M_WeepingAngel_Body.uasset` to match the new split
  (body no longer carries arm geometry/material slots).

### 4.2 Why this matters

1. **Reusability**: the pointing arm is now a first-class asset. It can be
   attached to any Angel pose, re-posed, or placed standalone without duplicating
   the whole body mesh.
2. **UI selector**: in `Lvl_MainMenu` the separated right arm is used as a
   visual option selector — a pointing finger that indicates the currently
   highlighted menu entry. Because it is a real mesh (not a 2D icon), it keeps
   the Angel's material language (bone stone) in the menu and ties the predator's
   silhouette to the first screen the player sees.
3. **Cleaner pipeline**: separating the arm forced the body material to stop
   referencing arm-only slots, which made the material pass (`M_WeepingAngel_Body`)
   simpler and more correct.

### 4.3 Main menu usage

`Lvl_MainMenu` was refreshed in commit `50db7a3` ("Update Weeping Angel meshes
and main menu") to use the new body + right-arm assets together with the rebuilt
menu level. The arm is positioned as a hero element pointing at menu options; the
body statue stands behind/ beside it as the menu's focal statue.

---

## 5. Screenshots

### Figure 1 — Blender: arm separated from the body

![Blender viewport showing the Weeping Angel with the right arm separated into its own mesh](images/weeping-angel/screenshot-01.png)

The Blender source with the right arm isolated as a distinct object, ready to be
exported as `SM_WeepingAngel_Point_RightArm` while the body exports as
`SM_WeepingAngel_Point_Body`.

### Figure 2 — Main menu: arm used as a visual option selector

![Main menu level using the separated right arm as a pointing option selector](images/weeping-angel/screenshot-02.png)

`Lvl_MainMenu` with the separated right-arm mesh placed as a pointing selector
over the menu entries, keeping the Angel's bone-stone material language.

### Figure 3 — Resulting statue / pose read

![Resulting Weeping Angel pose readability](images/weeping-angel/screenshot-03.png)

The Angel as it reads in-engine: a smooth, non-voxel, bone-white statue in a
voxel world, with the pointing pose (and its now-independent arm) clearly
legible.

---

## 6. Known gaps / next steps

- **No `_S` specular map** and the base colour is not yet authored as exact
  `#E3DCCB` (measured body is `#6E7C74`). See `Docs/art/art-bible.md` §8.1
  ordered fix list — fix #1 (material base colour) is projected to clear the
  `L_statue ≥ 1.4 × L_fog` criterion on its own.
- **Green bias** (`G − R = +14` vs bone's `-7`) to be killed in the albedo.
- **8-pose readability** must be validated at 25 blocks (not 60) after the
  `PlayerObserveRange` rebase to 25 (see `design/gdd/weeping-angels.md`).
- **Animation**: none by design — only movement (teleport-like snaps between
  poses) and the stone-grind sound. Do not add a walk cycle.

---

## 7. References

- Behaviour GDD: `design/gdd/weeping-angels.md`
- Legacy/historical GDD: `design/gdd/angels.md`
- Art bible §8 (contrast is the mechanic): `Docs/art/art-bible.md`
- Asset provenance: `Docs/assets-provenance.md` (planned priority asset #5)
- Technical contract (export/import): `Docs/technical-contract.md` §5–§6
- Commit `6f8713a` — Separate Weeping Angel right arm asset
- Commit `50db7a3` — Update Weeping Angel meshes and main menu