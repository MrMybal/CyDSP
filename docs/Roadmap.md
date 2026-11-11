# Roadmap

## Target priorities (owner direction, 2026-11-11)

Primary targets are x64 and ARM64. Keep existing x86/ARM32 compatibility and
correctness coverage as secondary targets. Focus upcoming performance work on:

1. x64: AVX2/AVX-512 kernels and FFT/convolution cost on realistic audio workloads.
2. ARM64: NEON on available hardware, with optional SVE validated separately when
   compatible hardware is available. ARM64 applications do not require SVE.
3. Remaining 64-bit platform acceptance (desktop Linux/macOS and Windows ARM64),
   followed by application/audio integration on the intended target.

ARM32-specific long-IR progressive MIMO tuning is lower priority. Its measured
regression remains documented and the nonuniform mode remains available. Existing
32-bit support is retained; prioritizing 64-bit does not change the numerical API,
backend availability checks or the owner's still-pending release-license choice.

## Implemented milestones

1. Common C++20 API, planar/sub-block views, aligned storage and alias validation.
2. Prepared CPU dispatch, scalar/SSE2/AVX2/NEON vector kernels, float/double.
3. Gain and ramps, biquad state, expanded EQ design and filter cascades.
4. SIMD complex arithmetic and radix-2 FFT butterflies with one normalization.
5. Packed real FFT using a half-size complex plan and reusable workspace.
6. MIT-derived partitioned convolution, plus a hybrid direct-head/FFT-tail variant
   with no added block buffering latency.
7. MIT-derived windows/sinc equations and rational streaming resampling with
   SIMD double/mixed-precision accumulation.
8. Fractional delay with optional frame ramps shared with Gain, and SIMD direct FIR.
9. Pinned provenance, static package installation, compile-only numerical regression
   sources and a reproducible local validation/report script.
10. ARM64/NEON static cross-compilation with installed Clang and SDK headers,
    including all public headers, numerical contracts and regression objects.
11. Checked planar/interleaved conversion and tiled per-voice SIMD biquad banks
    with independent coefficients/states, including 128-bit AVX2 residual groups.
12. Two-bank biquad output transitions, shared frame progression, one coalesced
    queued target, immediate override, reset and callback-boundary behavior.
13. Allocation-free linear alias validation for ordered disjoint planar buffers,
    retaining full pairwise validation for arbitrary layouts.
14. Rectangular channel matrices with SIMD constant mixing, bounded input snapshots
    and exact cross-channel in-place routing/permutations.
15. Allocation-free matrix coefficient ramps, shared frame progression, immediate
    retargeting and reset semantics.
16. Hybrid IR output crossfades with transactional non-real-time staging, separate
    allocation-free activation and retained retired plans.

Implementation does not imply runtime acceptance or measured performance. See
Validation.md for the exact executed checks.

## Extended implementation and validation

17. Explicit native regression/measurement runners, allocation AND free auditing,
    AddressSanitizer and a second native Windows compiler (Clang).
18. Full multi-section EQ output transitions with preallocated dual banks and one
    coalesced following target.
19. Two-tier nonuniform convolution and sparse independent input/output IR routes.
20. Single-producer worker IR preparation with lock-free publication, bounded
    slots and no audio-thread plan destruction.
21. End-of-stream draining, history-preserving rate edits and measured resampling
    presets. The raw default increased from 64 to 128 taps after wider measurements.
22. Prepared DAG execution, topological validation and buffer reuse by last use.
23. Android NDK target compilation/linking and native ARM device validation through
    the owner's ADB-connected Quest 3. Exact ABI coverage is in Validation.md.

## Remaining acceptance on unavailable environments

Desktop Linux/GCC and macOS/Apple Clang need their target hosts. run_portable.py
is prepared for those environments. This Windows host has no enabled WSL or macOS
SDK. Android/Linux-kernel tests are reported as Android, not desktop Linux validation.
Windows ARM64 application execution likewise requires a compatible host/runtime.

## Optimization milestones

24. x64 AVX-512 with conservative CPU/OS state gates; ARM64 Linux/Android SVE with
    vector-length-agnostic predication. Explicit CMake/backend selection remains available.
25. Independent dot accumulators, balanced reductions, inline tiny FFT stages and
    trusted private kernel dispatch following public block validation.
26. Division-free scheduling of older-partition MAC work over the input interval,
    preserving existing latency. Forward/inverse FFT work remains at its deadline.
27. Shared per-input forward FFT/history and per-output inverse FFT in MIMO tiers;
    shared direct-head input rings, including sparse/duplicate/unequal-length routes.
28. Default live-input destination prewarm before serialized or asynchronous IR fades;
    bounded work and retained plans, with explicit cold-mode opt-out.
29. Prepared antialias envelopes and allocation-free real-time ratio ramps; exact
    callback-split tests and an independent long-double clock reference.
30. SIMD fusion of linear routing/gain chains, retaining stage rounding and mute behavior.
31. Benchmarks against independent MIMO routes and callback graph chains, spectral
    grids, variable-ratio DC continuity, sanitizer and expanded native/ARM checks.

32. Whole-stage FFT dispatch and SIMD across independent small-stage groups, with
    linear bit-reversal preparation and a same-build former-dispatcher benchmark.
33. ScheduledConvolver: prefix coverage supplies a full interval for progressive
    packed FFT, real spectral recombination, MAC, inverse FFT and overlap-add jobs.
34. Direct dense/sparse convolution, phase-boundary moves/reset, allocation auditing,
    explicit work budgets and numerical error measurements for the scheduled mode.

35. Common ConvolutionConfig/Convolver interface over hybrid, nonuniform and
    progressive engines; transactional preparation preserves configuration/history.
36. Engine selection inherited by retained serialized/asynchronous IR banks, with
    warm/cold/instant live-history oracles, partial-transform moves, concurrent
    long-IR publication and allocation/free audits for all modes.
37. Same-build direct/interface and two-engine fade measurements on available
    hardware, plus numeric IR-update error records and installed-package coverage.

38. Progressive shared MIMO: one forward FFT per active input and inverse per
    active output, with route-specific partitions, explicit job budgets and zero
    added block latency. Common config also selects hybrid/nonuniform MIMO.
39. Rectangular cross-alias and dense/sparse direct-convolution references, partial
    transform moves/reset, configuration transactions and realtime allocation audit.
40. Same-build 4x4 MIMO measurements at 8192/32768 taps and per-backend numerical
    error records; incremental butterfly scheduling shared by both processors.

## Further enhancements

- SVE native execution at several vector lengths requires an SVE-capable host.
- More FFT layouts/tiering and target-specific tuning remain workload dependent.
  ScheduledConvolver and scheduled MIMO split late transforms; existing uniform/
  nonuniform paths retain whole-transform boundaries and remain separately selectable.
- Higher-order/band-limited delay modulation and an explicit FMA/RISC-V policy.
- Arbitrary callback fusion needs explicit processor semantics; callbacks remain borrowed.
- The separate Unreal adapter and original-code release license remain owner decisions.

GitHub workflows are not part of the build configuration.
Native launches use explicit owner-authorized validation commands; production
presets remain static libraries with native targets disabled.
