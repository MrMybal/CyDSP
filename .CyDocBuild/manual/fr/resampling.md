---
id: resampling
title: Rééchantillonnage et suréchantillonnage
description: Gérer consommation variable, ratios, queues FIR et bancs multirates.
section: Guides
order: 21
sourceRevision: 1
parent: overview
---

## La consommation n’est pas la taille du bloc {#counts}

Resampler utilise une table de phases sinc à fenêtre Blackman préparée. ResampleSpec déclare taux rationnels d’entrée/sortie, canaux et capacités maximales ; ResampleResult retourne status, inputConsumed et outputProduced. Gardez le suffixe non consommé et utilisez seulement le préfixe de sortie valide. Un succès ne signifie pas que toute l’entrée est consommée.

L’application possède la FIFO et peut appeler avec une entrée vide pour vider une sortie disponible. `finish` fournit les zéros bornés de la queue FIR et annonce zéro entrée de l’appelant consommée ; après la fin, reset précède un nouveau flux. `setRates` est hors temps réel et conserve historique et prochaine sortie planifiée.

## Ratio variable et qualité {#ratio}

Pour utiliser `setRatio` sans allocation, préparez minimumRatio/maximumRatio autour du ratio nominal inputRate/outputRate. Les ratios comptent les frames d’entrée par frame de sortie ; les rampes comptent les frames produites. La coupure reste sûre sur toute l’enveloppe préparée, ce qui peut coûter de la qualité aux ratios plus modérés.

resampleSpecForQuality choisit taps/phases et augmente le support en réduction de fréquence dans la limite acceptée. Oversampler fournit des voies FIR, FIR demi-bande ou IIR. Faites correspondre les capacités de blocs et tenez compte de la latence ; la phase IIR n’est pas toujours représentable par un retard entier exact. FirRateConverter et DyadicFilterBank couvrent les dispositions entières/dyadiques spécialisées. Aucun moteur ne possède implicitement la file de flux qui l’entoure.

## API publique {#api}

- [cydsp/processors/Multirate.h](doc:api-processors-multirate)
- [cydsp/processors/Oversampler.h](doc:api-processors-oversampler)
- [cydsp/processors/Resampler.h](doc:api-processors-resampler)
