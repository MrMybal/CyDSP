---
id: ambisonics
title: Ambisonics and HOA binaural
description: Encode, rotate and decode ACN/SN3D orders zero through seven.
section: Guides
order: 38
revision: 1
parent: spatial
---

## Format and rotations {#format}

ambisonicCoefficients, ambisonicRotation and AmbisonicRenderer support orders 0–7, `(order+1)^2` channels: 1–64. Channel order is ACN and normalization SN3D. First-order order is W,Y,Z,X. Convert FuMa/N3D explicitly outside this API.

Rotations are active: encoding direction d then rotating equals encoding R*d. For listener orientation, supply the inverse rotation. Do not mix a world-to-listener coordinate transform with a second equivalent field rotation.

## Decode to speakers or ears {#decode}

Speaker decoding uses regularized least squares for the supplied layout. Choose regularization for poorly conditioned/underdetermined layouts. Extra mathematical channels cannot supply spatial resolution missing from sparse speakers; this is not an AllRAD or room-correction decoder.

HoaBinaural projects measured HRIRs into the harmonic field, embeds fractional acoustic delays into FIRs and prepares a two-ear MatrixConvolver with HOA inputs. Field rotation can precede convolution. Prepare all filters outside processing, retain shared HRTFs and include conversion/convolution latency and tail in graph timing.

```diagram
{"type":"flow","title":"HOA signal path","nodes":[{"id":"sources","label":"Sources","x":0,"y":0},{"id":"encode","label":"ACN/SN3D","x":190,"y":0},{"id":"rotate","label":"Rotation","x":390,"y":0},{"id":"ears","label":"HOA binaural","x":590,"y":0}],"edges":[{"from":"sources","to":"encode"},{"from":"encode","to":"rotate"},{"from":"rotate","to":"ears"}]}
```
## Choose the binaural filter design {#design}

The existing prepare(spec, order, bank, regularization, backend) keeps its time-domain least-squares projection. It is useful for compatibility and low added latency. Low-order projection of direction-dependent HRIR phase can lose high-frequency energy; increasing gain alone does not fix its direction-dependent coloration.

The explicit HoaDecoderDesign overload adds weighted spectral least squares and MagLS. The implementation is original MIT C++ based on the [published objective](https://pub.dega-akustik.de/DAGA_2018/data/articles/000301.pdf), using CyDSP FFTs and MatrixConvolver. It imports no IEM/SPARTA implementation or new dependency.

~~~cpp
#include <cydsp/spatial/SpatialRendering.h>
cydsp::HoaDecoderDesign design;
design.method = cydsp::HoaDecoderMethod::magLS;
design.crossoverHz = 2000;
design.transitionHz = 500;
design.maxRe = false;
cydsp::HoaBinaural<float> renderer;
const auto status = renderer.prepare({48000, 256, 4}, 1, bank, design);
if (status != cydsp::Status::ok) return status;
const auto report = renderer.designReport();
~~~

Below the transition, fitting uses the complex HRTF response, including direction-dependent ear arrival delays. Across the transition it blends complex and magnitude errors; above it, alternating projection fits magnitudes using frequency-continuous phase initialization. Iterations are bounded (default 12, 1–256), and a local solution is obtained, not a guaranteed global optimum. The default band spans 1750–2250 Hz; choose it for the material, order and listener. Relaxing high-band phase preserves the low-band LS fit, not every perceptual cue or exact spatial resolution.

The spectral designer uses every supplied bank point with its design weights; direct-renderer neighbor counts and fractional-delay kernel lengths do not control this projection. Fractional delays are evaluated spectrally at the design bins.

An ITD belongs to a direction. A scene with several directions cannot recover all original ITDs by adding one output-ear delay after mixing. This design retains complex low-band targets and relaxes high-band phase instead; there is no claim of a universal remove/reinsert ITD operation.

maxRe optionally tapers high-band harmonic degrees by the max-rE Legendre weights. It broadens the angular response and may change magnitude fit/levels. It is separate from MagLS, defaults OFF and is not a replacement for high-band magnitude optimization.

Spatial weights are empty for equal weighting, or contain one nonnegative weight per bank point. They are normalized internally. Use appropriate solid-angle weights for irregular measured grids. Regularization is applied to this normalized Gram matrix, so its scale is independent of grid point count; it is not numerically identical to the legacy overload's unnormalized regularization.

## Latency, memory and diagnostics {#design-limits}

The spectral designer runs only in prepare, on a worker/editor. It retains the whole designed FIR, with no silent truncation. Auto FFT size is the next power of two at least twice the full delayed HRIR length (minimum 32, maximum 65536). An explicit FFT must satisfy the same lower bound. The causal circular shift adds FFT-size/2 frames; latencyFrames includes this and prior HRTF sample-rate-conversion latency. For example, 2048 FFT frames add 1024 frames: 23.22 ms at 44.1 kHz. This is a processing-latency tradeoff, not a zero-latency algorithm. Very long delayed banks exceeding the FFT bound fail explicitly; the legacy projection remains a separate choice.

maximumStorageBytes (default 512 MiB) bounds estimated designer-owned matrices, spectra and FIR buffers; it is not a cap on the complete renderer, typed filter copies, convolver storage or the whole process. Dense grids, large FFTs and high orders increase preparation work substantially. Audio rendering reuses the prepared MIMO convolver; process/reset/setRotation allocate no memory. Serialize updates with processing on one instance.

designReport reports FFT size, FIR length, total declared latency and relative weighted RMS errors. lowBandComplexError measures complex error below the transition. highBandMagnitudeError measures magnitude error above it; initialHighBandMagnitudeError is the spectral LS baseline for the same design data. These are dimensionless errors over design directions and non-endpoint FFT bins, before float filter quantization, not speed ratios, listening scores or independent validation. spectralMetricsAvailable is false for the legacy overload: its zero-initialized error fields are not measurements. MagLS is not an individualized HRTF or a guarantee against coloration; evaluate actual content and listener motion.
