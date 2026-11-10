# UI Direction — LAMPLIGHTER

| Field | Value |
|---|---|
| Status | **Approved direction** (2026-11-10) — implementation not started |
| Owner | ux-designer / ui-programmer, arbitrated by creative-director |
| Product name | **LAMPLIGHTER** (decided 2026-11-10) |
| Code name | The module, `.uproject`, repo and build targets stay `MinecraftDeWish` until a dedicated rename task |
| Mockup | `Saved/UIMockups/lamplighter-ui.html` (throwaway, gitignored) |
| Related | `design/gdd/game-concept.md`, `design/gdd/items.md`, `design/gdd/light-and-power.md`, `Docs/SYSTEMS_REFERENCE.md`, `.opencode/rules/ue-ui.md` |

---

## 1. Overview

LAMPLIGHTER is a first-person voxel survival horror game. Its core mechanic is **observation**:
the Weeping Angels move only when unobserved and in darkness. The UI therefore has one job —
**never steal the gaze the player needs for the world**, and make every look away cost something.

The interface keeps **Minecraft's information architecture** (9-slot hotbar bottom-centre,
36-slot inventory, 2×2 and 3×3 crafting, furnace, chest, hover tooltips, death screen) because
that layout is free muscle memory. Everything else — material, typography, sound, motion,
light — is replaced by the game's own language: **enamel, sodium light, CRT, blueprint, bone
on concrete**. The player should recognise the grammar instantly and feel the world immediately.

The main menu boots **without generating a world**, on a small hand-built gatehouse scene
(`Lvl_MainMenu`), with the menu list drawn in UMG over a locked camera. This is the
"Long Watch" hybrid: the mood of a diegetic menu, the robustness of a conventional one.

---

## 2. Player Fantasy

> *"I do not get stronger. I get less afraid, because I stop having to look."*

The UI must feel like equipment that a survivor maintains, not like a game that talks to a
player. It is **quiet, small, and slightly alive**. At rest it breathes. In danger it stops.
It never celebrates, never jokes, never explains twice.

The horror curve **descends**: the interface gets calmer and more legible as the fortress
grows. A phase-3 player reads their power grid in a held overlay; a phase-1 player reads
nothing at all.

---

## 3. Detailed Rules

### 3.1 Design principles (binding)

1. **Minecraft's grammar, LAMPLIGHTER's material.** Do not redesign the hotbar, the inventory
   grid or the tooltip. Redesign their surface, type, motion and sound.
2. **Nothing persistent that the player can search for.** Persistent = hotbar, crosshair.
   Everything else is contextual or held.
3. **Critical values never flicker.** Flicker is allowed on backdrop, flavour text, camera
   feeds. Actionable text stays ≥ 4.5:1 and static.
4. **No text bare over the world.** Every text surface sits on a scrim ≥ 70 % `#0B0E11` with a
   1 px dark inner outline.
5. **The UI never fakes a system failure.** A fake "low fuel" is hostile design, not horror.
6. **No widget touches a gameplay object.** Widgets read value types from view models and send
   intent through delegates (see `.opencode/rules/ue-ui.md`).

### 3.2 Screen flow

```
Boot → Logo (2 s, skippable) → TITLE
  ├─ Continue ────────────────────────┐
  ├─ New World → [Name, Seed] → Generating → Loading → Gameplay
  ├─ Settings (also reachable from Pause)
  └─ Quit
  → LOADING → GAMEPLAY ⇄ PAUSE (Esc) ⇄ Settings / Controls / Accessibility
                     ⇄ INVENTORY (Tab, real time)
```

**Required for the vertical slice:** Title, Continue, New World (name + seed only), Settings
(Video / Game / Accessibility / Controls), Loading, Pause.

**Later:** save manager with thumbnails, world rename/delete, credits, subtitles page, held
power/camera overlay, colonist roster, death screen rework.

**Difficulty:** no Easy/Normal/Hard. A single **Dread** setting (Ambient / Standard / Merciless)
that only touches light decay rate, Angel responsiveness and jump-scare intensity. It never
touches player power — that would contradict the fantasy.

### 3.3 Title screen — "The Long Watch, drawn in UMG"

- **Scene:** `Lvl_MainMenu`, a hand-built gatehouse: concrete wall, black doorway, one sodium
  bulb. No landscape, no `AWorldGenerator`, no streaming. Lumen only.
- **Camera:** locked, 0.3° drift at 0.2 Hz (5 s cycle).
- **Bulb:** dims 8 % for 300 ms every 20–40 s (irregular), with a matching falloff change.
- **Wordmark:** `LAMPLIGHTER`, Oswald 600, 112 px, tracking +75, bone `#D9D2C0`, stamped-steel
  treatment (1 px top highlight, 1 px bottom shadow, one chipped corner), 4 s breathing pulse
  at 3 %.
- **Menu:** single left-aligned column at x = 150 px, width 780 px, starting y = 250 px.
  Oswald 400, 31 px, tracking +175, uppercase, `rgba(227,220,203,.62)`.
  Selected/hover: `#E3DCCB`, tracking +200, and a 3 px sodium bar that grows to 26 px height
  in 160 ms. **No box, no arrow, no glow.**
- **Sound:** bulb buzz (loop), mechanical latch on confirm. **No hover sound.**
- **Footer:** build number left, world summary right (`1 monde · 0 colon · 4 lampes`), mono 12 px, ash.

### 3.4 Panels (New World, Settings, Pause)

- Surface `#171C21` over a 72 % `#0B0E11` scrim with 2 px backdrop blur.
- 3 px sodium rail on the leading edge.
- 1 px border `rgba(227,220,203,.14)`; inner top highlight 1 px at 5 %.
- Padding 54/56 px. Kicker (mono 11 px, tracking +260, uppercase, ash) above the title.
- Title: Oswald 500, 26 px, tracking +200, uppercase.
- Entrance: 180 ms `cubic-bezier(.2,0,0,1)`, translate Y +12 px → 0, opacity 0 → 1.
- **Form fields** are recessed: `#0B0E11` at 85 %, 56 px tall, 1 px border, 18 px IBM Plex Sans.
  Seeds and numeric values use IBM Plex Mono in teal `#4FA6A0`.
- **Buttons:** Oswald 17 px, tracking +180, uppercase, 16/34 px padding. Primary = 10 % sodium
  fill + 55 % sodium border. Hover = 14 % fill + full sodium border, 140 ms.

### 3.5 Pause and inventory — the two-tier rule

The decisive argument is not comfort, it is the predator: **if the world freezes, pausing is an
escape from an Angel**, and the central threat becomes a keypress.

- **Esc → the world stops.** `SetPause`, no time dilation. This is the accessibility escape
  hatch and the only route to settings.
- **Tab → inventory and crafting run in real time.** The world is dimmed to 55 % and
  desaturated 25 % — **never blacked out**, the player must still see the room. Audio is ducked
  −12 dB, never muted. Panel in over 180 ms.
- **Threat tell:** an Angel within 15 m adds a 6 px warm vignette pulsing at 0.8 Hz around the
  panel edge. That is the entire warning. No icon, no text, no number.
- **Assist toggle:** "Pause inventory", **default off**, exposed in Accessibility and repeated in
  the first pause menu. One bool; makes the game playable for players who cannot multitask
  under threat.

### 3.6 HUD

| Element | Rule |
|---|---|
| Crosshair | 4 px dot, 1 px dark outline. Fades to 0 after 3 s without an interactable, 120 ms ease-in. |
| Hotbar | 9 slots, 48 px, 2 px gap, bottom-centre, 24 px from the edge. Empty slots at opacity 0 (already implemented). Selection = 2 px sodium ring, no glow. |
| Health | **No bar, no hearts.** A soft red vignette scaling 0 → 40 % opacity with damage, invisible at full health. |
| Light / fuel | Only while a light source is equipped: a 2 px arc around the crosshair. Amber below 20 %. |
| Block tooltip | 120 ms dwell, 150 ms fade, 8 px above the crosshair, max 260 px wide, mono 12 px. |
| Angel proximity | **Never an icon, arrow or number.** Audio (stone drag, breath) + 15 % desaturation within 10 m, 0.6 s ease-in. |
| Power grid / camera coverage | Phase 3 only, and only in a **held** overlay — a deliberate look away, never ambient. |
| Subtitles | Bottom-centre, 32 px above the hotbar, 2 lines max, speaker label in sodium. |

Removed from Minecraft: hearts, hunger, XP bar, bed, multiplayer.

### 3.7 Horror without hostility

1. **Stillness is the tell.** At rest the HUD breathes (scale 1.000 → 1.008, 4 s, ease-in-out).
   In danger it freezes completely. Calm moves; wrongness is motionless.
2. **Flicker budget.** Opacity 0.92–1.00 only, 0.5–2 Hz, on flavour text and backdrops only.
   Critical values never flicker.
3. **Static is diegetic-only** (camera feeds, menu backdrop, loading) and **sound carries the
   horror, not the pixels**: dry cloth and stone clicks, no "bloop", no hover sound.

**The line not to cross:** never degrade the legibility of an actionable element, never move a
hit target, never fake a system failure the player might act on, never make the player wait for
text to finish glitching. Unsettling ≠ unreadable.

### 3.8 Copy

Second person, present tense. Personality lives in the **world voice** (notes, signage, radio),
never in the **control voice** (buttons, errors, settings). One flourish per screen.

| Don't | Do |
|---|---|
| "Continue Investigation" | "Continue" |
| "The Weeping Angel is nearby!" | "Something moved." |
| "You died!" | "Contact." + one line of consequence |
| "Nothing to see here!" | "No cameras installed." |
| "Awesome! You crafted a lamp!" | "Lamp installed. 3 more to cover the ward." |
| "Loading… have fun!" | "Restoring power to Sector 3." |
| "A dozen blocks away" | "12 blocks" |
| Jokes in menus | Jokes only in notes and radio |
| "We recommend…" | "Suggested:" |
| Third-person systems voice | Second person, present tense |

---

## 4. Formulas

**UI scale.** Authored at 1920×1080. At runtime:

```
S       = min(ViewportW / 1920, ViewportH / 1080)
UIScale = S × UserTextScale        // UserTextScale ∈ {0.85, 1.00, 1.15, 1.30}
```

**Safe zone.** `Margin = 0.05 × min(ViewportW, ViewportH)`, clamped to [24, 64] px. The hotbar
and subtitles respect the safe zone, not the raw edge.

**Text sizes.** Authored px × UIScale. Floor 14 px at 1.00. Display text never below 28 px on
the title screen. A line of UI text never exceeds 90 characters at 1.00.

**Contrast.** Every text/background pair ≥ 4.5:1. Scrim opacity on world background ≥ 0.70 of
`#0B0E11`. Danger red `#C8462F` is restricted to ≥ 18 px text and icons.

**Damage vignette.** `Opacity = clamp(1 − Health/MaxHealth, 0, 1) × 0.40`.

**Fuel arc.** `ArcDegrees = 360 × Fuel/MaxFuel`. Colour lerps to `#E8A33D` below 20 %.

**Hotbar.** `SlotSize = 48 × S`, `Gap = 2 × S`, `BottomOffset = 24 × S` inside the safe zone.

**Threat vignette.** `PulseHz = 0.8`, `Thickness = 6 × S` px, active while
`min(AngelDistance) < 15 m`.

---

## 5. Edge Cases

| Case | Resolution |
|---|---|
| 21:9 ultrawide | Layout is left-anchored, so the painted column stays at 150 px; the scene camera widens. HUD stays centred on the viewport, not the safe zone. |
| 4:3 / 16:10 | S clamps to height; the wordmark shrinks to 88 px minimum. Never wrap the wordmark. |
| Text scale 130 % | Panels grow vertically, rows never clip; settings scrolls with a 3 px sodium scrollbar. Verified on one screen before mass-producing widgets. |
| Gamepad only | Every screen reachable with D-pad/stick + A/B. Focus rect = 3 px sodium outline, 2 px offset, never animated. |
| No save present | "Continue" is absent, not disabled — a disabled button is a broken promise. |
| Corrupted save | Continue → panel: "The record is damaged." with *Delete* / *Back*. Never silently overwrite. |
| Quit during generation | Generation is cancellable; the menu returns and the partial save is discarded. |
| Death while inventory is open | Inventory closes, death screen takes input immediately, no overlap. |
| Alt-tab / focus loss while paused | Remains paused; on return, focus is restored to the previously selected item. |
| Controller disconnected | Auto-pause (this is the one place where auto-pause is mandatory), panel: "Controller lost." |
| Resolution change | Re-layout immediately; never require a restart. |
| Missing font asset | Fall back to `Roboto` (ships with the engine); log once at `Warning`. |

---

## 6. Dependencies

**Systems**
- `UWorldDescriptorSubsystem` (new, GameInstanceSubsystem) — carries `{Name, Seed, Dread}` across level travel.
- `UGameSettingsSubsystem` (new, GameInstanceSubsystem) — **single writer** of `[MinecraftDeWish.GameSettings]`.
- `UUIRouterSubsystem` (new, WorldSubsystem) — owns the CommonUI stacks and back-stack invariants.
- `AWorldGenerator` — becomes a **consumer** of settings; it currently writes `GGameUserSettingsIni` directly, which conflicts with `UGameUserSettings::SaveSettings`.
- `UQuickSlotsInventorySystem`, `AC_Inventory`, `AC_DestroySystem` — existing, unchanged.
- Save system — `Saved/SaveGames/World_<seed>`; world name and seed live in the save, not in ini.

**Engine / plugins**
- `CommonUI` (engine plugin, production-grade in 5.8, hard-depends on `EnhancedInput`).
- Note: in 5.8 the input layer is the separate `CommonInput` module; `CommonUIInputData` moved to `/Script/CommonInput.CommonUIInputData`.
- `ModelViewViewModel` is still **Beta** in 5.8 — not adopted. Delegates + `NativeConstruct`/`NativeDestruct` first; MVVM only for the inventory grid and recipe lists, later.
- Loading screen: `IMoviePlayer` + `PreLoadMap`. `CommonLoadingScreen` ships with Lyra, not the engine.

**Assets to create**
- Fonts: Oswald, IBM Plex Sans, IBM Plex Mono (SIL OFL 1.1, Latin Extended, French accents covered).
- `Lvl_MainMenu`, `BP_MenuGameMode`, `AMenuPlayerController`, `WBP_*` set, one shared HUD material instance.

---

## 7. Tuning Knobs

| Knob | Default | Range | Storage |
|---|---|---|---|
| Render distance | 8 | 2–64 | custom ini |
| Wind strength | 1.00 | 0.00–2.00 | custom ini |
| Leaf particles | On | On/Off | custom ini |
| UI text scale | 1.00 | 0.85 / 1.00 / 1.15 / 1.30 | custom ini |
| FOV | 90 | 70–110 | custom ini |
| Mouse sensitivity | 1.00 | 0.20–3.00 | custom ini (Enhanced Input user settings, verify) |
| Subtitles | On | On/Off | custom ini |
| Subtitle size | 1.00 | 0.85–1.30 | custom ini |
| Colourblind critical indicators | Off | Off/Deuteranopia/Protanopia/Tritanopia | custom ini |
| Pause inventory (assist) | Off | On/Off | custom ini |
| Camera shake | 1.00 | 0.00–1.00 | custom ini |
| Motion blur | Off | On/Off | custom ini |
| Dread preset | Standard | Ambient/Standard/Merciless | SaveGame |
| Bulb flicker in menus | On | On/Off (photosensitivity) | custom ini |
| HUD breathing | On | On/Off | custom ini |
| UI scale (resolution-derived) | auto | auto/100/125/150 | `UGameUserSettings` |

---

## 8. Acceptance Criteria

1. Launching the packaged game opens the title screen **without generating or streaming a
   world**; no `AWorldGenerator` exists in `Lvl_MainMenu` (verifiable in the log).
2. "New World" with a name and a seed reaches playable gameplay in one click, and the generated
   world matches the seed shown in the menu.
3. "Continue" is absent when no save exists, and restores the exact world (name, seed, player
   position, inventory) when one does.
4. Every setting in §7 applies **live** (no restart) and survives a full restart.
5. Only one code path writes `[MinecraftDeWish.GameSettings]`; an automation test asserts that
   `AWorldGenerator` never flushes the ini.
6. Esc pauses the world; Tab opens the inventory without pausing; the assist toggle inverts that
   for the inventory and is honoured after restart.
7. Full HUD costs **< 1.5 ms** Slate render and **< 40 draw calls** at 1080p (`stat slate`,
   `stat ui`), measured on a GTX 1060-class GPU.
8. No widget ticks: `stat slate` shows no per-frame UMG work beyond the invalidation panel.
9. Every screen is fully operable with keyboard only and with gamepad only, with a visible focus
   rect at all times.
10. All player-facing text is `FText` from a string table; no hardcoded string in a widget.
11. Text at 130 % scale produces no clipping on any vertical-slice screen (screenshot checklist).
12. The title screen reaches 60 fps on the hardware floor with Lumen enabled.

---

## 9. Naming

New widgets use `WBP_<Area>_<Purpose>`. The three existing `WB_*` assets are **not** renamed
now: renaming an asset that a working Blueprint HUD depends on risks a silent break, for a
cosmetic gain. Each is renamed **when its screen is rebuilt** (`WB_HUD` → `WBP_HUD_Root` in
phase 5). No new `WB_` asset may be created in the meantime.

| Widget | Parent | C++ backing | Phase |
|---|---|---|---|
| `WBP_MainMenu_Root` | `UCommonActivatableWidget` | `UMainMenuWidget` | 1 |
| `WBP_MainMenu_NewWorld` | `UCommonActivatableWidget` | `UMainMenuWidget` | 1 |
| `WBP_MainMenu_WorldSelect` | `UCommonActivatableWidget` | `UWorldSelectWidget` | 4 |
| `WBP_Settings_Root` + `_PageGraphics/_PageGameplay/_PageAccessibility/_PageControls` | `UCommonActivatableWidget` | `USettingsWidget` | 2 |
| `WBP_Pause_Root` | `UCommonActivatableWidget` | `UPauseWidget` | 3 |
| `WBP_HUD_Root` | `UUserWidget` (HUD layer) | none | 5 |
| `WBP_HUD_QuickSlots` | `UUserWidget` | existing | 5 |
| `WBP_HUD_BlockInfo` | `UUserWidget` | existing | 5 |
| `WBP_HUD_Crosshair`, `WBP_HUD_Subtitle` | `UUserWidget` | subtitle subsystem | 5 |
| `WBP_Inventory_Root` | `UCommonActivatableWidget` | `UInventoryWidget` | 6 |
| `WBP_Crafting_Root` | `UCommonActivatableWidget` | `UCraftingWidget` | 6 |
| `WBP_Common_Button/_Slider/_Toggle/_Modal` | `UCommonButtonBase` etc. | none | 1 |

---

## 10. Visual identity

| Hex | Role |
|---|---|
| `#0B0E11` | Void — background |
| `#171C21` | Surface — panels |
| `#2A3138` | Raised / borders |
| `#E3DCCB` | Bone — primary text (never pure white) |
| `#E8A33D` | Sodium — accent, interactive |
| `#C8462F` | Rust — danger (≥ 18 px and icons only) |
| `#4FA6A0` | Signal teal — power, grid, cameras |
| `#5C6268` | Ash — disabled, secondary |

**Type:** Oswald 400/500/600 (display, caps, tracked) · IBM Plex Sans 400/500/600 (UI) ·
IBM Plex Mono 400/500 (numerals, seeds, readouts). All SIL OFL 1.1, Latin Extended.

**Materials:** at most two per screen — enamel panel, CRT, blueprint grid, manila.
**Motion:** 140 ms hover, 180 ms panels, 400 ms page transitions, `cubic-bezier(.2,0,0,1)`.
Lights, power transitions and the danger vignette may flicker. **Text never does.**

---

## 11. Implementation order

| # | Step | Weight |
|---|---|---|
| 0 | Enable `CommonUI`, add the module to `Build.cs`, PIE check that `IA_Break` / `IA_Baubles` still work | ½ d |
| 1 | `Lvl_MainMenu` + `BP_MenuGameMode` + `AMenuPlayerController` + `UWorldDescriptorSubsystem` + Title / New World / Quit — **boots without a world** | 1–1.5 d |
| 2 | `UGameSettingsSubsystem` (single writer) + 4 settings pages + persistence | 1.5 d |
| 3 | Pause menu (`IA_PauseMenu` on the PlayerController, `IMC_UI`) | ½ d |
| 4 | Loading screen + Continue list | ½ d |
| 5 | HUD restyle (crosshair, hotbar, tooltip) + subtitles | 1 d |
| 6 | Inventory 36 slots + crafting (vertical slice 2) | 2–3 d |

---

## 12. Open questions

- Wordmark treatment: stamped enamel steel (current) vs engraved stone. Decided in the mockup
  review — enamel retained unless revised.
- "Continue" carries the world state inline (`— Maison de verre · 23:41`). To revisit after the
  first real save exists.
- Project rename (`MinecraftDeWish` → `Lamplighter`) is a module-wide change (`.uproject`,
  `Source/`, `Config/`, build targets, save paths). Needs its own ADR and a dedicated session.
