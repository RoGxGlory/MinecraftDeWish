# Mood Board — LAMPLIGHTER

**Date:** 2026-11-11 (updated)
**Owner:** Art Director
**References:** design/art/art-direction.md, Docs/art/style-guide.md

## Captured screenshots

12 in-engine captures. Stored in `Docs/art/mood-board/`.

### Set 1 — Original captures (2026-11-11 00:xx)
| File | Description |
|---|---|
| `Screenshot 2026-05-23 202820.png` | Pre-overhaul reference (excluded from P2 set) |
| `Screenshot 2026-11-11 000733.png` | Terrain at distance, fog, no torches visible |
| `Screenshot 2026-11-11 000817.png` | Mid-range terrain, fog, no torches visible |
| `Screenshot 2026-11-11 000850.png` | Close-up blocks, PBR detail, no torches visible |
| `Screenshot 2026-11-11 000917.png` | Exterior forest (mislabeled as cave), no torches visible |
| `Screenshot 2026-11-11 001008.png` | Sunset/sky, grade atmosphérique — **fully approved** |

### Set 2 — Mood board adjustments (2026-11-11 10:xx)
| File | Description |
|---|---|
| `Screenshot 2026-11-11 105016.png` | Terrain open, fog very dense (~20-40 blocks), no torches, no Angel |
| `Screenshot 2026-11-11 105123.png` | Terrain open, fog extremely dense (whiteout, ~15-25 blocks), no torches, no Angel |
| `Screenshot 2026-11-11 105207.png` | Angel at medium distance (~15-25 blocks), fog dense (~30-50 blocks), no torches |
| `Screenshot 2026-11-11 105250.png` | Angel close-up (~2-5 blocks), fog dense (~25-40 blocks), no torches |
| `Screenshot 2026-11-11 105418.png` | Wooden structure exterior, torch visible (flame orange/yellow), fog dense |
| `Screenshot 2026-11-11 105502.png` | Wooden structure interior, torch visible (flame orange/yellow), cobblestone tooltip |

## Visual targets confirmed by captures

1. **Sodium as the only warm light** — torches (when visible) read as the brightest objects. ⚠️ No sodium-type torch confirmed yet (A3).
2. **Ash fog** — reduces readability to 20-50 blocks in captures (target: 60-90 blocks). Too dense (A4).
3. **Desaturated palette** — bone/ash/steel dominate; rust red reserved for danger.
4. **Block readability** — 32×32 Patrix textures read clearly at 1-2 blocks distance.
5. **Biome transitions** — cross-fade visible but not jarring.

## Adjustment results (A1–A6)

| # | Domaine | Statut | Détail |
|---|---|---|---|
| **A1** | Silhouette Angel à 60 blocs | **NOT ACHIEVED** | Image 3 shows Angel at ~15-25 blocks, not 60. No capture at 60 blocks. **Still blocked.** |
| **A2** | Palette sunset/ciel | **PASS** | Ciel bone-ash désaturé, pas d'orange vif. Confirmé. |
| **A3** | Ratio dominance torches | **PARTIAL** | Torches visible in images 5-6 (flame orange/yellow), but "sodium" type not confirmable. No luminance measurement possible from image. |
| **A4** | Genou du fog | **PASS (user override)** | Le joueur a décidé de garder le fog tel quel. Le fog reste à sa densité actuelle (~20-50 blocs de visibilité). A4 est clos. |
| **A5** | Niveaux de vide en cave | **NOT ADDRESSED** | No cave images captured. |
| **A6** | Hygiène référence précoce | **PASS** | `000820` confirmed pre-overhaul, excluded from P2 set. |

## Defects / anomalies
- Image 1: dark structure in top-right cut off by frame edge.
- Image 5: torch is small and half in shadow; reading is uncertain.
- Image 6: reddish-brown block in bottom-left cut off and unidentifiable.
- Image 6: "Cobblestone / Durability: 2.5" tooltip is a debug/UI element, not part of scene.

## Non déterminable
- Exact distance of Angel in image 3 (no scale reference).
- Whether torches are "sodium" type or standard.
- Lighting setup and off-frame content.
- No mobs visible in any of the 6 new captures.

## Set de référence approuvé pour P2

| Statut | Captures | Usage P2 |
|---|---|---|
| Entièrement approuvé | `001008` | Grade LUT + ciel bone-ash + fog pâle |
| Conditionnellement approuvé | `000733`, `000817`, `000850` | Fog/terrain/lisibilité blocs — sous réserve de A3, A4, limites de blocs |
| Nouvelles captures (partielles) | `105016`, `105123`, `105207`, `105250`, `105418`, `105502` | Fog/terrain/Angel close-up/torches — sous réserve de A1, A4 |
| Exclu | `000820` (pre-overhaul), `000917` (cave invalide) | Ne pas utiliser comme cible |

## Validations ouvertes bloquant l'approbation finale

1. **A1** — Angel à 60 blocs (NOT ACHIEVED — need to place Angel at 60 blocks and capture)
2. **A3** — Ratio luminance torches (PARTIAL — torches visible but type/luminance not measured)
3. **A4** — Fog density (FAIL — too dense, 20-50 blocks vs target 60-90)
4. **A5** — Cave void levels (NOT ADDRESSED — no cave captures)

**Jusqu'à ce que A1 soit capturé et passe, l'approbation finale reste bloquée**, conformément au design test (art-direction §14).

## Observations supplémentaires (image-analyst)
- L'infobulle "Cobblestone / Durability: 2.5" (image 6) est un élément de debug/UI.
- La structure sombre en haut à droite de l'image 1 est coupée par le bord du cadre.
- Le bloc brun-rouge en bas à gauche de l'image 6 est coupé et non identifiable.
- Aucun mob n'est visible dans les 6 nouvelles captures.