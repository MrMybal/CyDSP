---
id: graphs
title: Graphes DSP et compensation de latence
description: Connecter processeurs préparés, sidechains et transitions conservées.
section: Guides
order: 33
sourceRevision: 1
parent: overview
---

## Préparer la topologie {#topology}

AudioGraph fournit un DAG monobus préparé. MultiBusGraph ajoute DspNode, DspBus et DspConnection avec ID de bus explicites locaux à chaque nœud. graphInputNode/graphOutputNode identifient les frontières externes. La préparation copie la topologie mais emprunte contextes/fonctions ; gardez-les vivants et sérialisés jusqu’au remplacement du graphe.

MultiBusGraph accepte au plus 256 nœuds, 4096 connexions et 32 bus par disposition. Les cycles sont rejetés. Déclarez le DspTiming fixe de chaque nœud et ses formes entrée/sortie. Les sidechains sont de vrais bus d’entrée, sans convention cachée sur les canaux principaux.

## Alignement, erreurs et transitions {#timing}

Les branches entrant dans un nœud et les sorties externes s’alignent sur la plus grande latence déclarée. Bypass garde le processeur chaud et utilise le bus principal 0 retardé à sa latence. Une queue infinie utilise SIZE_MAX ; les queues finies respectent les limites de chemin acceptées. Une erreur de callback réduit la sortie externe au silence sans annuler les états des nœuds antérieurs.

GraphTransition conserve deux graphes préparés sous un budget de latence fixe : stage hors traitement, préchauffage par entrée vivante, puis fondu. Utilisez des contextes mutables indépendants entre graphes. Aucun graphe n’est détruit en fin de fondu ; remplacement/destruction ultérieurs se font hors callback. La latence reste fixe jusqu’à nouvelle préparation/staging. Le retard IIR dépendant de la fréquence et les effets créatifs de hauteur exigent une politique de temps au-delà d’une PDC entière.

## API publique {#api}

- [cydsp/processors/AudioGraph.h](doc:api-processors-audiograph)
- [cydsp/processors/MultiBusGraph.h](doc:api-processors-multibusgraph)
