---
id: spectral
title: STFT and spectral effects
description: Process spectral callbacks, freeze, vocoding and separation.
section: Guides
order: 20
revision: 1
parent: fft
---

## STFT reconstruction {#stft}

Stft uses causal weighted overlap-add, zero-padded startup and N-frame latency. FFT length is a power of two in [16,65536]; hop divides N and the window must support reconstruction. A null callback reconstructs delayed input within FFT rounding.

A spectral callback receives one channel's bins and must be noexcept, allocation-free, owner-serialized and leave them finite. DC/Nyquist imaginary parts are discarded. `flush` feeds 2N zeros then reports end_of_stream; reset before a new input stream. Context ownership remains with the application.

## Effects and complementary outputs {#effects}

SpectralEffects provides freeze, vocoder, morphing and frequency-bin filtering. Inputs and sidechains must satisfy the selected mode's shapes; stored spectral state needs warmup. FrequencyShifter uses a matched-delay Hilbert quadrature path: frequency shift adds Hz rather than multiplying musical pitch.

HarmonicPercussive separates time/frequency median masks and reconstructs harmonic/percussive outputs with a complementary sum. Median history adds latency and startup dependence. Choose window/hop/median sizes for the content; separation is a signal heuristic rather than source recognition. SpectralPitchShifter handles phase propagation and pitch/formants separately in [pitch/time](doc:pitch).

## Public API {#api}

- [cydsp/processors/Hilbert.h](doc:api-processors-hilbert)
- [cydsp/spectral/HarmonicPercussive.h](doc:api-spectral-harmonicpercussive)
- [cydsp/spectral/SpectralEffects.h](doc:api-spectral-spectraleffects)
- [cydsp/spectral/Stft.h](doc:api-spectral-stft)
