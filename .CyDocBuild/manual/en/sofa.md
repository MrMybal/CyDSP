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
find_package(CyDSP 0.4.4 CONFIG REQUIRED COMPONENTS Sofa)
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

HrirBinaural selects direct or double-precision FFT overlap-save convolution. Automatic uses FFT for FIRs of at least 96 taps and calls of at least 32 frames when the plan fits its limit and delay/tap ratio. Short calls and oversized plans retain direct processing. Very long reference FIRs and all-point updates can still require offline/editor budgets. A quality label does not promise faster CPU execution or perceptually better localization for every dataset. HoaBinaural uses existing matrix convolution. Prepared process/reset/setDirection allocate no memory; serialize updates with audio on the same instance. Shared HrtfSet coefficients are immutable; voice histories and fades are independent.

## Portable cache and rights {#cache}

encodeHrtfAsset stores decoded measurements and original metadata in a versioned little-endian float32/64 cache with CRC32. decodeHrtfAsset validates sizes, checksum and data transactionally, requiring only CyDSP::HrtfAssets. Narrowing a double cache requires allowPrecisionLoss. This serializes decoded measurements, not a renderer or EQ result; call prepareSofaHrtf after loading with the chosen quality/EQ. CRC detects accidental corruption, not malicious replacement.

The optional SADIE II selection is described below. Synthetic fixtures remain original MIT signals. Original CyDSP code stays MIT; optional reader/SDK notices retain their permissive licenses. Dataset rights remain separate: retain [component and data notices](doc:licensing).

## Optional measured bank below 100 MB {#bundled}

The repository includes three unmodified files from [SADIE II version 2-2](https://zenodo.org/records/12092466). Choose one alternative listener profile; the files are not combined automatically.

| Profile | Subject | Directions | SOFA bytes |
| --- | --- | --- | --- |
| Neumann KU100 mannequin | D1 | 8802 | 36310650 |
| KEMAR mannequin | D2 | 8802 | 36310650 |
| Human listener | H10 | 2114 | 8753949 |

All selected files contain original 48 kHz, 256-tap, two-ear float64 measurements (SimpleFreeFieldHRIR). Signal files total 81375249 bytes, or 81.38 decimal MB. The complete folder, including metadata and notices, must stay strictly below 100000000 bytes. No cropping, resampling, re-encoding, equalization or normalization was applied. The source filename mentions 24-bit recordings; the stored Data.IR arrays are float64.

The folder is data/hrtf/sadie-ii-v2-2. Installation defaults OFF and does not embed data into the static library:

~~~sh
cmake -S . -B build/release -DCYDSP_INSTALL_HRTF_DATA=ON
cmake --build build/release --config Release
cmake --install build/release --config Release --prefix build/install
# Or install only the measured-data component:
cmake --install build/release --component HrtfData --prefix build/install
~~~

The default destination is share/CyDSP/hrtf/sadie-ii-v2-2. The installed package sets CyDSP_HRTF_DATA_DIR when the data option is enabled. The host chooses a filename, reads its bytes and passes them to loadSofa; there is no implicit disk access or default profile in audio rendering. All three profiles exceed HrtfPreparation's default 2048 directions: choose realtimeHigh (up to 16384) or set sufficient maximumPoints explicitly before prepareSofaHrtf. A bank's stored size is separate from decoded memory, preparation scratch and per-voice rendering cost. realtimeHigh at 48 kHz retains these 256 taps; EQ or conversion can extend them. The complete reader preserves float64 for editor/reference use; the compact reader converts to float32 for ordinary rendering. Share the prepared immutable HrtfSet between voices and crossfade direction changes.

manifest.json pins the original archive/member, bytes and SHA-256 for every selected file. LICENSE.txt reproduces the original Apache-2.0 text; NOTICE.txt records University of York attribution and provenance. Keep these with distributed data; mark later modifications. The C++ code remains MIT. These profiles provide a starting choice, not a guarantee of individual fit or superior localization for every listener.

## Separate training corpus {#training}

More measured data can be used without increasing the shipped selection. The explicit Python standard-library fetcher uses a pinned catalog for all 20 SADIE II subjects (D1, D2 and H3–H20):

~~~sh
python scripts/fetch_hrtf_training.py --list
python scripts/fetch_hrtf_training.py
# Smaller explicit selection, if wanted:
python scripts/fetch_hrtf_training.py --subjects H3 H4 H5 --rate 48000
~~~

The default is 96 kHz, 512 taps per ear: all 20 SOFA files total 498669509 bytes (498.67 MB). Verified source ZIPs total 267332358 bytes; keeping archives and extracted files takes about 766 MB, plus small indexes/notices. Outputs stay under ignored build/hrtf-training/sadie-ii-v2-2 and are excluded from the data install. The fetcher checks pinned archive size/checksum, extracts only the selected original member and records each SOFA SHA-256 with its data rights.

The fetcher does not train a model. The separate experimental CUDA pilot fits one known listener with bounded peaking filters and low-band ITD. It uses an optional isolated PyTorch/NumPy/h5py environment; these tools are not C++ runtime dependencies. Run it explicitly with python scripts/train_hrtf_pilot.py --subject D2 --steps 30000 --batch-size 512. Its self-checks validate response equations, gradients, poles, known ITD and checkpoint serialization. Checkpoints, source snapshots, timing and held-out metrics stay under ignored build/hrtf-runs; the experimental native renderer API is described in [native neural HRTF](doc:neural-hrtf), with separate weights. The pilot measures within-listener directional interpolation, not full HRIR phase, listening quality or CPU speed.

A multi-listener evaluation should split by listener, keeping all directions, sample rates and augmented copies of one person in the same partition. The catalog suggests H3–H16 for training, H17–H18 for validation, H19–H20 for held-out tests; D1/D2 are mannequin references. This is a preparation split, not a measured model result. A faster neural renderer would still need CPU, spectral/ITD and listening comparisons against the existing HRTF renderer.

## Comparing experimental neural fields {#neural-quality}

The optional comparison tool registers its cases and selection criterion before training. It evaluates logarithmic/hybrid filter centers, predictor capacity and 12/24 peaking sections per ear. The hybrid grid places more sections in the upper band; it does not claim a perceptually optimal distribution. The mixed objective balances logarithmic and uniform frequency sampling.

~~~sh
python scripts/compare_hrtf_quality.py --subject D2 --screen-steps 8000 --refine-steps 20000
~~~

Screening and refinement use validation only. One model per filter budget is chosen and both checkpoint hashes are frozen before reserved test evaluation. For manual experiments, --validation-only defers testing; --evaluate-run explicitly evaluates a frozen local run. --initial-weights fine-tunes compatible weights with a fresh optimizer/RNG state, preserving the original normalization and retaining the initial model if it remains better.

All experiments stay under ignored build/hrtf-quality and build/hrtf-runs. The diagnostic tool fits free coefficients on training examples to inspect representation limits; those fits are neither a deployable field nor a proven optimum. check_hrtf_field.py verifies source/data hashes, predicted poles, a complex128 response oracle, CPU/GPU agreement and plain weight export without executing archived source.

The network delay head predicts desired total ITD. The filters add their own interaural phase: the experimental compensated_delays_us helper subtracts the cascade phase-slope ITD before assigning nonnegative external ear delays. Use those residual delays instead of adding the raw head output as well. This analysis/preparation helper may allocate and is not an audio callback API. It aligns one low-band slope, not full HRIR phase. Restore magnitudeOffsetDb for absolute level. Current models target 96 kHz; another processing rate requires an adapted and validated design.

## Colab / L4 and A100 training with Drive {#colab}

The optional notebooks/CyDSP_HRTF_A100.ipynb accepts a CUDA GPU, including L4 or A100 40/80 GB, and Google Drive. Put the matching CyDSP-HRTF-Colab.zip from build/colab in MyDrive/CyDSP-HRTF, select an L4 or A100 runtime and execute its cells. The historical notebook filename is retained; REQUIRE_A100=False accepts L4 without changing the training recipe or existing source bundle. No private GitHub token is needed. PyTorch 2.9.1/CUDA 12.8 and NumPy/h5py come from official package indexes; pinned SADIE II SOFA data come from Zenodo. Only the optional Python tools use these dependencies.

The default refines the selected D2 hybrid-24 field, width 96 and four direction feature bands, for 100000 further updates. Its reference weights have already been tested; final metrics track that test, not a fresh independent holdout. The batch benchmark measures training compute only. GPU memory capacity does not guarantee a speedup; float32 with TF32/AMP disabled is retained. Models remain per-listener and target 96 kHz.

The notebook freezes an experiment plan, works on local storage, and uploads verified immutable checkpoints to Drive about every 60 seconds and on exit. resume.pt restores Adam, RNG, weights, best model and the original schedule; --initial-weights starts a new optimizer instead. A restart uses the same source bundle and PyTorch version. Changing the GPU can change rounding. Abrupt termination can lose work since the last successful backup. Earlier backup generations remain available. Colab GPU availability and session length are controlled by the service.

For sessions where Drive authorization fails, notebooks/CyDSP_HRTF_L4_Direct.ipynb accepts the source ZIP directly and downloads a verified result ZIP with weights and resumable checkpoints. It never mounts Drive. Keep the downloaded archive before releasing the Colab VM; unexported checkpoints remain ephemeral. Rebuild with python scripts/make_hrtf_direct_colab.py.

The separate notebooks/CyDSP_HRTF_L4_Quality.ipynb profile refines the validated 48 kHz D2 field for 140000 additional updates at lower learning rates (0.0003 to 0.00001). At the earlier L4 timing this estimates about 30 compute minutes, excluding setup/validation/Drive. Rebuild its separate notebook and ZIP with python scripts/make_hrtf_colab.py --profile l4-quality. It preserves the earlier experiment and the current integration assets; improved fidelity is assessed after validation and listening.

python scripts/make_hrtf_colab.py rebuilds a matched notebook/source bundle. On a checkout without local experimental reference weights, add --without-reference-weights. The small bundle contains scripts/notices and optional frozen references, with no raw SOFA or CUDA binaries. The training bundle adds no C++ runtime dependency; the separate native API remains experimental.

These are experimental filter budgets, not public C++ renderer quality modes or measured audio CPU speed ratios. Report spectral errors on both log and linear grids, low/high bands and ITD. Lower average error does not imply that every frequency improves. The measured SOFA renderer remains available; this workflow makes no listening, full-phase or unseen-listener quality claim.

## Public API {#api}

- [cydsp/spatial/Sofa.h](doc:api-spatial-sofa)
