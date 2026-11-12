<table width="100%">
  <tr>
    <td width="300" align="left" valign="top">
      <a href="docs/Modules.md">Documentation</a><br>
      <a href="docs/Migration.fr.md">Migration guide (French)</a><br><br>
      <sub>README</sub><br>
      <a href="README.md">English</a> · <a href="README.fr.md">Français</a>
    </td>
    <td width="400" align="center" valign="middle"><strong>CyDSP</strong><br><sub>C++20 audio</sub></td>
    <td width="300" align="right" valign="top">
      <p><sub>Library license</sub><br><a href="LICENSE"><strong>MIT</strong></a></p>
      <p><sub>Version</sub><br><a href="VERSION.md"><strong>0.2.0</strong></a><br><strong>IN DEVELOPMENT</strong></p>
    </td>
  </tr>
</table>

<h1 align="center">CyDSP</h1>

A C++20 DSP library for filters, FFT, convolution, resampling, effects and numerical synthesis, with a shared processing API for modern x64 and ARM64 systems.

> **IN DEVELOPMENT · 0.2.0**
>
> Source only; no binary release is published. Read the [version notes](VERSION.md).

## About CyDSP

Processors share AudioBlock, AudioBuffer, ProcessSpec, Status and float/double samples. Prepare memory before the callback; process/reset retain their storage. CyDSP works independently. Audio services belong to CyAudioCore.

## Features

- Filters, dynamics, effects, mixing, DSP graphs and MIMO convolution.
- FFT, windows, SIMD, resampling and oversampling.
- Oscillators, envelopes, wavetables, sample data and prepared interpolation.
- Scalar, SSE2, AVX2, AVX-512, NEON and optional SVE, selected for CPU/OS support.
- Static Stretch module for time, pitch and formants.

---

## Installation

Build the static archives for the target ABI using CMake 3.24+ and a C++20 compiler. CyAudioCore requires CyDSP 0.2.0. See the [migration guide](docs/Migration.fr.md).

```cmake
find_package(CyDSP 0.2 CONFIG REQUIRED)
target_link_libraries(MyApplication PRIVATE CyDSP::CyDSP)
```

## Source, build and required libraries

```sh
cmake -S . -B build/release -G "Visual Studio 17 2022" -A x64
cmake --build build/release --config Release
cmake --install build/release --config Release --prefix build/install
```

Tests and measurements are off by default. scripts/run_native.py explicitly enables Windows validation; --sanitize enables AddressSanitizer. Platform backends require their system SDKs. Configuration never downloads dependencies.

---

## License

CyDSP uses the [MIT license](LICENSE). Dependencies retain their own terms; complete notice guidance is in [Licensing.md](docs/Licensing.md).

| Library · source | License · terms | Use / scope |
| --- | --- | --- |
| [DSPFilters](https://github.com/vinniefalco/DSPFilters) | [MIT](third_party/licenses/DSPFilters.txt) | RBJ low-pass, high-pass, constant-peak band-pass, notch, all-pass, peaking and low/high shelf coefficient equations only; CyDSP lifecycle, state, validation and block API are new. Chebyshev I/II pole/zero equations, bounded elliptic prototype factorization and Laguerre/Bessel root equations, adapted into preparation-only CyDSP SOS design; processing uses existing CyDSP cascades. |
| [FFTConvolver](https://github.com/HiFi-LoFi/FFTConvolver) | [MIT](third_party/licenses/FFTConvolver.txt) | Uniform partitioning and overlap-add adapted into CyDSP independent-channel/shared MIMO streaming, including progressive late jobs; uses original CyDSP FFT/complex kernels and scheduling, no upstream AudioFFT runtime. |
| [r8brain-free-src](https://github.com/avaneev/r8brain-free-src) | [MIT](third_party/licenses/r8brain-free-src.txt) | Windowed-sinc and cosine window equations only; CyDSP rational streaming, phase tables, memory and lifecycle are independent; not the complete r8brain SRC algorithm. |
| [signalsmith-linear](https://github.com/Signalsmith-Audio/linear) | [MIT](third_party/licenses/signalsmith-linear.txt) | Packed half-bin FFT reconstruction and STFT/window normalization adapted; upstream FFT implementation replaced by prepared CyDSP ComplexFft and shared VectorOps; original files retained as provenance only. |
| [signalsmith-stretch](https://github.com/Signalsmith-Audio/signalsmith-stretch) | [MIT](third_party/licenses/signalsmith-stretch.txt) | Pitch/time/formant algorithm adapted into the private CyDSP engine using its prepared SIMD FFT, STFT and common public lifecycle; upstream header is provenance only. |

## References and links

- [Documentation](docs/Modules.md)
- [Architecture](docs/Architecture.md)
- [Migration](docs/Migration.fr.md)
