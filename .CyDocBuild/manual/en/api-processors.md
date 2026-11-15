---
id: api-processors
title: Audio processors
description: Audio processors
section: API reference
order: 1004
revision: 1
parent: api
---

| Header | Purpose |
| --- | --- |
| [AdaptiveFilter.h](doc:api-processors-adaptivefilter) | Prepared LMS/NLMS coefficient adaptation from program and desired signals. |
| [AdvancedDynamics.h](doc:api-processors-advanceddynamics) | ADAA saturation, dynamic EQ, de-essing, transient shaping, multiband dynamics and true-peak limiting. |
| [AdvancedEffects.h](doc:api-processors-advancedeffects) | Filtered/modulated multitap echoes, amplitude modulation, plate and spring reverbs. |
| [AdvancedFilterDesign.h](doc:api-processors-advancedfilterdesign) | High-order band designs, FIR equalization and minimum-phase FIR conversion. |
| [AsyncConvolver.h](doc:api-processors-asyncconvolver) | One producer stages impulses while one audio consumer uses retained plans without waiting. |
| [AudioGraph.h](doc:api-processors-audiograph) | Prepared single-bus DAG processing, retained scratch and borrowed callbacks. |
| [BandGain.h](doc:api-processors-bandgain) | Prepared three-band propagation gains using the existing filter foundations. |
| [Biquad.h](doc:api-processors-biquad) | Normalized stable coefficients, peaking/shelf designs and a prepared multichannel biquad. |
| [BiquadBank.h](doc:api-processors-biquadbank) | Independent coefficients and histories per voice, with SIMD across independent voices. |
| [BiquadCascade.h](doc:api-processors-biquadcascade) | Cascade of validated biquad sections with independent channel histories. |
| [ChannelMatrix.h](doc:api-processors-channelmatrix) | Output-major routing matrices, gain ramps and exact cross-channel aliases. |
| [ComplexFft.h](doc:api-processors-complexfft) | Complex transforms with explicit forward/inverse direction and backend selection. |
| [Convolver.h](doc:api-processors-convolver) | Select a prepared convolution strategy and its partition configuration. |
| [DelayLine.h](doc:api-processors-delayline) | Prepared fractional delay with controlled delay changes. |
| [Diffusion.h](doc:api-processors-diffusion) | Schroeder allpass diffusion and orthogonal Hadamard/Householder mixing. |
| [Dynamics.h](doc:api-processors-dynamics) | Compressor, expander and gate with peak/RMS detection, sidechain and prepared look-ahead. |
| [EchoCanceller.h](doc:api-processors-echocanceller) | Independent microphone NLMS paths with aligned mono reference and linked double-talk gating. |
| [Effects.h](doc:api-processors-effects) | Chorus, phaser and Schroeder-style reverb using the common prepared processing API. |
| [FilterBank.h](doc:api-processors-filterbank) | Complementary FIR bands whose neutral sum reconstructs the delayed input. |
| [FilterDesign.h](doc:api-processors-filterdesign) | Butterworth, Chebyshev I/II, elliptic and Bessel low/high-pass designs. |
| [Fir.h](doc:api-processors-fir) | Causal FIR filtering; tap zero multiplies the current sample. |
| [Gain.h](doc:api-processors-gain) | Linear gain with a ramp advancing once per frame across all channels. |
| [Generators.h](doc:api-processors-generators) | Oscillators and ADSR envelopes with sample and block processing. |
| [Hilbert.h](doc:api-processors-hilbert) | Matched-delay FIR quadrature signals for analytic-signal processing. |
| [HybridConvolver.h](doc:api-processors-hybridconvolver) | Immediate direct head plus FFT tail without added block latency. |
| [MatrixConvolver.h](doc:api-processors-matrixconvolver) | MIMO impulse routes with shared input transforms and prepared output coverage. |
| [Mixing.h](doc:api-processors-mixing) | Panning, dry/wet laws, elementary waveshaping and DC bias. |
| [MultiBusGraph.h](doc:api-processors-multibusgraph) | Explicit buses, sidechains, latency alignment and transitions between prepared DAGs. |
| [Multirate.h](doc:api-processors-multirate) | Integer FIR rate conversion and dyadic analysis/synthesis filter banks. |
| [MultiTapDelay.h](doc:api-processors-multitapdelay) | Bounded input/output taps, crossfaded configurations and sequential Thiran fractional delay. |
| [NonuniformConvolver.h](doc:api-processors-nonuniformconvolver) | Early and late partition sizes to reduce the cost of long impulse responses. |
| [Oversampler.h](doc:api-processors-oversampler) | Prepared FIR, half-band FIR or IIR up/down paths and their latency. |
| [PartitionedConvolver.h](doc:api-processors-partitionedconvolver) | Uniform FFT partitions with explicit block latency and finite tail. |
| [PitchEffects.h](doc:api-processors-pitcheffects) | Pitch-target correction and multivoice harmony through delay or spectral engines. |
| [PolynomialInterpolator.h](doc:api-processors-polynomialinterpolator) | Lagrange/Catmull–Rom polynomial sample readers. |
| [RealFft.h](doc:api-processors-realfft) | Real transforms exposing DC through Nyquist bins, with a normalized inverse. |
| [Resampler.h](doc:api-processors-resampler) | Fixed/variable sinc sample-rate conversion with mandatory consumption/production counts. |
| [Restoration.h](doc:api-processors-restoration) | Spectral noise attenuation, hum removal and bounded click detection/repair. |
| [ScheduledConvolver.h](doc:api-processors-scheduledconvolver) | Spread late FFT/MAC work across input frames while retaining an immediate onset. |
| [SincInterpolator.h](doc:api-processors-sincinterpolator) | Prepared windowed-sinc interpolation for fractional positions. |
| [SmoothedBiquad.h](doc:api-processors-smoothedbiquad) | Crossfade between stable filter states rather than interpolating potentially unstable coefficients. |
| [SmoothedBiquadCascade.h](doc:api-processors-smoothedbiquadcascade) | Prepared cascade transitions with a coalesced following target. |
| [SmoothedConvolver.h](doc:api-processors-smoothedconvolver) | Stage a new impulse off the audio thread, prewarm it and crossfade retained plans. |
| [TimeStretch.h](doc:api-processors-timestretch) | Optional time, pitch and formant stretching in the separate CyDSP::Stretch target. |
| [TptFilter.h](doc:api-processors-tptfilter) | TPT state-variable and ladder filters, Linkwitz–Riley splitting and additional filter designs. |
