# Camera System — GDD

| Field | Value |
|---|---|
| Status | Draft (2026-11-11) |
| System owner | systems-designer / gameplay-programmer / ue-umg-specialist |
| Related | `design/gdd/game-concept.md`, `design/gdd/weeping-angels.md`, `design/gdd/weather-system.md`, `design/gdd/audio-system.md`, `design/gdd/player-vitals.md`, `design/ui/ui-direction.md`, `design/art/art-direction.md`, `Docs/SYSTEMS_REFERENCE.md` |
| Source of truth for code | `UCameraComponent` on the player pawn (`BP_HorrorCharacter`), `APlayerViewModel` (`Source/MinecraftDeWish/Public/PlayerViewModel.h`), `UGameSettingsSubsystem` (FOV / CameraShake / MotionBlur), `UDayNightCycleSystem` (weather state, sky exposure) |
| Bidirectional note | `weeping-angels.md` §3.2 fixes the player observation cone at 110° (half-angle 55°) regardless of the camera FOV; `ui-direction.md` §3.3 fixes the menu camera drift; `weather-system.md` §3.4 drives rain-on-lens and fog |

---

## 1. Overview

The camera is the player's only window into the world and the instrument through
which observation — the game's core mechanic — is performed. It is a first-person
camera with a configurable field of view, a walking head bob, a first-person
viewmodel (arms + equipped tool), weather-driven lens effects (rain droplets,
fog), and a small, deliberately limited set of camera-shake triggers. It is
**never** shaken, pulsed, or destabilised by the Weeping Angels: the horror is
delivered by stillness, not by motion (ui-direction.md §3.7 — *"Stillness is
the tell"*).

The camera has two modes:

- **Gameplay camera** — attached to the player pawn, driven by mouse look, with
  head bob, viewmodel sway/bob, and weather lens effects.
- **Menu camera** — a hand-placed `CameraActor` in `Lvl_MainMenu` with a slow
  sinusoidal drift and no player input, no bob, no shake.

In one sentence: **the camera is a faithful, stable eye that the player trusts;
every effect it applies is either a walking artifact, a weather condition, or a
deliberate impact — never a threat cue.**

---

## 2. Player Fantasy

> *"I see what I look at. The world does not move unless I move it."*

The fantasy is **trust in the eye**. The player's gaze is the resource the whole
game is built on (game-concept.md pillar P1). If the camera lies — if it
shakes when an Angel is near, if it bobs unpredictably, if the viewmodel
obscures the world — the observation mechanic breaks, because the player can no
longer tell whether a statue moved or the screen moved. The camera must therefore
be the most honest system in the game: it reports the world exactly as it is, and
its only motions are ones the player caused (walking, turning) or ones the world
caused (rain, explosions).

Three beats the camera must deliver:

1. **The steady aim.** The player lines up a block, holds the crosshair on it,
   and mines. The view does not drift, does not bob while stationary, does not
   shake. They learn that stillness is a tool — and that the game will not
   punish them for holding still.
2. **The walk.** The player moves and the world gently rises and falls with
   their stride. The head bob is subtle enough to read as *their* body, not as
   a camera effect, and it stops the instant they stop. They learn that motion
   has a rhythm and that the rhythm is theirs.
3. **The rain.** The player steps out into a storm and droplets streak down the
   lens. The world is still perfectly readable; the rain is on the glass, not
   in the world. They learn that weather is a condition they observe, not a
   distortion they fight.

The camera must never make the player sick, must never hide an Angel behind a
viewmodel, and must never imply danger through motion. Danger is communicated
by audio and by stillness — never by the camera.

---

## 3. Detailed Rules

### 3.1 Gameplay camera — core

- A single `UCameraComponent` (`Camera`) is attached to the player pawn at eye
  height (~170 cm / 170 UU above the feet). It is the listener for the audio
  system (audio-system.md §3.7) and the origin of the observation ray for
  Angel detection (weeping-angels.md §3.2).
- Mouse look (Enhanced Input, `IA_Look`) rotates the camera yaw/pitch. Pitch is
  clamped to `[-89°, +89°]`. Mouse sensitivity is `Settings.MouseSensitivity`
  (0.2–3.0), applied as a multiplier on the raw input delta.
- The camera FOV is `Settings.FOV` (default 90°, range [70, 110]), applied live
  to `Camera->FieldOfView`. Changing it in the settings menu takes effect
  immediately, with no restart and no level reload.
- The camera does **not** `Tick` for its own sake beyond what the engine
  requires; head bob, sway and lens effects are driven by the pawn's movement
  events and timers, not by a per-frame camera `Tick`.

### 3.2 Head bob (walking)

Head bob is a small, rhythmic offset of the camera while the player is moving on
foot. It is a **walking artifact**, not a shake, and it is the only camera motion
the player does not directly cause with the mouse.

Rules:

1. Head bob is active only while the pawn's horizontal speed exceeds
   `BobSpeedThreshold` (default 10 cm/s). While stationary, bob decays to zero
   over `BobDecayTime` (default 0.2 s).
2. Bob has two components: a vertical component (`BobVertical`) and a lateral
   component (`BobLateral`), both sinusoidal in a shared phase.
3. The shared phase advances only while moving:
   `Phase += 2π × BobFrequency × dt`. When the player stops, the phase resets
   to 0 once the bob has fully decayed (so the next step starts on the same foot).
4. Amplitude scales with speed:
   `SpeedFactor = clamp(HorizontalSpeed / WalkSpeedNormal, 0, MaxBobSpeedFactor)`,
   where `WalkSpeedNormal` is the default walk speed (430 cm/s) and
   `MaxBobSpeedFactor` (default 1.5) caps the bob on sprint.
5. Head bob is **disabled entirely** when `bReduceMotion` is true or when
   `Settings.CameraShake == 0` and the motion-sickness mode is on (§3.8). It is
   also disabled while the player is in a menu, inventory, or dead.
6. Head bob never applies while airborne (jumping/falling); it resumes on
   landing after a short settle (0.1 s blend-in).
7. Head bob amplitude is **not** modulated by Angel proximity. The camera does
   not react to threats (§3.5).

### 3.3 Viewmodel (first-person arms + tool)

The viewmodel is `APlayerViewModel`: a skeletal-mesh arms actor
(`bOnlyOwnerSee`) attached to the camera, with a tool socket `weapon_r` and an
`AEquippedToolActor` whose mesh is swapped on equip (ADR-0006). Full rules:

- **Sway.** The viewmodel lags behind camera rotation. Each frame the camera's
  yaw/pitch delta is recorded; the viewmodel target offset is
  `-CameraRotationDelta × SwayFactor`, clamped to `[-SwayMax, +SwayMax]` (degrees),
  and blended toward the target at `SwayLerpRate` (per second). Sway decays to
  zero when the camera stops moving.
- **Bob.** The viewmodel bobs in sync with the head bob but with a smaller
  amplitude and a phase offset:
  `ViewModelBob = ViewModelBobAmplitude × sin(Phase + ViewModelBobPhaseOffset)`.
  Vertical only; lateral bob is handled by sway.
- **Equip.** When the active tool changes (via `UBaublesSystem::OnActiveToolChanged`),
  `APlayerViewModel::SyncEquippedToolFromActive` swaps the tool mesh on
  `EquippedToolActor` and plays the matching swing montage entry. A short equip
  blend (~0.15 s) hides the swap; the arms never fully disappear.
- **Unequip.** When no tool is active, `UnequipTool()` clears the tool mesh; the
  arms remain visible (empty hands). The swing montage is not played.
- **Swing.** `PlaySwingMontage(ToolType, PlayRate)` plays the per-tool-class
  montage on the arms' anim instance. Swing does not affect camera position.
- **Visibility.** Arms and tool are `bOnlyOwnerSee`; in multiplayer (future) other
  players never see this actor. The viewmodel casts no shadow and has no
  collision.
- **No occlusion of the world.** The viewmodel is rendered last (translucent-ish
  priority) and never blocks a trace. An Angel or block behind the arms is still
  observed / targeted normally.

### 3.4 Weather effects on the camera

Weather is driven by `UDayNightCycleSystem` (weather-system.md). The camera reacts
in two ways:

- **Fog.** Fog is a post-process / distance fog owned by the weather system
  (weather-system.md §3.4). The camera itself is unchanged; visibility is
  reduced by the world's fog density. No camera transform is modified.
- **Rain on lens.** When `CurrentWeather == Rain` and the player is outdoors
  (sky exposure > 0.3, per weather-system.md §5), droplets spawn on a
  transparent overlay in front of the camera:
  - Spawn rate scales with `RainParticleRate` (subset capped at
    `LensDropletMaxCount`, default 200).
  - Each droplet spawns at a random screen position, slides downward over
    `DropletLifetime` (0.5–1.5 s), and fades out.
  - Droplets are cosmetic only: they do not affect visibility, aim, or traces.
  - When the player moves indoors, droplets stop spawning and existing ones fade
    over 0.5 s.
  - When weather transitions away from Rain, droplets fade over
    `WeatherTransitionDuration`.
- **Ash storm.** No lens effect; the global low-pass and particle veil are owned
  by the audio/weather systems. The camera adds a faint grey vignette (opacity
  ≤ 0.15) while in an Ash Storm, to reinforce the choking feel without
  obscuring the world.
- **Clear / Overcast.** No camera effect.

### 3.5 Angel interaction — no camera shake

This is a binding rule, repeated here for emphasis:

- **Angels never trigger camera shake.** No proximity rumble, no "something is
  close" wobble, no heartbeat-driven camera motion. The `CameraShake` setting
  (§3.6) does not apply to Angels.
- **Angels never modulate head bob amplitude.** Walking feels the same whether an
  Angel is 5 m or 500 m away.
- **The only Angel-related camera/HUD change is stillness.** Per ui-direction.md
  §3.7, when an Angel is within detection range the HUD breathing animation
  freezes (scale stops at 1.000). The camera itself does not freeze — the player
  can still look and walk — but the *UI* goes still, which is the tell. This is
  a UI rule, not a camera rule, and is listed here only to state that the camera
  is not involved.
- **Rationale (pillar P1).** If the camera reacted to Angel proximity, the
  player would learn to read the camera instead of the world, and observation
  would become a camera-watching game. The Angel's tell must be its pose and its
  stone grind, perceived through the stable eye.

### 3.6 Camera shake (general, non-Angel)

Camera shake exists for physical impacts only. It is scaled by
`Settings.CameraShake` (0.0–1.0; 0 disables all shake).

Triggers:

| Trigger | Base amplitude | Duration | Falloff |
|---|---|---|---|
| Creeper detonation | 1.0 | 0.6 s | `1 / (1 + distance/500 cm)` |
| Fall damage landing (≥ 3 blocks drop) | 0.4 | 0.25 s | none (local) |
| Large block break (future: bedrock-adjacent) | 0.15 | 0.1 s | none |

Rules:

- Shake is applied as a rotational + positional offset that decays exponentially
  (`Shake(t) = Base × (1 - t/Duration)²`).
- Shake never stacks: a new shake replaces the current one if its amplitude is
  greater; otherwise it is ignored.
- Shake is disabled when `bReduceMotion` is true, regardless of the
  `CameraShake` setting.
- Shake never affects the viewmodel's sway target (the viewmodel keeps its own
  lag), only the camera transform.

### 3.7 Menu camera

The main menu (`Lvl_MainMenu`) uses a hand-placed `CameraActor` (or a camera
component on a static actor), not the player pawn. Per ui-direction.md §3.3:

- Locked transform: looks at the gatehouse (concrete wall, black doorway, one
  sodium bulb). No player input, no mouse look.
- Slow drift: `0.3°` yaw/pitch sinusoidal at `0.2 Hz` (5 s cycle). No head bob,
  no sway, no shake.
- FOV = 90° (fixed; does not read the settings FOV, since the menu is pre-world).
- The bulb dims 8 % for 300 ms every 20–40 s (irregular), with a matching
  falloff change — this is a lighting effect, not a camera effect, but it is
  driven from the same scene.
- The menu camera is the only camera in the game that is not attached to a pawn.

### 3.8 Accessibility (motion sickness)

A "Reduce motion" option is added to the Accessibility settings page (in addition
to the existing `CameraShake` slider and `bMotionBlur` toggle):

- `bReduceMotion` (default false). When true:
  - Head bob disabled (§3.2 rule 5).
  - Viewmodel sway disabled (sway offset forced to 0; viewmodel stays locked to
    camera).
  - Viewmodel bob disabled.
  - Rain-on-lens droplets disabled (rain still visible in the world).
  - Camera shake forced to 0 (overrides `CameraShake`).
  - Motion blur forced off.
  - Menu camera drift reduced to 0 (static frame).
- The FOV slider (70–110) already exists and is the primary motion-sickness lever
  for players who feel sick at wide FOV; it is listed here for completeness.
- All of the above are live-applied on toggle, with no restart.

### 3.9 Observation and the camera (cross-reference)

The Angel observation test (weeping-angels.md §3.2, §6.1) uses the **player
camera's** forward vector and position as the observer, but with a **fixed**
cone (`FOVHalfAngle = 55°`, i.e. 110° total) and a fixed range (60 blocks),
independent of `Settings.FOV`. Consequences:

- A player who sets FOV to 70° still observes Angels in a 110° cone — the
  observation rule is about what the player *could* see, not what the lens
  shows. This is deliberate (weeping-angels.md §6.1 rationale) and is not a
  camera bug.
- The camera FOV never widens or narrows the observation cone. The two are
  intentionally decoupled.

---

## 4. Formulas

### 4.1 FOV application

```
EffectiveFOV = clamp(Settings.FOV, FOVMin, FOVMax)   // degrees, default 90
Camera->FieldOfView = EffectiveFOV
```

Applied on settings load and on `OnSettingsChanged`.

### 4.2 Head bob

```
HorizontalSpeed = length(PawnVelocity.XY)            // cm/s
Moving          = HorizontalSpeed > BobSpeedThreshold // 10 cm/s

if Moving:
    SpeedFactor = clamp(HorizontalSpeed / WalkSpeedNormal, 0, MaxBobSpeedFactor)
    Phase      += 2π × BobFrequency × dt
    BobVertical = BobAmplitudeV × SpeedFactor × sin(Phase)
    BobLateral  = BobAmplitudeL × SpeedFactor × cos(Phase)
else:
    BobVertical = lerp(BobVertical, 0, BobDecayRate × dt)
    BobLateral  = lerp(BobLateral,  0, BobDecayRate × dt)
    if |BobVertical| < 0.001 and |BobLateral| < 0.001:
        Phase = 0

CameraOffset = (BobLateral, 0, BobVertical)   // in camera space (X fwd, Y right, Z up)
```

Airborne: `Moving` forced false and phase frozen; bob blends in 0.1 s after
landing.

### 4.3 Viewmodel sway

```
CameraRotationDelta = CurrentCameraRotation - PreviousCameraRotation   // deg, per frame
SwayTargetYaw   = clamp(-CameraRotationDelta.Yaw   × SwayFactor, -SwayMax, +SwayMax)
SwayTargetPitch = clamp(-CameraRotationDelta.Pitch × SwayFactor, -SwayMax, +SwayMax)

SwayOffsetYaw   = lerp(SwayOffsetYaw,   SwayTargetYaw,   SwayLerpRate × dt)
SwayOffsetPitch = lerp(SwayOffsetPitch, SwayTargetPitch, SwayLerpRate × dt)

ViewModelRotationOffset = (SwayOffsetPitch, 0, SwayOffsetYaw)   // applied to arms mesh
```

When `bReduceMotion`: `SwayOffsetYaw = SwayOffsetPitch = 0`.

### 4.4 Viewmodel bob

```
ViewModelBobV = ViewModelBobAmplitude × sin(Phase + ViewModelBobPhaseOffset)
```

Uses the same `Phase` as head bob (§4.2). Applied as a vertical offset on the
arms mesh root (or on the tool socket), in addition to sway. Zero when
`bReduceMotion`.

### 4.5 Rain on lens

```
if Weather == Rain and SkyExposure > 0.3:
    SpawnRate = LensDropletBaseRate × (RainParticleRate / RainParticleRateRef)
    ActiveDroplets = min(ActiveDroplets + SpawnRate × dt, LensDropletMaxCount)
else:
    ActiveDroplets = max(0, ActiveDroplets - LensDropletFadeRate × dt)

Per droplet:
    Age += dt
    Position.Y += DropletSlideSpeed × dt          // slides down the lens
    Opacity    = 1 - (Age / DropletLifetime)     // fades over lifetime
    if Age >= DropletLifetime: remove
```

### 4.6 Camera shake

```
ShakeIntensity(t) = BaseAmplitude × (1 - t/Duration)^2 × Settings.CameraShake
                    × (bReduceMotion ? 0 : 1)
```

Applied as a decaying rotational + positional offset. A new shake replaces the
current one only if its `BaseAmplitude` exceeds the current residual amplitude.

### 4.7 Ash storm vignette

```
AshVignetteOpacity = (Weather == AshStorm) ? AshVignetteMax : 0   // max 0.15
```

Lerped over `WeatherTransitionDuration`.

---

## 5. Edge Cases

| Case | Resolution |
|---|---|
| FOV changed while an Angel is in view | Observation cone unchanged (fixed 110°). Only the lens changes. No gameplay effect. |
| Head bob while sprinting | Amplitude capped by `MaxBobSpeedFactor` (1.5). No runaway bob. |
| Head bob while jumping / falling | Disabled; phase frozen; blends in 0.1 s after landing. |
| Head bob while airborne then landing hard | Landing triggers a small camera shake (fall damage rule) independent of bob blend-in. |
| Player stationary but camera moving (mouse look only) | No head bob; sway applies to viewmodel. |
| Viewmodel with no tool equipped | Arms visible, tool mesh cleared. Sway/bob still apply. Swing montage not played. |
| Tool swapped mid-swing | Current montage stops; new tool's mesh swaps with equip blend; no crash. |
| Rain indoors | Droplets stop spawning; existing droplets fade over 0.5 s. |
| Weather transition during rain | Droplets fade over `WeatherTransitionDuration`. |
| `bReduceMotion` toggled mid-game | All motion effects disable on next frame; no restart. Shake cancels immediately. |
| `CameraShake = 0` | All shake disabled; head bob still active unless `bReduceMotion`. |
| Camera shake during inventory (Tab) | Shake continues (world is live). Pause (Esc) freezes the world and cancels active shake. |
| Menu camera with `bReduceMotion` | Drift disabled; static frame. |
| Missing arms mesh asset | `EnsureArmsMeshLoaded` no-ops; arms invisible but camera and gameplay unaffected. Logged once. |
| Missing tool mesh on equip | `EquipToolMesh` receives null; tool invisible but slot still functional. Logged. |
| Player dead | Head bob, sway, lens effects, shake all disabled. Camera freezes at death pose (ragdoll handled by vitals system). |
| Multiplayer (future) | Viewmodel is `bOnlyOwnerSee`; other players never see it. Camera shake is local only. |
| Ultrawide (21:9) | FOV applies normally; UE handles aspect. Head bob amplitudes are in cm, not pixels, so no scaling needed. |
| Very low framerate | Phase increment uses `dt`; bob frequency stays correct but may stutter. No crash. |

---

## 6. Dependencies

| System | Why | Status |
|---|---|---|
| `UCameraComponent` (player pawn) | Camera transform, FOV, listener origin | Implemented (basic, in `BP_HorrorCharacter`) |
| `APlayerViewModel` | Arms + tool mesh, sway/bob/equip | Implemented (basic) |
| `UBaublesSystem` | Active tool change → viewmodel equip/unequip | Implemented |
| `UGameSettingsSubsystem` | FOV, CameraShake, MotionBlur, MouseSensitivity, bReduceMotion (new) | Implemented (FOV/CameraShake/MotionBlur exist; `bReduceMotion` new) |
| `UDayNightCycleSystem` | Weather state, sky exposure for rain-on-lens | Implemented |
| `UPlayerVitalSystem` | Death → disable camera effects | Implemented |
| Weeping Angels system | Observation uses camera forward/position; no camera shake from Angels | Design phase (`design/gdd/angels.md`) |
| Audio system | Camera is the audio listener; shake is visual only | Design phase (`design/gdd/audio-system.md`) |
| UI system (LAMPLIGHTER) | Settings page (FOV slider, motion-sickness toggle, camera-shake slider), menu camera | Design phase (`design/ui/ui-direction.md`) |
| `Lvl_MainMenu` | Hand-placed menu camera | Exists (needs polish) |

Bidirectional note: `weeping-angels.md` §6 lists "Player camera — FOV, forward vector, eye position" as a dependency. `ui-direction.md` §3.3 lists the menu camera drift. `audio-system.md` §3.7 lists the listener orientation. `weather-system.md` §5 lists indoor detection (sky exposure) used by rain-on-lens.

---

## 7. Tuning Knobs

| Knob | Default | Range | Effect |
|---|---|---|---|
| `FOV` (settings) | 90° | 70–110 | Field of view; primary motion-sickness lever |
| `CameraShake` (settings) | 1.0 | 0.0–1.0 | Global shake multiplier; 0 disables all shake |
| `bMotionBlur` (settings) | false | On/Off | Motion blur post-process |
| `bReduceMotion` (settings, new) | false | On/Off | Disables head bob, sway, viewmodel bob, lens droplets, shake, menu drift |
| `MouseSensitivity` (settings) | 1.0 | 0.2–3.0 | Mouse look multiplier |
| `BobAmplitudeV` | 3.0 cm | 0.0–8.0 | Vertical head bob amplitude |
| `BobAmplitudeL` | 1.5 cm | 0.0–4.0 | Lateral head bob amplitude |
| `BobFrequency` | 1.8 Hz | 1.0–3.0 | Head bob cycles per second while walking |
| `BobSpeedThreshold` | 10 cm/s | 5–30 | Minimum speed for bob to activate |
| `WalkSpeedNormal` | 430 cm/s | — | Reference walk speed for amplitude scaling |
| `MaxBobSpeedFactor` | 1.5 | 1.0–2.0 | Bob amplitude cap on sprint |
| `BobDecayRate` | 5.0 /s | 2.0–10.0 | Bob decay speed when stopping |
| `BobDecayTime` | 0.2 s | 0.1–0.5 | Time for bob to fully settle |
| `BobLandBlendIn` | 0.1 s | 0.05–0.3 | Bob blend-in after landing |
| `SwayFactor` | 0.05 | 0.0–0.2 | Viewmodel lag per degree of camera rotation |
| `SwayMax` | 3.0° | 1.0–8.0 | Maximum viewmodel sway angle |
| `SwayLerpRate` | 8.0 /s | 4.0–16.0 | Sway blend speed toward target |
| `ViewModelBobAmplitude` | 1.0 cm | 0.0–3.0 | Viewmodel vertical bob amplitude |
| `ViewModelBobPhaseOffset` | 0.3 rad | 0.0–π | Phase offset between head bob and viewmodel bob |
| `EquipBlendTime` | 0.15 s | 0.05–0.3 | Tool-swap blend duration |
| `LensDropletMaxCount` | 200 | 50–500 | Max rain droplets on lens |
| `LensDropletBaseRate` | 30 /s | 10–80 | Base droplet spawn rate in rain |
| `DropletLifetime` | 1.0 s | 0.5–1.5 | Droplet slide duration |
| `DropletSlideSpeed` | 400 px/s | 200–800 | Droplet downward speed on lens |
| `LensDropletFadeRate` | 100 /s | 50–200 | Droplet fade when leaving rain |
| `AshVignetteMax` | 0.15 | 0.0–0.3 | Ash storm grey vignette opacity |
| `CreeperShakeBase` | 1.0 | 0.5–2.0 | Creeper detonation shake amplitude |
| `CreeperShakeDuration` | 0.6 s | 0.3–1.0 | Creeper shake duration |
| `FallShakeBase` | 0.4 | 0.2–0.8 | Fall-damage landing shake amplitude |
| `FallShakeDuration` | 0.25 s | 0.1–0.5 | Fall shake duration |
| `MenuDriftAngle` | 0.3° | 0.0–1.0 | Menu camera yaw/pitch drift |
| `MenuDriftFrequency` | 0.2 Hz | 0.1–0.5 | Menu camera drift cycle rate |
| `MenuFOV` | 90° | — | Fixed menu camera FOV |
| `EyeHeight` | 170 cm | 150–190 | Camera height above feet |
| `PitchClamp` | ±89° | — | Mouse-look pitch limit |

All knobs are `EditAnywhere` / data-driven. No knob value may be hardcoded in C++.
`bReduceMotion` and `CameraShake` are persisted via `UGameSettingsSubsystem` (custom
ini). The bob/sway/lens knobs live on the player pawn or a `UCameraSettings`
Data Asset.

---

## 8. Acceptance Criteria

1. **FOV applies live.** Changing FOV in settings (70–110) updates the camera
   immediately, with no restart, and persists across sessions.
2. **Head bob activates only while moving.** Standing still produces zero bob
   within `BobDecayTime`. Walking produces a vertical + lateral oscillation at
   `BobFrequency`. Sprinting increases amplitude up to `MaxBobSpeedFactor`.
3. **Head bob stops while airborne** and blends in within `BobLandBlendIn` after
   landing.
4. **Viewmodel sways on camera rotation** and decays to zero when the camera
   stops. Sway is clamped to `SwayMax`.
5. **Viewmodel bobs in sync** with head bob, with `ViewModelBobAmplitude` and
   `ViewModelBobPhaseOffset`.
6. **Tool equip/unequip** swaps the tool mesh on `EquippedToolActor` with an
   `EquipBlendTime` blend and plays the correct swing montage. Empty hands show
   arms only.
7. **Rain on lens** appears only in Rain weather and only outdoors (sky exposure
   > 0.3). Droplets slide and fade. They disappear indoors and when weather
   changes.
8. **Fog reduces visibility** via the weather system post-process; the camera
   transform is unchanged.
9. **Ash storm** adds a grey vignette (≤ 0.15 opacity) and no lens droplets.
10. **No camera shake from Angels.** An Angel at 1 m, 5 m, or 50 m produces zero
    camera shake, zero head-bob modulation, and zero FOV change. Verified by
    automation test.
11. **Camera shake works for non-Angel impacts.** Creeper detonation and fall
    damage produce a decaying shake scaled by `CameraShake`. `CameraShake = 0`
    disables all shake.
12. **Reduce motion works.** `bReduceMotion = true` disables head bob, sway,
    viewmodel bob, lens droplets, shake, and menu drift, applied live.
13. **Menu camera** is a hand-placed `CameraActor` with 0.3° drift at 0.2 Hz,
    no head bob, no shake, no player input, FOV 90°. Drift disables under
    `bReduceMotion`.
14. **Observation cone is fixed at 110°** regardless of FOV. Changing FOV does
    not change Angel observation behaviour.
15. **Viewmodel never occludes traces.** An Angel or block behind the arms is
    still observed / targeted.
16. **No `Tick` on the camera** for bob/sway/lens; all driven by movement events
    and timers.
17. **Performance.** Camera update (bob + sway + lens) costs < 0.2 ms per frame
    at 60 FPS (`stat game`).
18. **Automation tests** exist for: FOV clamp, head-bob activation/decay, airborne
    gating, sway clamp/decay, shake replacement rule, rain-on-lens indoor gating,
    reduce-motion disable, and the no-shake-from-Angels invariant.