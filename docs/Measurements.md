# Ten-lot DSP / spatial expansion — 2026-11-13

The latest extension results, exact source digest, CPU costs and numerical errors
are in [ExpansionValidation.fr.md](ExpansionValidation.fr.md). The
[API guide](ExpansionGuide.fr.md) defines the supported configurations and limits.
The older measurements and validation snapshots below are retained as history;
their hashes and coverage must not be attributed to the current extension.

---

# Granulation and audio-analysis measurements — 2026-11-12

Feature-source SHA-256: `9047361b0fbc9ba1372504c7cfecc9a8e9a06cdd4ea168dc1d7b1643f5262148`.
All rows process **128 stereo float frames at 48 kHz per complete public call**.
Times are **microseconds per call**, not speedup ratios. x64 uses AMD Ryzen 9 7900X,
MSVC 19.40.33821.0 Release with AVX-512 kernels where applicable; Android uses
Quest 3 ARM64/NEON. Levels and loudness have no explicit SIMD backend selection.
Each operation receives 24 warmup calls and 400 individually timed calls. No CPU
frequency lock/affinity or global floating-point changes are applied. These are
separate observations on each platform, not a controlled CPU comparison.

| Operation | x64 mean us | x64 p95 us | Quest mean us | Quest p95 us |
| --- | ---: | ---: | ---: | ---: |
| Granulator, 8 active grains | 29.648 | 29.800 | 26.628 | 26.615 |
| Granulator, 32 active grains | 117.135 | 118.800 | 111.286 | 179.844 |
| Levels, 2048-frame window | 3.980 | 4.000 | 6.147 | 6.198 |
| True peak, 4x / 64 taps | 8.087 | 8.200 | 22.675 | 22.448 |
| Spectrum, N=2048 / hop=512 | 7.088 | 26.200 | 22.969 | 84.739 |
| Constant-Q, 48 bins / hop=512 | 6.651 | 24.500 | 28.264 | 108.385 |
| Loudness process, 256-entry capacity | 2.718 | 2.800 | 5.127 | 5.105 |

Granulation borrows a prepared 8192-sample mono source, uses band-limited reading
at ratio 1.5, Hann envelopes, fixed positions and spread pans, and renders all
8/32 active grains for the entire run. Preparation, cloud triggering and source-mip
construction are excluded. Mono sampling/envelope work is shared across stereo
outputs, followed by selected VectorOps accumulation. The maximum block size is
128, and 60-second grain duration prevents voices ending during timing.

Analysis reads a repeated 128-frame sinusoidal input block. FFT and constant-Q
means amortize work across the 512-frame hop: some calls only collect input, while
others calculate a new frame. Their p95 includes those analysis calls. Constant-Q
uses 220 Hz minimum and 12 bins/octave, with its longest kernel filled before timing.
The loudness row measures processing/current-level access only, not integrated
history scans or LRA sorting. Its short-term window is not yet full during this
short timing run. Reported p95 values are individual-call observations, not worst
case real-time guarantees; outliers can make a mean exceed p95.

The harness also records double and every available forced backend: 44 new feature
performance cases on x64 and 24 on Quest. The complete suites retain 774 and 526
records respectively, including existing quality/performance records. Original
raw JSON is kept in `build/granulation-analysis-2026-11-12/`. Reproduce with the
native/device `--measure` runners described in [Validation.md](Validation.md).
Numerical and realtime evidence and limitations are recorded there; API units,
storage, latency and capacity rules are in [GranulationAndAnalysis.md](GranulationAndAnalysis.md).

The before/after optimization study below belongs to its earlier source snapshot;
its ratios are retained as historical measurements and are not claims about every
operation added here.

---

# CyDSP 0.2.0 measurements and optimizations

2026-11-12. Baseline: split commit `350523c`; measured on AMD Ryzen 9 7900X,
Windows x64, MSVC 19.40.33821.0 Release, `/O2 /fp:strict`. No fast-math, FMA contraction,
CPU affinity, frequency lock or global floating-point setting changes.
The baseline library/executables were retained before production edits. Both
versions use the same focused measurement source. This compares CyDSP with its
previous implementation; it is not a JUCE comparison.

## Measured changes

Each processor row uses **128 stereo frames at 48 kHz, float**. Times are
microseconds per complete public process call. **Speedup = before time / after
time**: 3x means the same measured operation takes one third of its former time.

| Operation | Before us | After us | Speedup |
| --- | ---: | ---: | ---: |
| Wavetable, 2048 points, 440 Hz | 2.791 | 0.969 | 2.88x |
| Gate, peak, no look-ahead | 8.244 | 2.513 | 3.28x |
| Gate, peak, 5 ms look-ahead | 9.363 | 3.378 | 2.77x |
| Gate, RMS, 5 ms look-ahead | 10.088 | 4.981 | 2.03x |
| Limiter, peak, no look-ahead | 9.691 | 8.134 | 1.19x |

Wavetables choose harmonic banks and their blend once per block, refreshing the
view on every call. Phase progression and interpolation remain unchanged. The gate
uses its existing linear gain directly, retains the -160 dB floor, and calculates
the final meter in dB once per block. Hold count, knee/slope, limiter ceiling and
shared delay-read position are moved outside their repeated channel/sample work.
Zero blocks preserve state. Compressor/expander cases are controls; this change
does not establish a general speedup for those modes.

AVX-512 copy, scale, addScaled and fill use unmasked complete vectors followed by
one masked remainder. Examples below include public validation/dispatch overhead,
use **128 scalar elements with offset 1**, and report **nanoseconds per call**.
They are separate vector operations, not stereo audio blocks.

| AVX-512 operation / sample type | Before ns | After ns | Speedup |
| --- | ---: | ---: | ---: |
| Copy / float | 10.94 | 10.16 | 1.08x |
| Scale / float | 11.52 | 10.16 | 1.13x |
| Add scaled / float | 12.89 | 12.30 | 1.05x |
| Fill / double | 15.04 | 11.33 | 1.33x |

The proposed complex-MAC rewrite regressed measured cases and was removed. Simple
multiplication showed no clear benefit and retains its previous implementation.
Untouched dot/FFT/convolution paths remain in the general measurement suite.
Automatic ISA selection still follows CPU/OS support; these results do not justify
changing the dispatch policy for all processors or other CPUs. Sample-by-sample
wavetable next/value probes also showed no slowdown in this run; their local raw
results are in `build/optimization-2026-11-12/next-comparison.json`.

## Repetition and limits

`tests/FocusedMeasurements.cpp` supplies 680 float/double cases: available scalar,
SSE2, AVX2 and AVX-512 vectors at lengths 16/31/128/511/4096 and offsets 0/1;
dynamics at 32/128/512 frames with peak/RMS and 0/5 ms look-ahead; wavetable blocks
at four fixed frequencies. Offset zero does not promise 64-byte alignment.

Six before/after pairs alternate executable order. Each pair uses the same
shuffled case seed. Each case warms up for two batches, then records 51 timed
batches (256 vector calls, 16 dynamics blocks or 32 oscillator blocks per batch).
Tables use the median of the six trial medians. Raw p95BatchMeanNs values describe
batch-average costs, not individual callback percentiles or real-time bounds.
The original general suite separately retains single-call p95/p99/max records.
The smaller nanosecond cases remain sensitive to code layout, caches and OS noise;
unchanged control cases also vary. A whole-library speedup cannot be inferred.

Original executables and before/after JSON are retained locally under
`build/optimization-2026-11-12/`. The [compact result data](measurements/optimization-2026-11-12.json)
retains selected raw trial medians, compiler-input hashes, quality and platform
coverage. The complete comparison is `build/optimization-2026-11-12/final-comparison.json`.
To reproduce, build the opt-in `cydsp_focused_measurements` target separately at
each revision with the same harness, retain both executables, then run:

```sh
python -B scripts/compare_optimizations.py --before path/to/before.exe --after path/to/after.exe --baseline-report path/to/native-before.json --output build/comparison.json --trials 6
```

## Numerical, spectral and realtime validation

MSVC x64, clang-cl x64, explicit-SIMD-disabled x64, x64 AddressSanitizer and
Quest 3 ARM64 scalar/NEON regressions pass. The independent dynamics oracle scans
the look-ahead window directly and retains the former dB/gain equations. It covers
all four modes, peak/RMS, linked/unlinked transitions, knee changes, silence,
hold/hysteresis, changing delay, reset, in-place processing and irregular/empty
blocks. Maximum absolute sample error is 0 for float and 5.55112e-17 for double;
the final gain-reduction meter matches exactly on both tested machines.
Wavetable tests cover block/next equality, phase/frequency changes, add mode,
Nyquist, moved/reprepared tables and invalid/empty blocks. Existing independent
harmonic-projection checks pass. Allocation/free auditing passes for process/reset.
ASan tail tests include offset sources ending at the last valid sample.

| Quality observation | x64 | Quest ARM64 |
| --- | ---: | ---: |
| Numerical/spectral quality records | 321 | 285 |
| Worst float FFT roundtrip absolute error | 5.901e-7 | 5.901e-7 |
| Worst double FFT roundtrip absolute error | 1.510e-15 | 1.299e-15 |
| Maximum sampled preset passband error, dB | 0.001333 | 0.001333 |
| Weakest sampled preset stopband gain, dB | -79.127 | -79.127 |

All 321 x64 quality records are identical to the retained baseline. The general
runs contain 730 x64 and 502 Quest records including performance/capability rows.
These are finite numerical and spectral grids, without listening tests or physical
audio I/O. The connected Quest does not advertise SVE. SVE is compiled, linked and
disassembled in the Android ARM64 build, with runtime still unverified. Windows
ARM64 libraries/headers compile; Windows ARM64 execution, desktop Linux and macOS
remain unverified. See [validation](Validation.md).

<details>
<summary>Earlier monolithic measurements, retained for history</summary>

The following numbers precede the project split and do not establish current
platform coverage or before/after gains for the changes above.

# Historical monolithic measurements

2026-11-12. Explicit Release validation runs; strict floating-point policy, no
fast-math/FMA contraction or global CPU-environment changes. Raw JSON retains
all cases. Native programs perform numerical audio DSP without opening an audio
device; no listening or physical audio I/O measurement is claimed.

## Performance samples

All rows below are float, two-channel convolution of a 32768-tap IR in 128-frame
blocks. Uniform hybrid uses P=128; nonuniform uses early=128 and late=1024.
Times are microseconds per call, including normal OS scheduling noise.

| Host / backend | Algorithm | Mean us | Median us | p99 us | Maximum us |
| --- | --- | ---: | ---: | ---: | ---: |
| Ryzen 9 7900X / scalar | hybrid128 | 67.13 | 66.90 | 77.60 | 83.40 |
| Ryzen 9 7900X / scalar | nonuniform128 | 27.71 | 24.10 | 56.00 | 57.10 |
| Ryzen 9 7900X / sse2 | hybrid128 | 28.83 | 28.70 | 32.70 | 46.90 |
| Ryzen 9 7900X / sse2 | nonuniform128 | 11.79 | 9.10 | 31.20 | 34.90 |
| Ryzen 9 7900X / avx2 | hybrid128 | 19.69 | 19.60 | 25.10 | 27.60 |
| Ryzen 9 7900X / avx2 | nonuniform128 | 9.15 | 7.20 | 23.30 | 27.00 |
| Ryzen 9 7900X / avx512 | hybrid128 | 19.00 | 18.90 | 26.10 | 30.20 |
| Ryzen 9 7900X / avx512 | nonuniform128 | 9.44 | 7.60 | 22.40 | 25.50 |
| Quest 3 ARM64 / scalar | hybrid128 | 115.31 | 114.74 | 129.17 | 144.84 |
| Quest 3 ARM64 / scalar | nonuniform128 | 55.04 | 47.40 | 110.62 | 120.63 |
| Quest 3 ARM64 / neon | hybrid128 | 99.89 | 99.32 | 112.86 | 120.47 |
| Quest 3 ARM64 / neon | nonuniform128 | 33.33 | 26.93 | 80.73 | 92.29 |
| Quest 3 ARM32 / scalar | hybrid128 | 266.54 | 268.49 | 506.77 | 1229.22 |
| Quest 3 ARM32 / scalar | nonuniform128 | 93.13 | 77.86 | 208.33 | 212.24 |
| Quest 3 ARM32 / neon | hybrid128 | 153.28 | 151.46 | 229.84 | 238.18 |
| Quest 3 ARM32 / neon | nonuniform128 | 77.65 | 48.44 | 431.35 | 663.02 |

The nonuniform late tier runs less often, so lower average work can coexist with
larger callback peaks. Choose a configuration against both mean and tail cost.
These are sampled call distributions, not hard real-time bounds. No CPU affinity,
frequency lock, thermal policy or OS/antivirus setting was changed. ARM32 runs in
the Quest's compatibility mode; this is not a second independent ARM device.

Each operation has 24 warmup calls and 400 measured samples. Dot products batch
256 calls per timing sample to reduce clock quantization; their percentiles are
of batch-average costs. Other reported calls use batchSize=1. Compiler/ABI/sample
type and dimensions are in raw records. Measurements include dot, complex FFT,
voice-parallel filters, FIR, hybrid/nonuniform convolution, shared versus independent
MIMO, fused versus callback graph chains, and rational/variable resampling.

## Numerical and spectral checks

The finite resampling grid uses four rate pairs (48->44.1, 44.1->48, 48->16 and
96->44.1 kHz), three presets, 10 passband frequencies through 0.8 times the lower
Nyquist frequency and 12 stopband frequencies from 1.05 times output Nyquist
through 0.98 times input Nyquist when downsampling. Tones are projected over
8192 output frames after startup. Preset acceptance: absolute sampled passband
error <0.1 dB and sampled stopband gain <-60 dB. Additional raw 64/128/256-tap
configurations are measured independently without declaring every setting high quality.

| Host | Measurement records | Maximum preset passband error dB | Weakest sampled preset rejection dB | Worst float FFT roundtrip error | Worst double FFT roundtrip error |
| --- | ---: | ---: | ---: | ---: | ---: |
| Windows x64 | 734 | 0.00133235 | -79.13 | 5.9e-07 | 1.51e-15 |
| Quest ARM64 | 506 | 0.00133235 | -79.13 | 5.9e-07 | 1.3e-15 |
| Quest ARM32 | 506 | 0.00133235 | -79.13 | 5.9e-07 | 1.3e-15 |

The raw 64-tap configuration reached -0.449 dB gain at
6400.39 Hz for 48000->16000 Hz.
That finding motivated the 128-tap raw default and ratio-aware support in the
quality presets. Explicit short filters remain available for a chosen CPU/quality
tradeoff; they do not inherit the preset acceptance results.

The 16->48 kHz image test projects a 1 kHz source onto its 15 kHz image using a
Hann window; all three presets must keep that image below -60 dB. FFT checks cover
64..4096 points, both sample types and every available backend. A 512-frame linear
IR transition between gains +1 and -1 on constant input must follow its reference
trajectory within 1e-12 and limit each step to 2/512. These pass on Windows and both
Quest ABIs. Variable-ratio DC measurements sweep spacing through [0.75,3], with
2048-output-frame ramps after startup, and require error/sample steps <1e-10.
Default IR prewarming supplies complete finite destination history before fading;
explicit cold mode, phase cancellation and deliberately faded old tails remain available.

Raw data is generated locally in `build/native-x64.json`,
`build/device-arm64-v8a.json` and `build/device-armeabi-v7a.json`.
See [validation](Validation.md) for the executed checks and coverage.

## Same-build optimization comparisons

Float, 128-frame blocks. MIMO is 4 inputs x 4 outputs, 16 routes, 8192 taps per route; graph is a 32-stage two-channel gain chain. Both alternatives use the same selected backend. Fused kernels process four independent register groups per gain. Ratios use sampled mean call times.

| Host / backend | Shared MIMO us | Independent MIMO us | Ratio | Fused graph us | Callback graph us | Ratio |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Ryzen 9 7900X / avx512 | 29.69 | 72.93 | 2.46x | 0.34 | 3.96 | 11.63x |
| Quest ARM64 / neon | 123.84 | 224.70 | 1.81x | 2.47 | 6.43 | 2.61x |
| Quest ARM32 / neon | 192.70 | 354.90 | 1.84x | 3.06 | 10.80 | 3.53x |

The reference MIMO executes one NonuniformConvolver per route and sums its outputs. The reference graph retains identity callbacks at each stage. Fusion applies to null routing nodes whose semantics are known; this does not fuse arbitrary processors. Costs and ratios depend on workload, ISA, scheduling and hardware. CPU/OS capability records explicitly distinguish AVX-512 execution from SVE compile/link-only coverage.

## Whole-stage FFT and progressive convolution

The FFT comparison uses a 1024-point float forward transform and the same prepared tables. The reference retains the former group dispatcher and scalar tiny stages. All timings include input permutation; this is a same-build comparison, not a cross-library ranking.

| Host / backend | Full-stage FFT us | Group reference us | Reference / full-stage |
| --- | ---: | ---: | ---: |
| Ryzen 9 7900X / avx512 | 1.52 | 2.96 | 1.95x |
| Quest ARM64 / neon | 8.16 | 8.80 | 1.08x |
| Quest ARM32 / neon | 13.16 | 14.88 | 1.13x |

The convolution rows use the same 32768-tap IR, two float channels, 128-frame callbacks, early partition 128 and late partition 1024. ScheduledConvolver covers the first 2048 taps with its hybrid prefix, then spreads tail FFT/recombination/MAC/overlap work across a full input interval. NonuniformConvolver covers 1024 early taps and retains boundary transforms. Both produce the same causal response with zero added block latency.

| Host / backend | Nonuniform mean us | Scheduled mean us | Mean cost ratio | Nonuniform p99 us | Scheduled p99 us |
| --- | ---: | ---: | ---: | ---: | ---: |
| Ryzen 9 7900X / avx512 | 9.44 | 10.31 | 1.09x | 22.40 | 11.80 |
| Quest ARM64 / neon | 33.33 | 34.35 | 1.03x | 80.73 | 47.24 |
| Quest ARM32 / neon | 77.65 | 58.77 | 0.76x | 431.35 | 71.04 |

Job counts bound the scheduled tail only, not the synchronous prefix or OS scheduling. Chunk costs differ by phase. The sampled p99/max distributions are not hard deadline guarantees. Both modes remain selectable because reducing peaks may increase average CPU. No affinity/frequency or device setting was changed.

| Host | Worst float scheduled/direct error | Worst double scheduled/direct error |
| --- | ---: | ---: |
| Ryzen 9 7900X | 2.09e-07 | 4.16e-16 |
| Quest ARM64 | 2.09e-07 | 4.16e-16 |
| Quest ARM32 | 2.09e-07 | 4.16e-16 |

The numerical grid uses a 521-tap finite IR, 769 nonzero input frames followed by zeros, two channels, 137-frame callback limits and 64-frame late partitions. Every available backend is checked against explicit time-domain convolution, including startup and the complete tail.

## Common convolution interface and active IR fades

The same 32768-tap IR, two float channels and 128-frame callbacks use early=128 and late=1024. Configuration and IR publication are outside timing. Fade measurements follow full destination prewarming; a 1048576-frame fade keeps both engines active through all 24 warmups and 400 measured calls. The asynchronous benchmark measures the consumer with no concurrent producer.

| Host / backend | Engine | Direct mean us | Common interface mean us | Serialized fade mean / p99 us | Async fade mean / p99 us |
| --- | --- | ---: | ---: | ---: | ---: |
| Ryzen 9 7900X / avx512 | hybrid | 19.00 | 19.15 | 44.25 / 57.70 | 43.86 / 67.50 |
| Ryzen 9 7900X / avx512 | nonuniform | 9.44 | 9.61 | 20.01 / 45.40 | 20.40 / 47.30 |
| Ryzen 9 7900X / avx512 | scheduled | 10.31 | 11.18 | 21.80 / 27.20 | 23.07 / 26.30 |
| Quest ARM64 / neon | hybrid | 99.89 | 86.50 | 191.81 / 631.09 | 182.94 / 597.97 |
| Quest ARM64 / neon | nonuniform | 33.33 | 32.76 | 68.88 / 163.65 | 69.81 / 163.59 |
| Quest ARM64 / neon | scheduled | 34.35 | 39.66 | 78.24 / 95.21 | 78.39 / 96.46 |
| Quest ARM32 / neon | hybrid | 153.28 | 152.21 | 352.35 / 374.12 | 355.43 / 418.91 |
| Quest ARM32 / neon | nonuniform | 77.65 | 65.14 | 106.03 / 265.57 | 106.44 / 264.95 |
| Quest ARM32 / neon | scheduled | 58.77 | 58.30 | 117.66 / 134.69 | 118.63 / 165.57 |

Separate runs have normal scheduling noise, so small direct/interface differences do not establish dispatch overhead. Transition peaks include two active convolution banks and output blending. Sampled distributions do not guarantee real-time deadlines.

| Host | Worst float IR-update error | Worst double IR-update error |
| --- | ---: | ---: |
| Ryzen 9 7900X | 2.68e-07 | 5e-16 |
| Quest ARM64 | 2.68e-07 | 5.27e-16 |
| Quest ARM32 | 2.68e-07 | 5.27e-16 |

The numeric grid changes a unit IR at frame 131 to a dense 273-tap IR, prewarms for 272 frames, then fades for 37 frames. Two channels and arbitrary callbacks up to 53 frames include startup, live history and complete tail. Both serialized and asynchronous outputs are compared to direct time-domain references for every available backend and sample type. Regression tests additionally cover cold/instant transitions, failed replacement, queued long-IR publications, moves and allocation/free accounting.

## Progressive shared MIMO

These 4-input/4-output cases use 16 routes, identical finite IR lengths, float samples and 128-frame callbacks. Both modes retain one forward transform per active input and one inverse per active output. Early partition=128, late=1024. Progressive prefix coverage extends to 2048 taps; the nonuniform prefix ends at 1024.

| Host / backend | Taps per route | Nonuniform mean / p99 us | Progressive mean / p99 us | Mean cost ratio |
| --- | ---: | ---: | ---: | ---: |
| Ryzen 9 7900X / avx512 | 8192 | 29.69 / 57.30 | 32.71 / 39.10 | 1.10x |
| Ryzen 9 7900X / avx512 | 32768 | 38.47 / 66.20 | 46.92 / 53.50 | 1.22x |
| Quest ARM64 / neon | 8192 | 123.84 / 236.98 | 148.16 / 179.69 | 1.20x |
| Quest ARM64 / neon | 32768 | 200.65 / 539.95 | 208.99 / 267.24 | 1.04x |
| Quest ARM32 / neon | 8192 | 192.70 / 373.44 | 224.40 / 255.10 | 1.16x |
| Quest ARM32 / neon | 32768 | 337.40 / 570.78 | 388.47 / 442.14 | 1.15x |

The scheduled late budget is 2528 chunks per 1024-frame interval (at most 3/frame) for 8192 taps, or 9056 (at most 9/frame) for 32768. Those counts exclude direct/early-tier work. Each phase has a different cost; a job count does not bound wall time. The larger prefix and scheduling can increase average CPU; sampled p99/max cannot guarantee an audio deadline and are subject to OS/device variation. No automatic mode switch is introduced.

| Host | Worst float MIMO/direct error | Worst double MIMO/direct error |
| --- | ---: | ---: |
| Ryzen 9 7900X | 8.2e-08 | 1.73e-16 |
| Quest ARM64 | 8.2e-08 | 1.67e-16 |
| Quest ARM32 | 8.2e-08 | 1.67e-16 |

The numerical measurement uses 4 inputs, 5 outputs, 6 sparse/duplicate routes with unequal dense IR lengths 1..521, 64-frame late partitions, 769 source frames followed by zeros and variable callbacks up to 137 frames. Independent double time-domain convolution covers every output and the full tail. Regressions also cover all three MIMO modes, rectangular exact aliases, smaller/larger partitions and invalid requests before state mutation.

## Expanded DSP (128 frames, stereo, 48 kHz)

Times below are microseconds per 128-frame callback, float samples, mean / p99. The audio period at 48 kHz is 2666.67 us. These are single-process synthetic workloads, not physical audio latency or universal speedup ratios. Scalar labels describe these processors without an explicit SIMD implementation; oversampling requests the automatic FIR backend.

| Host | Dynamics | TPT | Ladder | Chorus | Phaser | Reverb | FIR4x roundtrip |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Ryzen 9 7900X | 7.36 / 7.40 | 1.55 / 1.60 | 3.93 / 4.00 | 1.92 / 2.10 | 8.46 / 9.30 | 3.45 / 3.90 | 22.70 / 23.20 |
| Quest ARM64 | 19.57 / 25.94 | 3.37 / 3.39 | 12.35 / 12.45 | 5.18 / 5.47 | 13.46 / 13.59 | 9.03 / 9.12 | 57.90 / 72.92 |
| Quest ARM32 | 29.11 / 36.62 | 2.63 / 2.66 | 13.95 / 14.69 | 7.43 / 7.86 | 14.05 / 14.38 | 9.87 / 10.05 | 108.63 / 339.53 |

| Host | Float alias reduction dB | Double alias reduction dB |
| --- | ---: | ---: |
| Ryzen 9 7900X | -104.23 | -104.73 |
| Quest ARM64 | -104.89 | -104.73 |
| Quest ARM32 | -104.89 | -104.73 |

The quality fixture cubes a 0.8-amplitude 9.6 kHz sine at 48 kHz. Without oversampling the third harmonic aliases to 19.2 kHz with amplitude 0.8^3/4. Values are 20log10(measured alias amplitude/reference amplitude), so more negative means stronger rejection. FIR4x adds 32 input frames of latency. This one tone does not establish rejection for every frequency or nonlinear processor.

## Integrated synthesis and Stretch

The synth workload uses sine/ADSR voices with sustained notes, stereo float output at 48 kHz and 128-frame callbacks. Tables report mean microseconds per callback; the available period is 2666.67 us. Voice setup and note publication are outside timing. This is CPU processing cost, not device latency.

| Host / selected backend | 16 voices mean us | 64 voices mean us |
| --- | ---: | ---: |
| Ryzen 9 7900X / avx512 | 12.04 | 52.06 |
| Quest ARM64 / neon | 33.00 | 136.42 |
| Quest ARM32 / neon | 91.03 | 357.29 |

Stretch compares the pinned unmodified portable Signalsmith engine against the adapted CyDSP engine in the same strict-FP Release binary. The integrated implementation uses CyDSP FFT/vector kernels, half-bin reconstruction, fused packing/permutation and compact 3×power-of-two plans. Original headers are compiled only in this opt-in benchmark. The upstream label scalar identifies its portable implementation, not a ban on compiler auto-vectorization. Both engines use two channels, 128-frame input/output calls and identical configurations: a 4096-frame window/1024 hop without split computation, and the 48 kHz default 5760-frame window with split computation. No pitch change is requested in this CPU workload.

The ratio is **original mean time / integrated mean time**: 1.10x means about 10% more processing throughput in this measured case; below 1.00x means the integration is slower. Small differences are subject to OS noise. These are not comparisons to JUCE or a universal acceleration claim.

| Host | Type | Window | Original mean us | Integrated mean us | Original / integrated |
| --- | --- | --- | ---: | ---: | ---: |
| Ryzen 9 7900X | float | 4096 | 22.10 | 18.90 | 1.17x |
| Ryzen 9 7900X | float | default 5760 | 26.37 | 22.57 | 1.17x |
| Ryzen 9 7900X | double | 4096 | 27.19 | 23.23 | 1.17x |
| Ryzen 9 7900X | double | default 5760 | 29.79 | 27.19 | 1.10x |
| Quest ARM64 | float | 4096 | 39.54 | 37.57 | 1.05x |
| Quest ARM64 | float | default 5760 | 44.77 | 43.66 | 1.03x |
| Quest ARM64 | double | 4096 | 54.21 | 56.25 | 0.96x |
| Quest ARM64 | double | default 5760 | 63.07 | 64.61 | 0.98x |
| Quest ARM32 | float | 4096 | 63.05 | 63.26 | 1.00x |
| Quest ARM32 | float | default 5760 | 82.55 | 77.83 | 1.06x |
| Quest ARM32 | double | 4096 | 67.77 | 73.66 | 0.92x |
| Quest ARM32 | double | default 5760 | 82.47 | 84.22 | 0.98x |

The unchanged spectral regression also checks an octave shift of a 440 Hz tone, formants, float/double, allocation-free retained processing/reset and finite tail draining. Direct DFT tests independently validate the altered FFT normalization for small mixed sizes. These checks do not replace listening acceptance for transients, music or extreme duration ratios. Worker tests exercise exact native decode, 48-to-44.1 kHz tone timing, arbitrary epoch seeks and final-recording drain; filesystem completion times are deliberately excluded from audio CPU timings.

## DAW expansion measurements

Each timing is a 128-frame stereo callback, float, mean microseconds. These are measured configurations, not a universal speedup ratio. The gate has a scalar detector and no backend selector.

| Target | Backend | Half-band4x roundtrip µs | IIR4x roundtrip µs | Wavetable oscillator µs | Sampler alias rejection dB |
| --- | --- | --- | --- | --- | --- |
| Windows x64 | AVX-512 | 34.873 | 27.536 | 2.801 | 93.20 |
| Quest ARM64 | NEON | 53.368 | 56.238 | 5.401 | 93.20 |
| Quest ARM32 | NEON | 93.272 | 81.218 | 7.717 | 93.20 |

Sampler rejection is 10log10(linear-output power / filtered-mip-output power) for a0.35-cycles/sample sine read at2x, excluding boundaries. Higher positive dB means stronger alias reduction. This one out-of-band tone does not establish full-band transparency. Wavetable banks and mip samples are prepared before timing; device/file I/O is excluded. IIR delay is frequency dependent, and half-band FIR has a longer fixed delay than the original FIR mode; see the integration contracts.

</details>

Current second-extension evidence: [advanced validation](AdvancedExpansionValidation.fr.md).
Earlier measurements above retain their original source hashes.

## MusicalAnalyzer Mel and DCT preparation, 2026-11-18

MusicalAnalyzer now stores the nonzero bins of each triangular Mel band and
precomputes the DCT cosines in `prepare`. The comparison uses revision `68a423a`
as the baseline. Both versions processed the same mono float signal at 48 kHz
with a 2048-point FFT, 256-frame hop, 40 Mel bands and 13 MFCCs.

| Target | Before (µs/hop) | After (µs/hop) | Time saved |
| --- | ---: | ---: | ---: |
| Windows x64, Ryzen 9 7900X, MSVC Release, strict FP | 63.882 | 36.950 | 42.2% |
| Android ARM64, Quest 3, NEON, NDK 27.2 | 158.079 | 108.274 | 31.5% |

Each run reports the median of seven series of 3000 calls. The table uses the
median of four runs per version on Windows and three per version on Quest, with
the versions interleaved. The MFCC fingerprint matched the baseline on each
target. An additional Windows comparison covered float and double across scalar,
SSE2, AVX2 and AVX-512. All 86,016 MFCC array entries from 2,688 analysis frames
matched bit for bit, including a configuration with 22 empty Mel bands.

The updated 64-grain benchmark keeps the forward and reverse voices inside
the sample bank through warmup and timing. On Windows x64 its median for a
128-frame stereo float block was 217.1 µs with AVX-512. Earlier figures used
silent reverse voices and represent a lighter workload.
