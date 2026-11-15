---
id: sources
title: Sources nombreuses et données partagées
description: Rendre des emplacements fixes avec banques de samples/tables partagées.
section: Guides
order: 25
sourceRevision: 1
parent: synthesis
---

## Capacités et durées de vie {#capacity}

SourceBatch prépare jusqu’à 1024 emplacements fixes, 64 canaux de sortie, 256 banques de samples et 256 banques de tables, avec une limite interne de 2^24 valeurs rendues de voix. La préparation copie les pointeurs, pas les données. Gardez objets et contenus stables et immuables jusqu’à l’arrêt de tous les lecteurs.

Chaque BatchSource choisit sinus, table ou grain, puis phase/fréquence/gain, départ différé et gains de sortie. Un emplacement occupé n’est pas volé ; start retourne would_block. L’hôte choisit la politique de vol/réessai et possède la correspondance avec les entités de scène.

## Rendu et placement {#render}

Les sinus utilisent des récurrences complexes vectorisées entre voix et une renormalisation périodique. Les tables emploient les mips harmoniques ; les grains réutilisent interpolation/boucles de SampleData avec une enveloppe Hann. Recherche et positions de lecture restent partiellement scalaires. La somme exige de la réserve de niveau.

setPosition à l’ordre -1 fournit un pan stéréo à puissance constante et une atténuation 1/max(1,distance). Les ordres 0–7 exigent exactement `(order+1)^2` sorties ACN/SN3D. Le pan stéréo seul ne donne pas les indices d’élévation HRTF. Utilisez la [convention spatiale](doc:spatial#coordinates) et le [rendu binaural/HOA](doc:binaural) pour ces usages.

## API publique {#api}

- [cydsp/synthesis/SourceBatch.h](doc:api-synthesis-sourcebatch)
