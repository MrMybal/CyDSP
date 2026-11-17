---
id: neural-hrtf
title: Native neural HRTF
description: Prepare a compact directional model and render its binaural filters.
section: Guides
order: 44
revision: 1
parent: hrtf
---

## What runs in the application {#runtime}

NeuralHrtfModel predicts two cascades of bounded peaking sections, ear levels and the desired right-minus-left ITD from a listener-relative direction (+X front, +Y left, +Z up). NeuralBinaural<float/double> renders those parameters using CyDSP biquads and fractional delays. The C++ static library needs no PyTorch, Python, CUDA, ONNX or model download. Training/export tools are separate optional Python programs. Learned assets are experimental and are not automatically installed with CyDSP.

The model approximates magnitude and low-band ITD for one known listener. It does not reconstruct the complete measured HRIR phase or supply a personalized HRTF for an arbitrary listener. Keep HrirBinaural with measured SOFA assets for the measured reference and demanding offline use.

## Load and prepare {#prepare}

The host reads a model asset and passes its bytes to load. Loading validates bounds, dimensions, rate, finite coefficients and supported encoding, then owns an immutable shared copy of the weights. Copies of NeuralHrtfModel share those weights; const predictions may run concurrently. Loading, assignment and destruction belong outside the audio callback. Scalar/SSE2/AVX2/AVX-512/NEON/SVE selection reuses CyDSP's compiled kernels. Validate each deployed platform separately.

```cpp
#include <cydsp/spatial/NeuralHrtf.h>

cydsp::Status prepareVoice(std::span<const std::byte> bytes,
                           cydsp::NeuralHrtfModel& model,
                           cydsp::NeuralBinaural<float>& voice) noexcept {
    auto status = model.load(bytes);
    if (status != cydsp::Status::ok) return status;
    // Audio must actually be running at this exact asset rate.
    status = voice.prepare({48000, 128, 1}, model);
    if (status != cydsp::Status::ok) return status;
    return voice.setDirection({1, 0, 0}, 0);
}
```

A 96 kHz model is rejected by a 48 kHz voice. Retrain/adapt the field for the new rate and validate it; changing the RBJ sample rate alone changes the response. Preparation allocates both complete audio banks and bounded delay histories. The initial direction must be installed before process; the input is mono and output stereo, with disjoint sample storage.

## Predict, design and publish {#updates}

Call model.predict(direction, parameters), then designNeuralHrtf(parameters, design) on a worker/control thread. Publish the completed value through a host-owned bounded queue and call voice.setDesign(design, fadeFrames) between process calls. predict also accepts a span of directions with whole-batch preflight; batch input/output must be disjoint. Batch evaluation is sequential and reuses transposed immutable weights through CyDSP SIMD addScaled kernels. It is not a GPU batch or a BLAS dependency.

setDirection combines prediction, coefficient/phase design and installation without allocation, but its cost is greater than installing an already prepared design. Allocation-free is not a callback-time guarantee. The public design helper subtracts the cascades' intrinsic 200–1500 Hz phase-slope ITD, on a 4096-bin FFT frequency grid, before assigning nonnegative external ear delays. Ear levels already restore the training magnitude offset. Applying the raw ITD again or restoring the offset twice is incorrect.

process, reset, predict, designNeuralHrtf and voice updates allocate no memory. The host serializes voice updates and processing. A voice must never be modified concurrently with process. The model may be shared across voices; filter/delay state belongs to each voice.

## Transitions and precision {#quality}

A nonzero transition crossfades complete old/new filter, gain and delay outputs linearly. setDesign and setDirection return transition_in_progress until it finishes; the host retains/retries its most recent target. A first design or zero-length transition installs immediately and resets filter/delay states. The raw-input replay ring keeps tracking input until reset, which chooses the pending endpoint and clears all histories. Before a fading update, the new endpoint replays retained raw input under its own coefficients, gain and delay. NeuralBinauralOptions::transitionHistoryFrames defaults to 512, accepts 0–65536 and is allocated during prepare. Zero selects the earlier cold start. Longer replay costs more CPU in setDesign/setDesigns and approximates, rather than reconstructs, the infinite IIR past. Perceptual transparency is not guaranteed.

Biquad states and coefficients use double precision for float and double audio APIs. A first-order Thiran all-pass supplies fractional residual delays with unity magnitude, avoiding the treble attenuation of linear interpolation. Its phase delay is frequency dependent; this design matches low-band ITD, not the entire high-frequency phase. No approximate math or global floating-point flags are enabled. latencyFrames is zero common buffering latency; causal ear delays and IIR tails remain. The default external-delay capacity is 5 ms and can be prepared up to 100 ms. A design outside capacity is rejected, preserving the current voice. IIR tails are not finite FIR tails.

## Render several voices together {#batch}

Keep a contiguous array of prepared NeuralBinaural objects. processBatch(voices, sources, stereoPairs) takes N mono input channels and 2*N output channels ordered L0,R0,L1,R1,... . It renders up to four voices per kernel group and handles a final partial group. It does not mix sources; route or sum the resulting pairs with CyDSP mixing functions. All voices must share a sample rate and selected backend, with maximum blocks large enough for the call. Input/output sample ranges must be disjoint. Whole-call preflight precedes any processing or output change.

Use setDesigns(voices, completedDesigns, fadeFrames) on the processing owner between blocks to install one worker-prepared design per voice. The entire batch is validated before mutation and history replay is grouped. Each voice keeps independent filter/delay states and its own fade clock; individual updates remain available. Prediction/design may run on a worker, but applying a design or replaying live history must never run concurrently with processing those voices.

The specialized cascade keeps coefficients and states in double precision and reuses existing CyDSP biquad kernels. On x64, stereo groups use SSE2 when a wider backend was selected; full groups use the selected backend. Scalar, NEON and SVE follow their compiled kernels. No weights, training topology or filter count change in this execution path. Faster rendering does not promise lower total CPU for every history size, update rate or workload.

## Export assets and verify {#tools}

```sh
python scripts/fetch_hrtf_training.py --subjects D2 --rate 48000
python scripts/train_hrtf_pilot.py --subject D2 --sample-rate 48000 --width 96 --filters 24 --feature-bands 4 --center-grid hybrid --frequency-grid mixed --selection-grid linear --steps 30000
python scripts/export_hrtf_native.py --run build/hrtf-runs/YOUR_RUN --output build/hrtf-models/D2-48000/model.cyhrtf
python scripts/validate_hrtf_rendering.py --run build/hrtf-runs/YOUR_RUN
```

An explicit --transfer-sample-rate with --initial-weights allows transfer from another rate of the same verified listener/dataset; it starts a new optimizer/schedule and records both rates and source hashes. A same-rate continuation remains a separate experiment. Checkpoint source snapshots remain unchanged. Native validation exports actual rendered impulses, compares spectrum/ILD/ITD to SOFA, checks theoretical-response agreement and measures prediction, design, source mixing and full-branch transitions separately. CPU results depend on voice count, quality, rate, backend and block size. Objective metrics do not replace listening/localization tests.

The asset format is little-endian CYHRTF02: eight magic bytes; six uint32 values (inputs, width, outputs, sections per ear, Fourier bands, sample rate); one float32 magnitude offset; then float32 row-major W1,b1,W2,b2,W3,b3, section centers, shifts and Fourier phase constants. The predictor is inputs→width→width→outputs with tanh hidden activations and bounded output decoding. Bounds are width 16–256, 4–32 sections/ear, 1–6 Fourier bands and 44.1–192 kHz. An exact byte count is required. CYHRTF01 benchmark assets are not accepted. The exporter adds SHA-256/provenance and source data notices alongside the asset; the host verifies its selected asset hash and preserves applicable rights.

## Public API {#api}

- [cydsp/spatial/NeuralHrtf.h](doc:api-spatial-neuralhrtf)
