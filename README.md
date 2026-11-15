<table width="100%">
  <tr>
    <td width="300" align="left" valign="top">
      <a href="https://mrmybal.github.io/CyDSP/Documentation.html#/en/overview">Documentation</a><br>
      <a href="https://github.com/MrMybal/CyDSP">Repository</a><br><br>
      <sub>README</sub><br>
      <a href="README.md">English</a> · <a href="README.fr.md">Français</a>
    </td>
    <td width="400" align="center" valign="middle"><strong>CyDSP</strong><br><sub>C++20 DSP · x64 / ARM64</sub></td>
    <td width="300" align="right" valign="top">
      <p><sub>Library license</sub><br><a href="LICENSE"><strong>MIT</strong></a></p>
      <p><sub>Version</sub><br><a href="VERSION.md"><strong>0.3.2</strong></a><br><strong>IN DEVELOPMENT</strong></p>
    </td>
  </tr>
</table>

<h1 align="center">CyDSP</h1>

**CyDSP is a C++20 library for audio digital signal processing.** It provides reusable algorithms for effects, equalizers, synthesizers, audio analysis and 3D sound, with a common float/double API and SIMD implementations for x64 and ARM64.

> **IN DEVELOPMENT · 0.3.2**
>
> The source repository is available on GitHub. Build the library from a local checkout; read the [version notes and platform limitations](VERSION.md).

## About CyDSP

Use CyDSP to filter, transform, generate, analyze and spatialize audio in a DAW, a game, an audio plugin or a custom engine. Its processors share the same planar audio views, preparation conventions and numerical building blocks: FFTs, filters, convolution, resampling, interpolation and vector operations.

Prepare processors and storage before entering the audio callback, then process blocks through `AudioBlock<float>` or `AudioBlock<double>`. `ProcessSpec` describes the sample rate, maximum block size and channel count; `AudioBuffer` supplies aligned owning storage. Processing and reset reuse prepared memory without allocation, locks or I/O. The [processing contracts](https://mrmybal.github.io/CyDSP/Documentation.html#/en/contracts) describe lifetimes, parameter updates and thread ownership.

Combine processors directly or build multibus DSP graphs with explicit sidechains, latency compensation and transitions between prepared states. Prepared samples, wavetables and HRTFs can be shared across sources, while each processor retains its own mutable signal state.

## Features

| Area | Available processing |
| --- | --- |
| **Filters and EQ** | Biquads/cascades, Butterworth, Chebyshev, elliptic and Bessel designs, FIR and linear-phase EQ, minimum-phase FIR conversion, TPT/ladder filters, dynamic EQ, complementary band splitting and LMS/NLMS adaptive filters. |
| **Dynamics** | Compressor, expander, gate with hold/hysteresis/look-ahead, external sidechains, multiband compression, de-esser, transient shaping and oversampled true-peak limiting. |
| **FFT and spectral processing** | Complex/real FFT, public STFT analysis/synthesis, spectral freeze, vocoder, morphing, spectral filtering, frequency shifting and harmonic/percussive separation with complementary reconstruction. |
| **Convolution and IRs** | Direct, partitioned, nonuniform and shared MIMO convolution; prepared IR transitions; onset, early-reflection, clarity and band-decay/RT60 diagnostics, with nominal FDN matching. |
| **Sample-rate processing** | Fixed/variable resampling, sinc and polynomial interpolation, fractional-delay/all-pass primitives, FIR/half-band FIR/IIR oversampling and window generation. |
| **Delays and modulation** | Fractional and multitap delays, ping-pong/filtered echoes, chorus, phaser, flanger, vibrato, tremolo, autopan and wow/flutter. |
| **Nonlinear effects** | Waveshaping, clipping, rectification, ADAA saturation, wavefolding, asymmetry, multiband saturation and a bounded hysteresis model. |
| **Reverberation** | Schroeder, FDN networks, Dattorro-style plate, spring and hybrid IR/FDN reverbs, with diffusion, damping, freeze and shimmer controls where supported. |
| **Synthesis** | Bandlimited oscillators/wavetables, audio-rate FM/PM, hard sync with oversampling, wavetable morphing, unison, ADSR/multisegment envelopes, noise and seeded LFOs. |
| **Sampling and granulation** | Prepared sample banks, antialiasing playback, loop crossfades, manual grains and seeded clouds, live-input history, freeze, forward/reverse grains and multichannel routing. |
| **Physical sound** | Karplus–Strong, string/tube waveguides, modal resonators and membrane/plate mode banks; impact, sliding, rolling and bowed contact exciters with explicit physical units. |
| **Pitch and time** | Harmony/doubling and monophonic pitch correction; spectral pitch with formant retention, linked channel phases and transient protection; optional time/pitch/formant stretching through `CyDSP::Stretch`. |
| **Audio and musical analysis** | Peak/RMS/DC, stereo correlation, crest factor, true peak, LUFS/LRA, spectrum/features, Mel/log bands, constant-Q, YIN fundamental/confidence, onsets, tempo/beats, chroma, MFCC and key estimates. |
| **Restoration and diagnostics** | Spectral denoising, hum/click treatment and persistent-resonance attenuation; frequency/phase/group-delay responses, transfer/coherence, THD/THD+N, measurement signals and sweep deconvolution. |
| **Acoustic echo cancellation** | Mono far-end reference, independent microphone NLMS paths, linked double-talk gating, adaptation controls and diagnostic metrics. |
| **3D and spatial audio** | Distance/directivity, atmosphere/media, Doppler, acoustic paths, rooms/portals, analytic binaural, shared HRTF onset alignment/resampling/interpolation, VBAP, ACN/SN3D Ambisonics through order 7, binaural HOA and a thin-screen diffraction approximation. |
| **Realtime infrastructure** | Source batches with SIMD sine recurrences and shared assets, vector mixing, buffers/ramps, sample-accurate parameter events, multibus graphs, sidechain/PDC alignment, graph crossfades and common quality profiles. |

## Realtime processing and CPU architectures

CyDSP builds as a static library for the target architecture. Scalar, SSE2, AVX2 and AVX-512 kernels serve x64 targets; NEON and optional SVE kernels serve ARM64 targets. Selection takes place during preparation using compiled support and CPU/OS capabilities. A supported backend can also be requested explicitly.

SIMD is applied to suitable operations while recursive filters and envelopes preserve their ordered signal state. Quality and accuracy settings belong to individual processors; the portable library keeps precise floating-point compilation without global fast-math or native-CPU flags.

Windows x64 and Android ARM64 have runtime coverage. Windows ARM64 has static compilation coverage; SVE execution, macOS and desktop Linux still require target validation. See the [version notes](VERSION.md) for the current platform limits.

## Projects using CyDSP

**CyAudioCore** uses CyDSP for its numerical audio processing. CyDSP is also usable directly by applications and plugins through its public C++ API.

The [CyDoc manual](https://mrmybal.github.io/CyDSP/Documentation.html#/en/overview) documents processor contracts, supported configurations and latency, with a [public C++ API reference](https://mrmybal.github.io/CyDSP/Documentation.html#/en/api) and [spatial audio guides](https://mrmybal.github.io/CyDSP/Documentation.html#/en/spatial). HRTF datasets are supplied by the application; physical and musical models have the limits documented in those guides.

---

## Installation

Use CMake 3.24+ and a C++20 compiler to build static libraries for the target ABI. The main target is `CyDSP::CyDSP`; `CyDSP::Stretch` adds the optional stretching module. Windows x64 and Android ARM64 have runtime coverage. Other target and SVE execution limits are listed in the [version notes](VERSION.md).

After installing the CMake package, link it from your application:

```cmake
find_package(CyDSP 0.3.2 CONFIG REQUIRED)

target_link_libraries(MyApplication PRIVATE CyDSP::CyDSP)
# Optional time/pitch processing:
# target_link_libraries(MyApplication PRIVATE CyDSP::Stretch)
```

Public headers live under `cydsp/`; `cydsp/CyDSP.h` is the umbrella header. Start with the [processing example](examples/Processing.cpp), [granulation/analysis example](examples/GranulationAndAnalysis.cpp), [spatial/DAW example](examples/SpatialAndDaw.cpp) or [shared-HRTF/source-batch example](examples/AdvancedAudio.cpp).

## Source, build and required libraries

The repository includes five pinned MIT numerical adaptations and their complete notices. Configuration does not download dependencies. C++20 and CMake are required to use the library; Python is needed only for the repository's build/validation helper scripts.

From an authenticated checkout, build and install the Windows x64 static libraries:

```sh
git clone https://github.com/MrMybal/CyDSP.git
cd CyDSP
cmake -S . -B build/release -G "Visual Studio 17 2022" -A x64
cmake --build build/release --config Release
cmake --install build/release --config Release --prefix build/install
```

Set `CMAKE_PREFIX_PATH` in your application to the chosen install prefix. Set `CYDSP_BUILD_STRETCH=OFF` when only the core is needed. Tests, measurements and compile checks are opt-in; production builds create static libraries. The portable library does not require a global native-CPU architecture flag.

---

## License

CyDSP uses the [MIT license](LICENSE), copyright © 2026 MrMybal CyberAlien. Commercial use in closed-source products is permitted under its terms. Retain the complete [third-party notices](docs/Licensing.md); the table below is an index. The Stretch adaptation is used only by the optional `CyDSP::Stretch` target.

| Library · source | License · terms | Use / scope |
| --- | --- | --- |
| [DSPFilters](https://github.com/vinniefalco/DSPFilters) | [MIT](third_party/licenses/DSPFilters.txt) | Filter coefficient/prototype equations adapted to CyDSP preparation; processing uses CyDSP filters and cascades. |
| [FFTConvolver](https://github.com/HiFi-LoFi/FFTConvolver) | [MIT](third_party/licenses/FFTConvolver.txt) | Partitioning/overlap-add ideas adapted to CyDSP streaming and MIMO convolution, using CyDSP FFT and vector kernels. |
| [r8brain-free-src](https://github.com/avaneev/r8brain-free-src) | [MIT](third_party/licenses/r8brain-free-src.txt) | Windowed-sinc and window equations adapted to CyDSP resampling; not the full upstream SRC algorithm. |
| [signalsmith-linear](https://github.com/Signalsmith-Audio/linear) | [MIT](third_party/licenses/signalsmith-linear.txt) | FFT reconstruction and STFT/window normalization adapted to the CyDSP numerical core. |
| [signalsmith-stretch](https://github.com/Signalsmith-Audio/signalsmith-stretch) | [MIT](third_party/licenses/signalsmith-stretch.txt) | Time/pitch/formant algorithm adapted into the optional `CyDSP::Stretch` target; upstream files retained as provenance. |

## References and links

Read the [online CyDoc manual](https://mrmybal.github.io/CyDSP/). For offline use, open `Documentation/Documentation.html` from a local checkout and keep the entire `Documentation/` folder together.

- [Complete CyDoc manual](https://mrmybal.github.io/CyDSP/Documentation.html#/en/overview) — guides and public C++ API.
- [Processing contracts](https://mrmybal.github.io/CyDSP/Documentation.html#/en/contracts) · [Public C++ API](https://mrmybal.github.io/CyDSP/Documentation.html#/en/api).
- [Granulation](https://mrmybal.github.io/CyDSP/Documentation.html#/en/granular) · [Audio analysis](https://mrmybal.github.io/CyDSP/Documentation.html#/en/analysis) · [Spatial audio](https://mrmybal.github.io/CyDSP/Documentation.html#/en/spatial).
- [Version notes](VERSION.md).
- [Source repository](https://github.com/MrMybal/CyDSP) · [Issue tracker](https://github.com/MrMybal/CyDSP/issues).
