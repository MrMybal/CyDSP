# Licensing and provenance

The owner has not selected a release license for original CyDSP code. MIT imports
do not implicitly license the rest of the project. No first-party license grant
or public release has been created in this work.

| Upstream MIT component | Adapted scope | Source in CyDSP |
| --- | --- | --- |
| DSPFilters / Vinnie Falco | RBJ low/high/band-pass, notch, all-pass, peaking and shelf equations | src/processors/BiquadDesign.cpp |
| FFTConvolver / HiFi-LoFi | Partitioning and overlap-add, including progressive shared MIMO | src/processors/PartitionedConvolver.cpp; src/processors/MatrixConvolver.cpp; src/processors/ScheduledConvolver.cpp |
| r8brain-free-src / Aleksey Vaneev | Windowed-sinc and cosine window equations | src/math/WindowedSinc.cpp |

CyDSP rewrites lifecycle, public types, validation, channel state and memory use
under common conventions. Convolution uses CyDSP FFT/math rather than upstream
AudioFFT. Resampling is an independent rational streaming implementation; only
specified equations are adapted, not the complete r8brain conversion pipeline.

third_party/manifest.json records exact 40-character revisions, original URLs,
archive paths, SHA-256 digests, attribution, notice paths and adaptation scope.
Original files remain under third_party/*/upstream as text archives. Complete MIT
notices remain under third_party/licenses and attribution stays in adapted sources.
The source verifier checks every component independently. CMake installation carries
all three original notices, the manifest and this document. Source redistribution
must retain the attribution, notices and archived provenance.

The original FFT, buffers, gain, SIMD kernels, FIR, delay and shared infrastructure
have no imported copyleft dependency. No FFTW, JUCE, KFR, Eigen, Intel IPP, PFFFT,
LLVM runtime or complete external DSP framework is bundled. Future imports require
a separate per-file and dependency review; a repository label alone is insufficient.
