# Native measurements

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
| Ryzen 9 7900X / scalar | hybrid128 | 68.31 | 67.20 | 117.40 | 124.40 |
| Ryzen 9 7900X / scalar | nonuniform128 | 27.56 | 24.00 | 54.70 | 59.70 |
| Ryzen 9 7900X / sse2 | hybrid128 | 29.67 | 29.50 | 34.10 | 38.30 |
| Ryzen 9 7900X / sse2 | nonuniform128 | 11.91 | 9.20 | 31.30 | 32.00 |
| Ryzen 9 7900X / avx2 | hybrid128 | 19.27 | 19.20 | 22.90 | 24.50 |
| Ryzen 9 7900X / avx2 | nonuniform128 | 9.37 | 7.40 | 23.30 | 24.00 |
| Ryzen 9 7900X / avx512 | hybrid128 | 18.34 | 18.30 | 19.00 | 19.40 |
| Ryzen 9 7900X / avx512 | nonuniform128 | 9.31 | 7.50 | 21.90 | 25.10 |
| Quest 3 ARM64 / scalar | hybrid128 | 115.25 | 114.84 | 126.82 | 130.31 |
| Quest 3 ARM64 / scalar | nonuniform128 | 55.05 | 47.45 | 110.94 | 122.86 |
| Quest 3 ARM64 / neon | hybrid128 | 90.44 | 86.14 | 109.84 | 182.13 |
| Quest 3 ARM64 / neon | nonuniform128 | 28.81 | 23.33 | 69.58 | 82.97 |
| Quest 3 ARM32 / scalar | hybrid128 | 295.34 | 308.70 | 367.14 | 404.64 |
| Quest 3 ARM32 / scalar | nonuniform128 | 107.87 | 89.64 | 242.66 | 252.19 |
| Quest 3 ARM32 / neon | hybrid128 | 152.77 | 151.77 | 173.96 | 234.64 |
| Quest 3 ARM32 / neon | nonuniform128 | 52.95 | 42.24 | 142.34 | 143.49 |

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
| Ryzen 9 7900X / avx512 | 28.93 | 73.38 | 2.54x | 0.33 | 3.98 | 12.07x |
| Quest ARM64 / neon | 113.40 | 196.86 | 1.74x | 2.41 | 6.26 | 2.60x |
| Quest ARM32 / neon | 226.02 | 388.41 | 1.72x | 3.01 | 10.63 | 3.53x |

The reference MIMO executes one NonuniformConvolver per route and sums its outputs. The reference graph retains identity callbacks at each stage. Fusion applies to null routing nodes whose semantics are known; this does not fuse arbitrary processors. Costs and ratios depend on workload, ISA, scheduling and hardware. CPU/OS capability records explicitly distinguish AVX-512 execution from SVE compile/link-only coverage.

## Whole-stage FFT and progressive convolution

The FFT comparison uses a 1024-point float forward transform and the same prepared tables. The reference retains the former group dispatcher and scalar tiny stages. All timings include input permutation; this is a same-build comparison, not a cross-library ranking.

| Host / backend | Full-stage FFT us | Group reference us | Reference / full-stage |
| --- | ---: | ---: | ---: |
| Ryzen 9 7900X / avx512 | 1.40 | 2.85 | 2.04x |
| Quest ARM64 / neon | 9.32 | 10.23 | 1.10x |
| Quest ARM32 / neon | 13.10 | 14.80 | 1.13x |

The convolution rows use the same 32768-tap IR, two float channels, 128-frame callbacks, early partition 128 and late partition 1024. ScheduledConvolver covers the first 2048 taps with its hybrid prefix, then spreads tail FFT/recombination/MAC/overlap work across a full input interval. NonuniformConvolver covers 1024 early taps and retains boundary transforms. Both produce the same causal response with zero added block latency.

| Host / backend | Nonuniform mean us | Scheduled mean us | Mean cost ratio | Nonuniform p99 us | Scheduled p99 us |
| --- | ---: | ---: | ---: | ---: | ---: |
| Ryzen 9 7900X / avx512 | 9.31 | 10.17 | 1.09x | 21.90 | 11.70 |
| Quest ARM64 / neon | 28.81 | 37.27 | 1.29x | 69.58 | 176.82 |
| Quest ARM32 / neon | 52.95 | 58.31 | 1.10x | 142.34 | 72.29 |

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
| Ryzen 9 7900X / avx512 | hybrid | 18.34 | 19.03 | 44.88 / 46.20 | 45.63 / 51.40 |
| Ryzen 9 7900X / avx512 | nonuniform | 9.31 | 9.37 | 20.63 / 45.90 | 20.05 / 45.10 |
| Ryzen 9 7900X / avx512 | scheduled | 10.17 | 10.16 | 22.05 / 24.70 | 21.56 / 24.10 |
| Quest ARM64 / neon | hybrid | 90.44 | 92.36 | 181.69 / 413.75 | 186.31 / 523.85 |
| Quest ARM64 / neon | nonuniform | 28.81 | 29.18 | 70.06 / 359.43 | 60.71 / 157.97 |
| Quest ARM64 / neon | scheduled | 37.27 | 39.76 | 67.10 / 79.01 | 67.12 / 79.01 |
| Quest ARM32 / neon | hybrid | 152.77 | 152.67 | 327.90 / 744.32 | 325.62 / 1131.20 |
| Quest ARM32 / neon | nonuniform | 52.95 | 60.61 | 116.09 / 341.62 | 105.85 / 255.94 |
| Quest ARM32 / neon | scheduled | 58.31 | 58.31 | 136.68 / 170.88 | 135.93 / 165.05 |

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
| Ryzen 9 7900X / avx512 | 8192 | 28.93 / 56.50 | 32.82 / 36.00 | 1.13x |
| Ryzen 9 7900X / avx512 | 32768 | 38.18 / 65.80 | 46.70 / 53.80 | 1.22x |
| Quest ARM64 / neon | 8192 | 113.40 / 231.25 | 144.64 / 421.14 | 1.28x |
| Quest ARM64 / neon | 32768 | 179.52 / 722.24 | 219.74 / 701.77 | 1.22x |
| Quest ARM32 / neon | 8192 | 226.02 / 560.94 | 236.56 / 348.75 | 1.05x |
| Quest ARM32 / neon | 32768 | 319.02 / 1026.93 | 372.90 / 971.04 | 1.17x |

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
| Ryzen 9 7900X | 7.27 / 7.70 | 1.55 / 1.60 | 3.94 / 4.20 | 1.93 / 2.10 | 8.45 / 9.00 | 3.50 / 3.90 | 22.92 / 24.50 |
| Quest ARM64 | 19.49 / 22.97 | 3.34 / 3.39 | 12.39 / 12.86 | 5.11 / 5.31 | 13.61 / 25.68 | 9.07 / 9.43 | 57.32 / 63.33 |
| Quest ARM32 | 30.95 / 55.57 | 3.01 / 3.02 | 16.12 / 16.93 | 8.61 / 9.01 | 16.18 / 17.14 | 11.19 / 11.30 | 113.67 / 123.80 |

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
| Ryzen 9 7900X / avx512 | 12.07 | 52.03 |
| Quest ARM64 / neon | 37.96 | 156.54 |
| Quest ARM32 / neon | 99.81 | 384.41 |

Stretch compares the pinned unmodified portable Signalsmith engine against the adapted CyDSP engine in the same strict-FP Release binary. The integrated implementation uses CyDSP FFT/vector kernels, half-bin reconstruction, fused packing/permutation and compact 3×power-of-two plans. Original headers are compiled only in this opt-in benchmark. The upstream label scalar identifies its portable implementation, not a ban on compiler auto-vectorization. Both engines use two channels, 128-frame input/output calls and identical configurations: a 4096-frame window/1024 hop without split computation, and the 48 kHz default 5760-frame window with split computation. No pitch change is requested in this CPU workload.

The ratio is **original mean time / integrated mean time**: 1.10x means about 10% more processing throughput in this measured case; below 1.00x means the integration is slower. Small differences are subject to OS noise. These are not comparisons to JUCE or a universal acceleration claim.

| Host | Type | Window | Original mean us | Integrated mean us | Original / integrated |
| --- | --- | --- | ---: | ---: | ---: |
| Ryzen 9 7900X | float | 4096 | 22.34 | 19.04 | 1.17x |
| Ryzen 9 7900X | float | default 5760 | 26.51 | 22.58 | 1.17x |
| Ryzen 9 7900X | double | 4096 | 27.17 | 23.43 | 1.16x |
| Ryzen 9 7900X | double | default 5760 | 29.89 | 27.49 | 1.09x |
| Quest ARM64 | float | 4096 | 46.14 | 43.48 | 1.06x |
| Quest ARM64 | float | default 5760 | 51.76 | 50.58 | 1.02x |
| Quest ARM64 | double | 4096 | 54.44 | 56.35 | 0.97x |
| Quest ARM64 | double | default 5760 | 62.91 | 64.57 | 0.97x |
| Quest ARM32 | float | 4096 | 62.69 | 64.29 | 0.98x |
| Quest ARM32 | float | default 5760 | 82.74 | 77.99 | 1.06x |
| Quest ARM32 | double | 4096 | 67.42 | 82.18 | 0.82x |
| Quest ARM32 | double | default 5760 | 78.60 | 84.07 | 0.93x |

The unchanged spectral regression also checks an octave shift of a 440 Hz tone, formants, float/double, allocation-free retained processing/reset and finite tail draining. Direct DFT tests independently validate the altered FFT normalization for small mixed sizes. These checks do not replace listening acceptance for transients, music or extreme duration ratios. Worker tests exercise exact native decode, 48-to-44.1 kHz tone timing, arbitrary epoch seeks and final-recording drain; filesystem completion times are deliberately excluded from audio CPU timings.

## DAW expansion measurements

Each timing is a 128-frame stereo callback, float, mean microseconds. These are measured configurations, not a universal speedup ratio. The gate has a scalar detector and no backend selector.

| Target | Backend | Half-band4x roundtrip µs | IIR4x roundtrip µs | Wavetable oscillator µs | Sampler alias rejection dB |
| --- | --- | --- | --- | --- | --- |
| Windows x64 | AVX-512 | 34.168 | 27.552 | 2.800 | 93.20 |
| Quest ARM64 | NEON | 51.767 | 56.141 | 5.383 | 93.20 |
| Quest ARM32 | NEON | 115.583 | 93.658 | 8.893 | 93.20 |

Sampler rejection is 10log10(linear-output power / filtered-mip-output power) for a0.35-cycles/sample sine read at2x, excluding boundaries. Higher positive dB means stronger alias reduction. This one out-of-band tone does not establish full-band transparency. Wavetable banks and mip samples are prepared before timing; device/file I/O is excluded. IIR delay is frequency dependent, and half-band FIR has a longer fixed delay than the original FIR mode; see the integration contracts.
