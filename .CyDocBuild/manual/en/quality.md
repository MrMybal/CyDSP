---
id: quality
title: Quality and accuracy
description: Choose local policies and prepared transitions.
section: Guides
order: 8
revision: 1
parent: contracts
---

## Preparation profiles {#profiles}

`qualitySettings` recommends settings for `economy`, `realtimeHigh` and `offline`: spectral window/hop, resampling/interpolation, HRTF length, FDN size, oversampling, true peak, grain budget and fade duration. Apply them during processor preparation; there is no hidden global switch or automatic worker.

QualityGraph retains two MultiBusGraph plans with a fixed latency budget, warmup and fade. CPU/memory include both during transition. Contexts remain alive until replaced outside processing.

## Precise and fast {#accuracy}

MathAccuracy::precise keeps the processor's reference transfer; fast explicitly selects supported approximations locally. It is independent of SaturationParameters::antialias. Disabling antialiasing changes the algorithm and may increase spectral aliasing.

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

Compare actual content and gain envelopes before selecting defaults. No error on one corpus does not prove identity for every signal. Keep precise selectable when required by the application's quality contract.

## Public API {#api}

- [cydsp/control/Quality.h](doc:api-control-quality)
