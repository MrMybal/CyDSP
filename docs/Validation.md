# Validation

2026-11-12. Work is confined to CyDSP. The owner explicitly authorized native tests,
benchmarks and the connected Quest 3. Production configurations remain static,
with test/measurement/sanitizer options OFF. No CyCode or CyDAW_Project source, GitHub Actions or
antivirus setting was changed. No application was installed on the Quest.

Current compiler-input SHA-256: ba56976d76e6165443325a093c7411c226cb7853257629d4efb2471dfe2ceccd

## Executed and passed

| Environment | Evidence |
| --- | --- |
| Windows x64 MSVC 19.40 | Native scalar/SSE2/AVX2/AVX-512 regressions, allocation/free audit and measurements |
| Windows x86 MSVC 19.40 | Native regressions and allocation/free audit |
| Windows x64 without explicit SIMD | Native regressions and allocation/free audit |
| Windows x64 Clang 19.1.5 | Native regressions and allocation/free audit |
| Windows x64 AddressSanitizer | Instrumented library and full native regression suite |
| Quest 3 Android ARM64 | Native scalar/NEON regressions, allocation/free audit and measurements |
| Quest 3 Android ARM32 compatibility | Native scalar/NEON regressions, allocation/free audit and measurements; double NEON kernels fall back to scalar |
| Android ARM32/ARM64/x86/x86_64 NDK Clang 18 | Archives, isolated public headers, regression objects and target executable linking |
| Windows x64/x86/scalar/ARM64 Clang | Four Release production configurations, static libraries only, with compile-only checks |
| Installed CMake package | Byte-exact headers/notices/manifest and separate static consumer |

All 73 public headers compile individually. 95
static_assert sites pass under MSVC and Clang. Source checks cover 242 owned C++
files, local includes, Python syntax, documentation links, build/ISA policy and all
18 pinned commercially compatible component archives/notices. Final compiler build logs have no
warnings. An additional audit disassembles 1366 production objects across all
eight target archives, verifies per-file wide-ISA isolation and finds no fused FP
instructions. The instruction audit is generated locally in `build/simd-validation.json`. SVE is compiled and
linked for Android ARM64; no SVE-native execution is claimed.
The installed NDK emits upstream CMake deprecation notices during
configuration; its compiler build logs are clean.

Prepared property-service regressions additionally validate ResourceList/DeviceInfo/ChannelList/JSONSchema, resId isolation, RFC6901 partial SET, list pagination/totals, full/partial/notify delivery, TTL/page/binary caching, stale-GET protection, eviction/ACK pressure, timeout, cancellation/MUID cleanup and retained-memory request/patch/subscription processing. [Property service](CiProperty.md) records exact scope.

Native MIDI2 regressions check CI discovery/collisions/profiles/chunked properties, protocol confirmation/timeout, retained-memory processing and parallel audio/SysEx queues. Quest also exercises native-endian descriptor framing, saturation, ownership and disconnect through nonblocking sockets. No physical MIDI port is opened; Windows MIDI Services is unavailable on this Windows 10 host, Android JNI/AMidi hardware is not exercised, and Apple transport source remains uncompiled. See [MIDI 2](Midi2.md).

The integrated suites additionally check half-bin FFTs against direct DFTs for power-of-two and mixed-radix sizes on every available backend, MIDI 2 scaling including all 16384 pitch values, MPE zones/RPN/pedals, sample-accurate custom/oscillator/sampler voices, queued transport/loops/underruns, random epoch seeks with full-capacity prefill and centered sinc timing, and background recording/draining with exact file readback. Audio-thread memory auditing covers those new paths.

The expanded suites additionally check DSP transfer functions, envelopes, oversampling, modulation/reverb decay, MIDI/SMF/SPSC, independent WAV/AIFF fixtures, WAV/AIFF/FLAC/Ogg roundtrips, spectral pitch shifting, retained-memory processing/reset and silent virtual audio callbacks. The DAW expansion adds filter families/interpolators/windows, gate look-ahead, half-band/IIR oversampling, wavetable/mip sampling/presets, worker stretching/IR/peak caches, clock collection/reconnects/Function Blocks/controllers/profile dispatch, AIFC/CAF with retained marker/loop chunks and an independent successful MP3 fixture. Windows native MP3/AAC/M4A/WMA encode/decode/tone/decoded-seek tests pass. Exact scope and platform limits are recorded in [DAW integration](DawIntegration.md) and [Coverage](Coverage.md).

The native suites exercise direct DFT/convolution references, SIMD residuals and
unaligned views, invalid/overlapping requests, reset/moves, split callbacks, queued
filter/EQ targets, fractional delays, rectangular mixing and cross-channel aliases,
nonuniform partition boundaries, sparse MIMO route sums, EOS versus an explicit
zero stream, rate-change history, graph cycle rejection/topological sorting/slot
reuse/custom processor callbacks, and 200 long-IR producer publications per
convolution mode concurrent with audio consumption. All available backends also run shared-FFT MIMO versus direct
convolution, live-history prewarming versus explicit convolution, ratio ramps versus
an independent absolute-position clock, callback splits/moves, and fused gain chains
with exact stage rounding and NaN mute checks. Scheduled convolution is compared
with dense direct convolution at late sizes 1..256 and a sparse three-channel 1024
case, including zero tail, partial FFT moves/reset, boundary impulses and invalid
requests. Stage kernels are compared against scalar butterfly references on
unaligned guarded spans. Deterministic async tests cover replacement, empty callbacks,
queue pressure and immediate queued activation. The common Convolver interface
checks successful/failed engine changes, configuration/history transactions and
moves during partial FFT work. All three modes run serialized and asynchronous
updates against live-history direct references with/without prewarm and with
zero/positive fade durations. Progressive shared MIMO covers 4->3 and 2->5 channel
layouts at late sizes 1..256, sparse 1024-frame tiers, disconnected inputs/outputs,
unequal/duplicate routes, complete tails and rectangular cross-channel aliases.
Late work budgets explicitly check sharing per active input/output. Partial FFT
moves/reset and successful/failed engine changes retain their documented history.

The allocation audit replaces C++ allocation/deallocation entry points on the test
thread. It covers prepared process/reset/finish and fade activation for every
processor, including retired IR plans and progressive FFT jobs/reset. It detected zero allocations AND frees in
the exercised regions. Worker preparation remains intentionally non-real-time.
The audit does not make arbitrary user graph callbacks allocation-free.

## Integration report follow-up

Independent WAV LIST/adtl/INFO, odd chunks and native24-in-32 PCM references pass.
Public metadata versions/typed edits preserve unknown chunks. Fake/null device
tests distinguish negotiated values, unknown hardware facts and callback clocks.
MIDI tests cover ambiguous selection, two ports, fragmented12 KiB SysEx, epochs
and voice-release notifications.

Source tests cover time/pitch curves, repitch reference tones, extended ratios,
prime/tail contracts and an original external-provider fixture. Beats/texture
remain provider policies, not claimed native matches for CyDAW algorithms.
Long60-second AAC/WMA proxy fixtures compare exact near-EOF PCM, seek time,
bounded copy storage and sampled process private-memory increase.

Original CLAP/VST3/LV2 shared modules and JSFX scripts verify the ABI, multibus/sidechain, automation,
copied events, component/controller states, lifecycle and host allocation audit.
Windows tests actual child editors (including the JSFX canvas), resize/DPI, focus/keys/wheel and a separate
process worker: IPC, one-block pipeline, late-result silence, abrupt plugin exit
and restart. Android runs in-process module tests; standalone Android process
workers require an application service and are explicitly skipped.

See [Plugin contracts](Plugins.md) and [Remediation scope](IntegrationRemediation.md).
Application shell/model/routing migration and other plugin-format providers remain
outside this library-only change; a complete JUCE replacement is not claimed.

## Reports and reproduction

The consolidated report (`build/completion.json`) and raw Windows/Quest data are
generated locally under `build/`. See the [measurement analysis](Measurements.md)
for recorded results and limitations.
Static-only validation.json describes that runner alone; nativeRuntimeExecuted=false
there does not describe the separate native runs.

```text
python -B scripts/validate.py
python -B scripts/run_native.py --measure
python -B scripts/run_native.py --arch x86
python -B scripts/run_native.py --arch scalar
python -B scripts/run_native.py --compiler clang
python -B scripts/run_native.py --sanitize
python -B scripts/build_android.py --abi arm64-v8a --measure
python -B scripts/build_android.py --abi armeabi-v7a --measure
python -B scripts/build_android.py --abi x86
python -B scripts/build_android.py --abi x86_64
python -B scripts/run_android_device.py --abi arm64-v8a --measure
python -B scripts/run_android_device.py --abi armeabi-v7a --measure
python -B scripts/verify_simd.py
python -B scripts/summarize_validation.py
```

Device execution requires the intended owner-authorized ADB connection. The runner
uses a unique /data/local/tmp directory and removes only its exact files afterward.
No emulator, root access, application install or device setting change is required.
Every required report must pass and match the current compiler-input digest before
consolidation succeeds.

## Remaining target coverage

Desktop Linux/GCC and macOS/Apple Clang tests are deferred by the owner.
No corresponding host is available in this session; Windows WSL is not enabled. run_portable.py is ready to run natively on those hosts.
Android uses a Linux kernel and libc++, and is reported separately from desktop
Linux/glibc/GCC. SVE is compiled and linked in Android ARM64; the Quest reports SVE unavailable.
Native SVE execution at different vector lengths needs compatible hardware.
Windows ARM64 archives compile, but application linking/runtime
still needs its target SDK libraries/host; Android ARM64 execution does not validate
that Windows ABI. No CI workflow was enabled to bypass these missing environments.

The synthetic workloads and sampled spectral checks do not establish all possible
stream states, hard audio deadlines, listening quality, physical audio I/O latency
or performance on other machines. CyDSP uses the MIT license; upstream notices
are indexed in Licensing.md. Further implementation opportunities are in Roadmap.md.
