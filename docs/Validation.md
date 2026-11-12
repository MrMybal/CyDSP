# Optimization validation — 2026-11-12

All current reports identify compiler-input source SHA-256
`0f8e550f794a20e11817f56d4e72983898c09ebfc1ff50291d719a8e72b3cb2f`. See [measurements](Measurements.md) for workloads, quality bounds,
before/after data and reproduction instructions.

| Current CyDSP check | Result |
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

The source/header namespaces and CMake package boundary are breaking changes.
See [migration](Migration.fr.md). Candidate build logs and the exact application
plan are retained in the local build/separation-* directories, outside source packages.
README structure and relative links are verified locally; actual GitHub rendering
is unverified because no publication was requested.
