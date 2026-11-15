---
id: hrtf
title: Préparer et partager les HRTF
description: Séparer les onsets, convertir les FIR et partager les banques préparées immuables.
section: Guides
order: 37
sourceRevision: 1
parent: binaural
---

## Mesures en mémoire {#prepare}

HrtfSet accepte jusqu’à 2048 directions distinctes et 16384 frames d’entrée par oreille. L’application charge les données et fournit leur licence. Aucun jeu HRIR ni décodeur SOFA n’est inclus. Par défaut, le premier dépassement d’un seuil relatif au pic (-40 dB) définit un onset entier ; les retards fractionnaires fournis restent séparés.

La préparation retire l’onset et rééchantillonne les FIR par sinc fenêtrée si nécessaire. Les taux vont de 8 à 768 kHz et le ratio de conversion de 1/8 à 8. La conversion conserve le gain DC de l’IR et expose sa latence causale commune via latencyFrames. Les onsets acoustiques directionnels restent distincts de cette latence.

## Capacité et partage {#sharing}

La longueur FIR préparée est configurable jusqu’à 2048 taps. Vérifiez truncated(), notamment après conversion. Les bornes de queue des renderers comprennent FIR, retards acoustiques et latence de conversion.

Les renderers acquièrent un état préparé immuable partagé pendant prepare ; leurs historiques/fondus restent indépendants. Repréparer ou détruire le HrtfSet original n’invalide pas les renderers déjà préparés. Ce contrat diffère de l’emprunt brut de SampleData/Wavetable.

L’interpolation directionnelle utilise des poids Shepard sphériques normalisés à localité gaussienne. Une direction mesurée reproduit son point. angularWidthRadians règle la localité, pas la densité des mesures ou l’adaptation à l’auditeur. La détection d’onset n’est pas une calibration fractionnaire. Évaluez mouvements, couverture clairsemée et indices propres à l’auditeur sur des contenus réels.
