---
id: sofa
title: SOFA import and HRTF assets
description: Import measurements optionally; prepare quality and portable caches.
section: Guides
order: 43
revision: 1
parent: hrtf
---

## Optional targets {#targets}

| Build switch | Link target | Dependencies |
| --- | --- | --- |
| Default core | CyDSP::CyDSP | No SOFA/file-format dependency |
| CYDSP_BUILD_HRTF_ASSETS=ON | CyDSP::HrtfAssets | Core only |
| CYDSP_BUILD_SOFA=ON | CyDSP::Sofa | Private compact BSD-3-Clause libmysofa reader + MIT miniz |
| Also CYDSP_SOFA_COMPLETE=ON | CyDSP::Sofa | External HDF5 ≥1.14.6 and zlib SDKs |

All production targets are static. SOFA and assets default OFF; the compact backend defaults ON within an enabled importer. Automatic selects the complete reader if built, otherwise compact. No SDK is downloaded by CMake. The complete SDK must be present at the final link step; declare C and C++ languages and use CMAKE_PREFIX_PATH to locate it.

~~~cmake
# Source build:
set(CYDSP_BUILD_SOFA ON CACHE BOOL "")
add_subdirectory(CyDSP)
target_link_libraries(editor PRIVATE CyDSP::Sofa)
# Installed package:
find_package(CyDSP 0.4.0 CONFIG REQUIRED COMPONENTS Sofa)
~~~

A separately configured game build may enable HrtfAssets alone and consume decoded caches without either file-format reader.

## Memory import and errors {#import}

Include cydsp/spatial/Sofa.h. The host supplies bytes; loadSofa performs no application file reads. Import, preparation, EQ and cache loading allocate and belong in a worker/editor, never the audio callback. Failure preserves the previous output; SofaLoadReport supplies a specific error.

~~~cpp
cydsp::SofaHrtf asset;
cydsp::SofaLoadOptions options;
cydsp::SofaLoadReport diagnostic;
const auto status = cydsp::loadSofa(fileBytes, asset, options, &diagnostic);
~~~

SimpleFreeFieldHRIR requires M×2×N IRs. GeneralFIR requires allowGeneralFir=true and two-ear receiver geometry. Other conventions fail explicitly. Source spherical degrees/radians or Cartesian metres/cm/mm become unit directions relative to ListenerPosition/View/Up (+X front, +Y left, +Z up). Missing listener vectors use convention defaults. ReceiverPosition identifies left/right by local Y. Nonzero EmitterPosition and multi-emitter data are unsupported.

Data.Delay is read as source-rate frames from 1×2 or M×2 and expanded per point. It remains separate from FIR onset detection. SamplingRate must be constant (8–768 kHz). Imported distances and original metadata/license are retained.

The complete SDK backend supports superblocks 0/2/3, object headers 1/2, compact/dense links and attributes, contiguous/chunked data including single/fixed/extensible modern indexes, float32/64 and fixed/variable strings. Deflate, shuffle and Fletcher32 are allowed. External links/storage, virtual datasets and unknown filters are rejected before data reads; CyDSP has no custom filter loading path.

The compact reader covers object header v2, layout v3, legacy chunk B-tree v1 with shuffle then deflate (other filter pipelines fail), contiguous arrays and supported compact/dense metadata. Use complete for old object header v1 or modern layout/index v4. Compact converts arrays to float32. SofaHrtf64 therefore requires complete or explicit allowPrecisionLoss=true. Reports disclose precision reduction; float output deliberately narrows float64 input. A noexcept inflate callback may use host zlib; it must return ok only for an exact output size and checksum-verified zlib stream.

Default limits: 128 MiB file, 512 MiB decoded buffers, 64 KiB metadata, 100000 points, 32768 taps. Samples and output coordinate/delay arrays share a budget. The compact parser also bounds allocations, read iterations and recursion. These are CyDSP buffer bounds, not a process-wide cap on SDK caches, file-image copies and temporary FFT storage. Use the host worker/sandbox's overall resource limits for untrusted imports.

## Preparation and quality {#prepare}

~~~cpp
cydsp::SofaPreparation settings;
const auto quality = cydsp::hrtfPreparation(
    cydsp::HrtfQuality::realtimeHigh, 48000, settings.hrtf);
if (quality != cydsp::Status::ok) return quality;
settings.diffuseFieldEqualization = true; // optional common-ear correction
settings.normalization = cydsp::HrtfLevelNormalization::frontalEnergy;
cydsp::HrtfSet<float> bank;
cydsp::SofaPreparationReport report;
const auto prepared = cydsp::prepareSofaHrtf(asset, bank, settings, &report);
~~~

Preparation selects the measured distance closest to distanceMetres (default 1 m), retaining points within distanceToleranceMetres of that shell. It neither mixes duplicate directions across radii nor invents missing measurements.

Optional diffuse-field EQ averages both ears' power, smooths log power over one-third octave, clamps inverse gain (default ±12 dB), then creates a common minimum-phase FIR through CyDSP FFT/filter primitives. Provide per-original-point solid-angle weights for irregular grids; empty weights mean equal weights, reported explicitly. This is an approximate correction model, not headphone calibration or exact integration of an irregular grid.

FrontalEnergy applies one common gain so that mean left/right impulse energy at the nearest frontal point is one, preserving interaural level differences. It does not flatten the frontal spectrum. EQ grows the FIR; inspect selected distance, gain and truncation in the report.

| HrtfQuality | FIR cap | Direction interpolation | Fractional delay / SRC sinc half-length |
| --- | --- | --- | --- |
| economy | 256 taps | 4 nearest | Linear / 16 |
| realtimeHigh | 2048 taps | 8 nearest | 17-tap sinc / 64 |
| reference | 65536 taps | All points | 65-tap sinc / 128 |

reference rejects truncation, permits up to 100000 points and retains double precision with SofaHrtf64/HrtfSet<double>. Bounds are configurable within header limits and the coefficient budget. Default HrtfPreparation retains the prior all-point/linear behavior; presets are explicit. QualityProfile maps its three levels to these presets. Multiple profiles can coexist in one application.

Nearest modes use a prepared balanced spatial index and normalized Shepard weights. A finite neighbor set can change at boundaries; crossfade direction changes. All-point weighting retains full-bank continuity. HrirBinaural bakes the fractional-delay kernel during setDirection; The legacy HoaBinaural overload applies it during preparation. Its spectral design overload instead evaluates fractional delays by frequency-bin phase and declares its own additional latency. Extra common latency is kernel length divided by two, rounded down, exposed by each renderer.

HrirBinaural uses direct FIR convolution: very long reference FIRs and all-point updates target offline/editor budgets. A quality label does not promise faster CPU execution or perceptually better localization for every dataset. HoaBinaural uses existing matrix convolution. Prepared process/reset/setDirection allocate no memory; serialize updates with audio on the same instance. Shared HrtfSet coefficients are immutable; voice histories and fades are independent.

## Portable cache and rights {#cache}

encodeHrtfAsset stores decoded measurements and original metadata in a versioned little-endian float32/64 cache with CRC32. decodeHrtfAsset validates sizes, checksum and data transactionally, requiring only CyDSP::HrtfAssets. Narrowing a double cache requires allowPrecisionLoss. This serializes decoded measurements, not a renderer or EQ result; call prepareSofaHrtf after loading with the chosen quality/EQ. CRC detects accidental corruption, not malicious replacement.

No measured HRTF dataset is distributed. Test fixtures are original synthetic MIT signals. Original CyDSP code stays MIT; optional reader/SDK notices retain their permissive licenses. Dataset rights remain separate: verify each distributed HRTF bank's terms and retain [component notices](doc:licensing).

## Public API {#api}

- [cydsp/spatial/Sofa.h](doc:api-spatial-sofa)
