---
id: quality
title: Qualité et précision
description: Choisir les politiques locales et transitions préparées.
section: Guides
order: 8
sourceRevision: 1
parent: contracts
---

## Profils de préparation {#profiles}

`qualitySettings` recommande des réglages pour `economy`, `realtimeHigh` et `offline` : fenêtre/hop spectraux, rééchantillonnage/interpolation, longueur HRTF, taille FDN, suréchantillonnage, true peak, budget de grains et durée de fondu. Appliquez-les à la préparation ; aucun réglage global caché ni worker automatique n’est créé.

QualityGraph conserve deux plans MultiBusGraph avec budget de latence fixe, préchauffage et fondu. CPU/mémoire incluent les deux pendant la transition. Les contextes restent vivants jusqu’à leur remplacement hors traitement.

## Précis et rapide {#accuracy}

MathAccuracy::precise garde la fonction de transfert de référence ; fast choisit explicitement les approximations proposées localement. Ce choix est indépendant de SaturationParameters::antialias. Désactiver l’antialiasing change l’algorithme et peut augmenter le repliement spectral.

```cpp
#include <cydsp/CyDSP.h>
cydsp::Status prepareFastTransient(cydsp::TransientShaper<float>& effect) noexcept {
    auto status = effect.prepare({48000.0, 512, 2});
    if (status != cydsp::Status::ok) return status;
    cydsp::TransientParameters p;
    p.attack = 6; p.sustain = -3;
    p.math = cydsp::MathAccuracy::fast;
    return effect.setParameters(p);
}
```

Comparez les contenus et enveloppes de gain réels avant de choisir les défauts. Une erreur nulle sur un corpus ne prouve pas l’identité pour tous les signaux. Gardez le mode précis sélectionnable si le contrat de qualité de l’application l’exige.

## API publique {#api}

- [cydsp/control/Quality.h](doc:api-control-quality)
