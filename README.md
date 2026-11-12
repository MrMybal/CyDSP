<table width="100%">
  <tr>
    <td width="300" align="left" valign="top">
      <a href="docs/Modules.md">Documentation</a><br>
      <a href="examples/Processing.cpp">Processing examples</a><br>
      <a href="https://github.com/MrMybal/CyDSP">Repository · private</a><br><br>
      <sub>README</sub><br>
      <a href="README.md">English</a> · <a href="README.fr.md">Français</a>
    </td>
    <td width="400" align="center" valign="middle">
      <strong>CyDSP</strong><br>
      <sub>C++20 audio DSP</sub>
    </td>
    <td width="300" align="right" valign="top">
      <p><sub>Library license</sub><br><a href="LICENSE"><strong>MIT</strong></a></p>
      <p><sub>Version</sub><br><a href="VERSION.md"><strong>0.1.0</strong></a><br><strong>IN DEVELOPMENT</strong></p>
    </td>
  </tr>
</table>

<h1 align="center">CyDSP</h1>

A C++20 audio processing library for developers of audio applications, plug-ins and games. Combine filters, transforms, convolution and resampling through one API, with CPU implementations for modern x64 and ARM64 systems.

> **IN DEVELOPMENT · 0.1.0**
>
> Available as source; no binary release is published. Build for your target and read the [version notes and documented limitations](VERSION.md).

## About CyDSP

CyDSP builds as a static library and uses the `cydsp` namespace. Processors share planar audio blocks, explicit status returns and `float`/`double` sample types. Primary targets are **x64 and ARM64**; x86 and ARM32 remain secondary compatibility targets.

Prepare processors and their memory before entering the audio callback, then reuse the prepared instances. Processing and reset retain that memory without allocating or freeing it. CPU backend selection happens during preparation, with an explicit backend option when needed. Optional static modules supply audio/MIDI devices, audio-file I/O and time/pitch processing. The host owns callback scheduling; the optional Sources module supplies dedicated file workers.

See the [processing examples](examples/Processing.cpp) and [module contracts](docs/Modules.md) for buffer ownership, status handling and real-time use.

## Features

- **Shared audio API:** planar blocks, aligned buffers, interleaved conversion and channel matrices.
- **Filters and gain:** gain ramps, FIR, biquads, cascades, parallel voice banks and smooth EQ changes.
- **Transforms:** complex and real FFTs, vector arithmetic and window functions.
- **Convolution:** uniform, hybrid, nonuniform and progressive engines; shared-FFT MIMO and prepared impulse-response transitions.
- **Resampling and time:** rational conversion, prepared variable-rate ramps, explicit stream draining and integer/fractional delays.
- **Audio graphs:** prepared routing, branch mixing and compatible gain-chain fusion.
- **Generators and effects:** ADSR, oscillators, dynamics, TPT/crossover/ladder filters, oversampling, pan/mix, chorus, phaser and reverb.
- **Synthesis and MIDI:** prepared polyphonic oscillator/sampler/custom voices, MPE zones/RPN, sample-ordered MIDI 1/UMP/MIDI 2 events, [MIDI-CI/native UMP](docs/Midi2.md) and a [prepared property server/client](docs/CiProperty.md) with instances, partial updates, pagination and caching.
- **Playback and recording:** memory sources, queued transport/loops, tempo maps, worker read-ahead with shared sinc conversion and bounded background recording.
- **Optional modules:** audio/MIDI devices, WAV/AIFF/FLAC/Ogg/MP3 reading, WAV/AIFF/FLAC/Ogg writing, and pitch/time/formant processing. See the [coverage inventory](docs/Coverage.md) for restrictions and remaining work.
- **CPU implementations:** scalar, SSE2, AVX2, AVX-512, NEON and optional SVE. Availability depends on the target ABI, compiled kernels and CPU/OS support.
- **CMake integration:** static `CyDSP::CyDSP` target and an installable package.

---

The optional [Plugins module](docs/Plugins.md) hosts CLAP/VST3/LV2/JSFX with native editors and a separate worker process. The [integration remediation](docs/IntegrationRemediation.md) also covers metadata, devices, MIDI and sources.

## Installation

Access to the [private source repository](https://github.com/MrMybal/CyDSP) is required. Build a static library for the application's target ABI and use the matching C++ runtime. There is no installer or prebuilt download.

After building and installing with the steps below, add the install prefix to your application's `CMAKE_PREFIX_PATH`, then link its existing CMake target:

```cmake
find_package(CyDSP 0.1 CONFIG REQUIRED)
target_link_libraries(AudioApp PRIVATE CyDSP::CyDSP)
```

Here, `AudioApp` is the consuming application's target. Alternatively, include the CyDSP checkout with `add_subdirectory` and link the same `CyDSP::CyDSP` target. Include `<cydsp/CyDSP.h>` for the public API.


The optional modules have separate targets: `CyDSP::Files`, `CyDSP::Devices`,
`CyDSP::Stretch`, `CyDSP::Sources`, `CyDSP::Plugins`. Sources links Files. Request them as CMake package components and link the needed
targets. Disable unneeded modules with `CYDSP_BUILD_FILES=OFF`,
`CYDSP_BUILD_DEVICES=OFF`, `CYDSP_BUILD_STRETCH=OFF`, `CYDSP_BUILD_SOURCES=OFF`, `CYDSP_BUILD_PLUGINS=OFF`.
See the original [DAW building-block example](examples/DawBuildingBlocks.cpp); it is compiled without opening devices or files.

## Source, build and required libraries

### Requirements

- [CMake](https://cmake.org/cmake/help/v3.25/manual/cmake-presets.7.html) **3.25+** for the included presets, or **3.24+** for direct configuration; a **C++20** compiler with its standard library/runtime.
- Visual Studio 2022 with the C++ workload for the Windows presets.
- Git and authorized repository access to clone the source.
- Python only when using the optional scripts in `scripts/`; Android cross-builds additionally need the Android NDK.

The MIT adaptations and optional permissive backends are included with pinned source provenance. No separate DSP framework is required. Build tools are installed separately and retain their own terms.

### Get the source

```sh
git clone https://github.com/MrMybal/CyDSP.git
cd CyDSP
```

### Build and install

<details>
<summary>Windows x64 and other target toolchains</summary>

From a shell with CMake available, build the Windows x64 Release library:

```sh
cmake --preset windows-x64
cmake --build --preset windows-x64
cmake --install build/windows-x64 --config Release --prefix build/stage
```

For another configured C++20 toolchain:

```sh
cmake -S . -B build/local -DCMAKE_BUILD_TYPE=Release -DCYDSP_BUILD_TESTS=OFF
cmake --build build/local --config Release
cmake --install build/local --config Release --prefix build/stage
```

Use separate build/install folders for each target ABI. Production presets build static libraries with native tests disabled. `CYDSP_ENABLE_SIMD=OFF` selects the scalar implementation; `CYDSP_ENABLE_AVX512=OFF` or `CYDSP_ENABLE_SVE=OFF` omits those optional kernels. Other compiled backends remain available.

The [architecture guide](docs/Architecture.md) describes dispatch and platform boundaries. SVE is optional on compatible ARM64 Linux/Android targets; it is not required for ARM64 use. See the [version notes](VERSION.md) for platforms still awaiting native acceptance.

</details>

---

## License

**CyDSP uses the [MIT license](LICENSE).** Copyright © 2026 MrMybal CyberAlien.

### Third-party libraries

| Library · source | License · terms | Use / scope |
| --- | --- | --- |
| [DSPFilters](https://github.com/vinniefalco/DSPFilters) | [MIT notice](third_party/licenses/DSPFilters.txt) | Adapted RBJ filter-coefficient equations; compiled into CyDSP. |
| [FFTConvolver](https://github.com/HiFi-LoFi/FFTConvolver) | [MIT notice](third_party/licenses/FFTConvolver.txt) | Adapted partitioning and overlap-add for convolution; uses CyDSP's FFT. |
| [r8brain-free-src](https://github.com/avaneev/r8brain-free-src) | [MIT notice](third_party/licenses/r8brain-free-src.txt) | Adapted windowed-sinc/window equations; not the complete upstream sample-rate converter. |

| [miniaudio](https://github.com/mackron/miniaudio) | [MIT-0 alternative](third_party/licenses/miniaudio.txt) | Private audio-device backend. |
| [Windows MIDI Services declarations](https://github.com/microsoft/MIDI) | [MIT](third_party/licenses/windows-midi.txt) | Adapted private native UMP ABI; no driver/service bundled. |
| [RtMidi](https://github.com/thestk/rtmidi) | [MIT](third_party/licenses/rtmidi.txt) | Private MIDI-device backend. |
| [CHOC](https://github.com/Tracktion/choc) | [ISC and embedded codec notices](third_party/licenses/choc.txt) | WAV/FLAC/Ogg/MP3 file codecs; BSD/public-domain codec terms retained. |
| [Signalsmith Stretch](https://github.com/Signalsmith-Audio/signalsmith-stretch) | [MIT](third_party/licenses/signalsmith-stretch.txt) | Pitch/time/formant backend. |
| [Signalsmith Linear](https://github.com/Signalsmith-Audio/linear) | [MIT](third_party/licenses/signalsmith-linear.txt) | Adapted STFT/half-bin equations, using CyDSP FFT at runtime. |

| [CLAP](https://github.com/free-audio/clap) | [MIT](third_party/licenses/clap.txt) | Private interfaces; original host and ABI fixture. |
| [VST3 interfaces](https://github.com/steinbergmedia/vst3_pluginterfaces) | [MIT](third_party/licenses/vst3-interfaces.txt) | Original host, controller and native editor adapters. |
| [LV2/Lilv](https://github.com/lv2/lilv) | ISC | Private host, RDF and Atom dependencies; original CyDSP adapter. |
| [ysfx](https://github.com/jpcima/ysfx) | Apache 2.0 and embedded notices | JSFX runtime; original host/canvas, no JUCE wrapper; Bison parser exception retained. |

Retain CyDSP's LICENSE and the complete upstream notices. The [provenance manifest](third_party/manifest.json) pins revisions, archived files and adaptation scope; [licensing details](docs/Licensing.md) explain their distribution. This table is an index of those components and their terms.

## References and links

- [Module and API documentation](docs/Modules.md) · [Processing examples](examples/Processing.cpp).
- [Architecture](docs/Architecture.md) · [Audio conventions](docs/Conventions.md).
- [Version notes and limitations](VERSION.md).
- [Private source repository](https://github.com/MrMybal/CyDSP) · [Issue tracker](https://github.com/MrMybal/CyDSP/issues) — authorized GitHub access is required.

<details>
<summary><strong>AI Usage</strong></summary>

- [Start here: project agent guide](AGENTS.md) — project scope, development rules and important documentation.
- [Architecture](docs/Architecture.md) — CPU dispatch, processor lifetime and integration contracts.
- [Public API and module contracts](docs/Modules.md) — preparation, processing and buffer behavior.

</details>

The [DAW integration additions](docs/DawIntegration.md) cover extra filters, wavetables, filtered sampling, presets, worker stretch, waveform caches, reconnects, MIDI timing, UMP and AIFC/CAF files. Available Windows codecs also provide MP3/AAC/M4A/WMA export.
