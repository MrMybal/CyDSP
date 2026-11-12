# Audio coverage inventory

2026-11-12. This inventory compares functional areas against the public
[JUCE module documentation](https://docs.juce.com/master/index.html), without using
JUCE implementation code. API compatibility and identical algorithms are not goals.
It records delivered scope and the remaining work; it does not claim complete JUCE
replacement. GUI rendering remains application work. CLAP/VST3/LV2/JSFX hosting and native editor
management are delivered; AU uses an external provider; AAX hosting rights remain unestablished.

| Area | CyDSP implementation | Remaining coverage |
| --- | --- | --- |
| Blocks, buffers, vectors, mixing | Planar float/double, layouts, matrices, ramps, gains, bias, pan/dry-wet laws | Extra convenience/source adapter APIs |
| FFT and windows | Complex/real transforms, vector kernels, Hann/Hamming/Blackman, Blackman-Harris/Nuttall/flat-top/Kaiser/triangular windows and normalization | Additional application-specific windows |
| FIR and resampling | FIR, Kaiser low/high-pass design, streaming rational/variable conversion, centered random-access sinc, Lagrange/Catmull-Rom and EOS | More FIR design methods |
| IIR and filter design | RBJ filters/EQ, cascades/banks/transitions, Butterworth SOS1..32, Chebyshev I/II/elliptic/Bessel LP/HP1..16, TPT, LR4, low-pass ladder | Additional ladder responses and design tools |
| Dynamics | Compressor/expander/gate/limiter, peak/RMS detector, ballistics, gate hold/hysteresis and bounded look-ahead | More listening/preset acceptance |
| Generators/envelopes | Oscillators, white noise, ADSR and harmonic wavetable mip banks | Band-limited triangle |
| Nonlinear/oversampling | Wave shapers, custom borrowed function, FIR/half-band FIR/IIR1/2/4/8/16x | More quality/latency policies |
| Modulation/reverb | Original chorus, phaser, Schroeder reverb | Alternative topologies and listening/preset acceptance |
| Convolution | Uniform/hybrid/nonuniform/progressive, shared MIMO and IR publication; shared file/IR loader | Additional application IR formats |
| Math/utilities | dB, polynomial, immutable lookup, smoothing, bounded SPSC | General matrix algebra, additional fast-math approximations |
| Processing composition | Prepared AudioGraph and custom borrowed processor callbacks | Typed chain/source convenience adapters |
| Time/pitch/formants | Adapted MIT pitch/formant/STFT engine using CyDSP SIMD FFT, including 3×power-of-two sizes and worker stretch/seek priming | More transient/material quality studies |
| Audio devices | Private miniaudio, planar callback adapter, persistent/ambiguous selectors, native capabilities, negotiated formats/maps and callback clock/diagnostics | Physical duplex/latency/hotplug acceptance; ASIO is not bundled; Physical manager acceptance/UI |
| MIDI devices | RtMidi MIDI1 plus owned native UMP Windows Services/CoreMIDI/Linux/AMidi adapters, bounded timed worker, reconnect coordinator, MIDI1 virtual ports and API33 JNI bridge | Physical MIDI2 interoperability and Apple build/runtime |
| MIDI protocol | Wire MIDI1, sample-ordered bounded buffer, streaming/SysEx parser, sustain/keyboard state, SMF0/1/2 and tempo/SMPTE; MPE zones/instrument, RPN/NRPN, UMP/MIDI 2 voice/SysEx7/8 and conversions, CI discovery/profiles/chunked Property Exchange, ResourceList/DeviceInfo/ChannelList/JSONSchema, instances/partial SET/pagination, subscriptions/client cache/Notify, UMP Stream Configuration, clock collector, typed Flex/Stream/Function Blocks, RPN/NRPN and profile audio dispatch | zlib PE, instrument-specific standardized profile semantics and physical interoperability |
| Audio files | WAV/AIFF/AIFC/CAF PCM/FLAC/Ogg read-write, MP3 read, optional native MP3/AAC/M4A/WMA, seek/EOS, native marker/loop chunks | Apple codec validation; compressed CAF and cross-container marker translation |
| File playback/source infrastructure | AudioSource/memory source, queued play/pause/stop/seek/loops, tempo map, worker read-ahead/sinc/stretch/channel mapping, persistent waveform and PCM editing caches, absolute time/pitch curves, repitch/tonal/complex modes and recording queue | Network sources and more application adapters |
| Synthesis | Prepared oscillator/wavetable/mip sampler/custom voices, loop crossfades, CyDSPInstrument v1 presets, ADSR, voice stealing/fades, sample-accurate MIDI1/MPE/UMP expression | SF2/SFZ formats, sound-bank/tuning-table application and more MIDI profile sound engines |
| JSON/XML | CHOC JSON internally for metadata | Application-facing serialization/XML API is outside this DSP/device/file increment |
| Plugin hosting and native editors | Original CLAP/VST3/LV2/JSFX adapters, multibus/automation/state, native embedding/resize/DPI/focus and separate-process worker | AU/AAX hosting; non-Windows JSFX canvas; cross-platform UI/third-party plugin acceptance and application exports |

## Validation boundaries

The expanded regression suite checks independent transfer functions, envelope time
constants, LR4 summed response, FIR rejection, oversampling latency/DC/alias reduction,
static chorus delay, phaser all-pass DC, reverb decay, block splits and retained
processing/reset allocations. File tests include independent PCM16 WAV and AIFF
fixtures, blockwise PCM24 WAV/AIFF/FLAC roundtrips, lossy Ogg error, seeking and EOS.
A 440 Hz tone shifted one octave is checked spectrally for float/double, including
formants, retained processing/reset memory and tail draining. MIDI checks cover
fragmentation/running status/realtime inside SysEx, SMF fixtures and concurrent SPSC
order. An original successful MP3 silence fixture checks the portable decoder. Windows native MP3/AAC/M4A/WMA tone roundtrips and exact decoded seeks pass; listening acceptance remains for integration. New suites check direct half-bin DFTs (power-of-two and mixed radix), exhaustive 14-bit pitch roundtrips, MPE/sostenuto, sample-accurate/custom synthesis, stereo sampler/EOS, transport loops/underruns, queued epoch seeks/full-capacity prefill, centered sinc timing and exact recording drain/readback. Physical MIDI enumeration does not establish send/receive quality.

Audio callback tests use only miniaudio's null device. No real microphone is read,
no speaker output is emitted, no application is installed and no device setting is
changed. Build/runtime evidence and its current source digest belong in
[Validation](Validation.md); measured workloads belong in [Measurements](Measurements.md).
Desktop Linux/macOS, Windows ARM64 application runtime and SVE hardware still need
their corresponding machines. Compiling a platform is distinct from executing it.

## Integration boundary

Stretch's runtime uses owned adapted MIT sources in `src/detail`, not the archived
upstream FFT. Its compact 3×power-of-two transforms compose the existing SIMD
ComplexFft stages and retain half-bin STFT normalization. Original upstream headers
are retained byte-for-byte for provenance and an opt-in same-build benchmark only.
File workers use CyDSP AudioBuffer, VectorOps, SPSC and SincInterpolator; the sampler
uses the common oscillator/ADSR/channel matrix/mixing. Device callbacks reuse common
layout conversion. miniaudio/RtMidi/CHOC remain private drivers/codecs: those platform
and format implementations have not all been rewritten. Their licenses are listed
in [Licensing](Licensing.md); they are not all MIT.

MIDI 2 packets use groups 0..15 and channels 1..16. Framing includes opaque reserved,
Flex and Stream types; factories and synth handlers cover the documented subset.
Native MIDI 2 uses the owned adapters described in [MIDI 2](Midi2.md), with explicit
platform and physical acceptance limits. The host supplies matching audio-clock anchors, property resource policies and explicit profile bindings. Read-ahead converts sample rate/channels and schedules time/pitch on its worker. Native-rate sampling is exact; pitched samples can select prepared filtered mips. No CyDAW_Project source or JUCE implementation was imported.

The [property service](CiProperty.md) now supplies generated resource discovery and
DeviceInfo/ChannelList/JSONSchema, custom instances, full/partial SET, paginated
lists and notify/full/partial subscriptions with retained replies. Its client adds
prepared TTL caching, coalesced refresh and stale-GET protection. General schema
evaluation, zlib and instrument-specific profile semantics remain host policy or
future work; no complete MIDI-CI conformance certification is claimed.
macOS/Linux desktop validation is deferred by the owner.
