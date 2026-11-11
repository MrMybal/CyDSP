# CyDSP

Standalone C++20 audio DSP with one namespace (`cydsp`), common planar buffers,
explicit preparation and reusable processing memory. Selected MIT algorithms are
adapted into the same implementation conventions; upstream versions and notices
are preserved. No engine, JUCE, JIT or embedded compiler dependency is required.
The application must use the matching target ABI and standard C++ runtime.

Primary targets are **x64 and ARM64**. x86/ARM32 remain secondary compatibility
targets; new optimization and integration work prioritizes 64-bit applications.

## Implemented

| Area | Current implementation |
| --- | --- |
| Audio storage | Planar `AudioBlock<T>`, zero-copy sub-blocks, aligned `AudioBuffer<T>`, planar/interleaved conversion |
| Channel mixing | Rectangular SIMD matrices, in-place routing/permutations and frame-consistent coefficient ramps |
| Vector math | Copy, scale, add-scaled, fill, multiply, dot, complex multiply-accumulate and FFT butterflies |
| CPU backends | Scalar, SSE2, AVX2, AVX-512, NEON, scalable SVE; preparation-time selection and explicit backend requests |
| Gain | Constant SIMD gain and frame-consistent linear ramps |
| IIR / EQ | Biquads, peaking/shelving designs, cascades of up to 64 sections |
| Parallel / automated filters | Per-voice SIMD banks, single-section and full-EQ queued output transitions |
| FIR | Causal direct filtering using SIMD dot kernels, independent channel history |
| Fourier transforms | Radix-2 complex FFT with whole-stage SIMD dispatch and cross-group small stages; packed real FFT |
| Convolution | Uniform/hybrid/two-tier nonuniform with distributed MAC work, shared-FFT MIMO with optional progressive late work, prewarmed IR transitions; common selectable hybrid/nonuniform/progressive engines, including asynchronous IR updates |
| Sample rates | Rational resampling, SIMD double accumulation, quality presets, history-preserving edits, prepared real-time ratio ramps and explicit end-of-stream draining |
| Time / windows | Integer/fractional delay with optional ramps; rectangular, Hann, Hamming and Blackman windows |
| Audio graph | Prepared DAG, topological order, branch mixing, SIMD routing-chain fusion and buffer reuse from last-use analysis |
| Integration | Static `CyDSP::CyDSP` CMake target and relocatable install/export package |

All processors support float and double. Biquad and BiquadCascade retain their
scalar per-channel recurrence. BiquadBank vectorizes independent voices with
per-voice coefficients; SmoothedBiquad crossfades two banks with fixed coefficients.
Resampling uses double precision with scalar/SSE2/AVX2/AVX-512/ARM64 NEON/SVE dot implementations;
ARM32 double work falls back to scalar.
Native measurements and numerical/allocation regressions run in explicit validation
builds; their machine-specific results are recorded in docs/Measurements.md. No
performance superiority on unmeasured workloads or hardware is implied.

MIT adaptations: DSPFilters coefficient equations, FFTConvolver uniform partitioning,
and r8brain windowed-sinc/window equations. This is not the complete upstream
r8brain converter. See [licensing and provenance](docs/Licensing.md).
The owner has not selected a release license for original CyDSP code.

## Build and use

CMake 3.24+ and a C++20 compiler are required. On the installed Windows toolchain:

```text
python -B scripts/build.py --preset windows-x64
python -B scripts/build.py --preset windows-x86
python -B scripts/build.py --preset windows-x64-scalar
python -B scripts/build_arm64.py
```

The standard wrapper finds CMake on PATH or in VS 2022 Community; pass `--cmake PATH`
to override. Optional `CYDSP_ENABLE_AVX512=OFF` and `CYDSP_ENABLE_SVE=OFF` omit
those private kernels; `CYDSP_ENABLE_SIMD=OFF` omits all explicit SIMD kernels.
The ARM64 cross-build wrapper uses installed clang-cl/llvm-lib, Ninja
and MSVC/SDK headers, with explicit path overrides available. Each command produces
a static archive and keeps native tests off. Install different architectures into
separate prefixes and use the package matching the application target.
For other toolchains, configure directly:

```text
cmake -S . -B build/local -DCMAKE_BUILD_TYPE=Release -DCYDSP_BUILD_TESTS=OFF
cmake --build build/local --config Release
cmake --install build/local --config Release --prefix build/stage
```

Use `add_subdirectory` then `target_link_libraries(app PRIVATE CyDSP::CyDSP)`, or
install and `find_package(CyDSP 0.1 CONFIG REQUIRED)`. Headers are under
`include/cydsp`; `<cydsp/CyDSP.h>` includes the full API.

Call `prepare` outside the audio callback. `SmoothedConvolver::stageImpulse` also
prepares memory/FFT plans outside the callback and requires serialization with
processing; `startTransition` prewarms the new IR on live input before fading,
without allocation. Pass `prewarm=false` for the explicit cold-history behavior. Reuse the prepared instance with
caller-owned planar views. Check every returned status. For resampling, also
retain unconsumed input and use only the produced output prefix.

Choose an engine at preparation time with `ConvolutionConfig`. The same configuration
works with `Convolver`, `SmoothedConvolver`, `AsyncConvolver` and `MatrixConvolver`:

```cpp
const cydsp::ConvolutionConfig config{cydsp::ConvolutionMode::scheduled, 128, 1024};
cydsp::AsyncConvolver<float> reverb;
auto status = reverb.prepare({48000, 512, 2}, impulse, config);
// On one worker: reverb.stageImpulse(nextImpulse, 2048).
// On audio: reverb.process(input, output), with no plan replacement or destruction.
```

`hybrid` keeps a short direct head and one FFT partition size; `nonuniform` reduces
average long-IR work with two partition sizes; `scheduled` spreads late FFT work
across input frames and may cost more average CPU. Measure on the intended target.
Existing partition-only IR-transition overloads keep their hybrid behavior;
MatrixConvolver keeps its previous nonuniform default. Configuration
changes require non-real-time preparation; staged IRs inherit the prepared choice.

[Processing examples](examples/Processing.cpp), [module contracts](docs/Modules.md),
[architecture](docs/Architecture.md), [conventions](docs/Conventions.md) and
[roadmap](docs/Roadmap.md) describe the current boundaries.

## Validation

```text
python -B scripts/validate.py
```

This explicitly runs source/provenance checks, Windows x64/x86/scalar and ARM64/NEON
static builds, compile-only regression sources and a static installed-package
consumer. Logs and a machine-readable report go under `build/`. Individual commands
and unexecuted checks are recorded in [Validation](docs/Validation.md).

The static validation runner does not launch native code. The owner also explicitly
authorized separate native regression/measurement builds:

```text
python -B scripts/run_native.py --measure
python -B scripts/run_native.py --arch x86
python -B scripts/run_native.py --arch scalar
python -B scripts/run_native.py --sanitize
python -B scripts/run_native.py --compiler clang
python -B scripts/build_android.py --abi armeabi-v7a
```

Android builds cross-compile/link target tests without executing them. Actual ARM
hardware, desktop Linux/macOS and Unreal acceptance require their target environments.
The production presets remain static libraries with native test targets disabled.
