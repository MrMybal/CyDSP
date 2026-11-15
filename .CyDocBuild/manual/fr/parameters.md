---
id: parameters
title: Paramètres et automation
description: Rendre les événements à ID stables et les rampes à l’échantillon.
section: Guides
order: 6
sourceRevision: 1
parent: contracts
---

## Descripteurs et événements {#events}

`ParameterDescriptor` fournit ID stable, nom, unité, bornes, valeur initiale et échelle linéaire/logarithmique. ParameterBank copie les noms à la préparation. Sa sortie double a un canal planaire par descripteur, dans leur ordre.

Fournissez les événements triés par `frameOffset` dans le bloc courant. Une rampe avance dès le premier échantillon de l’événement et continue sur les blocs suivants. ParameterBank produit des valeurs de contrôle ; l’application les raccorde au traitement. Il n’inspecte pas les champs et n’appelle pas automatiquement les setters.

Utilisez `parameterToNormalized`/`parameterFromNormalized` avec le même descripteur ; les plages logarithmiques exigent un domaine positif valide. Les unités d’interface ne changent pas celles des processeurs.

## Choisir le mécanisme de lissage {#smoothing}

SmoothedValue et les rampes de gain évitent les sauts scalaires. Fondus entre filtres stables, lecteurs de délais et IR/graphes préparés répondent à des discontinuités différentes. Gardez les contextes jusqu’à la fin et respectez `transition_in_progress`.

Pour modifier un processeur de blocs précisément à l’échantillon, découpez l’audio à l’événement, appliquez un setter autorisé et continuez. La sortie de ParameterBank ne transforme pas un setter par bloc en entrée de modulation audio-rate.

## API publique {#api}

- [cydsp/control/Parameters.h](doc:api-control-parameters)
- [cydsp/math/DspMath.h](doc:api-math-dspmath)
