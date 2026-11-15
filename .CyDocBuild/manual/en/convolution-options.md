---
id: convolution-options
title: Tune convolution partitions
description: Balance average CPU, boundary peaks and zero-latency heads.
section: Guides
order: 18
revision: 1
parent: convolution
---

## Configuration {#options}

ConvolutionConfig.mode selects hybrid, nonuniform or scheduled. earlyPartition defaults to 128; latePartition defaults to 1024 and is unused by hybrid. Respect the power-of-two and relative-size constraints accepted during preparation.

`ConvolutionConfig::spectralHead` is used by MatrixConvolver only. If a call presents a complete early partition at an aligned boundary, the first earlyPartition taps use the early tier's frequency-domain transform instead of a direct FIR. A partial/unaligned piece still needs its causal direct computation. Latency stays zero; FFT rounding can depend on the host block pattern.

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

## Decide with the real callback pattern {#cost}

Nonuniform processing can reduce mean cost but concentrates late work at boundaries. Scheduled processing spreads late jobs while retaining head/early work. A 256-frame native probe and a graph processing 480 internal frames can favor different partitions. Test actual channel routes, IR lengths, block sizes and tails.

workUnitsPerPartition/maximumWorkUnitsPerFrame describe scheduled late chunks only. Different jobs have different CPU costs; these counts do not bound total callback time. Likewise an observed maximum includes OS scheduling and is not a hard realtime guarantee. Preserve sound/routing/latency requirements when choosing a faster configuration.

## Public API {#api}

- [cydsp/processors/MatrixConvolver.h](doc:api-processors-matrixconvolver)
- [cydsp/processors/ScheduledConvolver.h](doc:api-processors-scheduledconvolver)
