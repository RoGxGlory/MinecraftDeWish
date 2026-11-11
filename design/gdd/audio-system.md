# Audio System — GDD

| Field | Value |
|---|---|
| Status | Draft (2026-11-11) |
| System owner | systems-designer / audio-director / sound-designer |
| Related | `design/art/art-direction.md`, `design/ui/ui-direction.md`, `design/gdd/weather-system.md`, `design/gdd/weeping-angels.md`, `design/gdd/mob-creeper.md`, `design/gdd/mob-zombie.md`, `design/gdd/mob-spider.md`, `design/gdd/mob-skeleton.md`, `design/gdd/light-and-power.md`, `Docs/SYSTEMS_REFERENCE.md` |
| Source of truth for code | `UAudioComponent`, `UGameplayStatics`, MetaSounds, Sound Classes, Sound Mixes, Sound Attenuation assets |

---

## 1. Overview

The audio system is the **primary horror channel** of LAMPLIGHTER. Where the visuals are
deliberately restrained (no VFX on the Angel, no glow, no aura), sound carries the threat:
the grating of stone that proves a statue moved, the hiss that announces a Creeper's fuse,
the wind that masks a breach. The game's design mantra is *"sound carries the horror, not
the pixels"* (ui-direction.md §3.7).

Audio is organised into five layers:

1. **Ambience** — continuous bed per context (day/night, weather, biome, POI, cave).
2. **SFX** — discrete events (block break, tool swing, mob actions, Angel grating, UI).
3. **Music** — dynamic tension/exploration/combat cues, layered and mixed.
4. **Tells** — the non-diegetic-but-world-anchored warning sounds (Angel stone-grind,
   Creeper hiss, Spider scuttle, Skeleton rattle) that are the player's only reliable
   early-warning system.
5. **UI audio** — clicks, hovers, validation, all of which are deliberately sparse
   ("no bloop", ui-direction.md §3.7).

The system is **data-driven**: every sound asset, attenuation curve, volume bus and
concurrency rule is exposed as a Sound Class / Sound Mix / Data Asset / MetaSound parameter
so designers can tune without code. Audio occlusion is enabled (walls block sound) and
reverb volumes are placed per biome/POI.

---

## 2. Player Fantasy

> *"I heard it before I saw it. That is why I am still alive."*

The player should feel that **their ears are a second pair of eyes** — and that the game
knows it. The fantasy is **acoustic vigilance**: the world is rarely silent, and the sounds
that matter (stone grating, hiss, scuttle) are designed to cut through the ambience at
exactly the moment they need to. The horror is not loud; it is **specific**. A distant
stone-grind at the edge of audibility is more frightening than a stinger.

The system serves the core loop (observation as a resource): audio is the channel that
reaches the player while their eyes are on the world. When the player looks away to mine,
craft, or check a camera feed, audio is the only sense that can warn them. A player who
learns to listen survives; a player who only looks dies.

Three design commitments (non-negotiable):

1. **No false alarms.** A sound that means "danger" always means danger. No ambient
   sound mimics a tell. The player's trust in audio is the mechanic.
2. **No celebration.** No fanfares, no "success" jingles, no reward stingers. Audio
   rewards are the *absence* of threat, not a cue.
3. **Silence is a resource.** Standing still in the dark is the Angel's window — and it
   is also when the ambience drops to its quietest. The player is forced to choose
   between the safety of stillness and the information of motion.

---

## 3. Detailed Rules

### 3.1 Ambience (continuous beds)

Ambience is a set of **looping, crossfading beds** selected by context. Only one bed per
category is active at a time; transitions use a 2–4 s crossfade (no hard cuts).

| Context | Bed | Character |
|---|---|---|
| Day, Clear | `AMB_Day_Clear` | Wind, distant birds (rare), open-air hiss; the "breathing" state |
| Day, Overcast | `AMB_Day_Overcast` | Muted wind, low cloud rumble; baseline |
| Day, Rain | `AMB_Day_Rain` | Rain wash, wet surface hiss, gutter drip; + surface-wetness SFX |
| Day, Fog | `AMB_Day_Fog` | Muffled, close, almost claustrophobic; low drone |
| Day, Ash Storm | `AMB_Day_Ash` | Gritty wind, ash hiss, distant rumble; masks tells |
| Night, Clear | `AMB_Night_Clear` | Crickets (rare), cold wind, distant owl; the "almost peaceful" trap |
| Night, Overcast | `AMB_Night_Overcast` | Baseline night: wind, low drone, occasional creak |
| Night, Rain | `AMB_Night_Rain` | Rain wash + night drone; high tension |
| Night, Fog | `AMB_Night_Fog` | Deep drone, muffled, close; highest tension baseline |
| Night, Ash Storm | `AMB_Night_Ash` | Gritty wind + drone; most hostile ambience |
| Cave (any) | `AMB_Cave` | Drip, echo, low rumble; overrides weather ambience when sky exposure < 0.3 |
| Interior (POI) | `AMB_Interior_<Type>` | Per-POI bed (hospital: distant drip + hum; police station: fluorescent buzz; chapel: faint echo) |
| Base (fortress) | `AMB_Base` | Low generator hum, wind outside; the "safe" (never fully safe) bed |

**Rules:**

- Ambience is **always playing**. There is no total silence — even in the deepest cave,
  a low drone remains. Total silence would break the audio-as-information contract.
- Ambience volume is modulated by `WeatherAmbientFactor` (weather-system.md §3.4) and by
  the player's `AmbientVolume` setting (§7).
- Cave/interior beds **override** weather beds when the player is indoors (sky exposure
  < 0.3). Weather beds fade back in over 1 s when the player exits.
- When an Angel is within 10 m, ambience is **ducked −6 dB** (Sound Mix) to make the
  stone-grind tell audible. This is the only ambience ducking in the game.
- When the player opens the inventory (Tab), ambience is ducked −12 dB (ui-direction.md
  §3.5), never muted.

### 3.2 SFX (discrete events)

SFX are one-shot or short-loop sounds triggered by gameplay. Each has a Sound Class
(`SFX_Blocks`, `SFX_Tools`, `SFX_Mobs`, `SFX_Angel`, `SFX_Weather`, `SFX_UI`) and an
attenuation asset.

**Blocks:**

| Event | Sound | Notes |
|---|---|---|
| Block break | `SFX_BlockBreak_<Material>` | Per-material variant (stone, wood, dirt, sand, glass). Pitch ±5 % random. |
| Block place | `SFX_BlockPlace_<Material>` | Softer, shorter than break. |
| Block step (player) | `SFX_Step_<Material>` | Footstep on the surface the player stands on; volume scales with movement speed. |
| Crack stage change | `SFX_Crack_<Stage>` | Subtle tick when the mining crack overlay advances a stage. |
| Torch place / pick up | `SFX_Torch` | Loudness 2 (Angel hearing, weeping-angels.md §3.4). |
| Furnace / crafting table open | `SFX_StationOpen` | Mechanical latch. |

**Tools:**

| Event | Sound | Notes |
|---|---|---|
| Swing (pickaxe/shovel/axe/hoe/sword) | `SFX_Swing_<Tool>` | 5 swing sets (art-direction §7), not 25. |
| Hit block | `SFX_Hit_<Tool>_<Material>` | Impact + material resonance. |
| Tool break (durability 0) | `SFX_ToolBreak` | Sharp snap. |
| Tool equip / unequip | `SFX_ToolEquip` | Subtle. |

**Mobs:**

| Mob | Idle | Chase | Attack | Death | Special |
|---|---|---|---|---|---|
| Zombie | Low groan (rare) | Groan (periodic) | Swipe | Collapse | Sunlight burn sizzle |
| Skeleton | Bone rattle (rare) | Rattle (periodic) | Bow twang / arrow whoosh | Bone scatter | — |
| Spider | Scuttle (legs) | Fast scuttle | Bite | Squish | Wall-climb scrape |
| Creeper | **Silent** (no idle) | **Silent** (no chase) | Hiss (fuse) | — (detonation) | Detonation boom |

**Mob audio rules:**

- Mobs are **spatialised** (3D, attenuated). A mob behind a wall is muffled (occlusion).
- Zombie/Skeleton idle sounds are **rare and quiet** (every 5–10 s) so they do not
  become noise. The player must *want* to listen.
- The Creeper is **silent until the fuse**. This is the design: its hiss is the only
  warning, and it is a sudden, sharp sound. No ambient hiss, no "breathing".
- Spider scuttle is a **tell** (§3.3): the sound of legs on stone is audible before
  the Spider is visible, especially on ceilings.

**Angel:**

| Event | Sound | Notes |
|---|---|---|
| Spawn (first appearance) | None | The Angel never announces itself. |
| Movement (unobserved) | None (silent) | The Angel moves silently. |
| Freeze (pose snap) | `SFX_Angel_Grind` | The stone-grind tell. Plays on every pose change. |
| Breach (block shake) | `SFX_Angel_BreachGrind` | Continuous grind for the full 5 s breach duration. |
| Contact (kill) | `SFX_Angel_Contact` | A single, low, final sound. Not a jump-scare stinger. |

**Angel audio rules (from weeping-angels.md §3.8, §3.11):**

- The Angel's only sound is the **grating of stone** on freeze and on breach. No
  footsteps, no breathing, no ambient hum.
- `SFX_Angel_Grind` is the **primary tell**. It must be audible through ambience
  and weather. When an Angel is within 10 m, ambience ducks −6 dB to ensure the
  grind cuts through.
- Rain and Ash Storm **mask** the grind (intentional acoustic veil, weeping-angels.md
  §3.11). The player may not hear a breach starting during a storm. This is design.
- The grind is **spatialised** and **occluded**: an Angel grinding behind a wall is
  muffled, but the player can still hear that *something* is grinding. Direction is
  not given; only distance and muffledness.

### 3.3 Audio tells (early-warning system)

Tells are the sounds the player must learn to recognise. They are **not** UI sounds;
they are world-anchored, spatialised, and occluded. They are the player's only
reliable early-warning system.

| Tell | Source | Audible range | Character | Purpose |
|---|---|---|---|---|
| Stone grind | Angel (freeze) | 30 m (occluded) | Low, harsh, grating | "It moved." |
| Breach grind | Angel (breaching) | 40 m (occluded) | Continuous, rhythmic | "It is breaking through." |
| Hiss | Creeper (fuse) | 16 m (occluded) | Sharp, rising, urgent | "Detonation in 1.5 s." |
| Scuttle | Spider (legs) | 12 m (occluded) | Fast, light, skittering | "Something is above/behind you." |
| Bone rattle | Skeleton (idle) | 8 m (occluded) | Dry, intermittent | "A skeleton is nearby." |
| Arrow whoosh | Skeleton (shot) | 20 m (occluded) | Sharp, fast | "Incoming projectile." |
| Mob groan | Zombie (chase) | 10 m (occluded) | Low, periodic | "A zombie is pursuing." |
| Thunder | Weather | Global | Deep, rolling | "Storm is here; tells are masked." |
| Wind gust | Weather (Ash Storm) | Global | Gritty, howling | "Ash storm; visibility and audio reduced." |

**Tell rules:**

- Tells are **never** masked by their own source's ambience. A hiss cuts through
  `AMB_Night_Fog` because it is played on a higher-priority Sound Class with a
  minimum volume floor.
- Tells are **subject to occlusion and distance attenuation** (§4). A tell behind 3
  solid blocks is barely audible. The player must move to hear better.
- Tells are **never** accompanied by a UI indicator (ui-direction.md §3.6: "Angel
  proximity: never an icon, arrow or number. Audio + desaturation only"). The audio
  IS the indicator.
- Tells are **never** duplicated by a non-diegetic stinger. If the game wants to
  signal danger, it plays the tell louder or closer, not a separate cue.

### 3.4 Weather audio

Weather has its own audio layer that sits above ambience and below tells in the mix.

| Weather | Audio elements | Notes |
|---|---|---|
| Clear | None (ambience only) | The quietest state. Tells are clearest. |
| Overcast | Low cloud rumble (periodic, distant) | Subtle; adds weight. |
| Rain | Rain bed (wash), drip, puddle splash, thunder (random, 30–90 s interval) | Rain bed masks tells (intentional). Thunder is a global event, not a tell. |
| Fog | Muffled ambience, faint distant drip | Fog does not add noise; it *removes* clarity (low-pass filter on all non-tell sounds). |
| Ash Storm | Wind howl, grit hiss, ash patter, occasional distant rumble | Masks tells most aggressively. Reduces all non-tell audio by −3 dB (Sound Mix). |

**Weather audio rules:**

- Weather audio is **modulated by the weather transition** (30–60 s lerp). No instant
  pops.
- Rain intensity scales with `RainParticleRate` (weather-system.md §7). Light rain =
  gentle wash; heavy rain = loud wash + frequent thunder.
- Thunder is **randomised** (interval 30–90 s, volume 0.6–1.0). It is never synced to
  a gameplay event. It is atmosphere, not a tell.
- During Fog, a **low-pass filter** (MetaSound or Sound Mix EQ) is applied to all
  non-tell, non-UI audio at 2 kHz cutoff, simulating muffled hearing. Tells are
  exempt (they cut through).
- During Ash Storm, the same low-pass is applied at 1.5 kHz, plus a −3 dB global
  duck on non-tell audio.

### 3.5 Music

Music is **dynamic and layered**, not a playlist. It consists of three stems that are
mixed in real time based on game state. There is no "menu music" in the traditional
sense (the title screen uses only the bulb buzz + mechanical latch, ui-direction.md §3.3).

**Stems:**

| Stem | Character | Trigger |
|---|---|---|
| `MUSIC_Exploration` | Low drone, sparse notes, cold | Default state; plays when no threat is detected. |
| `MUSIC_Tension` | Rising drone, dissonant intervals, heartbeat pulse | Active when an Angel or hostile mob is within detection range but not in combat. |
| `MUSIC_Combat` | Percussive, urgent, low brass | Active when the player is in direct combat (taking damage or attacking a mob). |

**Music rules:**

- Music is **always present** but **never loud**. Exploration stem at −18 dB; Tension
  at −12 dB; Combat at −6 dB. Music never dominates SFX or tells.
- Stems **crossfade** (2 s) based on a `ThreatLevel` parameter (0 = Exploration,
  1 = Tension, 2 = Combat). `ThreatLevel` is computed by the audio system from
  mob proximity and Angel observation state.
- Music is **suppressed** (faded to 0 over 1 s) when:
  - The player is in a menu (Pause, Inventory, Settings).
  - The player is dead (death screen uses silence + a single low tone).
  - The Dread preset is "Ambient" (music stems disabled entirely; only ambience plays).
- Music is **never** a tell. It signals mood, not specific threats. The player must
  not learn to rely on music for danger detection — tells are the channel.
- Music uses **MetaSounds** for procedural layering (heartbeat pulse rate scales with
  `ThreatLevel`).

### 3.6 UI audio

UI audio is **deliberately minimal** (ui-direction.md §3.7: "dry cloth and stone
clicks, no bloop, no hover sound").

| Event | Sound | Notes |
|---|---|---|
| Button hover | **None** | No hover sound. (ui-direction.md §3.3: "No hover sound.") |
| Button click / confirm | `UI_Click` | Mechanical latch click. Short, dry. |
| Slider adjust | `UI_SliderTick` | Subtle tick per step. |
| Toggle on/off | `UI_Toggle` | Mechanical switch. |
| Modal open | `UI_ModalOpen` | Soft panel slide. |
| Modal close | `UI_ModalClose` | Soft panel slide (reverse). |
| Inventory open (Tab) | `UI_InvOpen` | Cloth rustle + wood creak. |
| Inventory close | `UI_InvClose` | Cloth rustle. |
| Item pickup (inventory) | `UI_ItemPickup` | Subtle clink. |
| Error / invalid action | `UI_Error` | Low, dull thud. Not a buzzer. |
| Death screen appear | `UI_Death` | Single low tone. Silence follows. |
| Achievement / level up | **None** | No reward stinger. |
| Subtitle appear | **None** | Subtitles are visual only. |

**UI audio rules:**

- All UI sounds are **2D** (no spatialization, no attenuation). They play at a fixed
  volume regardless of world position.
- UI sounds are routed through the `UI` Sound Class, which is **not** affected by
  weather low-pass or ambience ducking. UI audio is always clear.
- UI sounds respect the `UIVolume` setting (§7) independently.
- No UI sound exceeds 0.3 s duration. Short, dry, functional.

### 3.7 Spatialization (3D audio)

All world-anchored SFX, tells and mob sounds are **3D-spatialised** using Unreal's
audio system.

**Attenuation model:**

| Sound category | Shape | Inner radius | Falloff distance | Falloff curve | Max concurrent |
|---|---|---|---|---|---|
| Block SFX (break/place) | Sphere | 0 m | 25 m | Logarithmic | 8 |
| Tool SFX (swing/hit) | Sphere | 0 m | 15 m | Logarithmic | 6 |
| Footsteps (player) | Sphere | 0 m | 10 m | Linear | 1 (player) |
| Footsteps (mob) | Sphere | 0 m | 12 m | Logarithmic | 4 |
| Mob sounds (groan/rattle) | Sphere | 0 m | 20 m | Logarithmic | 8 |
| Mob attack (bite/swipe) | Sphere | 0 m | 15 m | Logarithmic | 6 |
| Angel grind (tell) | Sphere | 0 m | 30 m | Logarithmic (gentle) | 4 (one per Angel) |
| Angel breach grind | Sphere | 0 m | 40 m | Logarithmic (gentle) | 4 |
| Weather (rain/thunder) | Global (2D) | — | — | — | 1 each |
| Ambience | Global (2D) | — | — | — | 1 per category |
| Music | Global (2D) | — | — | — | 3 stems |

**Spatialization rules:**

- **Occlusion is enabled** (`bEnableOcclusion = true`) on all 3D sounds. Solid voxel
  blocks attenuate sound by −6 dB per block (up to −24 dB at 4 blocks). Beyond 4
  blocks of solid material, the sound is inaudible (but the tell's minimum floor
  still applies at −24 dB for Angel grind).
- **Reverb volumes** are placed per biome and POI:
  - Cave: large reverb (long tail, low dampening).
  - Interior (POI): medium reverb (short tail, high dampening).
  - Surface (open air): no reverb (or very short outdoor tail).
  - Base (fortress): small reverb (tight, concrete).
- **Doppler effect** is disabled. The game is voxel-based and speeds are low; Doppler
  would add noise without information.
- **Panning** follows the listener (player camera) orientation. Stereo spread is
  standard (100 %).
- **Distance-based LPF**: sounds beyond 50 % of their falloff distance have a
  low-pass filter applied (cutoff 4 kHz → 1 kHz at max distance). This simulates
  air absorption and helps tells feel "far away".
- **Tell minimum floor**: Angel grind and Creeper hiss have a minimum audible volume
  of 0.05 (5 %) regardless of distance/occlusion, up to their falloff distance.
  Beyond falloff, they are inaudible. This ensures a distant grind is always
  perceptible as "something is there" without being loud.

### 3.8 Accessibility

Accessibility is a first-class requirement, not an afterthought (ui-direction.md §3.7,
`.opencode/rules/ue-ui.md`).

| Feature | Implementation |
|---|---|
| **Subtitles** | All tells and mob sounds have subtitle entries. Subtitles appear at the bottom-centre, 32 px above the hotbar, 2 lines max, speaker label in sodium (ui-direction.md §3.6). Toggle: On/Off (default On). |
| **Subtitle size** | Scale 0.85–1.30 (ui-direction.md §7). |
| **Separate volume buses** | Master, Music, SFX, Ambience, UI, Voice/Subtitles (if any). Each independently adjustable 0.0–1.0. |
| **Mono audio** | Toggle to collapse stereo to mono (for single-ear or hearing-impaired players). |
| **Visual tell indicators** | Optional (accessibility setting): a subtle screen-edge pulse (1 px, 0.5 s) in the direction of an Angel grind or Creeper hiss. Default Off (the game's intent is audio-only tells). When On, the pulse is subtle and does not replace the audio. |
| **Closed captions** | Describe non-speech sounds ("stone grinding", "hissing", "scuttling") with speaker labels ("Angel", "Creeper", "Spider"). |
| **Audio ducking transparency** | When ambience is ducked for an Angel tell, no visual indicator is shown (the ducking is the mechanic). Accessibility mode may optionally show a subtle vignette pulse. |
| **Photosensitivity** | No strobing audio-visual sync. Thunder flashes are not implemented (audio only). |
| **Controller vibration** | Optional: haptic feedback on tells (Angel grind = low rumble, Creeper hiss = sharp pulse). Default Off. |

**Subtitle content rules:**

- Subtitles describe the **sound**, not the threat: "[stone grinding]", "[hissing]",
  "[scuttling]", not "Angel nearby" or "Creeper about to explode".
- Subtitles include a **direction indicator** when the sound is 3D and the player can
  turn toward it: "[stone grinding — left]", "[hissing — behind]".
- Subtitles respect the `SubtitleSize` setting and the safe zone (ui-direction.md §4).
- Subtitles are **not** shown for ambience, music, or UI sounds.

---

## 4. Formulas

### 4.1 Volume mixing

```
FinalVolume(sound) = BaseVolume(sound)
                   × SoundClassVolume(class)
                   × MasterVolume
                   × DistanceAttenuation(distance)
                   × OcclusionFactor(blocks)
                   × WeatherDuck(weather)
                   × ThreatDuck(threat)
                   × Clamped(0.0, 1.0)
```

Where:

- `BaseVolume(sound)` — per-asset base volume (0.0–1.0), set in the Sound Wave/MetaSound.
- `SoundClassVolume(class)` — per-class volume from settings (Music, SFX, Ambience, UI).
- `MasterVolume` — global master (0.0–1.0).
- `DistanceAttenuation(distance)` — from the attenuation curve (§3.7).
- `OcclusionFactor(blocks)` — 1.0 if no occlusion; 0.5^(blocks) per solid block, min 0.0625 (−24 dB) for tells, 0.0 for others beyond 4 blocks.
- `WeatherDuck(weather)` — 1.0 (Clear/Overcast), 0.7 (Rain), 0.5 (Fog), 0.5 (Ash Storm) for non-tell sounds; 1.0 for tells.
- `ThreatDuck(threat)` — 0.5 (−6 dB) when an Angel is within 10 m and the sound is ambience; 1.0 otherwise.

### 4.2 Distance attenuation (logarithmic)

```
Attenuation(d) = 1.0 - (log10(d / InnerRadius + 1) / log10(FalloffDistance / InnerRadius + 1))
```

Clamped to [0.0, 1.0]. At `d = 0`, attenuation = 1.0. At `d = FalloffDistance`,
attenuation = 0.0. At `d = FalloffDistance / 2`, attenuation ≈ 0.5 (logarithmic).

For linear falloff (footsteps):

```
Attenuation(d) = 1.0 - (d / FalloffDistance)
```

### 4.3 Occlusion

```
OcclusionFactor = max(MinOcclusion, 0.5 ^ SolidBlocksBetween)
```

Where `SolidBlocksBetween` is the number of solid voxel blocks on the line between the
listener and the sound source (raycast). `MinOcclusion` = 0.0625 (−24 dB) for tells,
0.0 for all other sounds.

### 4.4 Threat level (music mixing)

```
ThreatLevel = max(
    AngelThreat,   // 0 if no Angel within 30 m; 1 if within 30 m; 2 if within 10 m
    MobThreat      // 0 if no hostile mob within 15 m; 1 if within 15 m; 2 if taking damage
)
```

Clamped to [0, 2]. Mapped to music stems:

- `ThreatLevel = 0` → Exploration stem at −18 dB, Tension 0, Combat 0.
- `ThreatLevel = 1` → Exploration −24 dB, Tension −12 dB, Combat 0.
- `ThreatLevel = 2` → Exploration 0 (muted), Tension −18 dB, Combat −6 dB.

Stem volumes lerp over 2 s. No instant snap.

### 4.5 Tell minimum floor

```
TellVolume = max(TellMinimumFloor, CalculatedVolume)
```

Where `TellMinimumFloor = 0.05` (5 %) and `CalculatedVolume` is the result of §4.1.
This floor applies only to Angel grind and Creeper hiss. It does not apply beyond
`FalloffDistance` (the sound is inaudible, floor or not).

### 4.6 Weather low-pass filter

```
CutoffFrequency(weather) =
    20000 Hz   Clear / Overcast
    2000 Hz    Fog
    1500 Hz    Ash Storm
    8000 Hz    Rain (rain is broadband, not muffled)
```

Applied to all non-tell, non-UI audio. Tells and UI are exempt (full bandwidth).

### 4.7 Subtitle timing

```
SubtitleDuration = max(SoundDuration, 1.5 s)
SubtitleFadeIn   = 0.1 s
SubtitleFadeOut  = 0.3 s
```

A subtitle stays on screen for at least 1.5 s even if the sound is shorter, to allow
reading. Maximum 2 lines; if a third tell fires, the oldest subtitle is replaced.

---

## 5. Edge Cases

| Case | Resolution |
|---|---|
| **Angel grind during Ash Storm** | The grind is masked (WeatherDuck = 0.5 for non-tells, but grind is a tell so WeatherDuck = 1.0). However, the Ash Storm low-pass (1.5 kHz) and −3 dB global duck still apply to the ambience, making the grind *relatively* louder. The player may still miss it if the storm is loud. This is intentional. |
| **Multiple Angels grinding simultaneously** | Each Angel's grind is a separate 3D sound. Concurrency limit = 4. If 5+ Angels grind at once, the quietest (farthest) is culled. The player hears the closest threats. |
| **Creeper hiss during Rain** | Hiss is a tell (WeatherDuck = 1.0). Rain low-pass (8 kHz) does not muffle it. The hiss cuts through rain. The player always hears a hiss within 16 m, rain or not. |
| **Spider scuttle on ceiling** | Scuttle is spatialised at the Spider's position (above the player). The player hears it overhead. Occlusion applies (ceiling blocks). |
| **Player in inventory (Tab) during an Angel grind** | Ambience ducks −12 dB (inventory rule). The grind is a tell and is not ducked by the inventory rule (only ambience is). The player hears the grind at full tell volume while in inventory. This is the "you hear it while looting" horror moment. |
| **Player dead, death screen** | All audio fades to silence over 1 s except a single low tone (`UI_Death`). Music muted. Ambience muted. No tells play (the player is dead). |
| **Player in menu (Pause/Settings)** | Music muted. Ambience continues at −12 dB (paused world, but audio breathes). UI sounds play at full UI volume. No tells play (world is paused). |
| **Audio device disconnected** | Audio system pauses all playback. On reconnect, audio resumes from the current state (no replay of missed sounds). A log warning is emitted. |
| **Sound asset missing** | Log warning, play a fallback (silence or a placeholder beep in dev builds). Never crash. |
| **Occlusion raycast hits a non-solid block (torch, leaf)** | Non-solid blocks do not count toward occlusion. The raycast passes through them (same rule as Angel observation, weeping-angels.md §3.2). |
| **Two sounds at the same position** | Concurrency rules apply per Sound Class. If the limit is reached, the quietest (or oldest, per the concurrency asset) is culled. |
| **Weather transition during a tell** | The tell's volume is not affected by the transition (tells are exempt from weather ducking). The ambience transitions smoothly around it. |
| **Subtitle queue overflow** | Maximum 2 lines. A third tell replaces the oldest subtitle. Subtitles never queue more than 2. |
| **Mono audio mode** | All stereo panning is collapsed to centre. Spatial information is lost, but subtitles with direction indicators compensate. |
| **Volume set to 0** | The Sound Class volume is 0.0. No audio plays for that class. Tells are also silenced (accessibility trade-off: the player chose silence). Subtitles still appear if enabled. |
| **Dread preset = Ambient** | Music stems are disabled. Only ambience and SFX/tells play. The game is quieter but tells remain. |
| **Dread preset = Merciless** | Music stems are louder (Exploration −12 dB, Tension −6 dB, Combat 0 dB). Ambience ducking during Angel proximity is disabled (no warning). Tells play at normal volume but ambience does not duck. |
| **Player places a torch near an Angel** | The Angel freezes. If it was mid-grind (breach), the grind stops. The freeze sound (`SFX_Angel_Grind`) plays. The player hears the grind stop abruptly — a tell in itself. |
| **Indoor detection (cave/POI)** | When sky exposure < 0.3, the cave/interior ambience bed replaces the weather bed. Weather audio (rain, wind) fades out over 1 s. Thunder is still audible (global, not occluded). |

---

## 6. Dependencies

| System | Why | Status |
|---|---|---|
| `UDayNightCycleSystem` | Day/night and weather state for ambience selection | Implemented |
| `AWorldGenerator` | Sky exposure (indoor detection), block raycast (occlusion), block material (SFX variant) | Implemented |
| `AMobCharacter` | Mob state (chase/attack/death) triggers SFX | Implemented |
| `UPlayerVitalSystem` | Damage/death triggers audio | Implemented |
| `UBaublesSystem` | Tool type/tier for SFX variant | Implemented |
| `UMobSpawnerSystem` | Mob spawn triggers (no audio on spawn — intentional) | Implemented |
| Weeping Angels system | Angel state (freeze/breach/contact) triggers tells | Not implemented (design phase) |
| Weather system | Weather state drives ambience and weather audio | Implemented (modulation partial) |
| UI system (LAMPLIGHTER) | UI events trigger UI audio | Not implemented (design phase) |
| MetaSounds | Procedural audio (music heartbeat, weather filters) | Available (UE 5.7+) |
| Sound Classes / Sound Mixes | Volume buses and dynamic mixing | Available (UE built-in) |
| Reverb Volumes | Per-biome/POI reverb | Available (UE built-in) |
| Subtitle system | Subtitle rendering for tells | Not implemented (design phase) |

Bidirectional note: `weeping-angels.md` §3.8 references audio for breach grinding and
stone-grind tells. `weather-system.md` §6 references audio for rain/wind/thunder.
`ui-direction.md` §3.3 and §3.7 reference audio for bulb buzz, mechanical latch, and
the "sound carries the horror" principle. `art-direction.md` §11 references SFX-visual
pass (dust by material, creeper sodium flash).

---

## 7. Tuning Knobs

| Knob | Default | Range | Effect |
|---|---|---|---|
| `MasterVolume` | 1.0 | 0.0–1.0 | Global master volume |
| `MusicVolume` | 0.7 | 0.0–1.0 | Music stem volume |
| `SFXVolume` | 1.0 | 0.0–1.0 | SFX volume |
| `AmbienceVolume` | 0.8 | 0.0–1.0 | Ambience bed volume |
| `UIVolume` | 1.0 | 0.0–1.0 | UI sound volume |
| `SubtitleVolume` | N/A | — | Subtitles are visual; no volume |
| `AmbienceCrossfadeDuration` | 3.0 s | 1.0–6.0 | Crossfade between ambience beds |
| `MusicCrossfadeDuration` | 2.0 s | 0.5–4.0 | Crossfade between music stems |
| `AngelGrindFalloffDistance` | 3000 cm | 1500–5000 | How far the Angel grind is audible |
| `AngelGrindMinFloor` | 0.05 | 0.0–0.15 | Minimum audible volume for Angel grind |
| `CreeperHissFalloffDistance` | 1600 cm | 800–3200 | How far the Creeper hiss is audible |
| `OcclusionAttenuationPerBlock` | 0.5 | 0.3–0.7 | Volume multiplier per solid block (occlusion) |
| `OcclusionMaxBlocks` | 4 | 2–6 | Beyond this, non-tell sounds are inaudible |
| `WeatherDuck_Rain` | 0.7 | 0.5–0.9 | Non-tell audio multiplier in Rain |
| `WeatherDuck_Fog` | 0.5 | 0.3–0.7 | Non-tell audio multiplier in Fog |
| `WeatherDuck_AshStorm` | 0.5 | 0.3–0.7 | Non-tell audio multiplier in Ash Storm |
| `AngelProximityDuck` | 0.5 (−6 dB) | 0.3–0.7 | Ambience duck when Angel within 10 m |
| `InventoryDuck` | 0.25 (−12 dB) | 0.1–0.5 | Ambience duck when inventory is open |
| `FogLowPassCutoff` | 2000 Hz | 1000–4000 | Low-pass cutoff in Fog weather |
| `AshStormLowPassCutoff` | 1500 Hz | 800–3000 | Low-pass cutoff in Ash Storm weather |
| `DistanceLPF_Start` | 0.5 | 0.3–0.7 | Fraction of falloff distance at which distance-LPF begins |
| `DistanceLPF_MinCutoff` | 1000 Hz | 500–2000 | Minimum cutoff at max distance |
| `ThunderIntervalMin` | 30 s | 15–60 | Minimum thunder interval in Rain |
| `ThunderIntervalMax` | 90 s | 60–180 | Maximum thunder interval in Rain |
| `ThunderVolume` | 0.8 | 0.5–1.0 | Thunder volume |
| `FootstepVolumeScale` | 1.0 | 0.5–1.5 | Player footstep volume multiplier |
| `MobIdleSoundInterval` | 7.0 s | 3.0–15.0 | Average interval between mob idle sounds |
| `MonoAudio` | Off | On/Off | Collapse stereo to mono |
| `VisualTellIndicators` | Off | On/Off | Screen-edge pulse for tells (accessibility) |
| `HapticTells` | Off | On/Off | Controller vibration on tells |
| `SubtitleScale` | 1.0 | 0.85–1.30 | Subtitle text scale |
| `MusicEnabled` | On | On/Off | Master music toggle (also controlled by Dread preset) |

All knobs are exposed as `EditAnywhere` on a `UAudioSettings` Data Asset or via the
`UGameSettingsSubsystem` (ui-direction.md §6). No knob value may be hardcoded in C++.

---

## 8. Acceptance Criteria

1. **Ambience plays continuously** in all weather states and transitions smoothly (no
   hard cuts) between day/night and weather changes.
2. **Cave/interior ambience** overrides weather ambience when sky exposure < 0.3.
3. **Block SFX** plays the correct material variant on break and place, with ±5 % pitch
   randomisation.
4. **Tool SFX** uses 5 swing sets (one per tool type), not 25.
5. **Mob SFX** plays correctly for all four mob types (Zombie, Skeleton, Spider, Creeper)
   including idle, chase, attack, death, and special (burn, climb, hiss, detonation).
6. **Creeper is silent** until the fuse begins; the hiss is the only warning and is
   audible within 16 m.
7. **Angel grind** plays on every pose change and is audible within 30 m (occluded).
8. **Angel breach grind** plays continuously during a breach and is audible within 40 m.
9. **Weather audio** (rain, wind, thunder, ash) plays correctly per weather state and
   transitions over 30–60 s.
10. **Fog low-pass** (2 kHz) applies to all non-tell, non-UI audio in Fog weather.
11. **Ash Storm low-pass** (1.5 kHz) and −3 dB duck apply to non-tell audio.
12. **Music stems** crossfade based on `ThreatLevel` (0/1/2) over 2 s.
13. **Music is suppressed** in menus, on death, and when Dread = Ambient.
14. **UI audio** is 2D, short (≤ 0.3 s), and has no hover sound.
15. **Spatialization** uses logarithmic attenuation with per-category falloff distances.
16. **Occlusion** attenuates sound by 0.5 per solid block (min −24 dB for tells).
17. **Reverb volumes** are placed per biome and POI.
18. **Subtitles** appear for all tells with direction indicators, respecting subtitle
    scale and safe zone.
19. **Separate volume buses** (Master, Music, SFX, Ambience, UI) are independently
    adjustable and persist across sessions.
20. **Mono audio** toggle collapses stereo to centre.
21. **Visual tell indicators** (accessibility) are optional and default Off.
22. **No audio plays** when the corresponding volume is set to 0.
23. **No crash** on missing sound asset (log warning + fallback).
24. **Performance**: total audio mixing cost < 2 ms per frame at 60 FPS with all
    categories active (measured via `stat soundwaves`).
25. **Automation tests** verify: volume formula (§4.1), distance attenuation (§4.2),
    occlusion (§4.3), threat level (§4.4), tell minimum floor (§4.5), weather low-pass
    (§4.6), subtitle timing (§4.7).