# Material Parameters — LAMPLIGHTER

**Last updated:** 2026-11-10
**Owner:** Technical Artist
**Rule:** every exposed parameter of a master material must be listed here.
A parameter not listed here must not be exposed.

## M_Terrain_Opaque / M_Terrain_Masked

| Parameter | Type | Range | Default | Effect |
|---|---|---|---|---|
| `Textures_Array` | Texture2DArray | — | T_Terrain_BC | Base color array (slice = UV1.X) |
| `Normals_Array` | Texture2DArray (Normal) | — | T_Terrain_N | Tangent-space normals |
| `Specular_Array` | Texture2DArray (Linear) | — | T_Terrain_S | R=smoothness, G=reflectance, A=emissive |
| `NormalStrength` | float | 0.0–2.0 | 1.0 | Normal map intensity |
| `RoughnessBias` | float | -0.3–0.3 | 0.0 | Shifts roughness from specular R |
| `SpecularScale` | float | 0.0–1.0 | 1.0 | Reflectance multiplier |
| `EmissiveBoost` | float | 0.0–5.0 | 1.0 | Emissive multiplier (minerals, lava) |
| `TilingScale` | float | 0.5–2.0 | 1.0 | UV tiling multiplier |
| `BiomeBlendWeight` | float (vertex) | 0.0–0.5 | 0.0 | Cross-fade weight (VertexColor.B) |
| `WindStrength` | float (MPC) | 0.0–1.0 | 0.0 | Foliage wind (masked only) |
| `OpacityMaskClipValue` | float (masked) | 0.1–0.9 | 0.333 | Alpha test threshold (masked only) |

## M_BlockCrack

| Parameter | Type | Range | Default | Effect |
|---|---|---|---|---|
| `CrackTex` | Texture2D | — | destroy_stage_0 | Crack overlay texture |
| `StageIndex` | int | 0–9 | 0 | Current crack stage |
| `EmissiveBoost` | float | 0.0–2.0 | 0.55 | Crack emissive multiplier |
| `OpacityMaskClipValue` | float | 0.05–0.5 | 0.15 | Mask clip threshold |

## M_Debris_Voxel

| Parameter | Type | Range | Default | Effect |
|---|---|---|---|---|
| `SliceOuter` | int | 0–127 | 0 | Outer face texture slice |
| `SliceInner` | int | 0–127 | 0 | Interior (exposed) face slice |
| `DissolveAmount` | float | 0.0–1.0 | 0.0 | Fade-out dissolve |

## M_ItemPickup

| Parameter | Type | Range | Default | Effect |
|---|---|---|---|---|
| `SliceIndex` | int | 0–127 | 0 | Block texture slice |
| `EmissiveBoost` | float | 0.0–2.0 | 1.0 | Emissive multiplier |

## M_Mob

| Parameter | Type | Range | Default | Effect |
|---|---|---|---|---|
| `BaseColorTex` | Texture2D | — | — | Base color (128x128, Nearest) |
| `NormalTex` | Texture2D | — | — | Tangent-space normal |
| `SpecularTex` | Texture2D | — | — | LabPBR specular |
| `RoughnessBias` | float | -0.3–0.3 | 0.0 | Roughness shift |
| `HitFlash` | float | 0.0–1.0 | 0.0 | Damage flash (white overlay) |

## M_Statue (Weeping Angel)

| Parameter | Type | Range | Default | Effect |
|---|---|---|---|---|
| `BaseColorTex` | Texture2D | — | — | Stone/albedo |
| `NormalTex` | Texture2D | — | — | Stone detail normal |
| `WetnessMask` | float | 0.0–1.0 | 0.0 | Wet surface (rain) |
| `MossBlend` | float | 0.0–1.0 | 0.0 | Moss overlay blend |
| `StoneDetailNormal` | Texture2D | — | — | Micro-detail normal |

## M_UI_Enamel

| Parameter | Type | Range | Default | Effect |
|---|---|---|---|---|
| `Tint` | LinearColor | — | `#E3DCCB` | Panel tint |
| `ScrimOpacity` | float | 0.0–1.0 | 0.7 | Background scrim opacity |
| `FlickerAmount` | float | 0.0–0.1 | 0.0 | Subtle flicker (photosensitivity toggle) |

## M_UI_CRT

| Parameter | Type | Range | Default | Effect |
|---|---|---|---|---|
| `Tint` | LinearColor | — | `#4FA6A0` | CRT tint |
| `ScanlineIntensity` | float | 0.0–1.0 | 0.3 | Scanline strength |
| `FlickerAmount` | float | 0.0–0.1 | 0.0 | Flicker (photosensitivity toggle) |

## M_UI_BlueprintGrid

| Parameter | Type | Range | Default | Effect |
|---|---|---|---|---|
| `Tint` | LinearColor | — | `#2A3138` | Grid tint |
| `GridScale` | float | 1.0–10.0 | 4.0 | Grid cell size |
| `GridOpacity` | float | 0.0–1.0 | 0.15 | Grid line opacity |

## M_VFX_Dust

| Parameter | Type | Range | Default | Effect |
|---|---|---|---|---|
| `SliceIndex` | int | 0–127 | 0 | Block texture slice (color source) |
| `Dissolve` | float | 0.0–1.0 | 0.0 | Particle dissolve |
| `Softness` | float | 0.0–1.0 | 0.5 | Edge softness |

## M_VFX_Chip

| Parameter | Type | Range | Default | Effect |
|---|---|---|---|---|
| `SliceIndex` | int | 0–127 | 0 | Block texture slice |
| `Dissolve` | float | 0.0–1.0 | 0.0 | Chip dissolve |

## M_VFX_AngelDistort

| Parameter | Type | Range | Default | Effect |
|---|---|---|---|---|
| `DistortAmount` | float | 0.0–0.05 | 0.0 | Refraction distortion (brief, on Angel move) |
| `Duration` | float | 0.0–1.0 | 0.3 | Effect duration |

## M_VFX_TorchFlame

| Parameter | Type | Range | Default | Effect |
|---|---|---|---|---|
| `FlameColor` | LinearColor | — | `#E8A33D` | Flame color |
| `Intensity` | float | 0.0–2.0 | 1.0 | Flame intensity |
| `FlickerSpeed` | float | 0.0–5.0 | 2.0 | Flicker frequency |