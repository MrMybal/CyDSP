---
id: reverb
title: Réverbérations algorithmiques et hybrides
description: Choisir Schroeder, FDN, plaque, ressort ou IR/FDN.
section: Guides
order: 16
sourceRevision: 1
parent: overview
---

## Choisir une topologie {#topology}

Reverb fournit un réseau de type Schroeder. Diffuser combine des étages passe-tout ; orthogonalMix propose Hadamard/Householder. FdnReverb utilise 4/8/16/32 lignes à rétroaction orthogonale et 1–8 canaux, avec RT60 nominales par bandes, prédélai, diffusion et amortissement.

PlateReverb est une réalisation originale d’une topologie de plaque en huit, avec diffusion d’entrée et réservoirs de retards croisés. Son shimmer injecte dans le feedback un signal à double tête transposé d’une octave. SpringReverb est un guide d’ondes dispersif à rétroaction, pas une simulation mécanique par éléments finis.

## Décroissance, freeze et IR hybride {#decay}

Les RT60 décrivent un modèle nominal. Les bornes passives conservatrices peuvent raccourcir les décroissances extrêmes des bandes FDN. Le freeze désactive l’injection selon le contrat et peut conserver une queue indéfiniment. Fournissez des zéros pour faire avancer le temps ; reset supprime volontairement la queue.

HybridReverb combine une IR précoce préparée avec une queue FDN. Préparez une nouvelle impulsion hors traitement, puis démarrez sa transition déjà réservée entre blocs. Budgétez les deux états pendant les fondus. RoomReverbBank conserve plusieurs réponses de pièces et leurs queues ; voir l’[acoustique des pièces](doc:acoustics).

ImpulseAnalysis estime les décroissances et peut proposer des réglages FDN nominaux. Il rapproche certaines métriques, pas toutes les caractéristiques d’une pièce mesurée.

## API publique {#api}

- [cydsp/processors/Diffusion.h](doc:api-processors-diffusion)
- [cydsp/spatial/FdnReverb.h](doc:api-spatial-fdnreverb)
- [cydsp/spatial/HybridReverb.h](doc:api-spatial-hybridreverb)
