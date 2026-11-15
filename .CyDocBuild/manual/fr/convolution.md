---
id: convolution
title: Convolution et réponses impulsionnelles
description: Choisir latence, routage et transitions d’impulsions.
section: Guides
order: 17
sourceRevision: 1
parent: overview
---

## Impulsions causales et moteurs {#engines}

Le tap h[0] multiplie l’échantillon d’entrée courant. PartitionedConvolver utilise des partitions FFT uniformes et annonce une latence de bloc. HybridConvolver combine une tête directe immédiate et une queue FFT. NonuniformConvolver ajoute une partition tardive plus grande. ScheduledConvolver répartit le travail tardif sur les frames. Convolver choisit hybrid/nonuniform/scheduled à la préparation, sans latence de bloc ajoutée.

Latence zéro signifie application immédiate du premier tap, pas suppression du silence/prédélai dans l’IR. tailFrames définit combien de zéros traiter après l’arrêt de la source. reset efface cet état.

## MIMO et mises à jour {#routes}

MatrixConvolver route les couples entrée/sortie `ConvolutionRoute<T>` ; les routes dupliquées s’ajoutent. Les impulsions sont copiées à la préparation et les FFT/historiques d’entrée sont partagés. Fournissez la véritable matrice pour du vrai stéréo : deux IR indépendantes n’implémentent pas nécessairement le croisement des canaux.

SmoothedConvolver sépare stageImpulse hors temps réel de startTransition sans allocation. Un préchauffage sur entrée vivante précède le fondu par défaut. Les deux plans sont conservés ; une mise à jour pendant le fondu peut retourner transition_in_progress. reset rejoint la destination d’un fondu en cours tout en conservant un stage non démarré. AsyncConvolver ajoute le [contrat producteur/consommateur](doc:threading#queues) explicite.

Consultez le [réglage des partitions](doc:convolution-options) pour les compromis de tête spectrale et travail tardif. Corrigez taux et routage des IR avant préparation ; chargement/décodage de fichiers appartiennent à l’application.

## API publique {#api}

- [cydsp/processors/Convolver.h](doc:api-processors-convolver)
- [cydsp/processors/HybridConvolver.h](doc:api-processors-hybridconvolver)
- [cydsp/processors/NonuniformConvolver.h](doc:api-processors-nonuniformconvolver)
- [cydsp/processors/PartitionedConvolver.h](doc:api-processors-partitionedconvolver)
- [cydsp/processors/SmoothedConvolver.h](doc:api-processors-smoothedconvolver)
