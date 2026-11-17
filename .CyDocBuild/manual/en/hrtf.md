---
id: hrtf
title: Prepare and share HRTFs
description: Separate onsets, convert FIR rates and retain immutable prepared banks.
section: Guides
order: 37
revision: 1
parent: binaural
---

## In-memory measurements {#prepare}

HrtfSet defaults to 2048 directions and 32768 input frames/ear; explicit limits permit 100000 points and 65536 frames. The [optional SOFA importer](doc:sofa) reads measurements from memory. A small optional [SADIE II selection](doc:sofa#bundled) is supplied separately from the code. By default the first crossing of a threshold relative to the peak (-40 dB) defines an integer onset; supplied fractional delays remain separate.

Preparation removes that onset and resamples FIRs with windowed sinc when needed. Rates are 8–768 kHz and the conversion ratio is 1/8–8. Conversion preserves IR DC gain and exposes a common causal conversion latency through latencyFrames. Acoustic directional onsets remain distinct from that common latency.

## Capacity and sharing {#sharing}

Prepared FIR length defaults to 2048 taps and is configurable up to 65536. Use rejectTruncation or check truncated(). See the [quality guide](doc:sofa#prepare). Renderer tail bounds include FIR, acoustic delays and conversion latency.

Renderers acquire a shared immutable prepared state during prepare; they retain independent mutable histories/filter fades. Repreparing or destroying the original HrtfSet does not invalidate already prepared renderers. This differs from raw SampleData/Wavetable borrowing.

Direction interpolation uses normalized spherical Shepard weights with Gaussian locality. Exact measurement directions reproduce their measured point. angularWidthRadians controls locality, not the measurement density or individual listener fit. Onset detection is not a fractional calibration procedure. Assess moving sources, sparse coverage and listener-specific cues on actual material.
## Generate analytic HRIRs and a grid {#analytic}

sphericalFibonacci fills caller-owned direction and optional solid-angle spans, without allocation. Directions use the CyDSP listener frame; weights are equal, sum to 4π and only approximate spherical integration. This is neither an exact spherical t-design nor a Lebedev rule. Use a sufficiently dense grid or supply a different grid/weights for precise quadrature.

~~~cpp
#include <cydsp/spatial/SpatialRendering.h>
#include <vector>
std::vector<cydsp::Vec3> directions(256);
std::vector<double> weights(256);
const auto grid = cydsp::sphericalFibonacci(directions, weights);
if (grid != cydsp::Status::ok) return grid;
cydsp::AnalyticHrirOptions options;
options.sampleRate = 48000;
options.taps = 256;
cydsp::HrirBank<float> owned;
cydsp::AnalyticHrirReport diagnostic;
const auto generated = cydsp::analyticHrirs(options, directions, owned, &diagnostic);
if (generated != cydsp::Status::ok) return generated;
cydsp::HrtfSet<float> bank;
cydsp::HrtfPreparation preparation;
preparation.sourceSampleRate = owned.sampleRate();
preparation.targetSampleRate = 48000;
const auto ready = bank.prepare(owned.points(), preparation);
~~~

analyticHrirs excites the existing AnalyticBinaural processor with unit impulses and copies the selected finite FIRs into an owning HrirBank<float/double>. Each point includes its acoustic delay in the FIR and has zero separate delay. HrtfSet can subsequently separate integer onsets during its usual preparation; do not add the same delays twice.

The owning bank retains the sample rate, taps and views. Views survive moving the owner, and remain valid until it is destroyed or successfully replaced. Failed generation preserves the previous bank. Generation allocates, so run it outside audio. The maximum 100000 directions/65536 taps are also constrained by a default 512 MiB sample/view budget.

The model is the existing generic head-shadow/ITD approximation, named AnalyticBinaural in this repository. There is no CuedBinaural class here, individualized ear/pinna geometry or measured elevation detail. Increasing taps does not create such cues. maximumTailSample inspects the next 64 output frames beyond the retained FIR: it is a diagnostic, not a bound on all omitted energy. Choose taps for the model/rate and check the tail.

## Interpolate FIRs and select the engine {#interpolation-engine}

HrtfSet::interpolate(direction, left, right, delays) fills two caller-owned spans of exactly bank.taps() samples and a two-element array of ear delays in frames. It uses the same normalized direction and Shepard/nearest policy as HrirBinaural, including exact measured directions. The FIRs contain the prepared sample-rate-conversion response; onset delays remain separate and the optional fractional-delay kernel is not baked. Do not add acoustic onsets twice. Storage is allocated before audio. The const call allocates/frees nothing; independent destinations can query a shared bank concurrently. Overlapping ear spans or overlap with the delay array fail without changing outputs. Float destinations accumulate with float rounding; the renderer interpolates in double storage.

~~~cpp
std::vector<float> left(bank.taps()), right(bank.taps()); // allocate outside audio
std::array<double, 2> delays{};
const auto status = bank.interpolate({0, 1, 0}, left, right, delays);
if (status != cydsp::Status::ok) return status;
// Keep delays separate or incorporate them once into the host FIR.
~~~

Existing HrirBinaural::prepare overloads now select automatic. Use the explicit HrirRenderConfig overload to force direct or FFT. maximumFftSize defaults to 32768 (power of two, 32..262144). An oversized forced FFT fails transactionally; automatic retains the direct path. FFT size covers the whole delayed FIR plus a tile of up to 128 current input frames: this is whole-FIR overlap-save, rather than an unbounded partition engine for long responses. It adds no block buffering latency. Both fade endpoints use the same full input history, with their own ear delays. The fade is shared by both ears and independent of host block boundaries. FFT and direct implement the same FIR/delay model with different numerical summation orders. FFT processing uses CyDSP double FFTs and SIMD accumulation without a reduced-quality approximation or external runtime.

~~~cpp
cydsp::HrirRenderConfig config;
config.convolution = cydsp::HrirConvolution::automatic;
cydsp::HrirBinaural<float> voice;
const auto ready = voice.prepare({48000, 128, 1}, bank, config);
if (ready != cydsp::Status::ok) return ready;
~~~

Declared latency remains the prepared sample-rate-conversion delay plus the optional fractional-delay kernel; causal ear delays remain in the spatial response. setDirection allocates nothing but prepares FIR spectra when a FFT plan exists. Include updates and double-filter fades in the CPU budget, particularly for many voices/all-point interpolation, and serialize them with process. Coefficients and scratch are allocated only in prepare. SmoothedConvolver::stageImpulse is a separate operation which may allocate/free and cannot run concurrently with processing: allocation-free HRTF interpolation does not make this staging operation suitable for the audio callback.
