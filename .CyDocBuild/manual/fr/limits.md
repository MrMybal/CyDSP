---
id: limits
title: Plateformes, capacités et limites des modèles
description: Distinguer algorithmes disponibles, cibles et hypothèses des modèles.
section: Guides
order: 41
sourceRevision: 1
parent: overview
---

## Cibles {#platforms}

| Cible | Périmètre actuel |
| --- | --- |
| Windows x64 | Couverture en exécution MSVC/clang-cl ; chemins SSE2/AVX2/AVX-512 compilés selon CPU/OS. |
| Android ARM64 | Couverture en exécution sur Quest 3 avec NEON ; il ne possède pas SVE. |
| Windows ARM64 | Couverture de compilation statique ; exécution sur appareil à valider. |
| ARM64 SVE | Compilation/liaison Android ; exécution matérielle selon les longueurs vectorielles à valider. |
| macOS / Linux desktop | Cibles de sources ; exécution à valider pour cette version. |
| x86 / ARM32 | Chemins de compatibilité existants ; non réexécutés pour cet état. Les noyaux double ARM32 peuvent revenir au scalaire. |

Les ABI ne sont pas interchangeables. La version 0.4.0 est en développement ; un déploiement exige son évaluation de contenu/performance. Le coût dépend du processeur, des dimensions, de la qualité, du backend et des frontières de blocs.

## Capacités préparées {#capacities}

| Domaine | Limite importante |
| --- | --- |
| Processeurs étendus usuels | Généralement 8–768 kHz, 1–64 canaux, bloc maximal jusqu’à 65536 frames ; certaines API sont plus petites. |
| SourceBatch | 1024 sources, 64 sorties et budgets explicites de stockage/banques. |
| Granulator | Jusqu’à 4096 emplacements ; aucun vol de voix implicite. |
| Chemins / pièces | 64 chemins et 10 s de retard préparé ; jusqu’à 32 réponses de pièces. |
| Ambisonics | Ordres 0–7, 1–64 canaux ACN/SN3D. |
| HRTF | Défaut 2048 directions/32768 frames d’entrée/2048 taps ; bornes de référence explicites 100000/65536/65536. Voir [SOFA](doc:sofa). |
| Graphes | MultiBusGraph : 256 nœuds, 4096 connexions et 32 bus/disposition. |

## Interpréter les modèles {#models}

Aucun jeu HRTF n’est fourni ; sa provenance/licence et son adaptation à l’auditeur relèvent de l’application. Binaural analytique, diffraction sur écran mince, transferts de pièces, physique des contacts et estimations musicales sont des modèles réduits/heuristiques. True peak et sonie ne sont pas présentés comme des produits broadcast certifiés. L’annulation d’écho est linéaire et exige horloges/référence alignées. Choisissez les limites de chaque configuration et évaluez des contenus réalistes sans extrapoler depuis un signal synthétique unique.
