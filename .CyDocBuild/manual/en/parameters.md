---
id: parameters
title: Parameters and automation
description: Render stable-ID events and sample-offset ramps.
section: Guides
order: 6
revision: 1
parent: contracts
---

## Descriptors and events {#events}

`ParameterDescriptor` supplies stable ID, name, unit, bounds, initial value and linear/logarithmic scale. ParameterBank copies strings during preparation. Its double output has one planar channel per descriptor, in descriptor order.

Submit events sorted by `frameOffset` within the current block. A ramp advances on the event's first sample and continues across blocks. ParameterBank produces control values; the application connects them to processing. It does not reflect fields or call setters automatically.

Use `parameterToNormalized`/`parameterFromNormalized` with the same descriptor; logarithmic ranges need a valid positive domain. UI units do not change processor units.

## Choose the smoothing mechanism {#smoothing}

SmoothedValue and gain ramps avoid scalar jumps. Stable-filter fades, delay-reader fades and prepared IR/graph transitions address different discontinuities. Keep contexts alive through completion and honor `transition_in_progress`.

For sample-accurate changes to a block processor, split the audio at the event, apply a supported setter and continue. ParameterBank output does not make a block-rate setter an audio-rate modulation input.

## Public API {#api}

- [cydsp/control/Parameters.h](doc:api-control-parameters)
- [cydsp/math/DspMath.h](doc:api-math-dspmath)
