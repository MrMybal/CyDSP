# Split validation — 2026-11-12

The delivered source snapshot was validated as two independent projects.

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

Gitleaks default-rule snapshot scanning found zero secrets in both candidates;
final source snapshots are scanned again after applying the split. History is not
scanned by this check. No CI, push, release, application migration, physical audio/MIDI
or CyUI integration was performed. Native editor/worker tests use original fixtures
and hidden Windows parent windows, not installed third-party plugins.

macOS and desktop Linux execution remain deferred by the owner. Android/Quest
runtime, x86, ARM32 and SVE execution have not been repeated for this split. Older
monolithic measurements and platform results do not establish this snapshot's
coverage. Windows ARM64 compilation does not prove execution on an ARM64 device.

The source/header namespaces and CMake package boundary are breaking changes.
See [migration](Migration.fr.md). Candidate build logs and the exact application
plan are retained in the local build/separation-* directories, outside source packages.
README structure and relative links are verified locally; actual GitHub rendering
is unverified because no publication was requested.
