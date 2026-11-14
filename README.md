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
      <p><sub>Version</sub><br><a href="VERSION.md"><strong>0.3.1</strong></a><br><strong>IN DEVELOPMENT</strong></p>
    </td>
  </tr>
</table>

<h1 align="center">CyDSP</h1>

A C++20 audio DSP library for DAWs, effects, synthesizers and game audio. Build processing chains, render many sound sources and work with spatial audio through a common float/double API, prepared storage and SIMD backends for x64 and ARM64.

> **IN DEVELOPMENT · 0.3.1**
>
> Available as source; no binary release is published. Read the [version notes and platform limitations](VERSION.md).

## About CyDSP

CyDSP provides the numerical building blocks for audio applications: filters and dynamics, synthesis and sampling, spectral processing, analysis, restoration and 3D sound. Combine planar `AudioBlock` views, aligned `AudioBuffer` storage and a shared `ProcessSpec`; prepare algorithms and memory before the audio callback. Processing and reset retain prepared storage, with no allocation, lock or I/O in those paths. Lifecycle and parameter ownership are documented in the [architecture guide](docs/Architecture.md).

Prepared tables, samples and HRTFs can be shared across sources. Graphs support explicit sidechains, latency compensation between branches and transitions between prepared states. Economy, high-quality realtime and offline profiles supply a common preparation policy.

Audio devices, MIDI, audio file I/O, playback transport, plugin hosting and user interfaces belong to the consuming audio engine or **CyAudioCore**. CyDSP can be used independently in a DAW, game, effect or synthesizer.

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

Scalar, SSE2, AVX2, AVX-512, NEON and optional SVE implementations are selected during preparation from compiled support and CPU/OS capabilities. Callers can request a supported backend explicitly. ISA-specific code stays in private implementation files.

The [module index](docs/Modules.md), [DSP/spatial guide](docs/ExpansionGuide.fr.md) and [advanced API guide](docs/AdvancedExpansionGuide.fr.md) describe exact limits, ownership and latency. Several physical and musical models are approximations; select and assess them for the intended source and scene.

---

## Installation

Use CMake 3.24+ and a C++20 compiler to build static libraries for the target ABI. The main target is `CyDSP::CyDSP`; `CyDSP::Stretch` adds the optional stretching module. Windows x64 and Android ARM64 have runtime coverage. Other target and SVE execution limits are listed in the [version notes](VERSION.md).

After installing the CMake package, link it from your application:

```cmake
find_package(CyDSP 0.3.1 CONFIG REQUIRED)

target_link_libraries(MyApplication PRIVATE CyDSP::CyDSP)
# Optional time/pitch processing:
# target_link_libraries(MyApplication PRIVATE CyDSP::Stretch)
```

Public headers live under `cydsp/`; `cydsp/CyDSP.h` is the umbrella header. Start with the [processing example](examples/Processing.cpp), [granulation/analysis example](examples/GranulationAndAnalysis.cpp), [spatial/DAW example](examples/SpatialAndDaw.cpp) or [shared-HRTF/source-batch example](examples/AdvancedAudio.cpp). Existing consumers can follow the [migration guide](docs/Migration.fr.md).

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

- [Module index and API contracts](docs/Modules.md) · [Architecture](docs/Architecture.md).
- [Granulation and analysis](docs/GranulationAndAnalysis.md) · [DSP/spatial integration](docs/ExpansionGuide.fr.md) · [Advanced APIs](docs/AdvancedExpansionGuide.fr.md).
- [Migration guide](docs/Migration.fr.md) · [Version notes](VERSION.md).
- [Source repository](https://github.com/MrMybal/CyDSP) · [Issue tracker](https://github.com/MrMybal/CyDSP/issues).
