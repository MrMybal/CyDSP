# Native measurements

2026-11-11. Explicit Release validation runs; strict floating-point policy, no
fast-math/FMA contraction or global CPU-environment changes. Raw JSON retains
all cases. Native programs perform numerical audio DSP without opening an audio
device; no listening or physical audio I/O measurement is claimed.

## Performance samples

All rows below are float, two-channel convolution of a 32768-tap IR in 128-frame
blocks. Uniform hybrid uses P=128; nonuniform uses early=128 and late=1024.
Times are microseconds per call, including normal OS scheduling noise.

| Host / backend | Algorithm | Mean us | Median us | p99 us | Maximum us |
| --- | --- | ---: | ---: | ---: | ---: |
| Ryzen 9 7900X / scalar | hybrid128 | 80.32 | 72.00 | 151.50 | 264.30 |
| Ryzen 9 7900X / scalar | nonuniform128 | 34.34 | 30.20 | 90.40 | 147.30 |
| Ryzen 9 7900X / sse2 | hybrid128 | 39.87 | 42.40 | 49.70 | 52.40 |
| Ryzen 9 7900X / sse2 | nonuniform128 | 15.58 | 11.90 | 42.60 | 42.90 |
| Ryzen 9 7900X / avx2 | hybrid128 | 21.38 | 19.80 | 31.10 | 69.30 |
| Ryzen 9 7900X / avx2 | nonuniform128 | 11.62 | 8.80 | 31.40 | 35.50 |
| Ryzen 9 7900X / avx512 | hybrid128 | 29.17 | 28.60 | 32.70 | 219.00 |
| Ryzen 9 7900X / avx512 | nonuniform128 | 12.74 | 10.00 | 34.50 | 62.50 |
| Quest 3 ARM64 / scalar | hybrid128 | 99.87 | 99.38 | 109.69 | 147.29 |
| Quest 3 ARM64 / scalar | nonuniform128 | 48.12 | 41.51 | 96.77 | 109.06 |
| Quest 3 ARM64 / neon | hybrid128 | 87.46 | 85.83 | 102.60 | 201.77 |
| Quest 3 ARM64 / neon | nonuniform128 | 28.91 | 23.44 | 70.57 | 77.40 |
| Quest 3 ARM32 / scalar | hybrid128 | 260.10 | 255.21 | 283.23 | 316.72 |
| Quest 3 ARM32 / scalar | nonuniform128 | 91.59 | 76.51 | 208.33 | 212.87 |
| Quest 3 ARM32 / neon | hybrid128 | 146.49 | 151.41 | 163.18 | 299.90 |
| Quest 3 ARM32 / neon | nonuniform128 | 51.82 | 40.62 | 135.57 | 157.86 |

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
| Windows x64 | 670 | 0.00133235 | -79.13 | 5.9e-07 | 1.51e-15 |
| Quest ARM64 | 458 | 0.00133235 | -79.13 | 5.9e-07 | 1.3e-15 |
| Quest ARM32 | 458 | 0.00133235 | -79.13 | 5.9e-07 | 1.3e-15 |

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
| Ryzen 9 7900X / avx512 | 39.40 | 103.94 | 2.64x | 0.35 | 4.40 | 12.55x |
| Quest ARM64 / neon | 107.33 | 188.69 | 1.76x | 2.40 | 5.91 | 2.46x |
| Quest ARM32 / neon | 192.06 | 339.47 | 1.77x | 3.01 | 10.79 | 3.58x |

The reference MIMO executes one NonuniformConvolver per route and sums its outputs. The reference graph retains identity callbacks at each stage. Fusion applies to null routing nodes whose semantics are known; this does not fuse arbitrary processors. Costs and ratios depend on workload, ISA, scheduling and hardware. CPU/OS capability records explicitly distinguish AVX-512 execution from SVE compile/link-only coverage.

## Whole-stage FFT and progressive convolution

The FFT comparison uses a 1024-point float forward transform and the same prepared tables. The reference retains the former group dispatcher and scalar tiny stages. All timings include input permutation; this is a same-build comparison, not a cross-library ranking.

| Host / backend | Full-stage FFT us | Group reference us | Reference / full-stage |
| --- | ---: | ---: | ---: |
| Ryzen 9 7900X / avx512 | 2.04 | 3.93 | 1.93x |
| Quest ARM64 / neon | 8.20 | 8.89 | 1.08x |
| Quest ARM32 / neon | 12.91 | 14.81 | 1.15x |

The convolution rows use the same 32768-tap IR, two float channels, 128-frame callbacks, early partition 128 and late partition 1024. ScheduledConvolver covers the first 2048 taps with its hybrid prefix, then spreads tail FFT/recombination/MAC/overlap work across a full input interval. NonuniformConvolver covers 1024 early taps and retains boundary transforms. Both produce the same causal response with zero added block latency.

| Host / backend | Nonuniform mean us | Scheduled mean us | Mean cost ratio | Nonuniform p99 us | Scheduled p99 us |
| --- | ---: | ---: | ---: | ---: | ---: |
| Ryzen 9 7900X / avx512 | 12.74 | 14.34 | 1.13x | 34.50 | 18.50 |
| Quest ARM64 / neon | 28.91 | 33.14 | 1.15x | 70.57 | 43.23 |
| Quest ARM32 / neon | 51.82 | 57.40 | 1.11x | 135.57 | 72.60 |

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
| Ryzen 9 7900X / avx512 | hybrid | 29.17 | 26.92 | 57.69 / 124.10 | 59.70 / 79.10 |
| Ryzen 9 7900X / avx512 | nonuniform | 12.74 | 12.67 | 23.22 / 66.10 | 26.84 / 66.90 |
| Ryzen 9 7900X / avx512 | scheduled | 14.34 | 13.83 | 28.60 / 49.70 | 26.33 / 40.00 |
| Quest ARM64 / neon | hybrid | 87.46 | 87.29 | 193.93 / 255.26 | 177.34 / 211.09 |
| Quest ARM64 / neon | nonuniform | 28.91 | 29.28 | 60.44 / 153.39 | 60.08 / 150.21 |
| Quest ARM64 / neon | scheduled | 33.14 | 33.37 | 73.85 / 264.84 | 67.66 / 85.52 |
| Quest ARM32 / neon | hybrid | 146.49 | 142.18 | 295.44 / 316.67 | 294.66 / 351.67 |
| Quest ARM32 / neon | nonuniform | 51.82 | 59.65 | 125.22 / 370.21 | 126.28 / 374.95 |
| Quest ARM32 / neon | scheduled | 57.40 | 66.83 | 117.25 / 137.45 | 118.55 / 202.29 |

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
| Ryzen 9 7900X / avx512 | 8192 | 39.40 / 106.30 | 39.73 / 78.30 | 1.01x |
| Ryzen 9 7900X / avx512 | 32768 | 60.23 / 179.10 | 71.10 / 115.30 | 1.18x |
| Quest ARM64 / neon | 8192 | 107.33 / 207.66 | 128.93 / 155.10 | 1.20x |
| Quest ARM64 / neon | 32768 | 170.51 / 303.12 | 200.86 / 273.59 | 1.18x |
| Quest ARM32 / neon | 8192 | 192.06 / 373.49 | 223.29 / 250.16 | 1.16x |
| Quest ARM32 / neon | 32768 | 306.83 / 578.23 | 393.93 / 703.39 | 1.28x |

The scheduled late budget is 2528 chunks per 1024-frame interval (at most 3/frame) for 8192 taps, or 9056 (at most 9/frame) for 32768. Those counts exclude direct/early-tier work. Each phase has a different cost; a job count does not bound wall time. The larger prefix and scheduling can increase average CPU; sampled p99/max cannot guarantee an audio deadline and are subject to OS/device variation. No automatic mode switch is introduced.

| Host | Worst float MIMO/direct error | Worst double MIMO/direct error |
| --- | ---: | ---: |
| Ryzen 9 7900X | 8.2e-08 | 1.73e-16 |
| Quest ARM64 | 8.2e-08 | 1.67e-16 |
| Quest ARM32 | 8.2e-08 | 1.67e-16 |

The numerical measurement uses 4 inputs, 5 outputs, 6 sparse/duplicate routes with unequal dense IR lengths 1..521, 64-frame late partitions, 769 source frames followed by zeros and variable callbacks up to 137 frames. Independent double time-domain convolution covers every output and the full tail. Regressions also cover all three MIMO modes, rectangular exact aliases, smaller/larger partitions and invalid requests before state mutation.
