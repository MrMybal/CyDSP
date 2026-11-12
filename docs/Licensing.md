# Licensing and provenance

Original CyDSP code is distributed under the MIT license.
Copyright (c) 2026 MrMybal CyberAlien. The complete grant is in the root LICENSE
file, also installed beside this document in CMake packages. Adapted third-party
code retains its original copyright and complete MIT notices below.

| Upstream MIT component | Adapted scope | Source in CyDSP |
| --- | --- | --- |
| DSPFilters / Vinnie Falco | RBJ low/high/band-pass, notch, all-pass, peaking and shelf equations | src/processors/BiquadDesign.cpp |
| FFTConvolver / HiFi-LoFi | Partitioning and overlap-add, including progressive shared MIMO | src/processors/PartitionedConvolver.cpp; src/processors/MatrixConvolver.cpp; src/processors/ScheduledConvolver.cpp |
| Signalsmith Stretch / Geraint Luff | Pitch/time/formants, common lifecycle and CyDSP STFT | src/detail/StretchAlgorithm.h |
| Signalsmith Linear / Signalsmith Audio | Half-bin packed real FFT reconstruction and STFT/window normalization, with CyDSP SIMD transforms | src/detail/StretchFft.h; src/detail/StretchStft.h |
| r8brain-free-src / Aleksey Vaneev | Windowed-sinc and cosine window equations | src/math/WindowedSinc.cpp |

CyDSP rewrites lifecycle, public types, validation, channel state and memory use
under common conventions. Convolution uses CyDSP FFT/math rather than upstream
AudioFFT. Resampling is an independent rational streaming implementation; only
specified equations are adapted, not the complete r8brain conversion pipeline.

third_party/manifest.json records exact 40-character revisions, original URLs,
archive paths, SHA-256 digests, attribution, notice paths and adaptation scope.
Original files remain under third_party (the initial components use upstream subdirectories), with exact pinned bytes. Complete MIT
notices remain under third_party/licenses and attribution stays in adapted sources.
The source verifier checks every component independently. CMake installation carries
the CyDSP LICENSE, all imported component notices, the manifest and this document. Source redistribution
must retain the attribution, notices and archived provenance.

The original FFT, buffers, gain, SIMD kernels, FIR, delay and shared infrastructure
have no imported copyleft dependency. No FFTW, JUCE, KFR, Eigen, Intel IPP, PFFFT,
LLVM runtime or complete external DSP framework is bundled. Future imports require
a separate per-file and dependency review; a repository label alone is insufficient.

## Optional private backends

Original CyDSP interfaces/adapters remain MIT. Dependencies retain their own grants;
permissive dependencies do not require opening the consuming application's code.
The installed `share/CyDSP/licenses` directory must accompany binary distributions
as required by the retained notices. No JUCE implementation, header or binary is
used by these modules.

| Component | Exact license selection | Built use |
| --- | --- | --- |
| miniaudio | MIT-0 alternative | Audio devices; no engine or built-in codecs compiled |
| RtMidi | MIT | MIDI1 devices, private WinMM/CoreMIDI/optional ALSA backend |
| Microsoft Windows MIDI Services native declarations | MIT | Adapted private COM ABI/GUID declarations; no service/driver binary bundled |
| CHOC | ISC | WAV/FLAC/Ogg/MP3 wrappers and required header closure |
| FLAC embedded in CHOC | BSD 3-clause | FLAC decoding and encoding |
| Ogg/Vorbis embedded in CHOC | BSD 3-clause, plus retained Degener/Bormann notice | Vorbis decoding and encoding |
| minimp3 embedded in CHOC | Upstream public-domain dedication | MP3 decoding only |
| Signalsmith Stretch | MIT | Pitch/time/formant processing |
| Signalsmith Linear 0.6.6 | MIT | Adapted STFT/half-bin equations; CyDSP replaces the upstream FFT at runtime |
| CLAP pinned C interfaces | MIT | Original host adapter and original ABI fixture plugin |
| Current Steinberg VST3 pluginterfaces | MIT | Original processor/controller/editor host adapter and original ABI fixture |

The manifest pins every imported file, including embedded codec headers, by commit,
URL and SHA-256. Optional IPP, Accelerate and PFFFT integrations are not enabled.
AIFF PCM, MIDI protocol/SMF, effects and new public adapters are original CyDSP code.
Pinned MIT CLAP and VST3 interface headers are imported privately; no GUI framework is imported. RtAudio, nlohmann/json, pugixml and
rigtorp/SPSCQueue are candidates rather than dependencies: the current modules use
miniaudio, CHOC JSON internally and an original bounded SPSC queue.

The Signalsmith originals are compiled only by the explicit measurement target to
provide an upstream comparison. The production Stretch target compiles the adapted
CyDSP engine, retains MIT notices and reuses ComplexFft, VectorOps and prepared
workspace. Read-ahead/synthesis/UMP/MPE/tempo/transport/recording are original CyDSP
implementations. Random-access sinc shares the previously attributed windowed-sinc
equations; it does not import another conversion library. No CyDAW/JUCE implementation code is imported. The supplied CyDAW integration report and test observations were read as diagnostic evidence.

Native UMP adapters, MIDI-CI state machines and UMP Stream negotiation are original
CyDSP implementations. The private Windows ABI header retains Microsoft's copyright
and full MIT notice with its pinned IDL provenance. Platform SDKs are build/runtime
requirements rather than vendored dependencies. No MIDI specification PDF or code
from another MIDI-CI implementation is included in the package.


The original AIFC/CAF and native Media Foundation/AudioToolbox adapters are MIT
CyDSP code. OS codecs are discovered at runtime and are not redistributed in this
repository. Missing native encoders return an unsupported status. No JUCE,
LAME/FFmpeg or other copyleft implementation was imported for this expansion.
Additional DSPFilters and Windows MIDI builder references remain byte-exact,
pinned and covered by their original MIT notices and manifest records.


Plugin interfaces and the original worker keep CyDSP's MIT grant. Exact SDK
revisions and notices are pinned in the manifest. No AAX/ASIO/VST2 SDK, JUCE implementation, JUCE JSFX wrapper or copyleft LV2
host is bundled. External providers retain
their own terms. OS/native or separately licensed SDK access is not granted by
CyDSP MIT. The built-in hosts are not a GUI framework or a complete DAW.


## LV2 and JSFX

LV2, Lilv, Serd, Sord, Sratom and Zix are pinned ISC components compiled privately
inside Plugins. ysfx is Apache2.0, with embedded WDL/EEL2/LICE (Zlib-style), ISC
Base64 helpers, MIT dr_libs and stb's published licensing choices. All original
source notices remain intact; the installed ysfx notice collects embedded grants.

The generated EEL2 parser has GPL-2.0-or-later WITH Bison-exception-2.2. The
exception explicitly permits a larger work under its own license when it is not a
parser generator using this skeleton. CyDSP uses it as a language runtime; it is
not a Bison-style parser generator. The complete exception and GPL2 text are
retained. This does not require CyDSP or a consuming commercial DAW to adopt GPL.
The complete combined expression and per-file hashes are in the manifest; an
Apache repository label alone is not substituted for the dependency review.

Original CyDSP host/editor/IPC code remains MIT. Adapted runtime files retain their
upstream grants, marked modifications and notice links. The complete distribution
therefore includes licenses besides MIT; imported components are not relicensed
by changing the library's top-level license. Plugin/script redistribution is a
separate question from implementing the host. No third-party effects are bundled;
fixtures are original CyDSP test code.
