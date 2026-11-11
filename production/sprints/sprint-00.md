# Sprint 00 — Phase P0: Audit & Cadrage

**Duration:** 2-3 weeks
**Goal:** Freeze the target before producing anything. Art bible, IP decision,
measured performance baseline, frozen technical contract, CI + tests.
**Owner:** Producer (coordination), Technical Director (technical), Creative
Director (art), Technical Artist (pipeline)

## Tasks

### T1 — Art bible v1 (Creative Director + Art Director) — PARTIELLEMENT FAIT
- [x] Palette verrouillée (hex + rôles) — design/art/art-direction.md §2
- [x] Règles de couleur non négociables — design/art/art-direction.md §3
- [x] Style des mobs (low-poly stylisé, non-voxel) — design/art/art-direction.md §5
- [x] Style des outils/armes (silhouettes Minecraft + détail bricolé) — design/art/art-direction.md §7
- [x] Style de l'Ange (pierre, aucune VFX, 8 poses) — design/art/art-direction.md §6
- [ ] Captures in-engine de référence (mood board)
- [ ] Validation par le joueur
- **Gate:** art bible approuvée par le joueur (validation restante)

### T2 — Décision IP Patrix (Producer + Creative Director) — PARTIELLEMENT FAIT
- [x] Recherche de packs CC0/libres (Docs/art/cc0-research.md) — 8 packs analysés, aucun ne fit parfaitement
- [x] Décision : art original (D2=A) — validée par le joueur
- [ ] Registre de provenance des assets (Docs/assets-provenance.md) — à créer
- [x] Plan de création : étudier Kenney/Cole Foster/Screaming Brain comme référence, ré-authorer
- **Gate:** route IP choisie et documentée (art original) ✅

### T3 — Baseline performance (Performance Analyst + Technical Director) — PARTIELLEMENT FAIT
- [x] Plan de baseline (Docs/performance/baseline-plan.md) — 8 scénarios, protocole, seuils, gate CI
- [ ] Mesurer avec UVoxelBenchSubsystem (nécessite l'éditeur ouvert + PIE)
- [ ] Captures Unreal Insights
- [ ] Rapport Markdown : frame time p50/p95/p99, draw calls, mémoire, VRAM
- [ ] Identifier les hitches et les quick wins
- **Gate:** baseline chiffrée documentée (plan écrit, mesures à faire)

### T4 — Contrat technique gelé (Technical Director) — PARTIELLEMENT FAIT
- [x] Vertex attribute contract (Docs/technical-contract.md §1) — documenté
- [x] M_Global / M_Terrain_Opaque/Masked — split planifié (ADR-0004)
- [x] Nommage des assets (Docs/technical-contract.md §3) — documenté
- [x] LOD policy (Docs/technical-contract.md §4) — documenté
- [x] Collision policy (Docs/technical-contract.md §5) — documenté
- [x] Écrire ADR 0004 (politique de rendu)
- [x] Écrire ADR 0005 (architecture combat)
- [x] Écrire ADR 0006 (présentation équipement)
- [x] Écrire ADR 0007 (modèle de données outils) — CRÉÉ
- [x] Écrire ADR 0008 (pipeline Blender → UE)
- [x] Écrire ADR 0009 (architecture mobs)
- [x] Écrire ADR 0010 (squelettes mobs)
- [x] Contrat technique (Docs/technical-contract.md) — CRÉÉ
- [ ] Validation du contrat par le joueur
- **Gate:** contrat technique approuvé, ADRs 0004-0010 écrits (validation joueur restante)

### T5 — CI + tests (QA Lead + DevOps)
- [ ] Configurer CI (GitHub Actions ou équivalent)
- [ ] Tests des systèmes touchés (voxel, crafting, vitals, inventory, mobs, save/load)
- [ ] Validation automatisée des assets (nommage, dimensions, compression)
- **Gate:** CI verte, tests passent

### T6 — Pipeline Blender v1 (Technical Artist)
- [ ] Script d'export Blender (échelle, axes, nommage)
- [ ] Presets d'import FBX (FbxImportOptions)
- [ ] Script de validation (échelle, pivot, nommage, LOD, matériaux)
- [ ] 1 asset de référence de bout en bout (Blender → UE → validation)
- **Gate:** asset de référence importé et validé

### T7 — Risk register (Producer) — FAIT
- [x] Registre de risques initial (production/risk-register.md) — CRÉÉ
- [x] Mitigations pour les risques critiques
- [ ] Validation par le joueur
- **Gate:** risk register approuvé (validation restante)

## Livrables P0

1. design/art/art-direction.md (approuvé)
2. Docs/architecture/adr-0004..0010.md (écrits)
3. production/risk-register.md
4. Baseline perf mesurée (rapport + CSV)
5. Contrat technique gelé (Docs/technical-contract.md)
6. Pipeline Blender v1 (scripts + asset de référence)
7. CI configurée
8. Recherche CC0 (rapport)

## Gates de sortie P0

- Art bible approuvée
- Route IP choisie et documentée
- Baseline perf chiffrée
- Contrat technique gelé
- ADRs 0004-0010 écrits
- CI verte

## Durée estimée : 2-3 semaines

## Parallélisation

- T1 (art bible) ∥ T2 (IP) ∥ T3 (baseline) ∥ T4 (contrat) — indépendants
- T5 (CI) dépend de T4
- T6 (pipeline Blender) dépend de T4 (contrat)
- T7 (risk register) dépend de T1-T4

## Sub-agents mobilisés

- creative-director : art bible, direction artistique
- art-director : specs d'assets, palette verrouillée
- technical-director : ADRs, contrat technique, perf
- technical-artist : pipeline Blender, validation assets, recherche CC0
- performance-analyst : baseline, benchmarks
- qa-lead : plan de test, CI
- devops-engineer : CI, LFS, .gitattributes
- producer : sprint plan, risk register, coordination