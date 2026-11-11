# Weather Asset Creation Guide — MinecraftDeWish

**Owner:** Technical Artist / Technical Director
**Engine:** Unreal Engine 5.8
**Status:** Pending authoring (assets not yet created)
**References:** `design/gdd/weather-system.md`, `Docs/architecture/adr-0016-weather-system.md`,
`Docs/art/weather-asset-parameters.md`, `Docs/art/material-parameters.md`

This guide describes how to author the three content assets consumed at runtime by
`UDayNightCycleSystem` via soft references (`FogPostProcessMaterial`, `RainNiagaraSystem`,
`AshNiagaraSystem`). No C++ changes are required; everything is authored in the editor.

---

## 0. Target paths

Create all assets under `Content/Weather/` (create the folder if missing):

| Asset | Type | Path |
|---|---|---|
| `NS_Rain` | Niagara System | `Content/Weather/NS_Rain` |
| `NS_AshStorm` | Niagara System | `Content/Weather/NS_AshStorm` |
| `M_PP_Fog` | Material (Post Process) | `Content/Weather/M_PP_Fog` |
| `MI_PP_Fog` (optional) | Material Instance | `Content/Weather/MI_PP_Fog` |

---

## 1. NS_Rain — rain particle system

### 1.1 Create the system
1. Right-click `Content/Weather/` → **FX → Niagara System**.
2. Choose **Empty** template (we author emitters manually).
3. Name it `NS_Rain`. Open it.

### 1.2 Emitter: `RainDrops`
- **Emitter State:** Active, looping.
- **Spawn:** Add module **Spawn Rate**. Set it to a **User Parameter** (see §1.4).
  - Default value: `500.0` (particles/sec), range 100-2000.
- **Initialize Particle:**
  - Lifetime: `1.0 - 1.5 s` (random).
  - Position: spawn in a box above the camera/player. Use **Box Location** with
    size `X=2000, Y=2000, Z=100` (cm), offset `Z=+800` cm relative to the component.
  - Velocity: `Z = -1500 to -2500 cm/s` (falling rain), small X/Y wind drift `±100`.
  - Size: `1.0 - 2.0 cm` (thin streaks) or use a mesh/streak sprite.
  - Color: white/blueish, alpha `0.4 - 0.6`.
- **Update:**
  - **Gravity Force** (optional, velocity already set).
  - **Drag** (small, 0.1) for wind feel.
  - **Kill Particles in Volume** or **Collision** with world geometry: enable
    **Collision (Scene Depth)** or a simple kill plane at ground. Simpler: kill when
    `Z < 0` relative or after lifetime.
- **Render:** **Sprite Renderer** with a stretched sprite, or **Mesh Renderer** with
  a thin cylinder. Set **SubUV** off. Material: a simple translucent rain streak material
  (or default). Set blend mode **Translucent**, lighting **Unlit** or **Default Lit**.

### 1.3 Performance
- Cap max particles: **Emitter → Max Particle Count = 2000** (matches global budget;
  rain alone should stay ≤ 1500).
- Use **Fixed Bounds** (box around spawn volume) so the system is not culled incorrectly.
- Disable **Cast Shadows** on the renderer.

### 1.4 Expose the `SpawnRate` user parameter (CRITICAL)
The C++ drives spawn rate via:
```cpp
RainNiagaraComponent->SetFloatParameter(FName("SpawnRate"), static_cast<float>(RainParticleRate) * RainFactor);
```
Therefore the system **must** expose a user float parameter named exactly `SpawnRate`.

1. In the Niagara editor, open **User Parameters** (bottom panel).
2. Add parameter: Type **Float**, Name **SpawnRate**, Default **500.0**.
3. In the emitter's **Spawn Rate** module, bind the rate to `{SpawnRate}` (use the
   parameter picker, not a literal).
4. Verify by playing the system in the editor and changing the parameter in the
   preview panel — particle count should scale.

### 1.5 Save
Save `NS_Rain`. Test in a blank level: drag it into the world, play, confirm rain falls
and stops when deactivated.

---

## 2. NS_AshStorm — ash particle system

### 2.1 Create
Same as §1.1, name `NS_AshStorm`, Empty template.

### 2.2 Emitter: `AshMotes`
- **Spawn Rate:** user parameter `SpawnRate`, default `300.0`, range 50-1000.
- **Initialize:**
  - Lifetime: `3.0 - 6.0 s`.
  - Position: box around player, `X=3000, Y=3000, Z=1500`, offset `Z=+500`.
  - Velocity: slow drift `X/Y ±200`, `Z = -50 to -150` (falling ash), plus a
    horizontal wind component `X=+300` (constant wind).
  - Size: `2.0 - 6.0 cm` (larger, visible motes).
  - Color: grey, `RGB(0.4, 0.4, 0.4)`, alpha `0.3 - 0.5`.
- **Update:**
  - **Curl Noise Force** or **Vortex Force** for turbulent drift.
  - Slight rotation (random angular velocity).
  - Kill after lifetime.
- **Render:** Sprite Renderer, translucent, unlit. Material: grey soft particle.
  Enable **Depth Fade** for soft edges against geometry.

### 2.3 Performance
- Max particles: **1000** (ash is sparser than rain).
- Fixed bounds. No shadows.

### 2.4 Expose `SpawnRate`
Same as §1.4 — user float parameter `SpawnRate` bound to the Spawn Rate module.

### 2.5 Save and test
Confirm ash motes drift with wind and fade.

---

## 3. M_PP_Fog — fog post-process material

### 3.1 Create
1. Right-click `Content/Weather/` → **Material**.
2. Name `M_PP_Fog`. Open it.
3. In the material details, set **Material Domain = Post Process**.
4. Set **Blendable Location = Before Tonemapping** (or After Tonemapping; test both;
   before tonemapping keeps fog in linear space).
5. Set **Blendable Priority = 0**.

### 3.2 Graph
Goal: a distance-based fog that tints the scene grey and reduces contrast, driven
by the component's `BlendWeight` (no code-set parameters). All tuning via material
parameters (exposed to a material instance).

Nodes:
- **SceneTexture:PostProcessInput0** (or `SceneColor`) → base color.
- **Depth** (SceneTexture:SceneDepth) → distance from camera.
- Convert depth to world distance: `LinearDepth = SceneDepth / (View.ForwardVector · CameraVector)` or use the
  built-in **PixelDepth** node. Simpler: use **PixelDepth** (world units from camera).
- **FogDensity** (ScalarParameter, default 0.3, range 0-1) — overall strength.
- **FogColor** (VectorParameter, default `RGB(0.35, 0.37, 0.40)` grey-blue) — fog tint.
- **FogStart** (ScalarParameter, default 500 cm) — distance where fog begins.
- **FogEnd** (ScalarParameter, default 4000 cm) — distance where fog is fully opaque.
- `FogFactor = saturate((PixelDepth - FogStart) / (FogEnd - FogStart))`.
- `FogFactor = FogFactor * FogDensity`.
- Lerp base color toward FogColor by FogFactor.
- Output: **EmissiveColor** = lerped color (post-process materials output via EmissiveColor
  when domain is Post Process; or use the **PostProcessMaterial** output nodes).
- Also darken slightly: multiply final by `1 - FogFactor * 0.3` for contrast loss.

### 3.3 Exposed parameters (material instance)
Create `MI_PP_Fog` instance of `M_PP_Fog` so designers can tweak without touching the
master. Document all parameters in `Docs/art/weather-asset-parameters.md`.

### 3.4 Save
Save `M_PP_Fog` (and `MI_PP_Fog` if created).

---

## 4. Assign soft references

1. Open the level containing `AWorldGenerator`.
2. Select the `DayNightCycleSystem` component on `AWorldGenerator`.
3. In the **Weather|Content** category, assign:
   - `FogPostProcessMaterial` → `M_PP_Fog` (or `MI_PP_Fog`).
   - `RainNiagaraSystem` → `NS_Rain`.
   - `AshNiagaraSystem` → `NS_AshStorm`.
4. Save the level.

---

## 5. Verification checklist (in editor / PIE)

- [ ] PIE: weather starts as Overcast; fog visible at density 0.3.
- [ ] Console: `mdw.Weather.Set 3` (Fog) → fog density ramps to `FogDensityMax` over ~45s.
- [ ] Rain activates only in Rain state and deactivates indoors.
- [ ] Ash activates only in AshStorm state (night) and deactivates indoors.
- [ ] `SpawnRate` parameter visibly scales particle count.
- [ ] No frame hitches on first weather transition (asset load).
- [ ] `stat unit` / Unreal Insights: particle count ≤ 2000.

---

## 6. Notes on indoor detection

`UpdateEffects` scans voxels upward from the player. If your level has a ceiling block
above the player, rain/ash deactivate. Test in a cave to confirm.

---

## 7. Related

- `Docs/art/weather-asset-parameters.md` — full parameter tables.
- `Docs/architecture/adr-0016-weather-system.md` — architecture and contracts.
- `design/gdd/weather-system.md` — design intent.