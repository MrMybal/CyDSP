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

## Project split

Audio devices, file codecs, MIDI system declarations and plugin runtimes now belong
to CyAudioCore. CyDSP contains only the five MIT numerical adaptations listed above.
Each original notice and pinned upstream file remains with its owning project.
