# DAW integration contracts

Work is confined to CyDSP. The owner will perform integration in their DAW.
Public types are original CyDSP interfaces; no JUCE implementation/API compatibility
layer or existing DAW source is used.

## DSP and preparation

FilterDesign supplies Butterworth, Chebyshev I/II, elliptic and Bessel LP/HP SOS
designs at orders1..16. Existing designButterworth also supports orders up to32.
Use BiquadCascade or the existing smoothed cascade to process returned sections.
Design commits output/count only on success. ChebyshevII cutoff is its stop-band
edge; Bessel cutoff is -3dB. Elliptic takes the pinned MIT DSPFilters transition
shape parameter (0..4), with an explicit pass-band ripple.

PolynomialInterpolator supports Lagrange orders1..8 and cubic Catmull-Rom.
Source guard samples are caller-owned; invalid bounds/overlap are rejected before
writes. Its retained SIMD dot selection shares the existing VectorOps core.
WindowParameters adds Blackman-Harris, Nuttall, flat-top, Bartlett, triangular,
cosine and Kaiser, optional peak/coherent-gain/RMS-energy normalization, and symmetry.

Dynamics prepares maximum look-ahead memory (20ms default). Parameters choose
actual lookAheadMs, holdMs and hysteresisDb. A bounded monotonic peak queue exposes
future transients to the detector; audio is delayed by exactly latencyFrames().
Gate hold counts samples below its lower close threshold. Changing look-ahead
length clears/reprimes the delay; the host should schedule that change appropriately.
Gate gain and detector use the supplied attack/release times.

Oversampler's original FIR mode remains the default. halfBandFir and iir use
prepared 2x stages for factors2/4/8/16. FIR staging reports exact integer linear-phase
latency. IIR has frequency-dependent group delay: latencyFrames() returns0 and
latencyAtDcFrames() reports the low-frequency estimate. A host must not treat that
estimate as universal phase compensation.

## Synthesis and instrument ownership

Wavetable prepares power-of-two cycles into harmonic FFT mip levels, using the
existing ComplexFft implementation. The oscillator/synth borrow a stable table.
Conservative mip blending reduces high-frequency aliasing. Tables can be shared
read-only between voices, provided lifecycle is externally serialized.

SampleData copies source audio and prepares filtered lower-rate levels using
SincInterpolator and VectorOps. SynthParameters selects linear or bandLimited
sampling. Native integer positions remain exact; pitched playback selects a
filtered level. This is a finite mip approximation, not an ideal reconstruction
filter at every ratio. Optional setLoop crossfades blend the tail into the head;
the loop resumes after the already-consumed crossfade region.

InstrumentPreset is the original version1 CyDSPInstrument JSON format. It stores
synth/envelope parameters, voice count, MPE zones, root note, relative sample path,
loop/crossfade positions or a wavetable cycle. Parse/serialize reject malformed
types/ranges/versions and preserve the previous in-memory value on failure.
JSON nesting is bounded; public headers expose no CHOC types.

Instrument (link Files) owns its SampleData/Wavetable before its Synthesizer so
borrowed lifetimes remain stable through moves. Preparation resolves sample paths
against a supplied base directory, loads/validates before commit, and bounds sample
allocation. process/reset retain memory. SF2/SFZ compatibility is not implied by
the original CyDSP format.

## Background playback and assets

ReadAheadOptions adds output rate/channel count, built-in durationRatio1/64..64,
pitchSemitones-48..48 and formant0.25..4. Requesting stretch requires the optional
Stretch target in the Sources build. Missing support returns unsupported_backend.
Ordinary native-rate playback keeps its exact copy path.

The default discrete channel matrix averages channels to mono, replicates mono,
otherwise preserves matching indices and silences additional outputs. It does not
guess surround speaker positions. The same function is used by synthesis, IR
loading and file-worker conversion.

The worker performs decoding, sinc conversion, channel mapping and stretching.
On an epoch seek it supplies past/future context to TimeStretch::prime, then
continues one processor stream across subsequent cache pages. The public length
includes the selected duration ratio; extra processor look-ahead is internal.
prefill is a control-thread operation; readAt retains memory and reports bounded
underruns. Lifecycle/configuration changes require an idle audio reader.

WaveformPeakCache stores per-channel min/max/RMS at power-of-two bucket scales.
Build/save/load perform I/O on the control/worker owner. Read-only level selection
and peak views allocate nothing. The original cache format carries source size/
modification time, audio shape and a payload checksum. A changed source or corrupt
cache is rejected before replacing the previous cache. File stamps are a cache
invalidation mechanism, not an authentication guarantee.

ImpulseResponse loads through AudioFileReader, optionally remaps/resamples with
common CyDSP processors, trims a silent tail and normalizes peak amplitude.
samples() supplies immutable planar IR spans for the existing convolution engines.
Loading may allocate; convolution preparation copies the selected impulse data.

## Devices, MIDI timing and endpoint services

AudioDeviceManager and MidiConnection have one serialized control/worker owner.
The application calls poll(nowMs); neither manager creates a polling thread.
They retain selection, enumerate changed ports and retry failed/disconnected
streams with configured intervals. AudioDeviceManager reports interruption,
rerouting and lifecycle notifications. Its driver hooks permit host-owned
platform integration without exposing miniaudio types.

MidiConnection accepts native MIDI1, native MIDI2, or a borrowed host driver.
The legacy MidiConnection selection keeps its original name/occurrence IDs.
New integrations use MidiPortSelector/resolveMidiPort and MidiInputGroup:
WinMM device-interface identities are preferred; name fallback is explicit and
duplicate names return ambiguous. Backends without persistent IDs remain
explicitly identified instead of guessing.
Native MIDI2 retains endpoint IDs. Android's host can provide its opened AMidi
connection through the driver interface. A full queue returns update_queue_full;
outgoing packets remain queued on temporary transport pressure. Disconnect
invalidates queued input/output epochs. Observe connected()/generation() at the
audio boundary and release held voices in the application. Never destroy,
prepare or move the manager while its audio owner is active.

MidiEventCollector has one device/control producer and one audio consumer.
The host supplies AudioClockAnchor in the same steady_clock nanosecond domain as
arrival timestamps. Collection maps events to offsets in the current block,
retains future events and orders equal timestamps stably. Late policy is clamp
or drop. It does not estimate hardware-to-system clock drift or invent a device
latency. invalidate() belongs to the producer; reset() requires both owners idle.

MidiInput/MidiOutput openVirtual is available only when RtMidi's selected native
backend supports virtual ports (CoreMIDI, ALSA, JACK). WinMM returns
unsupported_backend. Windows MIDI Services application endpoints are not emulated
by this MIDI1 API.

UmpExtensions supplies bounded UTF-8/ASCII text assembly, typed Flex tempo,
time signature, metronome, key/chord messages, Stream identity/text, clip
boundaries and Function Block discovery/information. UmpEndpointService owns
prepared endpoint metadata and replies, delegates protocol negotiation and keeps
replies under transport backpressure. Dynamic Function Block changes require a
non-static endpoint and queue a notification before committing. process/poll
share one owner; Sender must obey the caller's thread requirements. Peer text
can be consumed with UmpTextAssembler; full bidirectional endpoint catalogue
management remains application policy.

Midi2Controllers retains CC values and bounded sparse RPN/NRPN values, including
signed relative operations with saturation. Standard tuning/pitch ranges,
program/bank and MPE configuration fields are exposed. Synthesizer applies
volume/expression/pan/sustain, channel and per-note pitch/pressure/timbre, tuning,
controller reset and per-note detach/reset. Per-note registered pitch never
overrides a Note On Pitch79 attribute. Tuning program/bank selects an identifier;
applications provide actual tuning tables and sound/program banks.

CiProfileDispatcher binds explicit profile IDs to application-owned noexcept
handlers. CI/control calls notify; the audio consumer calls applyPending at a
block boundary. Unknown IDs are rejected. A received profile ID alone cannot
define an instrument's sound engine; use the binding to apply your implemented
profile behavior. This is not certification for every published MIDI profile.

## Audio containers, codecs and metadata

AudioFileReader/Writer add AIFC (NONE/twos/sowt and big-endian fl32/fl64) and CAF
packed linear PCM: signed8/16/24/32 and float32/64. CAF accepts its own endian
flags and a final unknown-size data chunk; compressed CAF packets and nonpacked
PCM are rejected. Writers are seekable and finalise sizes on flush/close.
AIFC floating-point export is big endian; requesting little endian is rejected.

WAV metadataJson keeps its native chunk representation for pass-through. The public AudioMetadata API adds versioned markers/regions/loops/tags. Native chunks (cue/smpl/inst and other
supported chunks). AIFF/AIFC/CAF metadata is an array of {type,hex} objects for
native ancillary chunk payloads. Chunk order and payloads are retained, including
MARK/INST or mark/regn/strg. Structural chunks are regenerated and cannot be
injected as metadata. Metadata is bounded to4096 chunks/16MiB decoded bytes.
Preservation does not automatically translate marker/loop semantics between
different container families. MP3/AAC/WMA marker embedding is not supplied;
native export rejects nonempty metadata instead of silently dropping it.

The optional Windows Media Foundation adapter supplies path-based MP3, AAC,
M4A and WMA export and platform reading. audioFileFormats probes installed
encoder/decoder availability. Export accepts mono/stereo integer-rate pcm16
input descriptions and quality128/192/256/320 kbit/s; the closest installed
matching encoder type is selected. Compressed data does not have a PCM bit depth.
write clips/quantizes its normalized float/double input. close reports encoder
finalization failure; flush leaves the encoder writable and does not promise an
on-disk, independently decodable file until close.

The Apple AudioToolbox adapter uses installed codec discovery, ExtAudioFile and
the same path API; its source awaits Apple compilation/runtime validation.
The other platforms currently report no native codec adapter; portable file
formats remain usable. No embedded MP3 encoder or copyleft dependency was added.

The Windows platform reader indexes decoded frames once with bounded memory.
Sequential reads are cached; a nonsequential seek replays decoding from the
beginning to preserve exact decoded-frame indexing. Large-file random seeks via
this fallback are therefore O(position); use portable formats for editing/cache
files. Portable MP3 reading remains the first-choice decoder. Native stream
overloads are not implemented; path APIs select native codecs.

Format references: [Apple CAF1.0](https://developer.apple.com/library/archive/documentation/MusicAudio/Reference/CAFSpec/CAF_spec/CAF_spec.html),
[Microsoft MP3 encoder](https://learn.microsoft.com/en-us/windows/win32/medfound/mp3-audio-encoder),
[AudioToolbox extended files](https://developer.apple.com/documentation/audiotoolbox/extended-audio-file-services).


## Integration report remediation

See [Integration remediation](IntegrationRemediation.md) and [plugin hosting](Plugins.md).
AudioDeviceContext exposes explicit backend/share options, native capability
tuples, persistent selectors and actual stream descriptions. Unknown buffer ranges,
duplex facts, hardware latency and ADC/DAC timestamps stay unknown. Extended
callbacks anchor MIDI timing to steady_clock with an explicit generation.
Callback cost/deadline misses are distinct from driver xruns.

EditingPcmProxy constructs an exact decoded disk cache and stamps its source.
Float64 is default; float32 is explicit. maximumBytes and
maximumWorkingBufferBytes bound the cache/copy buffer. Buffer statistics include
sample alignment and pointer table; metadata/codec workspace/OS caches are separate.
Windows tests use original60-second AAC/WMA sources, exact PCM comparison and
sampled process private memory. Atomic no-replace rename (or hard-link fallback) requires a
supporting filesystem; a missing capability returns an error rather than overwrite.

SourceStretchMode exposes none/repitch/tonal/complex/beats/texture/external.
Repitch couples speed/pitch; independent pitch and formant stay neutral. Its
variable-position sinc uses maximum curve step for conservative anti-aliasing.
Tonal uses an explicit cutoff; complex uses spectral processing. Pitch curves
update at a declared control quantum, not claimed sample-exact. SourceTimeMap
describes absolute source positions and pitch curves; unsupported controls are
rejected without clamping. Seek history and optional conservative tails are
explicit in SourceStretchInfo. Ordinary native-rate playback keeps exact copying.

Beats/texture require SourceStretchProvider to retain application-specific
algorithms. The returned processor is owned/driven by the source worker, with
latency/prime and curve contracts. No CyDAW algorithm is copied and identical
sound is not claimed. An absent provider returns unsupported_format.
APP-001/003/004 remain application shell/model/routing migration as stated in
the supplied report; CyDAW and its existing algorithms are not modified here.
