# CyDSP 0.4.5


**IN DEVELOPMENT.** Source distribution; no binary release is published.

## Changes

- 0.4.5: Optional removal of the earliest common acoustic HRTF delay at preparation. One bank-wide offset preserves interaural and directional timing; `removedCommonDelayFrames()` reports the actual advance in target-rate frames. Measurement timing is retained by default.

- 0.4.4: Specialized double-precision neural cascades on existing CyDSP SIMD kernels; explicit four-voice batch rendering and grouped design updates. Fading targets replay 512 retained input frames by default (configurable 0–65536); unchanged model assets and static transfer. Rebuild the static library and consumers together.

- 0.4.3: Allocation-free HRTF interpolation; direct/FFT/automatic HRIR rendering with shared input history, continuous FIR crossfades and no added block latency. Diffuse-field reference/decoder energy and a common level-compensation gain are reported by both HOA preparation paths. The causal least-squares path retains its low latency; spectral MagLS retains its declared half-FFT delay.

- 0.4.2: Experimental native neural HRTF API: immutable shared model, memory-only bounded assets, SIMD prediction, compensated ITD, double-precision binaural cascades and full-output crossfades. Optional 48/96 kHz training/export/quality tools; weights are separate experimental assets.

- 0.4.1: Optional original SADIE II SOFA selection: KU100, KEMAR and one human listener at 48 kHz, 81.38 MB of measurements. Apache-2.0 data/notices stay separate from MIT code. Explicit installation, integrity checks and a larger ignored 96 kHz training-corpus fetcher; no packaged neural renderer or rendering API change.

- 0.4.0: Optional memory-only SOFA import, decoded HRTF cache, diffuse-field EQ, common level normalization, nearest-direction index and explicit HRTF quality profiles. Core MIT; optional reader notices accompany the library.
- 0.4.0: Original MagLS/weighted HOA decoder design, optional max-rE, owned analytic HRIR generation and a procedural Fibonacci grid; explicit latency and numerical diagnostics.

- 0.3.2: fuse the direct wavefolder input/shape/mix in the x64 and ARM64 SIMD backends. Cache the saturation DC compensation when its shape/bias changes.
- 0.3.2: vectorize the precise transient's rational/decibel stage while retaining each scalar `std::pow(10, exponent)` and its original exponent. Two-lane NEON retains the faster original scalar precise loop measured on Quest 3. Process independent dynamic-EQ channels in pairs, with unchanged ordered double filter/envelope recurrences; cache unchanged band coefficients and time constants.
- 0.3.2: avoid convolution snapshots for disjoint input/output, duplicate clears of mapped outputs and repeated internal span validation; use the SIMD overlap emitter in regular partition tiers. Zero latency, routing, alias handling and scheduled work budgets are preserved.
- Public processing signatures and quality defaults are unchanged. Rebuild the archive and its consumers together: the private layout of `MatrixConvolver` changed. A fused tanh experiment was rejected after it slowed both measured CPUs; its original tiled path is retained.

- 0.3.1: share saturation and dynamics tile laws across AVX2, AVX-512, ARM64 NEON and vector-length-agnostic SVE. Add SIMD cubic (direct and ADAA), and vector tanh evaluation for the hysteresis model while retaining its sequential memory recurrence. Public APIs and default quality policies are unchanged. NEON retains the faster cached scalar primitive for cubic ADAA; its direct path is fused SIMD.
- 0.3.1: bound fused progressive FFT jobs to 64 butterflies, simplify partial real-partition packing and vectorize overlap publication. Existing latency and work-budget contracts remain in force.

- Version 0.3.1. The package version had stayed at 0.2.0 through several API additions; it now covers everything listed below. `find_package` accepts one minor version only, so consumers request `CyDSP 0.3.1` and rebuild their archives with these headers. `cydsp/core/Version.h` gives the version at compile time.
- Add block forms that return the same samples as their per-sample calls: `Adsr::process` and `SampleData::render` (one mip selection per block, loop wrap and crossfade included). `SpscQueue` advances its indices without an integer division.

- Evaluate saturation and dynamics as 64-frame tiles with width-generic x64 kernels (four doubles in AVX2, eight in AVX-512). Tanh and atan ADAA return the exact mean between consecutive samples (absolute error near 1e-15, no scalar fallback); unlinked, sidechained, lookahead, expander and gate dynamics, the dynamic equalizer and the `fast` transient gain use vector transfer curves; input validation is branch-free. Results: [tile kernel report](docs/TileKernelOptimization.fr.md).
- Add the opt-in `ConvolutionConfig::spectralHead` for MatrixConvolver: whole early partitions convolve their first taps with the early tier's own transforms instead of the direct FIR, at zero latency.
- Fuse the FFT bit-reversal with its first two stages, vectorize the inverse real-spectrum merge and scaling, and use ranged split/merge kernels in the progressive convolution tiers. Results stay bit-identical to the reference transform.

- Add SIMD source batches and shared banks, HRTF preparation (onsets, alignment, conversion and interpolation), order-7 Ambisonics and binaural HOA, diffraction, detailed IR analysis, harmonic/percussive separation, spectral pitch/formants, physical contacts, echo cancellation with double-talk gating and quality profiles with prepared transitions. See the [advanced API guide](docs/AdvancedExpansionGuide.fr.md).

- Ten-lot DSP/3D expansion: shared foundations, multibus/PDC graphs, propagation, acoustic rooms/reverbs, binaural/VBAP/HOA, advanced dynamics, effects/synthesis, musical/physical processing and restoration/diagnostics. Original MIT APIs; see [contracts and limits (French)](docs/ExpansionGuide.fr.md).

- Add original granular synthesis and audio-analysis APIs: levels/correlation, true peak, spectra/features, Mel/log bands, constant-Q and LUFS/LRA. Reuse CyDSP numerical kernels and bounded prepared storage; see [contracts and limits](docs/GranulationAndAnalysis.md).

- Provide a standalone numerical DSP library through the `cydsp` namespace, public `cydsp/` headers and exported CMake targets.
- Select wavetable harmonic banks once per block, calculate gate gains directly and reduce repeated dynamics calculations.
- Optimize AVX-512 copy, scale, scaled accumulation and fill while retaining bounds-masked tails.
- Add repeated before/after measurements and independent numerical, lifecycle and realtime regression coverage. Detailed results and workload limits are in [Measurements.md](docs/Measurements.md).

## Platforms and known limitations

Windows x64 MSVC/clang-cl and Android ARM64 on Quest 3 have executed regression coverage. Windows ARM64 has static compilation coverage. SVE compiles and links for Android ARM64, but execution and vector-length coverage require compatible hardware. macOS, desktop Linux and Windows ARM64 execution remain unverified; x86 and ARM32 have not been rerun for this snapshot. See [Validation.md](docs/Validation.md).

CyDSP provides numerical audio processing through its public C++ API. Performance depends on CPU, backend, block dimensions and algorithm settings; measured gains do not apply to the entire library.


An optional SADIE II selection is supplied under Apache-2.0, separately from MIT code; installation defaults off. Other HRTF datasets are supplied by the host. Diffraction uses a thin-screen approximation, echo cancellation assumes a linear path and aligned reference, and formant/transient/music processing needs source-specific assessment. The [advanced contracts](docs/AdvancedExpansionGuide.fr.md) detail these limits.

No separate bug-by-bug release list is published. Report reproducible issues through the [issue tracker](https://github.com/MrMybal/CyDSP/issues).
