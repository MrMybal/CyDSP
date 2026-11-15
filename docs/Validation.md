# CyDSP 0.3.2 optimization validation — 2026-11-14

Compiler-input SHA-256: `114977a9b190a6dd11a017bddf6c144cbc5d69230976cb0ef419782ec3a73bd7`.

MSVC x64 Release, clang-cl x64 Release, x64 AddressSanitizer and Quest 3
Android ARM64/NEON completed the regression suite. Windows ARM64 compiled its
static library, individual public headers and regression objects. Android ARM64
compiled/linked NEON and SVE; actual SVE execution/vector lengths remain unverified.

New independent checks cover direct SIMD wavefolder tails, exact same-channel
aliases and typed intermediate mix rounding; the precise transient's unchanged
scalar power argument; dynamic-EQ filter/envelope oracles with 1/2/3/7 channels,
mono/full sidechains, in-place processing, split blocks, resets and automation;
and saturation DC-cache transitions including signed zero. Existing convolution,
allocation/free, alias, latency and scheduled-budget regressions still pass.

Object inspection covered 84 x64, 84 Android ARM64 and 83 Windows ARM64 production
objects, with no fused FP instructions or wide-ISA leakage. The installed exact
0.3.2 package compiled, linked and ran a consumer with the updated private
MatrixConvolver layout. Rebuild headers, archive and consumers together.

Detailed timing repetitions, discarded experiments, raw audio comparisons and
integration-oriented notes remain local under the Git-ignored build directory.
These checks do not imply a listening test or a new CyMetaSound integration run.
macOS/Linux desktop execution remains deferred; Windows ARM64 and SVE runtime,
and x86/ARM32 reruns, are still outstanding for this snapshot.

---

# Ten-lot DSP / spatial expansion — 2026-11-13

The latest extension results, exact source digest, CPU costs and numerical errors
are in [ExpansionValidation.fr.md](ExpansionValidation.fr.md). The
[API guide](ExpansionGuide.fr.md) defines the supported configurations and limits.
The older measurements and validation snapshots below are retained as history;
their hashes and coverage must not be attributed to the current extension.

---

# Granulation and analysis validation — 2026-11-12

The feature snapshot has compiler-input SHA-256
`9047361b0fbc9ba1372504c7cfecc9a8e9a06cdd4ea168dc1d7b1643f5262148`.
These results supersede the platform coverage of the earlier optimization snapshot
below. Public contracts and limits are in [GranulationAndAnalysis.md](GranulationAndAnalysis.md).

| Check on this feature snapshot | Result |
| --- | --- |
| MSVC x64 Release regressions and 774 general measurement records | Passed; scalar/SSE2/AVX2/AVX-512 executed |
| clang-cl x64 Release regressions | Passed |
| x64 with explicit SIMD disabled | Passed |
| x64 AddressSanitizer | Passed |
| Process/reset/trigger allocation and free audit | Passed |
| Quest 3 Android ARM64 regressions and 526 general measurement records | Passed; scalar/NEON executed; temporary executables removed |
| Windows ARM64 static libraries, individual public headers, regression objects and integration example | Compiled; runtime unverified |
| Android ARM64 library, individual public headers, regression executable and integration example | Compiled/linked; SVE runtime unverified |
| ISA isolation / no fused floating-point contraction | Passed: 49 x64, 47 Windows ARM64, 48 Android ARM64 production objects |
| Source, build-policy and pinned-notice verification | Passed: 144 C++ sources/headers, same five pinned components |

Independent numerical tests include direct DFT/PSD Parseval checks, variable-window
complex projection references, naive sliding peak/RMS/DC/correlation windows,
analytical grain offsets/reverse/pan/envelopes, block splitting, seeded clouds,
overflow transaction checks, 4x/8x intersample sine peaks, and direct-form-I
BS.1770 weighting/gating and LRA quantiles. 44.1/48/96 kHz tone checks exercise
loudness rate handling. NaN/infinity rejection and failed preparation preserve
prior states. The compiled integration sketch uses only public CyDSP APIs.

The grain octave fixture (0.35 cycles/input sample, ratio 2) measures approximately
93.20 dB less residual alias energy with the existing band-limited reader than
linear reading, for both float/double. This is one synthetic fixture, not a general
stop-band guarantee. LUFS integrated error against the independent 48 kHz oracle
is at most 5.33e-15 LUFS across x64/Quest runs. A 997 Hz, -20 dBFS peak mono tone
reads approximately -23.01 LUFS. These checks do not establish official EBU/ITU
conformance; the complete official vector suite and certification have not run.

macOS and desktop Linux execution remain deferred. Windows ARM64 runtime,
SVE-capable hardware/vector lengths, x86 and ARM32 reruns remain unverified.
No physical audio-device test or listening test is implied. Production presets
still build static libraries with native tests and measurements disabled.

Local raw reports and logs are retained under
`build/granulation-analysis-2026-11-12/`, outside source distributions.
Reproduce with `scripts/run_native.py --measure`, its `--compiler clang`,
`--arch scalar` and `--sanitize` variants, `scripts/build_arm64.py --compile-checks`,
`scripts/build_android.py --abi arm64-v8a --measure`, and an explicitly selected
owner-authorized device via `scripts/run_android_device.py --abi arm64-v8a --measure`.

---

# Optimization validation — 2026-11-12

The preceding optimization reports identify compiler-input source SHA-256
`0f8e550f794a20e11817f56d4e72983898c09ebfc1ff50291d719a8e72b3cb2f`. See [measurements](Measurements.md) for workloads, quality bounds,
before/after data and reproduction instructions.

| Earlier optimization check | Result |
| --- | --- |
| MSVC x64 Release, scalar/SSE2/AVX2/AVX-512 regressions and measurements | Passed |
| Six interleaved before/after pairs, 680 focused cases each | Passed |
| clang-cl x64 Release regressions | Passed |
| x64 without explicit SIMD | Passed |
| x64 AddressSanitizer including offset/tail buffers | Passed |
| Process/reset allocation and free auditing | Passed |
| Quest 3 ARM64 scalar/NEON regressions and quality, 502 general records | Passed; temporary executables removed |
| Windows ARM64 libraries, public headers and regression objects | Compiled; runtime unverified |
| Android ARM64 SVE library and test executable | Compiled and linked; SVE runtime unverified |
| ISA isolation and no fused FP contraction | Passed: 43 x64, 41 Windows ARM64, 42 Android ARM64 production objects |

The actual runtime backends are scalar/SSE2/AVX2/AVX-512 on the Ryzen and
scalar/NEON on the Quest. macOS and desktop Linux runtime remain deferred.
Windows ARM64 runtime, SVE vector lengths, x86 and ARM32 reruns are outstanding.
No physical audio-device measurement or listening test is implied. Numerical
gate changes are bounded by the independent oracle; bitwise identity across all
architectures is not promised. Both static production targets retain tests and
measurements off by default.

---

# Earlier split validation — 2026-11-12

The preceding split was validated as two independent projects. CyAudioCore checks
below were not repeated as part of the subsequent CyDSP optimization work.

| Check | Result |
| --- | --- |
| Windows x64 Release, independent DSP regressions | Passed |
| Windows x64 Release, services/files/sources/MIDI/plugin regressions | Passed |
| VST3 optional controller state, short read/EOF/error/capacity, direct + worker | Passed, original fixtures |
| Windows x64 AddressSanitizer, both regression executables | Passed |
| Independent compilation of all public headers | Passed, x64 and ARM64 |
| Windows ARM64 clang-cl static archives with NEON | Passed; no ARM64 Windows runtime |
| SIMD isolation / no fused FP contraction | 181 x64 + 179 ARM64 objects inspected |
| Installed CMake packages and independent consumer compile/link/run | Passed |
| Installed headers, exact notices and relocatable exports | Passed |
| CyAudioCore -> public CyDSP boundary, no reverse dependency | Passed |
| Source manifests | 5 DSP + 13 service components; hashes/notices verified |

No CI, push, release, application migration, physical audio/MIDI
or CyUI integration was performed. Native editor/worker tests use original fixtures
and hidden Windows parent windows, not installed third-party plugins.

macOS and desktop Linux execution remain deferred by the owner. At that split
checkpoint Android/Quest runtime, x86, ARM32 and SVE execution had not been repeated. The optimization snapshot above now covers Quest ARM64.
x86, ARM32 and SVE runtime remain unverified for the latest snapshot. Older
monolithic measurements and platform results do not establish this snapshot's
coverage. Windows ARM64 compilation does not prove execution on an ARM64 device.

Candidate build logs and the application plan for that checkpoint are retained
in the local build/separation-* directories, outside source packages.
README structure and relative links are verified locally; actual GitHub rendering
is unverified because no publication was requested.

Current second-extension evidence: [advanced validation](AdvancedExpansionValidation.fr.md).
Earlier measurements above retain their original source hashes.
