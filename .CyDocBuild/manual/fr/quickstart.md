---
id: quickstart
title: Votre première chaîne de traitement
description: Préparer un gain stéréo et traiter des blocs empruntés.
section: Guides
order: 2
sourceRevision: 1
parent: overview
---

## Préparer une fois, traiter les blocs {#chain}

Cet exemple complet possède un gain stéréo. Appelez `prepare` hors callback. Les deux canaux fournis doivent contenir au moins `frames` échantillons ; `frames` ne doit pas dépasser 512.

```cpp
#include <cydsp/CyDSP.h>
struct StereoGain {
    cydsp::Gain<float> gain;
    cydsp::Status prepare() noexcept {
        auto status = gain.prepare({48000.0, 512, 2});
        if (status != cydsp::Status::ok) return status;
        return gain.setGain(0.5f);
    }
    cydsp::Status process(float* left, float* right, std::size_t frames) noexcept {
        float* channels[] = {left, right};
        cydsp::AudioBlock<float> block{channels, 2, frames};
        return gain.process(block, block);
    }
};
```

Un gain de 0,5 divise l’amplitude par deux, soit environ −6,02 dB. Aucun limiteur de sortie n’est implicite. `setGain(target, rampFrames)` change progressivement le gain ; 480 frames durent 10 ms à 48 kHz.

## Assembler les processeurs {#compose}

Préparez le filtre/effet suivant avec le même `ProcessSpec`, puis traitez la sortie précédente. Vérifiez chaque `Status`. Préparer plusieurs objets ne constitue pas une transaction pour toute la chaîne : ne publiez une nouvelle chaîne qu’après avoir préparé tous ses membres de façon cohérente.

Pour une sortie distincte, allouez un `AudioBuffer` avant le callback. Pour des branches parallèles, utilisez les [graphes](doc:graphs) ou alignez explicitement les [latences](doc:contracts#timing).
