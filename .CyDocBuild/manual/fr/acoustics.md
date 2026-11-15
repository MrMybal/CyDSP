---
id: acoustics
title: Chemins, matériaux, pièces et diffraction
description: Rendre chemins et bus de pièces préparés depuis la géométrie de l’hôte.
section: Guides
order: 35
sourceRevision: 1
parent: spatial
---

## Chemins et matériaux {#paths}

AcousticMaterial fournit absorption/perte de transmission trois bandes, épaisseur de référence et diffusion. PreparedAcousticMaterial/PreparedMaterialBand mettent en cache les réglages validés pour les réponses répétées selon l’épaisseur. Ces gains sont des approximations paramétriques, pas un solveur géométrique.

AcousticPathRenderer possède jusqu’à 64 emplacements à ID stable et au plus 10 s de retard préparé. Il partage l’historique mono et expose un canal par emplacement, avec retard, direction d’arrivée et gains trois bandes. L’entrée multicanale est moyennée ; gardez les émetteurs indépendants séparés. Retirer un chemin le fait disparaître progressivement avant libération, sans vol de chemin audible.

## Pièces, portails et retours {#rooms}

L’application fournit pièces, arêtes/ouvertures/pertes de portails et cellules de réponse. AcousticPortalGraph prépare les routes ; RoomCellGrid interpole une grille régulière. CyDSP ne lance pas de ray tracing et ne parcourt pas les acteurs du jeu.

RoomReverbBank conserve jusqu’à 32 réponses. Le cycle de bloc est beginBlock, un ou plusieurs send, render, puis mix ou lecture de roomOutput. Continuez à rendre les pièces sans send pour avancer les queues. L’application choisit les sends avant/après occlusion et les gains de retour par auditeur.

## Diffraction {#diffraction}

DiffractionPath décrit détour de bord et conditions d’ouverture depuis la géométrie de l’hôte. diffractionGain évalue une approximation sur écran mince dépendante de la fréquence ; DiffractionFilter applique sa réponse par bandes. Ce n’est pas un solveur d’ondes ou d’obstacles arbitraires. Conservez distances, ouverture et fréquences dans les unités de l’API et évaluez graves/aigus dans la scène visée.

## API publique {#api}

- [cydsp/spatial/AcousticPaths.h](doc:api-spatial-acousticpaths)
- [cydsp/spatial/AcousticRooms.h](doc:api-spatial-acousticrooms)
- [cydsp/spatial/Diffraction.h](doc:api-spatial-diffraction)
