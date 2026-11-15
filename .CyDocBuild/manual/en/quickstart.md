---
id: quickstart
title: Your first processing chain
description: Prepare a stereo gain and process borrowed blocks.
section: Guides
order: 2
revision: 1
parent: overview
---

## Prepare once, process blocks {#chain}

This complete example owns a stereo gain. Call `prepare` outside the callback. Both supplied channels must contain at least `frames` samples; `frames` must not exceed 512.

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

A gain of 0.5 halves amplitude, about −6.02 dB. There is no automatic output limiter. `setGain(target, rampFrames)` changes gain smoothly; 480 frames last 10 ms at 48 kHz.

## Compose processors {#compose}

Prepare the next filter/effect with the same `ProcessSpec`, then process the preceding stage's output. Check every `Status`. Preparing several objects is not a transaction for the whole chain: publish a new chain only after all its members form a coherent prepared state.

For separate output, allocate an `AudioBuffer` before the callback. For parallel branches, use [graphs](doc:graphs) or explicitly align [latency](doc:contracts#timing).
