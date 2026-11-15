---
id: integration
title: Integrate a DAW, plugin or game
description: Connect application-owned callbacks, controls and spatial scenes.
section: Guides
order: 39
revision: 1
parent: overview
---

## Audio callback boundary {#callback}

Match ProcessSpec to the actual stream rate, channel count and largest processing block. If a host callback can exceed that maximum, split it into bounded sub-blocks and preserve event offsets. Keep planar pointer tables alive for every call. Convert interleaved input/output through prepared application buffers when necessary.

Own device/MIDI/file/plugin-format services in the consuming engine. A CyDSP processor does not register a plugin, open an audio device or create a window. CyAudioCore is a consumer of the library; direct application use follows the same public contracts.

## Practical chains {#recipes}

| Use case | Construction and responsibility |
| --- | --- |
| DAW insert | Prepared filter → dynamics → output gain; declare look-ahead and FIR latency to the host. |
| Convolution reverb | Decode/rate-match IR off-thread → prepare MatrixConvolver → send/mix → drain tail. |
| Instrument | Schedule notes → own voice allocation → oscillators/samples → envelope/filter → headroom/limiting. |
| Game emitter | Convert pose/units → propagation/material paths → independent source renderer → binaural/HOA → room return. |
| Meter display | Process analysis → copy owner snapshot → publish to UI through an application queue. |

## Reconfiguration {#reconfigure}

Prepare replacement processors and immutable assets away from audio. Publish an ownership-safe command or stage a supported transition; never swap/destroy live contexts from an unrelated thread. On sample-rate/layout changes, stop or transition under an explicit policy and reprepare every affected component. Avoid double distance loss, double Doppler and uncompensated dry/wet latency.
