---
id: mixing
title: Gain, panoramique et routage
description: Router et mixer les canaux planaires avec des lois explicites.
section: Guides
order: 9
sourceRevision: 1
parent: overview
---

## Gain et panoramique {#gain}

Gain utilise l’amplitude linéaire ; `decibelsToGain` convertit les dB. Les rampes avancent une fois par frame pour tous les canaux et continuent entre blocs. Reset rejoint la cible courante. Choisissez explicitement la loi Panner/DryWetMixer : fondus à puissance constante et linéaires ont des milieux différents ; des signaux dry/wet corrélés peuvent dépasser l’unité en se sommant.

WaveShaper fournit les formes statiques élémentaires ; AdaaSaturator ajoute les choix avec antialiasing/mémoire décrits dans la [dynamique avancée](doc:advanced-dynamics). Bias ajoute un décalage continu ; il ne retire pas le DC.

## Matrices de canaux {#matrix}

Les coefficients ChannelMatrix sont indexés par sortie : `coefficients[out * inputChannels + in]`. Chaque sortie somme ses entrées pondérées. Les coefficients sont arrondis au format d’échantillon. Aucun placement d’enceintes, compensation de sonie, écrêtage ou normalisation n’est implicite.

`setMatrix` conserve la forme et peut ramper une fois par frame. Une cible identique répétée conserve la progression. Reset rejoint la cible. Les alias exacts entre canaux autorisent permutations et mixages rectangulaires en place grâce aux copies d’entrée conservées ; recouvrements partiels et entre sorties échouent. Des entrées en lecture seule peuvent s’aliaser.

Utilisez les coefficients mono/stéréo/downmix adaptés à l’application et définissez leur réserve de niveau. Les coefficients nuls omettent leur entrée ; les grandes sommes non muettes peuvent déborder.

## API publique {#api}

- [cydsp/processors/ChannelMatrix.h](doc:api-processors-channelmatrix)
- [cydsp/processors/Gain.h](doc:api-processors-gain)
- [cydsp/processors/Mixing.h](doc:api-processors-mixing)
