---
id: binaural
title: Rendu binaural et sur enceintes
description: Choisir binaural analytique, HRIR mesurées ou dispositions VBAP.
section: Guides
order: 36
sourceRevision: 1
parent: spatial
---

## Rendu au casque {#headphones}

AnalyticBinaural modélise ITD de tête sphérique, ombrage, coloration avant/arrière et proximité d’oreille pour jusqu’à 32 entrées placées. Le rayon de source réduit le détail directionnel. Deux entrées peuvent représenter un émetteur stéréo étendu. C’est un modèle paramétrique, pas une HRTF mesurée ou personnalisée.

HrirBinaural rend les réponses mesurées. Chaque HrirPoint brut fournit des FIR gauche/droite sans onset et des retards d’oreilles séparés au taux préparé. Garder l’onset acoustique dans les FIR et appliquer les retards séparés peut produire filtrage en peigne ou retard doublé. Préférez un [HrtfSet préparé](doc:hrtf) pour convertir/partager les mesures.

setDirection interpole filtres alignés et retards, puis fond les lecteurs. Une demande pendant le fondu retourne transition_in_progress. Conservez la dernière direction acceptée jusqu’à acceptation de la nouvelle ; ne bloquez pas le thread audio en attente.

## Dispositions d’enceintes {#speakers}

VbapLayout prépare paires 2D ou triplets d’enveloppe convexe 3D pour 2–64 enceintes et retourne des gains à énergie unitaire. Hors couverture, choisissez erreur ou repli vers l’enceinte proche à la préparation. Une disposition clairsemée/mal répartie ne reproduit pas toutes les directions également.

Choisissez les [Ambisonics](doc:ambisonics) pour partager un champ intermédiaire entre sources. Panoramique et indices binauraux sont des rendus différents ; appliquer les deux involontairement peut changer niveau et localisation.

## API publique {#api}

- [cydsp/spatial/SpatialRendering.h](doc:api-spatial-spatialrendering)
