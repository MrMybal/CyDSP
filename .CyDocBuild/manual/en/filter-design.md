---
id: filter-design
title: Filter design and multiband banks
description: Prepare higher-order IIR, linear/minimum-phase FIR and splits.
section: Guides
order: 11
revision: 1
parent: filters
---

## IIR families {#iir}

`designIir` creates Butterworth, Chebyshev I/II, elliptic or Bessel low/high-pass sections. Set the prototype order, sample rate, cutoff and family's ripple/attenuation controls before processing. `designBandIir` transforms a prototype order 1–16 into a bandpass/bandstop of doubled order. Keep both band edges below Nyquist and check returned Status.

Design functions are preparation work and may allocate. Verify the final coefficient response with [diagnostics](doc:diagnostics), particularly near Nyquist or for narrow/high-order bands. PeakingBiquadDesigner caches frequency/Q work for gain automation and offers sample-type stability validation.

## FIR phase and reconstruction {#fir}

`designFirEqualizer` designs a truncated/windowed response from frequency/gain points. Linear-phase FIR delays its output; `minimumPhaseFir` uses a real cepstrum to create an approximation with energy shifted earlier. Truncation/deep notches can alter the magnitude response: measure the result rather than assuming exact equivalence.

FirFilterBank uses differences of low-pass FIRs for up to 32 complementary bands. The neutral sum equals the input delayed by `(taps-1)/2`, within rounding. Output channel order is `band * inputChannels + channel`. Allocate enough outputs and compensate this latency before parallel dry/wet mixing. MultibandDynamics uses this foundation with its own smaller band limit.

## Public API {#api}

- [cydsp/processors/AdvancedFilterDesign.h](doc:api-processors-advancedfilterdesign)
- [cydsp/processors/FilterBank.h](doc:api-processors-filterbank)
- [cydsp/processors/FilterDesign.h](doc:api-processors-filterdesign)
