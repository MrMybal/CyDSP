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

HrtfSet defaults to 2048 directions and 32768 input frames/ear; explicit limits permit 100000 points and 65536 frames. The [optional SOFA importer](doc:sofa) reads measurements from memory. No measured dataset is bundled. By default the first crossing of a threshold relative to the peak (-40 dB) defines an integer onset; supplied fractional delays remain separate.

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
