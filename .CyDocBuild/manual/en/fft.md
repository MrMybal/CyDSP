---
id: fft
title: FFT, windows and normalization
description: Use complex/real transforms without confusing amplitude and PSD.
section: Guides
order: 19
revision: 1
parent: overview
---

## Transform convention {#transform}

The forward transform uses `exp(-i*2*pi*k*n/N)` without scaling. The inverse uses the opposite sign and divides by N. RealFft exposes N/2+1 bins, from DC through Nyquist. Use supported power-of-two lengths and the prepared span dimensions. Input/output overlap follows each transform's API rather than ordinary audio-block rules.

ComplexFft has immutable prepared tables and can be shared by independent callers with distinct buffers if lifecycle edits are serialized. RealFft owns mutable scratch and needs separate instances for simultaneous calls.

## Windows and levels {#windows}

makeWindow exposes window type, periodic/symmetric choice and normalization. A window suitable for spectral display is not automatically suitable for overlap-add reconstruction. Preserve FFT scaling when interpreting bins or multiplying spectra.

A raw magnitude is not dBFS or PSD. SpectrumAnalyzer normalizes amplitude by window sum and PSD by sample rate/window energy; see [analysis](doc:analysis). STFT handles weighted overlap-add and its explicit latency in the [spectral guide](doc:spectral).

## Public API {#api}

- [cydsp/math/Window.h](doc:api-math-window)
- [cydsp/processors/ComplexFft.h](doc:api-processors-complexfft)
- [cydsp/processors/RealFft.h](doc:api-processors-realfft)
