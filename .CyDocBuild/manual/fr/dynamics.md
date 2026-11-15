---
id: dynamics
title: Compresseurs, gates et sidechains
description: Contrôler les enveloppes du détecteur et l’anticipation préparée.
section: Guides
order: 12
sourceRevision: 1
parent: overview
---

## Modes et détecteur {#detector}

Dynamics propose compressor, expander, noiseGate et limiter. Préparez d’abord le mode et l’anticipation maximale ; réglez threshold/knee/makeup en dB, ratio, attaque/release/maintien en ms, hystérésis du gate et liaison des canaux via DynamicsParameters.

Les détecteurs peak et RMS ont des comportements d’enveloppe différents. BallisticsFilter expose aussi la détection indépendamment. Un détecteur lié partage les décisions de gain entre canaux ; il ne somme pas les sorties en mono.

## Sidechain externe et mixage {#sidechain}

La surcharge process à trois blocs accepte programme, sidechain et sortie. Un sidechain mono est diffusé aux canaux du programme ; sinon les nombres de canaux correspondent. Sidechain/sortie doivent être disjoints sauf si le sidechain est le bloc de programme lui-même. Alignez le détecteur sur l’événement visé dans le programme.

processMixed accepte des gains dry/wet linéaires finis de module au plus 16. Sa voie sèche suit le retard d’anticipation configuré. gainReductionDb et minimumAppliedGain décrivent le dernier bloc réussi non vide avant mixage ; lisez-les depuis le propriétaire du traitement ou publiez un instantané.

Consultez latencyFrames après choix de l’anticipation. Limiteur à détection d’échantillons et TruePeakLimiter suréchantillonné ont des contrats de reconstruction différents ; consultez le [guide avancé](doc:advanced-dynamics).

## API publique {#api}

- [cydsp/processors/Dynamics.h](doc:api-processors-dynamics)
