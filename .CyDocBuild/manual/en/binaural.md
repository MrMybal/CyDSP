---
id: binaural
title: Binaural and loudspeaker rendering
description: Select analytic binaural, measured HRIR or VBAP layouts.
section: Guides
order: 36
revision: 1
parent: spatial
---

## Headphone rendering {#headphones}

AnalyticBinaural models spherical-head ITD, head shadow, front/back coloration and near-ear effects for up to 32 placed inputs. Source radius reduces directional detail. Two inputs can represent an extended stereo emitter. It is a parametric model, not a measured or listener-personalized HRTF.

HrirBinaural renders measured responses. Each raw HrirPoint supplies onset-free left/right FIRs and separate ear delays at the prepared rate. Keeping acoustic onset inside FIRs while also applying separate delays can cause combing or doubled delay. Prefer a [prepared HrtfSet](doc:hrtf) when converting or sharing measurements.

setDirection interpolates aligned filters and delays, then fades readers. A request during a fade returns transition_in_progress. Retain the previous successful direction until the request can be accepted; do not block the audio thread waiting.

## Speaker layouts {#speakers}

VbapLayout prepares 2D pairs or 3D convex-hull triples for 2–64 loudspeakers and returns unit-energy gains. Outside coverage, choose error or nearest-speaker fallback during preparation. A sparse/poorly distributed layout cannot reproduce every requested direction equally.

Choose [Ambisonics](doc:ambisonics) when sources should share an intermediate sound field. Panning and binaural cues are different rendering choices; applying both unintentionally can alter level and localization.
## Diffuse level and early-reflection timing {#diffuse-level}

Both HoaBinaural preparation overloads fill diffuseReferenceEnergy[2], diffuseDecoderEnergy[2], diffuseGain and diffuseMetricsAvailable in designReport(). These are mean impulse energies for uncorrelated unit-white-noise plane waves over the design directions. The legacy time-domain path uses uniform weights and its causal linear/sinc delay model. The spectral path uses normalized spatialWeights and its frequency-bin delay model, including the real Nyquist endpoint. Decoder power includes interference between spherical-harmonic channels. Legacy measurements use the actual float/double FIRs; spectral metrics precede float coefficient quantization.

The recommended amplitude gain is sqrt(sum(reference energy) / sum(decoder energy)). It is common to both ears, is not applied automatically, and restores integrated broadband energy on the design grid. It does not correct frequency/direction-dependent coloration or certify loudness for arbitrary programme material. Silent/degenerate designs report diffuseMetricsAvailable=false and retain gain=1. Do not divide by zero.

~~~cpp
cydsp::HoaBinaural<float> early;
const auto status = early.prepare({48000, 128, 4}, 1, bank); // causal time-domain LS
if (status != cydsp::Status::ok) return status;
const auto report = early.designReport();
const double gain = report.diffuseMetricsAvailable ? report.diffuseGain : 1.0;
// Apply the fixed gain to both ears after rendering.
const auto latency = early.latencyFrames();
~~~

For early reflections aligned to direct sound, this causal LS overload avoids the spectral designer's half-FFT delay. Sample-rate conversion or the optional sinc kernel can still add declared latency. Explicit spectral MagLS retains its centered FIR and half-FFT delay. This update does not select minimum-phase MagLS or an automatic phase-changing shortcut. A broadband gain compensates level, not low-order LS high-frequency magnitude loss. Select the path according to timing and spectral requirements.


## Public API {#api}

- [cydsp/spatial/SpatialRendering.h](doc:api-spatial-spatialrendering)
