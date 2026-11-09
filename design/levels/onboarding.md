# Onboarding — No Tutorial

| Field | Value |
|---|---|
| Status | Draft (2026-11-09) — replaces the cancelled `tutorial-1-1.md` |
| Decision | **There is no tutorial.** No tutorial level, no scripted opening, no prompts, no tooltips. |
| Owner | level-designer / creative-director |
| Related | `design/levels/LEVEL_DESIGN_FRAMEWORK.md` §6.2, `design/levels/biome-grammar.md` §2.1, `design/gdd/poi-placement.md` §3.10, `design/gdd/angels.md` §3 |

---

## 1. What this document is

The user decision is that the game has **no tutorial**, and that the player starts with
**nothing**. That cancels the planned teaching level (`tutorial-1-1.md`).

This document does not reintroduce teaching through another door. It records three things:

1. the **spatial guarantees** that make learning without a tutorial possible at all,
2. the **first-hour shape** the player will actually experience,
3. the **risk** this decision creates, and how it will be measured rather than argued about.

Everything here is a constraint on generation, not a piece of content.

---

## 2. Why no tutorial is defensible

A tutorial would have to explain the Angel in words, and the Angel's rule is the game's entire
premise. Explaining it turns a discovery into a briefing. Worse, a tutorial teaches the player
that the game will explain things — which is the opposite of a game whose core skill is
noticing.

So the rule is learned the way the game is played: **by seeing a statue move when you look
away.** That is a genuinely rare experience, and it only works if the first time is survivable.

---

## 3. The spatial guarantees (these are the tutorial)

No text is used, so the teaching is compressed into four generation rules. Each one is
enforced elsewhere and listed here as a contract.

| # | Guarantee | Enforced by |
|---|---|---|
| **G1** | The player spawns in **Plains or Forest**, never Desert (no cover) and never Horror Forest (daylight does not protect there). | Spawn selection, `biome-grammar.md` |
| **G2** | The nearest **Horror Forest is at least ~1500 blocks** from spawn, and is reached only through Forest or Blight. | Biome adjacency rules, `biome-grammar.md` §2.1 |
| **G3** | The **first module of every POI is Angel-free**, so the player's first interior is readable before it is lethal. | `poi-placement.md` §3.10 |
| **G4** | A **village is within the first ~1200 blocks**, and its light is on. The first POI the player meets is a place where the rule is *visible* (lit = frozen) before it is *dangerous*. | POI tables, `poi-placement.md` §3.3 |

If any of these four is broken, the game has a tutorial-shaped hole and will kill new players
without teaching them anything. They are acceptance criteria, not flavour.

---

## 4. The first hour

Not a script — a shape that generation must make probable.

### 4.1 Minutes 0–5: nothing

The player starts with an **empty inventory**: no tools, no torch, no food. Punching a tree is
the first verb; a wooden pickaxe is the first crafted object. Every object the player holds
was found or made, so every object has weight.

**Consequence that is not optional:** the crafting UI must present **available recipes**, not
require the player to already know them. Recipe discoverability is a UI responsibility, not a
tutorial. A player who cannot see what a plank can become is stuck, not challenged.

### 4.2 Minutes 5–20: the first night

The first night arrives while the player has, at best, a stone tool and a torch. There is no
bed and no time skip (`game-concept.md` §5.4). The night is survived by one of three things:

- a hole in a hillside with a torch,
- reaching a lit village,
- distance from the Horror Forest, which the spawn rules already guarantee.

This is where the game states its contract: **the dark is the content.**

### 4.3 Minutes 20–60: the first statue

The player enters a POI. The first module is lit and empty (G3). Deeper, a statue stands in a
pose that is not quite right. The player looks away to loot, looks back, and it is closer.

Nothing is explained. The three readable tells are doing the work:

1. **Silhouette** — the pose is human and wrong.
2. **Displacement** — it moved only while unobserved.
3. **Light** — it does not move in a lit room.

A player who dies here loses a corpse run, not a save (`save-load.md` §3.7). The lesson costs
five minutes.

---

## 5. What replaces the tutorial, explicitly

| Tutorial would have… | Instead |
|---|---|
| Explained the Angel rule in text | A statue in a lit room that moves when unobserved |
| Given the player tools | An empty inventory and a crafting UI that shows what is craftable |
| Introduced a safe hub | A village whose safety is visible and stealable |
| Taught controls with prompts | A keybind screen in the menu, nothing in-world |
| Scripted the first night | The first night, unscripted, in a forgiving biome |

---

## 6. Accepted risk

**Risk:** a player encounters a *moving* Angel before ever seeing a *frozen* one, and dies
without understanding why. The rule then reads as "unfair monster" instead of "a rule I can
use".

**Why we accept it:** every alternative mitigation is a tutorial in disguise. The decision is
explicit.

**Mitigations that are not tutorials** (all spatial):

- G1–G4 above.
- Statue pose readability is a **character-art requirement**, not a UI hint: the frozen pose
  must read at 30+ blocks in poor light.
- Death is cheap: respawn at world spawn, corpse recoverable.
- Night Angels are reduced in number (`angels.md` §3.8), so the early game is sparse enough
  that a first encounter is usually a nest, i.e. a place the player chose to enter.

**How this gets decided:** playtest, not opinion.

| Metric | Target |
|---|---|
| Players who can state the Angel rule after their first Angel encounter | `≥ 80 %` |
| Deaths before understanding the rule | `≤ 1` median |
| Time to first statue seen | `≤ 45 min` median |
| Players who quit in the first hour citing "unfair" | `< 10 %` |

If the first metric fails, the fix is a **better tell** (pose, sound, lighting), never a
prompt.

---

## 7. Acceptance criteria

1. No tutorial level, no tutorial POI, no scripted opening exists in the build.
2. The player starts with an empty inventory.
3. The spawn is Plains or Forest, and the nearest Horror Forest is `≥ 1500` blocks away.
4. A lit village exists within `≤ 1200` blocks of spawn.
5. The first module of every POI contains no Angel nest anchor.
6. The crafting UI lists craftable recipes without the player having discovered them.
7. A frozen Angel is visually distinguishable at `30+` blocks in low light, verified by
   screenshot.
8. Death in the first hour costs a corpse run, and the corpse persists across a restart.

---

## 8. Open questions

- Does the keybind/settings screen count as onboarding? It is the only place controls are
  explained, and it is optional.
- Should the first statue be **guaranteed** in the first POI, or only probable? Guaranteeing
  it is a generation constraint; leaving it probable risks a player reaching day three without
  ever seeing one.
- Is the reduced night spawn (`angels.md` §3.8) enough to keep the first night survivable
  without a torch?
- Does the game need a "you died" screen that names the cause? Naming the cause is the
  smallest possible text that could replace a tutorial — and it is still text.
