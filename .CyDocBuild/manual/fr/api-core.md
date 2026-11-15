---
id: api-core
title: Socle et stockage
description: Socle et stockage
section: Référence API
order: 1001
sourceRevision: 1
parent: api
---

| En-tête | Rôle |
| --- | --- |
| [AudioBlock.h](doc:api-core-audioblock) | Vues planaires empruntées, sous-blocs et validation des formes et recouvrements. |
| [AudioBuffer.h](doc:api-core-audiobuffer) | Stockage planaire propriétaire, initialisé à zéro et aligné sur 64 octets, préparé hors callback. |
| [AudioLayout.h](doc:api-core-audiolayout) | Conversion sans allocation entre blocs planaires et spans entrelacés par frame. |
| [CircularHistory.h](doc:api-core-circularhistory) | Un historique préparé partagé par des lecteurs entiers et fractionnaires indépendants. |
| [Cpu.h](doc:api-core-cpu) | Détection des capacités CPU/OS, backends compilés et noms des backends. |
| [Memory.h](doc:api-core-memory) | Détection du recouvrement des plages mémoire empruntées. |
| [SpscQueue.h](doc:api-core-spscqueue) | Communication à capacité fixe entre un producteur et un consommateur, avec stockage conservé. |
| [Types.h](doc:api-core-types) | Types d’échantillons, dimensions de traitement, états et politiques de calcul. |
| [Version.h](doc:api-core-version) | Constantes et macros de version pour faire correspondre les en-têtes et la bibliothèque. |
