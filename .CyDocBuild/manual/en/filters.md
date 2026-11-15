---
id: filters
title: Filters and equalizers
description: Choose biquads, cascades, FIR, TPT or ladder processing.
section: Guides
order: 10
revision: 1
parent: overview
---

## Coefficients and state {#biquad}

BiquadCoefficients normalize a0 to 1. Feedback is subtracted: `-a1*y[n-1] - a2*y[n-2]`. Use validated design functions; frequency is strictly between zero and Nyquist and Q is positive/finite. Peaking gain uses dB; shelf slope is in (0,1].

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

Biquad shares coefficients across channels with independent histories. BiquadBank gives each channel/voice its own coefficients and can vectorize independent voices. A cascade applies ordered sections; an empty cascade bypasses. Parameter edits preserve histories unless the specific transition/reset contract says otherwise.

## Automation and alternatives {#alternatives}

SmoothedBiquad and SmoothedBiquadCascade crossfade stable banks. A new nonzero-duration target during a fade coalesces to the latest following target; zero-duration edits cancel transitions. Reset clears state and snaps to the latest target.

FIR uses causal h[0] on the current sample. TptFilter and LadderFilter offer state-variable/ladder responses; LinkwitzRiley splits bands. Use [filter design](doc:filter-design) for high-order families and complementary FIR bands. IIR phase/group delay is frequency dependent; it is not generally one integer delay.

## Public API {#api}

- [cydsp/processors/Biquad.h](doc:api-processors-biquad)
- [cydsp/processors/BiquadBank.h](doc:api-processors-biquadbank)
- [cydsp/processors/BiquadCascade.h](doc:api-processors-biquadcascade)
- [cydsp/processors/Fir.h](doc:api-processors-fir)
- [cydsp/processors/SmoothedBiquad.h](doc:api-processors-smoothedbiquad)
- [cydsp/processors/SmoothedBiquadCascade.h](doc:api-processors-smoothedbiquadcascade)
- [cydsp/processors/TptFilter.h](doc:api-processors-tptfilter)
