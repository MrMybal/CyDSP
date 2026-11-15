---
id: filters
title: Filtres et égaliseurs
description: Choisir biquads, cascades, FIR, TPT ou ladder.
section: Guides
order: 10
sourceRevision: 1
parent: overview
---

## Coefficients et état {#biquad}

BiquadCoefficients normalise a0 à 1. Le feedback est soustrait : `-a1*y[n-1] - a2*y[n-2]`. Utilisez les fonctions de conception validées ; fréquence strictement entre zéro et Nyquist, Q positif et fini. Le gain paramétrique est en dB ; la pente shelving appartient à (0,1].

```cpp
#include <cydsp/CyDSP.h>
cydsp::Status prepareLowPass(cydsp::Biquad<float>& filter) noexcept {
    auto status = filter.prepare({48000.0, 512, 2});
    if (status != cydsp::Status::ok) return status;
    cydsp::BiquadCoefficients c;
    status = cydsp::designBiquad(cydsp::BiquadType::lowPass, 48000.0, 8000.0, 0.707, c);
    if (status != cydsp::Status::ok) return status;
    return filter.setCoefficients(c);
}
```

Biquad partage les coefficients entre canaux, avec historiques indépendants. BiquadBank donne à chaque canal/voix ses coefficients et peut vectoriser entre voix indépendantes. Une cascade applique les sections dans l’ordre ; vide, elle laisse passer le signal. Les modifications conservent les historiques sauf contrat spécifique de transition/reset.

## Automation et autres filtres {#alternatives}

SmoothedBiquad et SmoothedBiquadCascade fondent des banques stables. Une nouvelle cible de durée non nulle pendant le fondu remplace la cible suivante en attente ; une modification immédiate annule les transitions. Reset efface l’état et rejoint la dernière cible.

FIR applique h[0] causalement à l’échantillon courant. TptFilter et LadderFilter proposent les réponses à variables d’état/ladder ; LinkwitzRiley sépare les bandes. La [conception des filtres](doc:filter-design) couvre les familles d’ordre élevé et les bandes FIR complémentaires. Le retard de phase/groupe d’un IIR dépend de la fréquence ; ce n’est généralement pas un retard entier unique.

## API publique {#api}

- [cydsp/processors/Biquad.h](doc:api-processors-biquad)
- [cydsp/processors/BiquadBank.h](doc:api-processors-biquadbank)
- [cydsp/processors/BiquadCascade.h](doc:api-processors-biquadcascade)
- [cydsp/processors/Fir.h](doc:api-processors-fir)
- [cydsp/processors/SmoothedBiquad.h](doc:api-processors-smoothedbiquad)
- [cydsp/processors/SmoothedBiquadCascade.h](doc:api-processors-smoothedbiquadcascade)
- [cydsp/processors/TptFilter.h](doc:api-processors-tptfilter)
