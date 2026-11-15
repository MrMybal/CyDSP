---
id: delay
title: Delay, history and interpolation
description: Read fractional history and choose delay transitions.
section: Guides
order: 14
revision: 1
parent: overview
---

## Shared history {#history}

CircularHistory writes each source sample once and supports independent reads. Age zero means the latest written sample; unavailable past/future gives zero. Integer windows are gathered into double. Available interpolation choices include nearest, linear, cubic Lagrange and Catmull–Rom; sinc readers provide a prepared filtered alternative.

Interpolation quality and the allowed read range are separate. Keep positions inside prepared history and preserve causal ordering. ThiranDelay is a sequential first-order allpass for a fractional delay in (0,1], not random-access history.

## Taps and modulation {#taps}

MultiTapDelay routes up to 4096 input/output taps within prepared channel/delay capacity. Delay is in frames. setTaps crossfades complete configurations; editing during transition returns transition_in_progress. Fading readers avoids the pitch glide of continuously moving a delay read head.

DelayEffect deliberately modulates read position for flanger, vibrato and wow/flutter; choose delay/depth/rate/mix rather than relying on a mode name as a hidden preset. See [effects](doc:effects). A changing physical propagation delay already produces Doppler: avoid applying the same pitch change twice.

## Public API {#api}

- [cydsp/core/CircularHistory.h](doc:api-core-circularhistory)
- [cydsp/processors/DelayLine.h](doc:api-processors-delayline)
- [cydsp/processors/MultiTapDelay.h](doc:api-processors-multitapdelay)
- [cydsp/processors/PolynomialInterpolator.h](doc:api-processors-polynomialinterpolator)
- [cydsp/processors/SincInterpolator.h](doc:api-processors-sincinterpolator)
