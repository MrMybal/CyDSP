# Integration remediation and plugin interfaces

Owner request: 2026-11-12, correct the integration report and implement/manage plugin
interfaces. Changes stay inside CyDSP. The supplied CyDAW report/test observations
are integration evidence; no CyDAW/JUCE implementation is copied or modified.

## Work and acceptance

- [x] CYDSP-001: preserve WAV LIST subtype/raw bytes, INFO/adtl and correct RIFF
  chunk lengths/padding; independently authored and parsed fixtures.
- [x] CYDSP-006: versioned public markers/regions/sampler loops/tags API, native
  unknown preservation and explicit inclusive/exclusive conversions.
- [x] CYDSP-002/003/004: detailed known/unknown device capabilities and negotiated
  stream description, explicit backend/share mode, persistent selection/resolution
  with ambiguity; fake-driver/null-device acceptance.
- [x] CYDSP-005: callback steady-clock anchor, frame/generation/xrun/cost diagnostics
  with validity/source fields; no invented hardware timestamps/physical latency.
- [x] CYDSP-007: bounded PCM editing proxy/cache for native codecs, exact random
  access, long-file seek and memory/work measurements.
- [x] CYDSP-008: explicit source/stretch policies/ranges/curves/seek/prime/tails,
  no silent narrowing or fake equivalence to application algorithms.
- [x] CYDSP-009: persistent/ambiguous MIDI selectors, multi-input fan-in, configurable
  SysEx capacity and backpressure, epochs/disconnect releases/fragmented messages.
- [x] Plugin core: multibus processing/events/parameters/gestures/state/latency/tails,
  original editor lifecycle/resize/DPI/thread contracts and native-window embedding.
- [x] Plugin formats: pinned permissive CLAP/VST3 host adapters with explicit
  capabilities, native editor lifecycle and platform bundle-loading source.
  AU/AAX/LV2/JSFX retain the explicit external provider boundary; no built-in
  adapter or separately licensed SDK grant is claimed.
- [x] Plugin worker: original bounded IPC protocol/client/server entry points,
  separate-process lifecycle and crash/disconnect recovery; no false isolation claim.
- [x] General application contracts from APP-001..004: identify reusable public
  platform/data/source/routing services separately from CyDAW migration and rendering.
- [x] BUILD-001: complete reproducible library snapshot/package and local revision;
  no automatic remote publication or GitHub Actions.
- [x] Final: installed consumer, native regressions/ASan, available cross-target
  builds/Quest, independent file/plugin fixture tests, provenance and Gitleaks.

Physical device acceptance and macOS/Linux runtime remain environment-dependent.
Do not open microphones/speakers/third-party installed plugins or change OS device
settings as part of automated regression tests. Test original fixture plugins.
No GUI framework, plugin implementation or utility code is imported from JUCE.

## Final available-target acceptance

All thirteen reports in build/completion.json pass against the same compiler-input
digest. Windows x64 MSVC/Clang, x86, scalar and AddressSanitizer execute the native
suite. Quest 3 ARM64 and ARM32 execute the in-process suites and measurements.
Four Android ABIs compile/link. Four Windows production configurations build
static libraries only; all73 public headers and the installed static consumer
compile. The ISA audit inspects666 objects and confirms local AVX-512/SVE flags
without fused floating-point instructions leaking into the portable paths.

Original CLAP/VST3 modules verify multibus audio, sample automation, events,
state, lifecycle and prepared host allocation behavior. Windows additionally
checks actual native child editors, resize/DPI/focus/key/wheel, VST3 UI gesture
recording, final-value retention after4096 automation points and a real separate
worker with controlled late replies, abrupt exit and restart. No third-party
installed plugin or physical audio/MIDI device is opened.

Long60-second native AAC/WMA fixtures check exact PCM proxy seeks near EOF,
bounded copy storage and sampled process private-memory increase. Independent
WAV fixtures check LIST/adtl/INFO, unknown/odd chunks and24-in-32 PCM. Source
policy/curve/provider and MIDI multiport/SysEx/epoch regressions pass.

Gitleaks scans all nonignored candidate files using default rules, full redaction
and no custom allowlists. Its local report and snapshot are under build/security.
This checks delivered files; it does not claim a scan of Git history.

## Reproducible local handoff

The complete library is committed locally, including the previously untracked
headers, implementations, notices, scripts and documentation. A source archive
named build/CyDSP-<commit>.zip pins that exact revision. build/handoff.json records
the revision, source digest, archive checksum and validation evidence. Build
outputs and ignored JUCE comparison material are absent from the source archive.
No remote push, release or GitHub Actions run is performed.

An application can use this checkout with add_subdirectory, or install it and
request the Sources, Devices and Plugins CMake package components. Link the
CyDSP::Sources, CyDSP::Devices and CyDSP::Plugins targets as needed; their private
driver/codec/interface dependencies do not become public SDK requirements.
Production tests and measurements remain opt-in. See [Plugin contracts](Plugins.md),
[DAW contracts](DawIntegration.md), [Validation](Validation.md) and
[Measurements](Measurements.md) before adapting an application.

AU/AAX/LV2/JSFX need external providers; no built-in adapter is claimed.
Beats/texture source policies likewise require an application-specific provider.
APP-001/003/004 are application shell/model/routing migrations in the supplied
report; CyDAW remains untouched. Desktop Linux/macOS, Windows ARM64 execution,
native SVE, physical audio/MIDI2 and compatibility with real third-party plugins
remain separate acceptance work.
