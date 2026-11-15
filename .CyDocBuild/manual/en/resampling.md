---
id: resampling
title: Resampling and oversampling
description: Manage variable consumption, ratios, FIR tails and multirate banks.
section: Guides
order: 21
revision: 1
parent: overview
---

## Consumption is not block length {#counts}

Resampler uses a prepared Blackman-windowed sinc phase table. ResampleSpec declares rational input/output rates, channels and maximum input/output capacities; ResampleResult returns status, inputConsumed and outputProduced. Keep the unconsumed suffix and use only the valid output prefix. Successful processing does not mean every input frame was consumed.

The application owns a FIFO and may call with empty input to drain available output. `finish` supplies bounded zeros for the FIR tail and reports zero caller input consumed; after completion reset before a new stream. `setRates` is non-realtime and retains history and the scheduled next output.

## Variable ratio and quality {#ratio}

To use allocation-free `setRatio`, prepare minimumRatio/maximumRatio enclosing the nominal inputRate/outputRate. Ratios mean input frames per output frame; ramps count produced output frames. Filter cutoff stays safe for the complete prepared envelope, which can cost quality at gentler ratios.

resampleSpecForQuality selects taps/phases and increases support for downsampling within the accepted cap. Oversampler provides FIR, half-band FIR or IIR up/down paths. Match input/output block capacities and account for latency; IIR phase cannot always be represented by one exact integer delay. FirRateConverter and DyadicFilterBank cover specialized integer/dyadic multirate layouts. No engine silently owns the surrounding stream queue.

## Public API {#api}

- [cydsp/processors/Multirate.h](doc:api-processors-multirate)
- [cydsp/processors/Oversampler.h](doc:api-processors-oversampler)
- [cydsp/processors/Resampler.h](doc:api-processors-resampler)
