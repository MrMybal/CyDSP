# CyDSP 0.3.0

**IN DEVELOPMENT.** Source distribution; no binary release is published.

## Changes

- Version 0.3.0. The package version had stayed at 0.2.0 through several API additions; it now covers everything listed below. `find_package` accepts one minor version only, so consumers request `CyDSP 0.3` and rebuild their archives with these headers. `cydsp/core/Version.h` gives the version at compile time.
- Add block forms that return the same samples as their per-sample calls: `Adsr::process` and `SampleData::render` (one mip selection per block, loop wrap and crossfade included). `SpscQueue` advances its indices without an integer division.

- Evaluate saturation and dynamics as 64-frame tiles with width-generic x64 kernels (four doubles in AVX2, eight in AVX-512). Tanh and atan ADAA return the exact mean between consecutive samples (absolute error near 1e-15, no scalar fallback); unlinked, sidechained, lookahead, expander and gate dynamics, the dynamic equalizer and the `fast` transient gain use vector transfer curves; input validation is branch-free. Results: [tile kernel report](docs/TileKernelOptimization.fr.md).
- Add the opt-in `ConvolutionConfig::spectralHead` for MatrixConvolver: whole early partitions convolve their first taps with the early tier's own transforms instead of the direct FIR, at zero latency.
- Fuse the FFT bit-reversal with its first two stages, vectorize the inverse real-spectrum merge and scaling, and use ranged split/merge kernels in the progressive convolution tiers. Results stay bit-identical to the reference transform.

- Add SIMD source batches and shared banks, HRTF preparation (onsets, alignment, conversion and interpolation), order-7 Ambisonics and binaural HOA, diffraction, detailed IR analysis, harmonic/percussive separation, spectral pitch/formants, physical contacts, echo cancellation with double-talk gating and quality profiles with prepared transitions. See the [advanced API guide](docs/AdvancedExpansionGuide.fr.md).

- Ten-lot DSP/3D expansion: shared foundations, multibus/PDC graphs, propagation, acoustic rooms/reverbs, binaural/VBAP/HOA, advanced dynamics, effects/synthesis, musical/physical processing and restoration/diagnostics. Original MIT APIs; see [contracts and limits (French)](docs/ExpansionGuide.fr.md).

- Add original granular synthesis and audio-analysis APIs: levels/correlation, true peak, spectra/features, Mel/log bands, constant-Q and LUFS/LRA. Reuse CyDSP numerical kernels and bounded prepared storage; see [contracts and limits](docs/GranulationAndAnalysis.md).

- Separate numerical DSP into CyDSP and audio services into CyAudioCore, with distinct headers, namespaces and CMake targets. See the [migration guide](docs/Migration.fr.md).
- Select wavetable harmonic banks once per block, calculate gate gains directly and reduce repeated dynamics calculations.
- Optimize AVX-512 copy, scale, scaled accumulation and fill while retaining bounds-masked tails.
- Add repeated before/after measurements and independent numerical, lifecycle and realtime regression coverage. Detailed results and workload limits are in [Measurements.md](docs/Measurements.md).

## Platforms and known limitations

Windows x64 MSVC/clang-cl and Android ARM64 on Quest 3 have executed regression coverage. Windows ARM64 has static compilation coverage. SVE compiles and links for Android ARM64, but execution and vector-length coverage require compatible hardware. macOS, desktop Linux and Windows ARM64 execution remain unverified; x86 and ARM32 have not been rerun for this snapshot. See [Validation.md](docs/Validation.md).

CyDSP provides numerical processing. Audio devices, MIDI, file formats, transport and plugin interfaces belong to CyAudioCore. Existing applications must adapt to the 0.2.0 package and namespace boundary. Performance depends on CPU, backend, block dimensions and algorithm settings; measured gains do not apply to the entire library.


HRTF datasets are supplied by the host. Diffraction uses a thin-screen approximation, echo cancellation assumes a linear path and aligned reference, and formant/transient/music processing needs source-specific assessment. The [advanced contracts](docs/AdvancedExpansionGuide.fr.md) detail these limits.

No separate bug-by-bug release list is published. Report reproducible issues through the [issue tracker](https://github.com/MrMybal/CyDSP/issues).
