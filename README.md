<table width="100%">
  <tr>
    <td width="300" align="left" valign="top">
      <a href="docs/Modules.md">Documentation</a><br>
      <a href="docs/Migration.fr.md">Migration guide (French)</a><br>
      <a href="https://github.com/MrMybal/CyDSP">Repository</a><br><br>
      <sub>README</sub><br>
      <a href="README.md">English</a> · <a href="README.fr.md">Français</a>
    </td>
    <td width="400" align="center" valign="middle"><strong>CyDSP</strong><br><sub>C++20 DSP · x64 / ARM64</sub></td>
    <td width="300" align="right" valign="top">
      <p><sub>Library license</sub><br><a href="LICENSE"><strong>MIT</strong></a></p>
      <p><sub>Version</sub><br><a href="VERSION.md"><strong>0.2.0</strong></a><br><strong>IN DEVELOPMENT</strong></p>
    </td>
  </tr>
</table>

<h1 align="center">CyDSP</h1>

A C++20 audio DSP library for developers of DAWs, effects, synthesizers and audio applications. It provides filters, FFT, convolution, resampling, granular synthesis and audio analysis with a common float/double API and CPU dispatch for x64 and ARM64.

> **IN DEVELOPMENT · 0.2.0**
>
> Source distribution; no binary release is published. Read the [version notes and platform limitations](VERSION.md).

The second extension adds shared HRTF preparation, HOA order 7, batched sources, diffraction, IR diagnostics, harmonic/percussive separation, spectral pitch, physical contacts, AEC and prepared quality profiles. See the [API guide](docs/AdvancedExpansionGuide.fr.md) and [validation results](docs/AdvancedExpansionValidation.fr.md).

## About CyDSP

Build a processing chain from planar `AudioBlock` views, owning `AudioBuffer` storage and a shared `ProcessSpec`. Prepare buffers and algorithms before the audio callback; `process` and `reset` retain their prepared storage. Parameters and lifecycle changes follow the ownership rules in the [architecture guide](docs/Architecture.md).

CyDSP is the numerical processing layer. Audio devices, MIDI, audio file I/O, playback transport and plugin hosting belong to **CyAudioCore**, which uses CyDSP. Applications can use CyDSP independently and connect it to their own audio engine and user interface.

## Features

- **Filters and dynamics:** biquads and cascades, Butterworth/Chebyshev/elliptic/Bessel designs, voice-parallel banks, compressor, expander, gate with hold/hysteresis/look-ahead, and limiter.
- **Transforms and convolution:** complex/real FFT, STFT, FIR, partitioned and nonuniform convolution, shared MIMO processing and prepared impulse-response transitions.
- **Sample-rate processing:** fixed and variable resampling, prepared interpolation, FIR/half-band FIR/IIR oversampling and window generation.
- **Effects and routing:** gain ramps, channel matrices, DSP graphs, delay, panning, dry/wet mixing, waveshaping, chorus, phaser and reverb.
- **Numerical synthesis:** oscillators, envelopes, harmonic-mip wavetables, sample data and anti-aliasing playback primitives; bounded manual grains and seeded granular clouds.
- **Audio analysis:** sliding peak/RMS/DC/correlation, 4x/8x true-peak estimates, streaming spectra and spectral features, Mel/log bands, constant-Q analysis and bounded-history LUFS/LRA metering. See the [API guide](docs/GranulationAndAnalysis.md).
- **Spatial audio and advanced processing:** acoustic paths/rooms, binaural HRIR, VBAP, Ambisonics, sidechains/PDC, multiband dynamics, live grains, FM/PM, physical synthesis, musical analysis and restoration. See the [ten-lot integration guide (French)](docs/ExpansionGuide.fr.md) for APIs and limits.
- **CPU backends:** scalar, SSE2, AVX2, AVX-512, NEON and optional SVE, selected during preparation according to compiled support and CPU/OS capabilities. Callers can request an explicit backend.
- **Optional Stretch module:** a separate static target for time stretching, pitch shifting and formant processing using the CyDSP numerical core.

---

## Installation

Use CMake 3.24+ and a C++20 compiler to build static libraries for the target ABI. The main target is `CyDSP::CyDSP`; `CyDSP::Stretch` adds the optional stretching module. Windows x64 and Android ARM64 have runtime validation; other platform limits are listed in the [version notes](VERSION.md).

After installing the CMake package, link it from your application:

```cmake
find_package(CyDSP 0.2 CONFIG REQUIRED)
target_link_libraries(MyApplication PRIVATE CyDSP::CyDSP)
# Optional time/pitch processing:
# target_link_libraries(MyApplication PRIVATE CyDSP::Stretch)
```

Public headers live under `cydsp/`; `cydsp/CyDSP.h` is the umbrella header. See the [processing example](examples/Processing.cpp), [granulation/analysis example](examples/GranulationAndAnalysis.cpp), [module contracts](docs/Modules.md) and [migration guide](docs/Migration.fr.md) for integration.

## Source, build and required libraries

The repository includes its pinned numerical dependencies; configuration does not download packages. From an authenticated checkout, the Windows x64 source build is:

```sh
git clone https://github.com/MrMybal/CyDSP.git
cd CyDSP
cmake -S . -B build/release -G "Visual Studio 17 2022" -A x64
cmake --build build/release --config Release
cmake --install build/release --config Release --prefix build/install
```

Set `CMAKE_PREFIX_PATH` in your application to the chosen install prefix. Set `CYDSP_BUILD_STRETCH=OFF` when only the core is needed. Architecture flags apply to individual SIMD implementation files; the portable library does not require a global native-CPU build.

Tests, measurements and examples are opt-in. Python is needed only for the repository's validation/build helper scripts, not for using the C++ library. Native validation commands and their actual coverage are documented separately in [Validation.md](docs/Validation.md).

---

## License

CyDSP uses the [MIT license](LICENSE), copyright © 2026 MrMybal CyberAlien. Adapted components retain their notices and pinned provenance. The table is an index; retain the complete [third-party notices](docs/Licensing.md).

| Library · source | License · terms | Use / scope |
| --- | --- | --- |
| [DSPFilters](https://github.com/vinniefalco/DSPFilters) | [MIT](third_party/licenses/DSPFilters.txt) | Filter coefficient/prototype equations adapted to CyDSP preparation; processing uses CyDSP filters and cascades. |
| [FFTConvolver](https://github.com/HiFi-LoFi/FFTConvolver) | [MIT](third_party/licenses/FFTConvolver.txt) | Partitioning/overlap-add ideas adapted to CyDSP streaming and MIMO convolution, using CyDSP FFT and vector kernels. |
| [r8brain-free-src](https://github.com/avaneev/r8brain-free-src) | [MIT](third_party/licenses/r8brain-free-src.txt) | Windowed-sinc and window equations adapted to CyDSP resampling; not the full upstream SRC algorithm. |
| [signalsmith-linear](https://github.com/Signalsmith-Audio/linear) | [MIT](third_party/licenses/signalsmith-linear.txt) | FFT reconstruction and STFT/window normalization adapted to the CyDSP numerical core. |
| [signalsmith-stretch](https://github.com/Signalsmith-Audio/signalsmith-stretch) | [MIT](third_party/licenses/signalsmith-stretch.txt) | Time/pitch/formant algorithm adapted into the optional `CyDSP::Stretch` target; upstream files retained as provenance. |

## References and links

- [Modules and API contracts](docs/Modules.md) · [Architecture](docs/Architecture.md).
- [Migration to CyDSP 0.2.0](docs/Migration.fr.md) · [Version notes](VERSION.md).
- [Performance measurements](docs/Measurements.md) · [Validation coverage](docs/Validation.md).
- [Source repository](https://github.com/MrMybal/CyDSP) · [Issue tracker](https://github.com/MrMybal/CyDSP/issues).
