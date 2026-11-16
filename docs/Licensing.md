# Licensing and provenance

Original CyDSP code is distributed under the MIT license.
Copyright (c) 2026 MrMybal CyberAlien. The complete grant is in the root LICENSE
file, also installed beside this document in CMake packages. Adapted third-party
code retains its original copyright and complete permissive notices below.

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
to CyAudioCore. The core retains the five MIT numerical adaptations listed above. Optional SOFA readers are described below.
Each original notice and pinned upstream file remains with its owning project.

## Ten-lot numerical extension

The DSP/3D expansion uses original CyDSP implementations and the existing MIT
numerical primitives. No additional dependency, HRTF dataset, application/plugin
source or copyleft code was imported. Public mathematical papers and standards
are references for equations, not bundled software. Host-supplied HRIR/IR/sample
data remains the host’s licensing responsibility.

The second advanced extension adds original MIT CyDSP algorithms and reuses the
existing five pinned adaptations; no new third-party source or HRTF dataset is
imported. See [the advanced guide](AdvancedExpansionGuide.fr.md).

## Optional SOFA import (0.4.0)

The core and decoded-asset/cache target do not depend on a file-format library.
CyDSP::Sofa optionally adds a memory-only reader adapted from libmysofa
(BSD-3-Clause), its portable-endian permissive grant, and miniz (MIT) for inflate.
Only the listed reader files are included, not libmysofa's renderer, resampler,
normalization or datasets. Original CyDSP code remains MIT; upstream parts retain
their own licenses/notices.

The complete reader optionally links external HDF5 (BSD-3-Clause) and zlib
(Zlib license). Neither SDK is vendored or downloaded by CMake. Supplied SDK notices
correspond to reviewed HDF5 2.2.0/zlib 1.3.1; distributors must retain notices for
their actual SDK build and enabled dependencies. No GPL/AGPL/LGPL component is added.

These legal texts are installed under share/CyDSP/licenses and included in CyDoc.
Measured HRTF banks have separate rights; none is shipped. Test fixtures are
original synthetic CyDSP signals covered by the root MIT license.

## Original HOA decoder and analytic HRIR additions

The MagLS/weighted-LS designer, max-rE Legendre taper, Fibonacci generator and
owning analytic HRIR helper are original CyDSP MIT code using its existing FFT,
matrix convolution and analytic binaural processor. The DAGA 2018 publication is
a mathematical reference. No IEM/SPARTA implementation, extra dependency or
measured HRTF dataset is included.
