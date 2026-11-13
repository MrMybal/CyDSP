# CyDSP 0.2.0

**IN DEVELOPMENT.** Source distribution; no binary release is published.

## Changes

- Ten-lot DSP/3D expansion: shared foundations, multibus/PDC graphs, propagation, acoustic rooms/reverbs, binaural/VBAP/HOA, advanced dynamics, effects/synthesis, musical/physical processing and restoration/diagnostics. Original MIT APIs; see [contracts and limits (French)](docs/ExpansionGuide.fr.md).

- Add original granular synthesis and audio-analysis APIs: levels/correlation, true peak, spectra/features, Mel/log bands, constant-Q and LUFS/LRA. Reuse CyDSP numerical kernels and bounded prepared storage; see [contracts and limits](docs/GranulationAndAnalysis.md).

- Separate numerical DSP into CyDSP and audio services into CyAudioCore, with distinct headers, namespaces and CMake targets. See the [migration guide](docs/Migration.fr.md).
- Select wavetable harmonic banks once per block, calculate gate gains directly and reduce repeated dynamics calculations.
- Optimize AVX-512 copy, scale, scaled accumulation and fill while retaining bounds-masked tails.
- Add repeated before/after measurements and independent numerical, lifecycle and realtime regression coverage. Detailed results and workload limits are in [Measurements.md](docs/Measurements.md).

## Platforms and known limitations

Windows x64 MSVC/clang-cl and Android ARM64 on Quest 3 have executed regression coverage. Windows ARM64 has static compilation coverage. SVE compiles and links for Android ARM64, but execution and vector-length coverage require compatible hardware. macOS, desktop Linux and Windows ARM64 execution remain unverified; x86 and ARM32 have not been rerun for this snapshot. See [Validation.md](docs/Validation.md).

CyDSP provides numerical processing. Audio devices, MIDI, file formats, transport and plugin interfaces belong to CyAudioCore. Existing applications must adapt to the 0.2.0 package and namespace boundary. Performance depends on CPU, backend, block dimensions and algorithm settings; measured gains do not apply to the entire library.

No separate bug-by-bug release list is published. Report reproducible issues through the [issue tracker](https://github.com/MrMybal/CyDSP/issues).

Second extension (0.2.0 development): [API/limits](docs/AdvancedExpansionGuide.fr.md),
[validation](docs/AdvancedExpansionValidation.fr.md).
