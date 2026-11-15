---
id: overview
title: CyDSP 0.3.2
description: Choisir et assembler les traitements audio numériques.
section: Guides
order: 0
sourceRevision: 1
---

## Ce que vous pouvez construire {#purpose}

CyDSP est une bibliothèque C++20 de traitement numérique du signal audio. Ses processeurs float/double s’utilisent dans un effet, un synthétiseur, un DAW, un jeu vidéo ou un moteur audio. Elle fournit filtres, dynamique, convolution, traitements spectraux, synthèse, sampling, analyse, restauration et son 3D avec un stockage préparé et une API planaire commune.

Commencez par l’[installation](doc:installation), puis une [première chaîne de traitement](doc:quickstart). Lisez les [contrats de traitement](doc:contracts) avant de raccorder un callback temps réel. La [référence de l’API](doc:api) couvre tous les en-têtes publics, avec champs de configuration, signatures exactes et arguments par défaut.

## Trouver le bon domaine {#domains}

| Objectif | Guide |
| --- | --- |
| Filtrer, égaliser et contrôler les niveaux | [Filtres](doc:filters), [dynamique](doc:dynamics), [traitements non linéaires](doc:advanced-dynamics) |
| Réverbérer ou traiter des impulsions | [Convolution](doc:convolution), [choix des partitions](doc:convolution-options), [réverbération](doc:reverb) |
| Générer ou transformer un son | [Synthèse](doc:synthesis), [samples](doc:sampling), [granulation](doc:granular), [hauteur/durée](doc:pitch) |
| Examiner ou réparer l’audio | [Mesureurs](doc:metering), [analyse](doc:analysis), [diagnostic](doc:diagnostics), [restauration](doc:restoration) |
| Rendre l’audio d’un jeu dans l’espace | [Coordonnées](doc:spatial), [acoustique](doc:acoustics), [binaural](doc:binaural), [Ambisonics](doc:ambisonics) |

## Intégrer la bibliothèque {#integration}

CyDSP traite l’audio en mémoire. L’application fournit le callback audio, possède les flux et gère les services qui les entourent. CyAudioCore est un projet utilisant son API publique. CyDSP peut être lié directement à une autre application.

La version 0.3.2 est en développement. Les [limites des plateformes et modèles](doc:limits) distinguent les algorithmes disponibles des cibles restant à valider en exécution. Conservez les [notices MIT et tierces](doc:licensing) avec les logiciels redistribués.

## API publique {#api}

- [cydsp/CyDSP.h](doc:api-cydsp)
