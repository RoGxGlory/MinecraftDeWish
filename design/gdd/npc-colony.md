# GDD — NPC Colony

| Field | Value |
|---|---|
| Status | Draft (2026-11-09) — **0% implemented** |
| System owner | game-designer / ai-programmer / ui-programmer |
| Related | `design/gdd/game-concept.md` §4–5, `design/gdd/angels.md` §3.2, `design/gdd/light-and-power.md`, `design/levels/biome-grammar.md` §6 |

---

## 1. Overview

The colony is the phase-3 answer to the Angels, and the reason the end-game is not an idle
game. A camera does not freeze anything by itself: **an observer must be watching its
feed**. The player can only be one observer, so covering a fortress means hiring people,
feeding them, watering them, and keeping them alive. Every camera in the base is a job
posting.

The colony is therefore an **attention economy with human beings as the unit of
attention**. It converts the game's core abstraction ("delegate the gaze") into staffing,
agriculture, and permanent loss.

---

## 2. Player Fantasy

> *"I built something that watches the dark for me. Now I have to keep it alive."*

The player moves from **sole survivor** to **responsible for others**. The emotional beats:

- **Relief** — someone else is watching the corridor. For one night, the player sleeps in a
  bed they built (the character does; the player is still playing).
- **Obligation** — the fortress consumes food and water every day. Growth means more
  mouths before it means more safety.
- **Grief** — colonists die permanently. A dead operator leaves four blind cameras, and the
  player knows exactly which corridor they were covering.

The colony is not a power fantasy. It is the **price** of the power fantasy, and that is
what keeps the horror alive after the player is safe.

---

## 3. Detailed Rules

### 3.1 Colonist model

Every colonist is a persistent actor with:

| Stat | Range | Decays | At zero |
|---|---|---|---|
| **Health** | 0–100 | Combat, fall, Angel contact | **Dies permanently** |
| **Hunger** | 0–100 | `FoodPerDay` per in-game day | Health drains |
| **Thirst** | 0–100 | `WaterPerDay` per in-game day | Health drains faster than hunger |
| **Nerve** | 0–100 (proposed) | Proximity of an unobserved Angel | Flees their post (§3.5) |
| **Morale** | 0–100 (proposed) | Deaths, starvation, darkness | Reduced work speed |

Stats are data-driven (`UNPCNeedData` DataAsset), not literals.

### 3.2 Needs and consumption

```
DailyFoodNeed  = FoodPerDay per colonist
DailyWaterNeed = WaterPerDay per colonist
ColonyFoodDrain  = ColonistCount × FoodPerDay  per day
ColonyWaterDrain = ColonistCount × WaterPerDay per day
```

Proposed values and the design target:

| Value | Proposed | Rationale |
|---|---|---|
| `FoodPerDay` | `1` unit | One meal per day keeps the loop readable |
| `WaterPerDay` | `2` units | Thirst matters more than hunger, as in reality |
| Farmer output | `8` food/day at a full plot | One farmer feeds 8 |
| Water output | `16` water/day at a full well | One water worker supports 8 |
| **Support overhead target** | **≤ 25 % of colonists** | At least 75 % of the colony must be able to do productive work, or the base becomes a pure treadmill |

At `FoodPerDay = 1` and `WaterPerDay = 2`, a colony of 12 needs ~1.5 farmers and ~1.5 water
workers. That leaves 9 colonists for security, industry and free roles — the intended feel.

If production falls below drain, **stockpiles are consumed first**, then hunger/thirst
decay, then health, then death. The player always has a visible countdown.

### 3.3 Recruitment

Three routes, the **mix depends on the colonist** (user decision). Each recruit has one
preferred route and at least one alternative, so the player is never hard-blocked.

| Route | Mechanic | Example |
|---|---|---|
| **Feed & heal** | Give food, water or medicine until trust reaches 100 | A survivor hiding in a hospital ward |
| **Trade** | Bring a specific rare resource from elsewhere | A mechanic who wants a battery before leaving |
| **Resolve their problem** | A short, concrete POI task | Clear the Angel from their cellar; recover a stash |

**Rule:** no recruitment dialogue tree longer than three beats. The game is about space and
attention, not conversation. Recruitment is a spatial and economic act, not a narrative one.

Each recruit has a **background** that maps to a role bonus:

| Background | Role affinity | Bonus |
|---|---|---|
| Farmer | Farmer | `+25%` food output |
| Medic | Watcher (stays calm) | `+30` Nerve ceiling |
| Ex-cop / soldier | Guard | `+20%` combat damage |
| Mechanic / engineer | Engineer | Faster generator repair, less fuel burn |
| Civilian | Any unskilled role | No bonus |

### 3.4 Roles and assignment

Roles are **assigned at the base**, and the available roles depend on what the base contains
(user decision: "selon ce qui est présent dans la base"):

| Role | Requires | Shift | Output |
|---|---|---|---|
| **Farmer** | Farmland plot | Day | Food |
| **Water worker** | Well / collector | Day | Water |
| **Watcher** (security agent) | Powered camera bank + monitor | **Night** | Freezes Angels in **4 cameras** |
| **Guard** | Weapon rack | Day and night | Fights classic mobs (never Angels) |
| **Engineer** | Generator / machinery | Any | Power, repairs, fuel efficiency |
| **Unassigned** | — | — | Idles, consumes, does not produce |

**One watcher covers four cameras** (user decision). Cameras must be **assigned to that
watcher's bank**; a camera assigned to nobody observes nothing, and a camera assigned to a
dead watcher observes nothing until reassigned.

**Shift rule (the colony's central constraint):**

- Watchers work the **night** shift, when the Angels are active.
- Farmers and water workers work the **day** shift.
- A colonist on a night shift **sleeps during the day** and cannot farm.
- Therefore **every watcher costs one day-shift worker**, permanently. Covering four
  cameras is not free — it is one person who will never grow food.

### 3.5 Nerve and panic (proposed)

When an unobserved Angel comes within `PanicRadius` of a colonist, that colonist's Nerve
drains. At `Nerve = 0` they **abandon their post** and run to the nearest safe room.

- A panicking watcher blinds four cameras.
- A panicking farmer abandons the plot, so food stops.
- Panic is the designed "human flaw" of the fortress: the safer the base, the rarer it is,
  but it never disappears.

Nerve recovers when the Angel is frozen, killed, or out of range, and faster in a lit room.
`Medic` backgrounds have a higher ceiling. **Value to be confirmed in playtest** — this is
the one rule in this document that may be cut if it makes the end-game unreadable.

### 3.6 Death

Colonists **die permanently** (user decision). There is no resurrection, no respawn, no
"downed but recoverable" state.

- Angel contact kills a colonist the same way it kills the player.
- Starvation and dehydration kill slowly and visibly.
- A dead colonist's role is vacated instantly, and every camera on their bank goes blind.
- A grave marker may be placed; it is cosmetic but it is the only memorial the game offers.

This makes the colony a **consumable** resource, which is what keeps phase 3 from becoming
a solved state.

### 3.7 The player's own hunger (optional)

The player's hunger and thirst are an **optional difficulty setting, off by default**.

- **Off:** the player has health and XP only (current code). Food and water are colony
  resources.
- **On:** the player consumes the same units, competes with colonists for stockpiles, and
  the survival loop applies to the player too.

The default is off because the horror layer already provides the pressure, and a personal
hunger bar competes with it for the player's attention — the exact resource the game is
about.

### 3.8 Colony size and soft cap

There is **no hard cap**. The cap is food and water:

```
SustainableColonists = min(FoodProduction / FoodPerDay,
                           WaterProduction / WaterPerDay)
```

Above that number, the stockpile drains and colonists begin to die. The player can always
over-recruit and watch the consequences, which is a legitimate (and instructive) choice.

### 3.9 The Source and the colony

The Source **can be reached solo** with very high-tier equipment (user decision). The colony
is the safer route, not the only route.

The intended difference:

- **Solo run:** one pair of eyes, one inventory, no one to cover the approach. Feasible but
  brutal, and the player loses everything on death.
- **Colony run:** watchers hold the approach corridors while the player pushes in; guards
  handle classic mobs; the player can retreat to a staffed line. Slower, safer, and it uses
  the system the whole game was built to teach.

This keeps two end-game styles alive without making the colony mandatory.

---

## 4. Formulas

| Formula | Value | Rationale |
|---|---|---|
| `FoodPerDay` | `1` | One visible meal per day |
| `WaterPerDay` | `2` | Thirst > hunger |
| `StarvationHealthDrain` | `5 HP/hour` | ~4 in-game hours to die from full — readable, not instant |
| `DehydrationHealthDrain` | `8 HP/hour` | Faster than hunger |
| `FarmerFoodPerDay` | `8` | Feeds 8 |
| `WaterWorkerOutputPerDay` | `16` | Supports 8 |
| `WatcherCameraCapacity` | `4` | User decision |
| `PanicRadius` | `20 blocks` | Close enough to be terrifying, far enough to be survivable |
| `NerveDrainPerSecond` | `4` | ~25 s of proximity to break a colonist |
| `NerveRecoveryPerSecond` | `2` in a lit room | Recovery is slower than loss |
| `RecruitTrustThreshold` | `100` | Feed/heal route |
| `MaxRecruitDialogueBeats` | `3` | Keeps recruitment spatial, not conversational |

---

## 5. Edge cases

| Case | Resolution |
|---|---|
| A watcher dies on shift | Their 4 cameras go dark immediately; the player must reassign on the spot |
| A watcher panics | Same as death, but temporary — they return once Nerve recovers |
| Two watchers assigned to the same camera bank | Allowed, but redundant; second watcher is wasted labour |
| Camera assigned to a watcher who is asleep | No observation. Shift assignment is explicit, not implicit |
| Colony starves | Colonists lose health in order of lowest current health; the player sees each death |
| Player recruits beyond sustainable size | Stockpile drains, then deaths. No hidden cap |
| Colonist assigned to a role with no facility | Treated as Unassigned and the UI warns |
| Recruit refuses a second attempt | Not allowed — every recruit has at least one alternative route |
| Colonist killed by an Angel | Permanent. Their grave is the only record |
| Player dies and respawns far away | Colony keeps simulating; watchers keep working unless panicked |
| Player's optional hunger is on and colonists are starving | Player and colonists draw from the same stockpile, in assignment order |
| A colonist is the last one alive | They can still watch 4 cameras; the base is functionally a solo run with one post covered |

---

## 6. Dependencies

| System | Why | Status |
|---|---|---|
| Angel system | Watchers observe Angels; panic is caused by Angels | Not implemented |
| Camera / power system | Watchers need powered cameras and a monitor bank | Not implemented |
| Food & water production (farming, wells) | Colony survival | Not implemented — no farming, no water |
| NPC actor + needs | The colonist itself | Not implemented |
| NPC pathing, schedules, work | Assignment and shifts | Not implemented |
| Recruit interaction UI | The three recruitment routes | Not implemented |
| Colony management UI | Assign roles, see needs and coverage | Not implemented |
| Save/load | Colonists, roles, needs, camera assignments | Not documented |
| Item system | Trade route needs real items, not block proxies | Not implemented |

**The colony depends on almost everything that does not exist yet.** It is the last system
to build, not the first.

---

## 7. Tuning knobs

| Knob | Default | Effect |
|---|---|---|
| `FoodPerDay` / `WaterPerDay` | 1 / 2 | Colony upkeep |
| `FarmerFoodPerDay` / `WaterWorkerOutputPerDay` | 8 / 16 | Support ratio |
| `WatcherCameraCapacity` | 4 | Staffing demand per camera |
| `StarvationHealthDrain` / `DehydrationHealthDrain` | 5 / 8 | How forgiving the economy is |
| `PanicRadius` / `NerveDrainPerSecond` | 20 / 4 | How often the fortress fails |
| `RecruitTrustThreshold` | 100 | Recruitment length |
| Role bonuses | 25–30 % | Value of a good recruit |

---

## 8. Acceptance criteria

1. A colonist with no food or water loses health and dies permanently.
2. A dead colonist's role is vacated and their cameras go blind immediately.
3. A watcher assigned to 4 cameras freezes Angels in all four cones.
4. A camera assigned to nobody observes nothing, even with power.
5. A watcher on night shift does not also farm during the day.
6. Production below consumption drains the stockpile, then kills, in a visible order.
7. All three recruitment routes exist and at least two are available per recruit.
8. Every colonist can be assigned, reassigned and unassigned without a reload.
9. The player's hunger/thirst toggle has no effect on colony stocks when off.
10. The Source is reachable solo; the colony reduces risk without being required.
11. No colonist ever respawns.
12. A colonist's background visibly changes their output or their Nerve ceiling.

---

## 9. Build order (implementation plan)

Blocked on everything else. Suggested order once the prerequisites exist:

1. **NPC actor with needs** (health, hunger, thirst) and a debug panel. No roles.
2. **Food and water production** (farmland, well) and stockpile.
3. **Role assignment UI** — farmer, water worker, unassigned only.
4. **Recruitment** — one route (feed & heal), one background.
5. **Watchers and camera banks** — requires the camera system.
6. **Shifts** — night watchers, day farmers.
7. **Nerve and panic** — last, because it is the first rule to cut if it reads badly.
8. **The other two recruitment routes** and backgrounds.

---

## 10. Open questions

- Is `Nerve` in or out? It is the most thematic rule and the most fragile.
- Do colonists fight back when an Angel reaches them, or are they simply taken?
- Can the player order a colonist to follow them outside the base?
- What happens to the colony when the player is away for many days? (Full simulation is
  expensive; a "colony tick" abstraction is likely needed.)
- Does the Source require at least one watcher to survive the approach, or is a solo run
  truly self-sufficient?
- Is there any form of colonist-to-colonist relationship, or is that scope creep?
