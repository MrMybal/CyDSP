---
id: diagnostics
title: Responses and measurement signals
description: Measure phase, coherence, distortion, sweeps and IR decay.
section: Guides
order: 30
revision: 1
parent: analysis
---

## Frequency and distortion measurements {#response}

firResponse/iirResponse evaluate complex response, phase and group delay from supplied filters. TransferAnalyzer accumulates reference/output spectra to estimate transfer and frequency-dependent coherence. Keep the reference aligned and choose window/averaging duration for the measurement; low coherence warns against treating a ratio as a reliable linear response.

MeasurementGenerator supplies configured deterministic measurement signals. measureDistortion estimates THD/THD+N from a periodic window and chosen fundamental. Use coherent frequencies or suitable windows and distinguish harmonic power from residual/noise; finite FFT resolution changes the result.

## Sweeps and impulses {#impulse}

inverseLogSweep/deconvolveSweep support swept-response reconstruction. Record enough pre/post time, align playback/capture and avoid clipping or time-varying processing if measuring a linear system. File/device I/O remains in the application.

analyzeImpulse reports onset, early reflections, energy, clarity and per-band decay/RT60 fits. Treat extrapolated decay fits according to their valid fit range and noise floor. matchFdnToImpulse proposes nominal FDN settings from those summaries, not a perfect room reconstruction. Measurement is preparation/analysis work; do not put a whole sweep deconvolution or large impulse analysis in a realtime callback.

## Public API {#api}

- [cydsp/analysis/Diagnostics.h](doc:api-analysis-diagnostics)
- [cydsp/analysis/ImpulseAnalysis.h](doc:api-analysis-impulseanalysis)
