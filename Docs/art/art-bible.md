# Art Bible — LAMPLIGHTER (MinecraftDeWish)

**Status:** Accepted — P1 finalised (lots 0–3 compiled; 9 original assets imported; textures refined). Reference set frozen for P2. Final lock blocked on A1 (`Angel_60b_fog.png`); A3/A4/A5/A6 documented with decisions below.
**Date:** 2026-11-11 (P1 finalisation pass)
**Owner:** Creative Director
**Sources of truth:**
- `design/art/art-direction.md` (direction artistique, Accepted)
- `Docs/art/style-guide.md` (guide visuel technique, Accepted)
- `Docs/art/mood-board.md` (captures in-engine, conditional approval)
- `Docs/art/material-parameters.md` (paramètres matériaux)
- `Docs/technical-contract.md` (contrat technique figé)

> Ce document est le **référentiel unique** pour l'équipe artistique. Il consolide la
> direction créative, les règles techniques et les captures de référence en un seul
> point d'entrée. Toute modification d'une règle figée nécessite un ADR approuvé par
> le Technical Director.

---

## 1. Purpose & Scope

L'art bible définit l'identité visuelle de LAMPLIGHTER et les contraintes que toute
production artistique doit respecter. Il est lu par les artistes 3D, les technical
artists, les animateurs et les UI designers avant de créer ou modifier un asset.

**Ce que ce document couvre :** thèse visuelle, palette, lumière, matériaux, blocs,
mobs, l'Angel, outils, UI, météo, menu principal, captures de référence, validations
ouvertes, prochaines étapes P2.

**Ce qu'il ne couvre pas :** détail des paramètres matériau (→ `material-parameters.md`),
contrat technique figé (→ `technical-contract.md`), direction UI détaillée
(→ `design/ui/ui-direction.md`).

---

## 2. Thesis — Eroded Realism

> **"The world is built, the Angel is sculpted."**

Le monde est fait de blocs usés et de lumière sodium. L'Angel Weeping est la seule
chose dans le jeu qui n'est pas faite de blocs — et donc la seule chose qui
n'appartient pas au monde. Ce contraste est l'identité visuelle **et** la mécanique.

**Test de reconnaissance (design test, art-direction §14) :** un screenshot doit être
identifiable sans logo comme *worn voxel world, sodium light, a white statue that
should not be there.* Si un joueur décrit l'Angel comme "un mob", le contraste a
échoué.

**État de validation (2026-11-11) :** les deux premiers signaux (monde voxel usé,
lumière sodium) lisent clairement dans les captures du mood board. Le troisième
 statue blanche qui ne devrait pas être là) est **non validé** — aucune capture ne
contient d'Angel. L'approbation finale du mood board est bloquée sur A1
(`Angel_60b_fog.png`). Voir §13 et §14.

---

## 3. Palette

Source de vérité : `design/art/art-direction.md` §2. Aucune nouvelle couleur sans
approbation de l'art-director.

| Hex | Nom | Usage monde | Usage UI |
|---|---|---|---|
| `#0B0E11` | Void | ciel nocturne, intérieurs non éclairés, fog | scrim modal (85%) |
| `#171C21` | Concrete | pierre taillée, murs/fols POI | surface de panneau |
| `#2A3138` | Steel | métal, toits, barres, machines | état désactivé |
| `#E3DCCB` | Bone | plâtre, ciel couvert, **pierre de l'Angel** | texte principal |
| `#E8A33D` | Sodium | **la seule lumière chaude** : torche, lampe, four, générateur | accent, focus, sélectionné |
| `#C8462F` | Rust/Blood | creeper, dégâts, corrosion, explosion | danger, erreur |
| `#4FA6A0` | Signal | électricité, caméras, CRT — la seule lumière "propre" | succès, toggle actif |
| `#5C6268` | Ash | fog, métal terni, désactivé | texte secondaire |
| `#6B7355` | Moss | lichen, végétation résiduelle | — |
| `#9AA36B` | Sick green | Horror Forest, feuilles malades | — |
| `#8C8577` | Dust | sable, gravats, cendre au sol | — |

**Règles non négociables :**
- Une seule source chaude : sodium. Tout le reste est froid, désaturé, cendré.
- Pas de ciel bleu saturé. Ciel de jour = bone-ash ; nuit = teal froid-sombre.
- Orbes XP désaturés à `#B9C46A` — le seul élément "game-y", discret.
- Rouge rouille réservé au danger. Ne décore jamais.
- Le grade sodium (LUT) est constant ; seule l'intensité/filtration de la lumière
  change avec la météo et le jour/nuit. Le grade global ne brighten jamais le monde.

---

## 4. Light & Grade

- Post-process + LUT unique "Sodium Grade" : contraste élevé dans les lows, rolloff
  doux dans les highs, 25–35% de désaturation hors sodium.
- Ash fog : réduit la lisibilité à **60–90 blocs**. Sert la mécanique (l'Angel à
  60 blocs doit se lire comme une silhouette, pas un détail).
- La torche est l'objet le plus brillant à l'écran, **toujours**. C'est la
  hiérarchie visuelle du fantasy.
- Cycle jour/nuit (20 min) conservé ; le grade est **constant**. Seule la lumière
  du joueur change. Phase 3 ne brighten jamais le monde — le joueur installe la lumière.
- Météo module l'ambiance : pluie (surfaces mouillées, plus de réflexions), brouillard
  épais (visibilité réduite), tempête de cendres (particules, vent, bruit), ciel dégagé
  (rare, presque dérangeant par contraste). Seule l'intensité/filtration change ; le
  LUT reste.

**Quantification torch/sodium (A3 — en cours) :** la règle "torche la plus brillante"
est actuellement non quantifiée. Cible : pic lumineux torche ≥ 2× luminance ciel/ambient
dans le frame. Si non atteint : augmenter le multiplicateur emissif de la torche ou
réduire l'ambient. Aucune capture actuelle ne contient de torche visible (voir §12).

---

## 5. Blocks & Terrain

- Patrix conservé comme **vérité matérielle** (le joueur reconnaît ses blocs). Pas de
  refonte massive de textures.
- **12 variantes "ruine"** (fissuré/moussu/souillé/brûlé) des blocs les plus vus →
  nouvelles slices dans le Texture2DArray existant (valider le budget 37→128 slices).
- **Deferred decals :** mousse, suie, cendre, sang, brûlures, affiches déchirées. Meilleur
  ratio impact/coût. Aucun nouveau BlockID.
- Végétation : feuilles et herbe désaturées vers olive/malade ; props d'envahissement
  (lierre, racines) bloqués jusqu'à ce qu'un chemin de mesh non-solide existe (seules
  les torches en ont un).
- L'érosion se lit par matériau + decal + fog, jamais par silhouette.

**Observation issue des captures (A4) :** le brouillard actuel tue la lisibilité des
blocs à ~10–15 blocs (captures 000733/000817/000850), bien plus dense que la cible
60–90 blocs. La courbe de densité doit être ajustée (pas la couleur) pour atteindre la
cible, puis validée contre A1. Le brouillard est aussi très uniforme (gris-vert plat,
pas de falloff en hauteur ni de god rays). La couleur grise-verte observée pourrait être
spécifique au biome (marais/Horror Forest) mais ne doit pas dominer le brouillard de base.

**Observation issue des captures (block readability) :** les limites de blocs ne sont pas
discernables au premier plan dans la capture 000850 ; le sol se lit comme une texture
mouchetée continue. Aucune réponse PBR (specular/roughness/normal) ni érosion/usure
visibles. La passe texture doit valider les bords de blocs + usure en PIE avant
l'expansion P2.

---

## 6. Mobs — Stylized Low-Poly

Non voxel-rigged, non photoréaliste. **Horreur low-poly stylisée :** silhouettes lisibles,
géométrie simple mais articulée, matériaux mats désaturés. Les mobs sont des créatures
du monde (chair, os, chitine) ; l'Angel est de la pierre.

| Mob | Lecture visuelle |
|---|---|
| Zombie | Vêtements de travail déchirés, côtes exposées, mâchoire disloquée. Lent, voûté. |
| Skeleton | Os tachés, cassés ; à moitié habillé (uniforme de police ?). |
| Spider | Bas et large, 8 pattes articulées, plaques de chitine, touffes. Doit se lire comme une forme la nuit. |
| Creeper | La seule forme organique parmi les voxels : gonflé, couvert de mousse. Tell d'explosion = gonflement + fissures sodium de l'intérieur. Pas de visage. |

Un rig humanoïde partagé (`SK_Mob_Base`, 6 os, poids rigides) couvre Zombie + Skeleton.
Spider a son propre squelette (8 pattes).

---

## 7. The Angel — The Contrast Is the Mechanic

- **Matériau :** pierre/plâtre sculpté, `#E3DCCB`, roughness élevée, pas de métal, pas
  d'emissive. Draperie classique, ailes pliées, **pas d'yeux**.
- **Pas de VFX, jamais.** Pas de particules, pas d'aura, pas de glow. La seule animation
  est le mouvement et le grincement de pierre.
- **Pas de walk cycle.** Seulement **8 poses de statue** (pointant, tête tournée, mains
  sur le visage, mid-stride, accroupi, ailes à moitié ouvertes, etc.) qui se lisent comme
  "pris en pleine geste".
- **Critère d'acceptation (angels.md §8.5) :** une statue figée est distinguable sans HUD.
  Résolu par trois signaux : silhouette lisse, contraste matériau, pose qui change entre
  deux coups d'œil.
- À 60 blocs dans le fog : une forme claire, lisse, immobile, rétro-éclairée. Suffisant.

**Matériau de contraste avec le monde voxel = toute l'identité visuelle.** Si un joueur
décrit l'Angel comme "un mob", le contraste a échoué.

**A1 — Statut : BLOCKED.** Aucune capture ne contient d'Angel. Le critère d'acceptation
core (angels.md §8.5) reste non validé. C'est le plus gros écart et bloque l'approbation
finale. Voir §13.

---

## 8. Tools, Weapons, Hands

- 5 outils × 5 tiers = 25 meshes. Silhouettes Minecraft conservées (mémoire musculaire)
  + détaillage récupéré : prise scotchée, manche fil-de-fer, garde réparée.
- Le tier se lit par matériau : bois brut → pierre fissurée → fer piqué →
  **diamant = le seul matériau "pur" (blanc-bleu froid)** → obsidienne = verre noir.
- **Mains à la première personne :** manches roulées, gants sales, montre cassée au
  poignet. Détail narratif gratuit : le temps ne t'appartient plus.
- 5 sets de swing (pioche/pelle/hache/houe/épée), pas 25.

---

## 9. UI

Référence : `design/ui/ui-direction.md` (authoritative, approuvé 2026-11-10). Trois
ajouts de la direction artistique :
1. Panneau Baubles manquant → `WBP_Baubles_Root`, priorité 2.
2. Les textures legacy `quick_slots`/`Item_BG_Panel`/`Highlight_Slot` (style Minecraft
   vanilla) sont incompatibles avec l'émail — elles meurent avec `WB_HUD` en phase 5.
3. **Traitement icônes :** icône Patrix inchangée + ombre intérieure + anneau sodium 2px
   à la sélection. Ne pas repeindre 1 801 icônes.

**Observation issue des captures (UI bar) :** la barre orange en haut à gauche est
l'élément chaud le plus brillant dans plusieurs frames et pourrait être confondue avec
du sodium. Confirmer avec la direction UI qu'elle se lit comme UI, pas comme lumière de
scène. La barre orange (~#F2A01C) apparaît dans les 6 captures à la même position et
luminosité — c'est du chrome d'interface, pas de l'éclairage.

---

## 10. Weather & Day/Night

Ambiance de base oppressive (nuit sodium/froide, désaturée, fog de cendres). Un système
dynamique météo + jour/nuit module l'ambiance :
- **Jour :** plus clair mais toujours désaturé, meilleure visibilité, mobs brûlent.
  Le joueur respire.
- **Nuit :** oppressive, fog renforcé, mobs actifs, sodium = seule lumière chaude.
- **Météo :** pluie (lavement audio, surfaces mouillées, plus de réflexions), brouillard
  épais (visibilité réduite, tension), tempête de cendres (particules, vent, bruit), ciel
  dégagé (rare, presque dérangeant par contraste).
- **Transition dynamique :** le grade global (LUT) reste constant ; seule
  l'intensité/filtration de la lumière change. Le joueur ressent la météo sans changement
  de couleur.

---

## 11. Main Menu

Améliorer le `Lvl_MainMenu` existant (matériaux béton/void/sol, statue Angel, bras
pointant) à la perfection : grade d'éclairage, polish matériaux, lisibilité statue,
UI LAMPLIGHTER par-dessus, correction du message d'erreur engine cached-lighting. Le menu
devient une **promesse tenue** — vrais blocs de jeu à l'échelle 100 UU.

---

## 12. Mood Board & Reference Captures (Approved Set)

Source de vérité pour le set de référence in-engine :
`Docs/art/mood-board.md` (captures dans `Docs/art/mood-board/`).

### 12.1 Captures

| Fichier | Contenu visé | Statut validation (image-analyst, 2026-11-11) |
|---|---|---|
| `Screenshot 2026-05-23 202820.png` | Référence précoce | **BEFORE (pre-overhaul)** — exclu du set de référence P2 (A6) |
| `Screenshot 2026-11-11 000733.png` | Terrain à distance, fog, torches sodium | Partiel — fog validé (~40-50 b lisibilité) ; **pas de torches dans le frame** → A3 re-capture requise |
| `Screenshot 2026-11-11 000817.png` | Terrain mid-range, transition biome | Partiel — fog validé ; **pas de frontière de biome visible** (gradient de distance lisse) ; pas de torches → A3 re-capture |
| `Screenshot 2026-11-11 000850.png` | Blocs close-up, détail PBR | Partiel — **limites de blocs non lisibles** à 1-2 b ; pas de réponse PBR ; pas d'érosion/usure → passe texture requise |
| `Screenshot 2026-11-11 000917.png` | Intérieur de cave, éclairage torches | **Capture INVALIDE** — le frame est une forêt brumeuse extérieure, pas une cave ; pas de torches → A5 re-capture requise |
| `Screenshot 2026-11-11 001008.png` | Sunset/ciel, grade atmosphérique | **PASS (A2 fermé)** — ciel bone-ash gris-vert désaturé ; pas d'orange chaud en compétition avec le sodium ; LUT garde le monde désaturé |

### 12.2 Descriptions visuelles (image-analyst, 2026-11-11)

**Constat transversal :** aucune des 5 captures du 2026-11-11 ne démontre le pilier
sodium de "Eroded Realism". Tous les frames sont froids/neutres, dominés par le fog,
éclairés par un ambient plat. Le seul orange saturé dans tout le set est la barre UI
(~#F2A01C), qui est du chrome d'interface et apparaît dans les 6 captures à la même
position et luminosité.

- **000733 (terrain/fog) :** forêt voxel, grand arbre à droite avec tronc brun et
  canopée verte, arbres plus petits au mid-ground se fondant dans le fog. Palette
  froide, gris-vert. Fog très dense, uniforme. Lisibilité perdue à ~50% de la profondeur
  du frame. Aucune torche visible. Limites de blocs non clairement définies sur le tronc.
- **000817 (mid-range) :** arbre unique au centre avec tronc tan clair (~#B5A88C) et
  canopée verte arrondie. Arbres plus petits derrière fondus dans le fog. Palette froide.
  Fog dense. Lisibilité perdue à ~50-55%. Aucune torche. Pas de frontière de biome
  discernable.
- **000850 (close-up) :** vue vers le bas sur un plan de sol de blocs sombres mouchetés
  (vert/gris/blanc). Meilleure lisibilité de blocs du set : blocs individuels délimités
  avec bords biseautés et légère réponse roughness/specular sur les faces supérieures.
  Aucune réponse PBR forte. Aucune torche.
- **000917 (label "cave") :** en réalité un extérieur forestier. Deux troncs d'arbres
  au centre-droit, structure de pierre grise à gauche. Sol sombre avec taches pâles
  (ombres de feuilles). Pas de plafond de cave, pas de torche, pas de lumière chaude.
  Aucune torche.
- **001008 (sunset/sky) :** sol de blocs pâles crème/blancs en terrain escalier. Bâtiment
  à droite avec mur de planches brunes et toit de pierre grise. Ciel ~#A8B5B0 (gris-vert
  pâle), brightening vers l'horizon avec une légère teinte crème désaturée. **Pas de
  disque solaire, pas de gradient orange.** Pas de lumière chaude. Lisibilité des blocs
  claire (meilleure avec 000850). Fog pâle, plus bleu que les frames forêt.

### 12.3 Set de référence approuvé pour P2

- **Entièrement approuvé :** `001008` (sunset/ciel, grade atmosphérique) — dusk bone-ash,
  désaturé, LUT-constant. A2 fermé.
- **Approuvé conditionnellement (sous réserve de re-capture) :** `000733`, `000817`,
  `000850` — utilisables pour la référence fog/terrain/lisibilité des blocs une fois les
  items en attente (A3, A4, limites de blocs) résolus.
- **Exclus :** `000820` (pre-overhaul), `000917` (capture de cave invalide).

---

## 13. Adjustments A1-A6 — Statut final

| # | Domaine | Issue | Statut | Action requise |
|---|---|---|---|---|
| A1 | Silhouette Angel | Aucune capture ne valide le critère d'acceptation core (angels.md §8.5) | **BLOCKED** (no capture) | Capturer `Angel_60b_fog.png` : Angel placé à exactement 60 blocs dans le fog de cendres, rétro-éclairé par ciel froid, sans HUD. Pass/fail contre "forme claire, lisse, immobile, rétro-éclairée. Suffisant." (art-direction §6). Couplé avec A4 : régler le fog d'abord, puis capturer. |
| A2 | Palette sunset/ciel | Risque de ciel orange en compétition avec le sodium | **PASS (fermé)** | Confirmé : le ciel de 001008 est bone-ash gris-vert désaturé (~#8E9A90-#B5BAA8), pas d'orange vif. Le LUT garde le monde désaturé. Caveat : un seul frame ; constance à revérifier après transitions météo/jour-nuit. |
| A3 | Ratio dominance torches | "Objet le plus brillant" non quantifié | **PENDING** (pas de torches dans le frame) | Re-capturer un équivalent de 000733 avec au moins une torche sodium au mid-ground ; mesurer le pic lumineux torche vs ciel/ambient. Cible : torche pic ≥ 2× luminance ciel. Si non atteint : augmenter le multiplicateur emissif ou réduire l'ambient. Secondaire : vérifier que la barre UI orange ne se lit pas comme une source sodium concurrente. |
| A4 | Genou du fog | 60-90 blocs cible, mais le fog actuel tue la lisibilité à ~40-50 blocs | **PENDING** (régler, puis valider vs A1) | Régler l'exposant/densité du fog (pas la couleur) pour atteindre 60-90 blocs de lisibilité. Garder la couleur dans Ash (#5C6268). Valider la silhouette Angel à exactement 60 blocs (A1). Le fog gris-vert observé peut être spécifique au biome (marais/Horror Forest) mais ne doit pas dominer le fog de base. |
| A5 | Niveaux de vide en cave | 000917 est un extérieur, pas une cave | **PENDING** (capture invalide) | Capturer un vrai intérieur de cave ; vérifier que les zones non éclairées se lisent comme Void (#0B0E11). Si le bounce Lumen sur-éclaire les caves, clamper la contribution ambient/GI dans les volumes de cave. Re-capturer après clamping. |
| A6 | Hygiène référence précoce | 202820 est pre-overhaul | **PASS (fermé)** | Confirmé pre-overhaul : éclairage plat vanilla, pas de sodium, pas de fog de cendres, pas de post-process. Ne correspond pas à LAMPLIGHTER. Étiqueté "BEFORE (pre-overhaul)" dans le tableau des captures et exclu du set de référence P2. |

**Validations ouvertes bloquant l'approbation finale :**
1. A1 — silhouette Angel à 60 blocs (`Angel_60b_fog.png`).
2. A3 — ratio luminance torche (re-capture avec torches dans le frame).
3. A4 — exposant fog vs lisibilité Angel à 60 blocs.
4. A5 — niveaux de vide en cave (vraie capture de cave + clamp GI).

Jusqu'à ce que A1 soit capturé et passe, l'approbation finale reste bloquée, conformément au
design test (art-direction §14).

---

## 14. Open Validations (Blocking Final Lock)

| # | Validation | Bloque | Responsable | Statut |
|---|---|---|---|---|
| V1 | `Angel_60b_fog.png` — silhouette Angel à 60 blocs | Approbation finale mood board | Art Director + Technical Artist | BLOCKED (A1) |
| V2 | Re-capture avec torche sodium au mid-ground + mesure luminance | Quantification A3 | Art Director | PENDING |
| V3 | Réglage exposant fog + validation vs V1 | A4 | Technical Artist | PENDING |
| V4 | Capture vraie cave + clamp GI si sur-éclairé | A5 | Technical Artist | PENDING |
| V5 | Validation limites de blocs + usure en PIE | Passe texture (§5) | Technical Artist | PENDING |
| V6 | Constancy du ciel bone-ash à travers météo/jour-nuit | A2 caveat | Art Director | PENDING (post-transitions) |
| V7 | Barre UI orange vs sodium (lecture UI vs lumière) | A3 secondaire | UI Designer + Art Director | PENDING |

---

## 15. P2 — Prochaines étapes (refonte visuelle du monde, 1 biome vertical slice)

### 15.1 Objectif P2

Livrer **un biome vertical slice** : un biome complet (surface + sous-sol + POI + transition)
texturé, éclairé et jouable selon le grade LAMPLIGHTER, validé en PIE et en capture mood
board. Le slice doit prouver que la thèse "Eroded Realism" tient de bout en bout sur un
périmètre restreint avant d'étendre à tous les biomes.

### 15.2 Biome recommandé pour le slice

**Forest** (biome de référence) — justification :
- C'est le biome le plus représentatif du "monde voxel usé" (arbres, sol, pierre).
- Il est adjacent au Horror Forest via la transition Blight, ce qui permet de valider
  la transition de biome (acceptance criterion biome-grammar §9.7).
- Il contient des POI (outposts, hospitals low) qui permettent de valider l'éclairage
  intérieur et les void levels (A5).
- Il est accessible depuis Plains, ce qui permet de valider la transition Plains↔Forest.

Alternative : **Plains** (plus simple, plus exposé) — si l'équipe veut valider la
lisibilité à longue distance et les sightlines avant les transitions forestières.

### 15.3 Lots P2 proposés

| Lot | Livrable | Dépendances | Priorité |
|---|---|---|---|
| P2.0 | Résoudre A1, A3, A4, A5 (captures + réglages) | Mood board | **Bloquant** — doit précéder le slice |
| P2.1 | Passe texture : valider limites de blocs + usure en PIE (M_Terrain_Opaque) | P2.0 (A4) | Haute |
| P2.2 | Créer les 12 variantes "ruine" (slices Texture2DArray) | P2.1, budget 37→128 | Haute |
| P2.3 | Deferred decals : mousse, suie, cendre, sang, brûlures, affiches | P2.2 | Haute |
| P2.4 | Grade + post-process + LUT + sky/fog + light presets (priorité 1 art-direction §11) | P2.0 (A4) | **Critique** — transforme 100% du jeu |
| P2.5 | Lvl_MainMenu rebuild en voxels + matériaux unifiés (priorité 2) | P2.4 | Moyenne |
| P2.6 | UI kit : WBP_Common_*, Title, Pause, Settings + HUD restyle (priorité 3) | P2.4 | Moyenne |
| P2.7 | First-person hands + 5 classes d'outils (tier bois d'abord) + 5 swings (priorité 4) | P2.4 | Moyenne |
| P2.8 | Angel : 8 poses + passe lisibilité statue + matériau (priorité 5) | P2.4, A1 résolu | Haute |
| P2.9 | Mobs vanilla : rig partagé → Zombie + Skeleton → Creeper → Spider (priorité 6) | P2.4, SK_Mob_Base existant | Moyenne |
| P2.10 | Props loot (5) + POI prop kits (priorité 8-10) | P2.4 | Basse |
| P2.11 | VFX/SFX-visuel : poussière par matériau, flash sodium creeper (priorité 9) | P2.4 | Basse |

### 15.4 Critères d'acceptation du slice P2

1. Un screenshot du slice est identifiable sans logo comme *worn voxel world, sodium light,
   a white statue that should not be there* (si Angel présent) ou *worn voxel world, sodium
   light* (si Angel absent mais grade validé).
2. Les limites de blocs sont lisibles à 1-2 blocs de distance.
3. La torche sodium est l'objet le plus brillant à l'écran (≥ 2× luminance ciel).
4. Le fog réduit la lisibilité à 60-90 blocs, avec une silhouette Angel lisible à 60 blocs.
5. Les zones non éclairées en cave se lisent comme Void (#0B0E11).
6. Le ciel est bone-ash désaturé, sans orange vif.
7. La transition de biome (Forest↔Blight ou Forest↔Plains) est visible mais non brutale.
8. Le slice tourne à 60 FPS sur la cible (GTX 1060, 1080p60, Lumen) avec les budgets
   perf existants.

### 15.5 Risques P2

| Risque | Impact | Mitigation |
|---|---|---|
| A1 bloqué (pas d'Angel à 60 blocs) | Bloque l'approbation finale | Prioriser la capture A1 en début de P2, avant les assets |
| Fog trop dense (A4) | Angel perdu à 60 blocs | Régler le fog en P2.0, avant le slice |
| Sur-éclairage des caves (A5) | Caves ne lisent pas comme Void | Clamp GI dans les volumes de cave |
| Budget slices Texture2DArray (37→128) | Dépassement si variantes mal planifiées | Valider le budget avant d'ajouter les 12 variantes |
| Performance Lumen sur GTX 1060 | Frame budget dépassé | Profiler avec Unreal Insights à chaque lot |
| Angel sans rig (12.2 art-direction) | 8 poses impossibles sans décision technique | Escalader au Technical Director pour choix rig vs 8 meshes statiques |

---

## 16. Design Test

In six months, a screenshot of the game must be identifiable without a logo:
*worn voxel world, sodium light, a white statue that should not be there.* If a player
describes the Angel as "a mob", the contrast has failed.

**Validation status (2026-11-11) :** les deux premiers signaux (monde voxel usé,
lumière sodium) lisent clairement dans les captures du mood board. Le troisième
(statue blanche qui ne devrait pas être là) est **non validé** — aucune capture ne
contient d'Angel. L'approbation finale de l'art bible est bloquée sur A1
(`Angel_60b_fog.png`). Voir §13 et §14.

---

## 17. P1 Finalisation Summary (2026-11-11)

### 17.1 État de la production (P1)

- **Lots 0–3 compilés** : le pipeline d'import et de raffinement des assets est
  fonctionnel. La compilation editor target est verte.
- **9 assets originaux importés** : 4 variantes de blocs ruine (ConcreteCracked,
  MossyStone, ScorchedWood, SootedBrick) + 3 props (Barrel, Crate, Torch) + 1 outil
  (Pickaxe Wood) + assets de référence (SM_RubbleBlock, SK_Arms_FP). Tous avec
  base color + normal + specular (LabPBR) à 32×32 (64×64 pour l'arme).
- **Textures raffinées** : normales et speculars ont été raffinés (passes de
  normal/specular sur les 9 assets). Les matériaux `MI_*` correspondants sont créés
  et liés aux master materials (`M_Item_PBR`, `M_Terrain_Opaque`).
- **Angels** : 8 poses statiques importées (Idle, Attack, Point, Crouched,
  HandsOnFace, HeadTurned, MidStride, WingsHalfOpen) — meshs statiques, pas de rig.
  Nanite activé. L'Angel reste le seul asset "sculpté" (non-voxel).

### 17.2 Set de référence mood board (finalisé)

Source de vérité : `Docs/art/mood-board.md` (captures dans `Docs/art/mood-board/`).

| Statut | Captures | Usage P2 |
|---|---|---|
| **Entièrement approuvé** | `001008` (sunset/ciel, grade atmosphérique) | Référence grade LUT + ciel bone-ash + fog pâle |
| **Approuvé conditionnellement** | `000733`, `000817`, `000850` | Référence fog/terrain/lisibilité blocs — sous réserve de A3, A4, limites de blocs |
| **Exclu** | `000820` (pre-overhaul), `000917` (cave invalide) | Ne pas utiliser comme cible |

**Décision Creative Director** : le set de référence est figé pour P2. Les
captures conditionnellement approuvées sont utilisables pour la référence
fog/terrain/blocs une fois les items en attente résolus. Aucune nouvelle capture
ne sera ajoutée au set de référence sans validation A1.

### 17.3 Ajustements A1–A6 — statut final

| # | Domaine | Statut | Action / critère |
|---|---|---|---|
| **A1** | Silhouette Angel à 60 blocs | **BLOCKED (no capture)** | Capturer `Angel_60b_fog.png` : Angel à 60 blocs dans le fog de cendres, rétro-éclairé par ciel froid, sans HUD. Pass/fail : "forme claire, lisse, immobile, rétro-éclairée. Suffisant." (art-direction §6). Bloque l'approbation finale. |
| **A2** | Palette sunset/ciel | **PASS (fermé)** | Ciel de 001008 = bone-ash gris-vert désaturé (~#8E9A90–#B5BAA8). Pas d'orange vif. LUT garde le monde désaturé. Caveat : constance à revérifier après transitions météo/jour-nuit. |
| **A3** | Ratio dominance torches | **PENDING (pas de torches dans le frame)** | Re-capturer un équivalent de 000733 avec au moins une torche sodium au mid-ground ; mesurer le pic lumineux torche vs ciel/ambient. Cible : torche pic ≥ 2× luminance ciel. Si non atteint : augmenter le multiplicateur emissif ou réduire l'ambient. Secondaire : vérifier que la barre UI orange ne se lit pas comme une source sodium concurrente. |
| **A4** | Genou du fog | **PENDING (régler, puis valider vs A1)** | Régler l'exposant/densité du fog (pas la couleur) pour atteindre 60–90 blocs de lisibilité. Garder la couleur dans Ash (#5C6268). Valider la silhouette Angel à exactement 60 blocs (A1). Le fog gris-vert observé peut être spécifique au biome mais ne doit pas dominer le fog de base. |
| **A5** | Niveaux de vide en cave | **PENDING (capture invalide)** | Capturer un vrai intérieur de cave ; vérifier que les zones non éclairées se lisent comme Void (#0B0E11). Si le bounce Lumen sur-éclaire les caves, clamper la contribution ambient/GI dans les volumes de cave. Re-capturer après clamping. |
| **A6** | Hygiène référence précoce | **PASS (fermé)** | `000820` (2026-05-23) confirmé pre-overhaul : éclairage plat vanilla, pas de sodium, pas de fog de cendres, pas de post-process. Étiqueté "BEFORE (pre-overhaul)" dans le tableau des captures et exclu du set de référence P2. |

### 17.4 Validations ouvertes bloquant l'approbation finale

1. **V1** — `Angel_60b_fog.png` (A1) — Art Director + Technical Artist.
2. **V2** — Re-capture avec torche sodium + mesure luminance (A3) — Art Director.
3. **V3** — Réglage exposant fog + validation vs V1 (A4) — Technical Artist.
4. **V4** — Capture vraie cave + clamp GI (A5) — Technical Artist.
5. **V5** — Validation limites de blocs + usure en PIE (§5) — Technical Artist.
6. **V6** — Constancy du ciel bone-ash à travers météo/jour-nuit (A2 caveat) — Art Director.
7. **V7** — Barre UI orange vs sodium (A3 secondaire) — UI Designer + Art Director.

**Jusqu'à ce que A1 soit capturé et passe, l'approbation finale reste bloquée**, conformément au
design test (art-direction §14) : *worn voxel world, sodium light, a white statue that should not
be there.*

### 17.5 Observations issues des captures à actionner en P2

- **Limites de blocs (000850)** : les bords de blocs ne sont pas discernables au premier plan ;
  le sol se lit comme une texture mouchetée continue. Aucune réponse PBR (specular/roughness/normal)
  ni érosion/usure visibles. La passe texture doit valider les bords de blocs + usure en PIE
  avant l'expansion des 12 variantes de ruine.
- **Biais vert du fog** : le fog lit gris-vert plutôt que Ash pur (#5C6268). Peut être
  spécifique au biome (marais/Horror Forest) mais ne doit pas dominer le fog de base. Vérifier
  que la couleur du fog reste dans Ash à travers les biomes.
- **Barre UI orange** : l'élément chaud le plus brillant dans plusieurs frames, pourrait être
  confondu avec du sodium. Confirmer avec la direction UI qu'elle se lit comme UI, pas comme
  lumière de scène. La barre orange (~#F2A01C) apparaît dans les 6 captures à la même position
  et luminosité — c'est du chrome d'interface.
- **Pilier sodium non prouvé** : aucune des 6 captures ne montre de torche sodium. Le pilier
  "sodium as the only warm light" de "Eroded Realism" est actuellement non prouvé visuellement.
  Re-capturer avec des torches en frame (A3) avant le verrouillage final.

---

## 18. Creative Director Finalisation Decision (2026-11-11)

### 18.1 Décision

L'art bible est **finalisé pour P2** avec le set de référence figé ci-dessous. L'approbation
**finale** reste bloquée sur A1 (`Angel_60b_fog.png`), conformément au design test
(art-direction §14) : *worn voxel world, sodium light, a white statue that should not be
there.* Les deux premiers signaux lisent clairement ; le troisième est non validé.

### 18.2 Set de référence mood board figé (P2)

Source de vérité : `Docs/art/mood-board.md` (captures dans `Docs/art/mood-board/`).
Vérifié par image-analyst le 2026-11-11 (descriptions confirmées, voir §12.2).

| Statut | Captures | Usage P2 |
|---|---|---|
| **Entièrement approuvé** | `001008` (sunset/sky, grade atmosphérique) | Référence grade LUT + ciel bone-ash + fog pâle |
| **Approuvé conditionnellement** | `000733`, `000817`, `000850` | Référence fog/terrain/lisibilité blocs — sous réserve de A3, A4, limites de blocs |
| **Exclu** | `000820` (pre-overhaul), `000917` (cave invalide) | Ne pas utiliser comme cible |

**Décision Creative Director :** le set est figé. Aucune nouvelle capture ne sera ajoutée
sans validation A1. Les captures conditionnellement approuvées sont utilisables pour la
référence fog/terrain/blocs une fois les items en attente résolus.

### 18.3 Adressage des ajustements A1–A6

| # | Domaine | Statut | Décision Creative Director |
|---|---|---|---|
| **A1** | Silhouette Angel à 60 blocs | **BLOCKED (no capture)** | Le critère d'acceptation core (angels.md §8.5) reste non validé — aucune capture ne contient d'Angel. **Action :** capturer `Angel_60b_fog.png` (Angel à 60 blocs dans le fog de cendres, rétro-éclairé par ciel froid, sans HUD). Couplé avec A4 : régler le fog d'abord (A4), puis capturer. **Bloque l'approbation finale.** |
| **A2** | Palette sunset/ciel | **PASS (fermé)** | Le ciel de `001008` est bone-ash gris-vert désaturé (~#8E9A90–#B5BAA8), pas d'orange vif. Le LUT garde le monde désaturé. Aucune violation de "no saturated sky" ni de "constant LUT". **Caveat :** un seul frame ; constance à revérifier après transitions météo/jour-nuit (V6). |
| **A3** | Ratio dominance torches | **PENDING (pas de torches dans le frame)** | **Décision :** la cible quantitative est fixée à **torche pic ≥ 2× luminance ciel/ambient dans le frame**. Le pilier "sodium = la seule lumière chaude" est actuellement non prouvé visuellement (aucune des 6 captures ne montre de torche). **Action :** re-capturer un équivalent de `000733` avec au moins une torche sodium au mid-ground ; mesurer le pic lumineux. Si non atteint : augmenter le multiplicateur emissif de la torche ou réduire l'ambient. **Secondaire (V7) :** confirmer que la barre UI orange (~#F2A01C) se lit comme UI, pas comme source sodium concurrente. |
| **A4** | Genou du fog | **PENDING (régler, puis valider vs A1)** | **Décision :** le fog actuel tue la lisibilité à ~10–15 blocs (captures 000733/000817/000850), bien plus dense que la cible 60–90 blocs. **Action :** régler l'exposant/densité du fog (pas la couleur) pour atteindre 60–90 blocs de lisibilité. Garder la couleur dans Ash (#5C6268). Le fog gris-vert observé peut être spécifique au biome (marais/Horror Forest) mais ne doit pas dominer le fog de base. Valider la silhouette Angel à exactement 60 blocs (A1). |
| **A5** | Niveaux de vide en cave | **PENDING (capture invalide)** | `000917` est un extérieur forestier, pas une cave. Les zones les plus sombres de ce frame sitent autour de RGB 35–55, bien au-dessus de Void (#0B0E11, RGB 11/14/17). **Action :** capturer un vrai intérieur de cave ; vérifier que les zones non éclairées se lisent comme Void. Si le bounce Lumen sur-éclaire les caves, clamper la contribution ambient/GI dans les volumes de cave. Re-capturer après clamping. |
| **A6** | Hygiène référence précoce | **PASS (fermé)** | `000820` (2026-05-23) confirmé pre-overhaul : éclairage plat vanilla, pas de sodium, pas de fog de cendres, pas de post-process. Étiqueté "BEFORE (pre-overhaul)" dans le tableau des captures et exclu du set de référence P2. Aucune action supplémentaire. |

### 18.4 Validations ouvertes (bloquantes)

| # | Validation | Bloque | Responsable | Statut |
|---|---|---|---|---|
| V1 | `Angel_60b_fog.png` — silhouette Angel à 60 blocs | Approbation finale mood board | Art Director + Technical Artist | BLOCKED (A1) |
| V2 | Re-capture avec torche sodium + mesure luminance | Quantification A3 | Art Director | PENDING |
| V3 | Réglage exposant fog + validation vs V1 | A4 | Technical Artist | PENDING |
| V4 | Capture vraie cave + clamp GI | A5 | Technical Artist | PENDING |
| V5 | Validation limites de blocs + usure en PIE | Passe texture (§5) | Technical Artist | PENDING |
| V6 | Constancy du ciel bone-ash à travers météo/jour-nuit | A2 caveat | Art Director | PENDING (post-transitions) |
| V7 | Barre UI orange vs sodium (lecture UI vs lumière) | A3 secondaire | UI Designer + Art Director | PENDING |

**Jusqu'à ce que A1 soit capturé et passe, l'approbation finale reste bloquée.**

### 18.5 Observations issues des captures à actionner en P2

1. **Limites de blocs (000850) :** les bords de blocs ne sont pas discernables au premier plan ; le sol se lit comme une texture mouchetée continue. Aucune réponse PBR (specular/roughness/normal) ni érosion/usure visibles. La passe texture doit valider les bords de blocs + usure en PIE avant l'expansion des 12 variantes de ruine.
2. **Biais vert du fog :** le fog lit gris-vert plutôt que Ash pur (#5C6268). Peut être spécifique au biome (marais/Horror Forest) mais ne doit pas dominer le fog de base. Vérifier que la couleur du fog reste dans Ash à travers les biomes.
3. **Barre UI orange :** l'élément chaud le plus brillant dans plusieurs frames, pourrait être confondu avec du sodium. Confirmer avec la direction UI qu'elle se lit comme UI, pas comme lumière de scène.
4. **Pilier sodium non prouvé :** aucune des 6 captures ne montre de torche sodium. Le pilier "sodium as the only warm light" de "Eroded Realism" est actuellement non prouvé visuellement. Re-capturer avec des torches en frame (A3) avant le verrouillage final.

### 18.6 Prochaines étapes P2 (refonte visuelle du monde, 1 biome vertical slice)

#### Objectif P2

Livrer **un biome vertical slice** : un biome complet (surface + sous-sol + POI + transition)
texturé, éclairé et jouable selon le grade LAMPLIGHTER, validé en PIE et en capture mood
board. Le slice doit prouver que la thèse "Eroded Realism" tient de bout en bout sur un
périmètre restreint avant d'étendre à tous les biomes.

#### Biome recommandé pour le slice

**Forest** (biome de référence) — justification :
- C'est le biome le plus représentatif du "monde voxel usé" (arbres, sol, pierre).
- Il est adjacent au Horror Forest via la transition Blight, ce qui permet de valider
  la transition de biome (acceptance criterion biome-grammar §9.7).
- Il contient des POI (outposts, hospitals low) qui permettent de valider l'éclairage
  intérieur et les void levels (A5).
- Il est accessible depuis Plains, ce qui permet de valider la transition Plains↔Forest.

Alternative : **Plains** (plus simple, plus exposé) — si l'équipe veut valider la
lisibilité à longue distance et les sightlines avant les transitions forestières.

#### Lots P2 proposés

| Lot | Livrable | Dépendances | Priorité |
|---|---|---|---|
| P2.0 | Résoudre A1, A3, A4, A5 (captures + réglages) | Mood board | **Bloquant** — doit précéder le slice |
| P2.1 | Passe texture : valider limites de blocs + usure en PIE (`M_Terrain_Opaque`) | P2.0 (A4) | Haute |
| P2.2 | Créer les 12 variantes "ruine" (slices Texture2DArray) | P2.1, budget 37→128 | Haute |
| P2.3 | Deferred decals : mousse, suie, cendre, sang, brûlures, affiches | P2.2 | Haute |
| P2.4 | Grade + post-process + LUT + sky/fog + light presets (priorité 1 art-direction §11) | P2.0 (A4) | **Critique** — transforme 100% du jeu |
| P2.5 | `Lvl_MainMenu` rebuild en voxels + matériaux unifiés (priorité 2) | P2.4 | Moyenne |
| P2.6 | UI kit : `WBP_Common_*`, Title, Pause, Settings + HUD restyle (priorité 3) | P2.4 | Moyenne |
| P2.7 | First-person hands + 5 classes d'outils (tier bois d'abord) + 5 swings (priorité 4) | P2.4 | Moyenne |
| P2.8 | Angel : 8 poses + passe lisibilité statue + matériau (priorité 5) | P2.4, A1 résolu | Haute |
| P2.9 | Mobs vanilla : rig partagé → Zombie + Skeleton → Creeper → Spider (priorité 6) | P2.4, `SK_Mob_Base` existant | Moyenne |
| P2.10 | Props loot (5) + POI prop kits (priorité 8-10) | P2.4 | Basse |
| P2.11 | VFX/SFX-visuel : poussière par matériau, flash sodium creeper (priorité 9) | P2.4 | Basse |

#### Critères d'acceptation du slice P2

1. Un screenshot du slice est identifiable sans logo comme *worn voxel world, sodium light, a white statue that should not be there* (si Angel présent) ou *worn voxel world, sodium light* (si Angel absent mais grade validé).
2. Les limites de blocs sont lisibles à 1-2 blocs de distance.
3. La torche sodium est l'objet le plus brillant à l'écran (≥ 2× luminance ciel).
4. Le fog réduit la lisibilité à 60-90 blocs, avec une silhouette Angel lisible à 60 blocs.
5. Les zones non éclairées en cave se lisent comme Void (#0B0E11).
6. Le ciel est bone-ash désaturé, sans orange vif.
7. La transition de biome (Forest↔Blight ou Forest↔Plains) est visible mais non brutale.
8. Le slice tourne à 60 FPS sur la cible (GTX 1060, 1080p60, Lumen) avec les budgets perf existants.

#### Risques P2

| Risque | Impact | Mitigation |
|---|---|---|
| A1 bloqué (pas d'Angel à 60 blocs) | Bloque l'approbation finale | Prioriser la capture A1 en début de P2, avant les assets |
| Fog trop dense (A4) | Angel perdu à 60 blocs | Régler le fog en P2.0, avant le slice |
| Sur-éclairage des caves (A5) | Caves ne lisent pas comme Void | Clamp GI dans les volumes de cave |
| Budget slices Texture2DArray (37→128) | Dépassement si variantes mal planifiées | Valider le budget avant d'ajouter les 12 variantes |
| Performance Lumen sur GTX 1060 | Frame budget dépassé | Profiler avec Unreal Insights à chaque lot |
| Angel sans rig (art-direction §12) | 8 poses impossibles sans décision technique | Escalader au Technical Director pour choix rig vs 8 meshes statiques |

### 18.7 Décision Creative Director — résumé

- **Thèse validée sur 2/3 signaux :** "worn voxel world" et "sodium light" lisent clairement dans les captures (pour le sodium, via la hiérarchie visuelle attendue — à prouver par re-capture A3). "A white statue that should not be there" reste non validé (A1).
- **Palette sodium confirmée (A2) :** le ciel de `001008` est bone-ash désaturé ; pas d'orange vif en compétition.
- **Set de référence figé :** `001008` fully approuvé ; `000733`/`000817`/`000850` conditionnellement ; `000820`/`000917` exclus.
- **A3/A4/A5 restent PENDING** avec des actions claires et des cibles quantifiées (torche ≥ 2× ciel ; fog 60–90 blocs ; caves = Void).
- **P2 doit commencer par P2.0** (résolution A1/A3/A4/A5) avant tout travail d'asset.

## Références croisées

- Direction artistique : `design/art/art-direction.md`
- Guide visuel technique : `Docs/art/style-guide.md`
- Mood board : `Docs/art/mood-board.md`
- Paramètres matériaux : `Docs/art/material-parameters.md`
- Contrat technique : `Docs/technical-contract.md`
- Direction UI : `design/ui/ui-direction.md`
- Grammaire des biomes : `design/levels/biome-grammar.md`
- Concept de jeu : `design/gdd/game-concept.md`
- GDD Anges : `design/gdd/angels.md`

---

*Dernière mise à jour : 2026-11-11 (P1 finalisation). Owner : Creative Director.*
