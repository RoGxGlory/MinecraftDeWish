# Weeping Angel — prêt pour Unreal Engine

## Contenu du dossier

| Fichier | Rôle |
| --- | --- |
| `WeepingAngel.blend` | Source nettoyée : 3 poses, pivots aux pieds, 1,85 m, matériaux branchés, script d'export intégré (Text Editor → `export_to_unreal.py`) |
| `SM_WeepingAngel_Idle.fbx` | Static mesh — pose repos |
| `SM_WeepingAngel_Attack.fbx` | Static mesh — pose attaque |
| `SM_WeepingAngel_Point.fbx` | Static mesh — pose « pointing » |
| `textures/` | 6 textures 1024×1024 : 3 base color + 3 normal maps |
| `source/WeepingAngel_original.blend` | Sauvegarde du fichier d'origine (non modifié) |
| `preview_poses.png` | Aperçu des 3 poses |

Chaque FBX a son **pivot aux pieds**, à l'**origine monde**, et mesure **1,85 m** de haut.

## Réglages d'import (Unreal Engine 5.8)

1. Glisser un FBX dans le Content Browser.
2. Options d'import :
   - **Static Mesh**
   - **Convert Scene** : ON
   - **Convert Scene Unit** : ON
   - **Combine Meshes** : OFF (un FBX = une pose)
   - **Generate Lightmap UVs** : OFF avec Lumen, ON si éclairage baké
   - **Nanite** : OFF (~7 100 tris par pose, pas utile)
   - **Import Materials** : ON (crée les 3 slots) ou OFF pour créer les matériaux à la main
3. Vérifier après import : hauteur ≈ **185 cm**, pivot aux pieds.

## Textures

| Texture | sRGB | Compression |
| --- | --- | --- |
| `angelbody.png`, `angelface.png`, `angelwings.png` | ON | Default (BC) |
| `angelbody_nrm.png`, `angelface_nrm.png`, `angelwings_nrm.png` | OFF | **Normalmap** |

Pas de canal alpha : aucun matériau en mode *Masked* / *Translucent* nécessaire.

## Matériaux Unreal

Créer trois matériaux et les affecter aux slots du static mesh :

- `M_WeepingAngel_Body` → `angelbody` + `angelbody_nrm`
- `M_WeepingAngel_Face` → `angelface` + `angelface_nrm`
- `M_WeepingAngel_Wings` → `angelwings` + `angelwings_nrm`

Brancher la base color sur `Base Color` et la normal map sur `Normal` (Normal Map node). Métallic 0, Roughness ≈ 0,8.

## Notes techniques

- **Orientation** : le modèle regarde vers **-Y** dans Blender. Export en Forward **-Z** / Up **Y** + *Convert Scene* : il doit arriver debout, face au forward Unreal (+X). À vérifier au premier import.
- **UV du corps en miroir** (U négatif) : si les normal maps semblent inversées sur une moitié du corps, inverser le canal vert (`Flip Green Channel`) ou corriger les UV.
- **Visage** : 30 arêtes ouvertes forment un petit trou (~3 cm, probablement la bouche). À boucher si visible de près.
- **Poids** : corps ≈ 2 250 tris, visage ≈ 2 970, ailes ≈ 1 904 par pose (total ≈ 7 100 tris).
- **Armature d'origine** : supprimée (rig factice de 2 os, sans animation). Les poses sont des maillages statiques.
