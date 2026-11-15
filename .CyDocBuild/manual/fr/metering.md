---
id: metering
title: Niveaux, true peak et sonie
description: Interpréter peak/RMS, crêtes interéchantillons et historique LUFS borné.
section: Guides
order: 28
sourceRevision: 1
parent: overview
---

## Niveaux et corrélation {#levels}

LevelMeter conserve une fenêtre glissante exacte préparée : peak, RMS, moyenne/DC et crête retenue depuis reset. Le démarrage utilise les échantillons réellement reçus. Le dBFS vaut 20log10(amplitude) ; le silence donne moins l’infini. Le facteur de crête est peak/RMS en dB, zéro pour le silence. Aucune décroissance d’affichage n’est ajoutée.

La corrélation stéréo utilise le produit croisé normalisé des deux premiers canaux, DC compris, borné à [-1,1] ; canaux manquants/silencieux donnent zéro. L’hôte gère moyennage d’affichage et publication d’instantanés.

## True peak {#true-peak}

TruePeakMeter utilise une reconstruction sinc fenêtrée 4x/8x et 16/32/64/128 taps par phase. Les maxima restent jusqu’au reset. Les crêtes d’échantillons sont immédiates ; les crêtes reconstruites ont une latence nominale d’un demi-nombre de taps. Appelez flush en fin de flux. Il estime les maxima interéchantillons dans son modèle de filtre/phases ; ce n’est pas une borne analogique universelle certifiée.

## Sonie et durée de session {#loudness}

LoudnessMeter applique la pondération K, des fenêtres de 400 ms momentanées et 3 s à court terme. La sonie intégrée utilise les gates absolu -70 LUFS et relatif -10 LU ; la LRA utilise -70/-20 et les percentiles 10–95. Au-delà du mono/stéréo, fournissez des poids explicites ; faites vous-même la correspondance LFE/surround.

maximumGatingBlocks réserve l’historique exact à la préparation (288000 par défaut, environ huit heures par pas de 100 ms). Si un bloc complet dépasserait la capacité, process retourne would_block sans le consommer. Aucun historique glissant approximatif n’est substitué. Préparez une capacité suffisante ou arrêtez/réinitialisez volontairement.

levels a un coût constant. integratedLufs parcourt l’historique et loudnessRangeLu trie aussi le scratch conservé : interrogez-les hors callback critique, de façon sérialisée. Les définitions mathématiques sont implémentées ; aucune certification/conformité exhaustive n’est revendiquée.

## API publique {#api}

- [cydsp/analysis/LevelMeter.h](doc:api-analysis-levelmeter)
- [cydsp/analysis/LoudnessMeter.h](doc:api-analysis-loudnessmeter)
- [cydsp/analysis/TruePeakMeter.h](doc:api-analysis-truepeakmeter)
