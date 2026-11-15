---
id: convolution-options
title: Régler les partitions de convolution
description: Équilibrer CPU moyen, pics aux frontières et tête sans latence.
section: Guides
order: 18
sourceRevision: 1
parent: convolution
---

## Configuration {#options}

ConvolutionConfig.mode choisit hybrid, nonuniform ou scheduled. earlyPartition vaut 128 par défaut ; latePartition vaut 1024 et n’est pas utilisé par hybrid. Respectez les contraintes de puissance de deux et de tailles relatives acceptées à la préparation.

`ConvolutionConfig::spectralHead` sert uniquement à MatrixConvolver. Si un appel présente une partition précoce complète sur une frontière alignée, les premiers earlyPartition taps passent par la transformation fréquentielle du niveau précoce au lieu du FIR direct. Une portion partielle/non alignée exige encore son calcul causal direct. La latence reste zéro ; les arrondis FFT peuvent dépendre du découpage des blocs.

```cpp
#include <cydsp/CyDSP.h>
cydsp::Status prepareStereoIR(cydsp::MatrixConvolver<float>& effect,
    std::span<const float> leftIR, std::span<const float> rightIR) noexcept {
    const cydsp::ConvolutionRoute<float> routes[] = {{0, 0, leftIR}, {1, 1, rightIR}};
    cydsp::ConvolutionConfig config;
    config.mode = cydsp::ConvolutionMode::nonuniform;
    config.earlyPartition = 128;
    config.latePartition = 1024;
    config.spectralHead = true;
    return effect.prepare({48000.0, 512, 2}, 2, routes, config);
}
```

## Décider avec les vrais blocs {#cost}

Le nonuniform peut réduire le coût moyen mais concentre le travail tardif aux frontières. Scheduled répartit les jobs tardifs tout en gardant le coût de tête/niveau précoce. Un test natif de 256 frames et un graphe traitant 480 frames internes peuvent favoriser des partitions différentes. Mesurez routes, tailles d’IR, blocs et queues réels.

workUnitsPerPartition/maximumWorkUnitsPerFrame décrivent uniquement les chunks tardifs planifiés. Leurs coûts CPU diffèrent ; ces comptes ne bornent pas le callback complet. Un maximum observé inclut aussi l’ordonnancement de l’OS et n’est pas une garantie temps réel absolue. Conservez les exigences de son, routage et latence en choisissant une configuration plus rapide.

## API publique {#api}

- [cydsp/processors/MatrixConvolver.h](doc:api-processors-matrixconvolver)
- [cydsp/processors/ScheduledConvolver.h](doc:api-processors-scheduledconvolver)
