# DAW integration expansion

Owner request: 2026-11-12. Work stays inside CyDSP; CyDAW_Project/CyCode are not
integration inputs. Original additions are MIT. Public contracts retain C++20,
prepared bounded memory, common planar blocks/statuses and private platform APIs.
macOS/Linux desktop testing remains deferred. The subsequent [integration remediation](IntegrationRemediation.md) adds original plugin host/editor/worker interfaces without importing a GUI framework.

## Implementation and evidence checklist

- [x] DSP: Chebyshev I/II, elliptic and Bessel coefficient designs; polynomial
  interpolators; gate hold/hysteresis/look-ahead; half-band FIR/IIR oversampling;
  additional windows and numerical/allocation tests.
- [x] Synthesis: prepared anti-aliased wavetable bank, band-limited sampling,
  loop crossfades, original versioned instrument/preset format and regressions.
- [x] File playback: worker stretch and automatic channel matrix, waveform peak
  cache, prepared impulse-response loading, seek/EOS/threading regressions.
- [x] Devices/MIDI: reconnect management and notifications, virtual MIDI1 ports,
  audio-clock event collector, typed Flex/Stream/Function Block support, MIDI2
  controller/profile behavior and fake-driver/protocol regression coverage.
- [x] Files: AIFC/CAF, optional OS codecs and MP3 export, retained marker/loop
  metadata, independent MP3 success fixture and lossy roundtrip checks.
- [x] Integration handoff: installed static consumer example, documentation,
  complete available-target regression/quality/measurement matrix, SIMD audit,
  provenance checks and Gitleaks snapshot.

Platform-dependent codecs must advertise actual runtime availability and return
explicit unsupported statuses for absent formats. New codecs/SDKs must preserve
the owner's closed-source commercial compatibility requirement. Source presence
must not be represented as physical device or unavailable-platform validation.

## Current checkpoint

DSP and synthesis APIs are implemented and pass the Windows x64 Clang regression
suite, including orders1..16 LP/HP edge attenuation, polynomial references, window
normalization, gate/latency boundaries, half-band/IIR gain/latency, harmonic
wavetable rejection, sampler alias rejection, loop crossfades, original preset
fixture/version handling and prepared allocation/free audits. Full target matrix,
documentation/measurement refresh and final review remain pending.

Elliptic transition shape uses the existing pinned MIT DSPFilters equations.
Additional exact reference files were archived and added to its manifest; runtime
filtering reuses CyDSP SOS cascades. Bessel is -3dB normalized. IIR oversampling has
frequency-dependent delay, with a separate DC-delay estimate; integer latency is
reported only for linear-phase modes.

The original CyDSPInstrument v1 JSON format and Instrument owner live in Files.
Sample paths resolve against a caller-owned base directory. Existing formats such
as SF2/SFZ are not implied by this original format.

Playback checkpoint: original WaveformPeakCache persistent pyramids with file-stamp
and payload checksum validation; ImpulseResponse uses the common channel matrix
and centered sinc conversion, normalization and optional tail trim. ReadAheadOptions
adds output channels, duration, pitch and formant choices; its worker primes
TimeStretch from past/future context after each epoch seek and preserves continuous
processing across pages. Audio reads retain the existing prepared SPSC protocol.
Windows Clang tests pass pyramid RMS/identity/corruption, IR phase/gain/moves,
mono mapping, 2x-duration/+12semitone spectra, seek priming and prime/process
allocation/free auditing. Source modules built without Stretch return
unsupported_backend when stretch is requested; no unavailable capability is implied.

MIDI/device progress: MidiEventCollector is implemented with bounded reusable
payload slots, a timestamp heap, stable equal-time ordering, late-event policy
and output-backpressure retention. MidiInput exposes steady-clock arrival stamps.
MIDI1 virtual input/output APIs advertise backend capabilities (WinMM has no
native virtual-port support). AudioDevice has atomic native notification flags;
AudioDeviceManager polls/retries on its serialized control owner and supports a
borrowed driver interface. Fake-driver disconnect/retry/interruption tests and
collector timing/retention/allocation tests pass on native Windows Clang.
MidiConnection now supplies native MIDI1/MIDI2 or a host driver, stable audio SPSC
queues, timestamp collection and reconnect epochs. Independent wire literals,
bounded transport retries, RPN/NRPN (absolute/relative), per-note range/detach,
profile audio-boundary dispatch and fake-driver reconnect tests pass. Unknown
profile semantics still require explicit application bindings.

Files checkpoint: original packed PCM AIFC (NONE/sowt/fl32/fl64) and CAF codecs,
byte-exact bounded ancillary metadata including marker/loop chunks, successful
independent MP3 silence fixture. Windows Media Foundation MP3/AAC/M4A/WMA
encoding/decoding/tone/decoded-seek tests pass on this host. AudioToolbox source
was added for Apple but cannot be compiled/tested here. Other systems advertise
no OS codec when this adapter is absent; portable WAV/AIFF/AIFC/CAF/FLAC/Ogg/MP3
reading remains independent of native codecs.


## Final available-target checkpoint

All thirteen required reports pass against compiler-input SHA-256
0ee6985c0a393e2d0fcdfc2b72b31118ad74972600921c2a9de61807da8632cc.
The final matrix includes Windows x64/x86/scalar/Clang/ASan, four Android ABIs,
Quest ARM64/ARM32 native regressions and measurements, production static builds,
65 isolated public headers, the installed static consumer and SIMD disassembly
of554 objects. Prepared processing audits detect zero allocations/frees in the
exercised new paths. Compiler build logs are clean.

Gitleaks8.30.1 default rules report zero findings across305 nonignored candidate
files (tracked and untracked, byte-exact snapshot; history is not scanned).
Reproduce with scripts/audit_secrets.py; the latest exact snapshot/report is
recorded locally in build/security-latest.json.

The owner can now run the intended DAW integration test using
[the integration contracts](DawIntegration.md) and
[the building-block example](../examples/DawBuildingBlocks.cpp).
[Validation](Validation.md) and [Measurements](Measurements.md) record actual
evidence. This handoff does not claim physical audio/MIDI2 interoperability,
Apple transport/codec compilation/runtime, desktop Linux/macOS acceptance,
Windows ARM64 runtime or SVE execution. Those require their respective targets.
This earlier expansion did not include GUI/plugin SDK integration. The subsequent
[plugin module](Plugins.md) adds CLAP/VST3 hosting. No CyDAW_Project change, publication, CI workflow
or device application installation was performed.
