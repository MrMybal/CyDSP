---
id: analysis
title: Spectral and musical analysis
description: Interpret FFT/PSD bands, constant-Q, pitch and musical estimates.
section: Guides
order: 29
revision: 1
parent: metering
---

## Spectrum and band units {#spectrum}

SpectrumAnalyzer prepares a periodic window and a double real FFT even for float input. N is a power of two 16–65536; hop is 1–N. The first result follows a full window, then each hop. Getters retain the latest completed frame; large calls may replace intermediate frames. Borrowed result spans change on analysis/reset and are invalidated by successful reprepare/destruction.

complexSpectrum contains raw windowed bins. Interior magnitudes are `2*abs(X)/sum(window)`; DC/Nyquist use factor 1. PSD is one-sided `abs(X)^2/(Fs*sum(window^2))`, doubled for interior bins. Sum PSD and multiply by Fs/N to obtain window-weighted mean square. Amplitude squared and mean PSD are different units.

SpectralBands builds normalized Mel/log triangular averages. Supplying PSD gives mean PSD, not integrated band energy. Unsupported empty bands fail preparation: increase FFT size or reduce bands. Features include centroid, rolloff, flatness, dominant bin and positive normalized flux. Dominant bin is not a refined fundamental estimate.

## Constant-Q and music {#music}

ConstantQAnalyzer uses direct variable-length Hann-windowed complex projections. Lower frequencies require longer kernels; budget total kernel length, channel count and hop. Results share an end timestamp but have different effective center delays. Defaults are 55 Hz, 12 bins/octave, 96 bins and 512-frame hop.

MusicalAnalyzer uses FFT-accelerated YIN for monophonic pitch/confidence, plus onset, tempo/beat, chroma, MFCC and key estimates. Choose N large enough for the minimum pitch; analysis starts after N samples. Key classes are 0=C through 11=B. Tempo, attacks and tonal key are heuristic and need confidence gating. A noexcept callback can receive completed features; it follows the same allocation-free owner contract as processing.

## Public API {#api}

- [cydsp/analysis/ConstantQAnalyzer.h](doc:api-analysis-constantqanalyzer)
- [cydsp/analysis/MusicalAnalyzer.h](doc:api-analysis-musicalanalyzer)
- [cydsp/analysis/SpectrumAnalyzer.h](doc:api-analysis-spectrumanalyzer)
