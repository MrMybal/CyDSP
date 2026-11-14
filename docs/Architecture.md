# Architecture

Public headers live in include/cydsp. Original public names use namespace cydsp;
SIMD types, function tables, overlap helpers and algorithm details remain private
in src. Core supplies planar views, aligned storage, status handling, block
validation and CPU detection. Math supplies immutable selected kernel tables.
Processors compose these services without exposing differing upstream APIs.

## CPU implementations

One static archive is built per target architecture. Scalar/SSE2/AVX2/AVX-512/NEON/SVE are
implementations selected inside that architecture, not interchangeable CPU ABIs.
CMake uses the target compiler architecture on MSVC, including Visual Studio cross
builds, before falling back to CMAKE_SYSTEM_PROCESSOR.

ISA flags apply only to the matching implementation translation unit. Baseline
CPU detection and processors have no AVX instruction-set option. x86 means a modern 32-bit SSE2
platform; pre-SSE2 systems are outside the supported baseline. x64 uses its SSE2
baseline. Disabling explicit SIMD does not remove compiler/ABI baseline instructions.

Auto chooses compiled AVX-512 on compatible x64, then AVX2/SSE2; compiled SVE on
compatible ARM64 Linux/Android, then NEON; otherwise scalar. This deterministic feature policy is not a benchmark-based
choice for every buffer size. Callers can force a supported backend during prepare.
An unavailable backend returns unsupported_backend and preserves the old plan.

AVX2 requires CPUID XSAVE/OSXSAVE/AVX, XCR0 XMM+YMM state and the CPUID AVX2 bit.
Feature detection occurs in preparation. There is no global dispatch switch or
implicit application floating-point environment mutation. SSE2/AVX2 kernels use
unaligned external loads and scalar tails; owning audio storage aligns each channel
to 64 bytes. Complex kernels use the scalar array-access guarantee of std::complex.

ARM64 uses NEON as its architectural baseline. ARM32 Linux/Android probes
HWCAP_NEON and Windows probes its platform feature; unknown ARM32 OSes use scalar.
ARM32 double and mixed-precision kernels fall back to scalar. ARM64 clang-cl static
compilation covers NEON float/double, mixed dots, all public headers and regression
objects. Android NDK cross-builds cover ARM32/ARM64 with libc++; the connected Quest 3
provides actual ARM execution separately from Windows ARM64 archive compilation.
Windows ARM64 application linking/runtime remains distinct and unverified.
Static Windows cross-build probes build archives instead of executables.
ARM64EC is explicitly rejected pending its own ABI/dispatch work. RISC-V vectors
and an explicit opt-in FMA arithmetic policy remain future work.

AVX-512 requires AVX2 plus CPUID AVX512F/DQ/CD/BW/VL and XCR0 mask 0xe6.
This conservative bundle covers all instructions the compiler may emit for the
private translation unit. It is compiled for x64 only; x86 keeps SSE2/AVX2.
SVE is compiled only for ARM64 Linux/Android and selected through HWCAP_SVE.
Predicated kernels use the current thread's vector length on every call; no fixed
VL, SVE2 dependency, prctl or CPU-state mutation is used. Windows/macOS ARM use
NEON. Both paths cover float/double, mixed dots, complex operations, voice banks
and fused gains. Masked AVX-512 and predicated SVE residuals respect span bounds.

Detection follows the [Intel ISA reference](https://www.intel.com/content/dam/develop/external/us/en/documents/319433-024-697869.pdf),
[Linux SVE ABI](https://docs.kernel.org/arch/arm64/sve.html) and
[Arm ACLE](https://arm-software.github.io/acle/main/acle.html).
The connected Ryzen executes AVX-512; the Quest executes NEON and reports no SVE.
SVE runtime and its different vector lengths still need compatible hardware.

## Lifecycle and memory

Preparation validates first and commits the new configuration only on success;
it may allocate and free memory. Explicit IR staging in SmoothedConvolver is also
non-real-time preparation and may replace/free an inactive plan. Processing and reset allocate no memory, perform
no I/O, acquire no locks and do not free prepared buffers. Re-preparation, moves,
destruction and parameter changes must be serialized with processing by the owner,
except AsyncConvolver's explicit single-producer publication API.
Moves leave owning signal processors unprepared and owning buffers empty.

Separate mutable instances can process independently. Sharing one mutable instance
concurrently is unsupported except AsyncConvolver's one producer/one consumer. ComplexFft has immutable tables and a const process
method; distinct caller buffers can share a prepared plan if preparation/moves
are externally serialized. RealFft and convolution own mutable scratch and cannot
share one instance across simultaneous calls.

AudioBlock owns neither samples nor the channel pointer table. subBlock shares
that same table plus a frame offset, so both must outlive every sub-view. channel()
applies the offset; channelData() exposes the original pointer table. Invalid
subBlock bounds return an empty view. Pointer checks cannot validate the size or
lifetime of arbitrary caller allocations.

## Processing contracts

Fixed-rate planar processors validate channel counts, equal input/output frames,
prepared block limits and alias hazards before changing output or signal state.
Exact same-channel in-place operation and disjoint buffers are supported.
ChannelMatrix uses separate input/output counts and permits exact cross-channel
aliases by snapshotting every input tile before writing outputs. Partial
overlap, hazardous cross-channel aliases and overlapping output channels fail.
Zero-frame blocks with prepared channel counts are no-ops. Buffers are not silently
truncated when limits are exceeded.

For ordered, internally disjoint planar input/output ranges, alias validation uses
a linear merge sweep. Exact corresponding-channel aliases are accepted; any other
intersection is rejected. Views with unsorted or internally overlapping ranges use
the complete pairwise check. Empty blocks skip range work. The fast path requires
no scratch or allocation and preserves the same accepted/rejected layouts. Its
integer-range policy is checked against a brute-force interval oracle at compile time.

Resampling has separate input/output limits and consumes a variable number of
frames. It rejects all input/output overlap and reports both consumed and produced
counts. FFT has span-based shape and overlap rules; real FFT requires disjoint
input/output storage. See Modules.md for these specific contracts.

## Granulation and analysis composition

Granulator borrows prepared SampleData, uses its harmonic mips/loop reader and
prepares envelope tables with makeWindow. Mono reads/envelopes are shared across
stereo outputs; accumulation uses VectorOps. Fixed grain pools and scratch are
allocated only at preparation. Analyzer windows, FFT plans, filter states, kernels,
peak queues and histories follow the same lifecycle. Spectrum and CQT retain only
the latest completed frame, without creating background threads or UI snapshots.
Loudness uses existing double biquads and exact, bounded gating-energy arrays;
process returns would_block transactionally at capacity. Its integrated and LRA
queries are allocation-free history scans/sorts, not bounded callback work.
All access remains owner-serialized. See [the API guide](GranulationAndAnalysis.md).

## Voice-parallel IIR and transitions

BiquadBank uses one voice per planar channel. Coefficients are stored in five
contiguous per-voice fields and state in two fields. Up to 64 frames are packed
into private frame-major scratch for banks above two voices. Mono/stereo use
planar register histories to avoid packing overhead. SIMD runs across larger banks; each lane keeps its own
coefficients and recurrence history. Each vector group retains its coefficients
and state while processing a tile, then writes state back before the next tile.
SSE2, AVX2, AVX-512, NEON and SVE provide float/double implementations; ARM32 double falls back
to scalar. AVX2 processes 128-bit residual groups before individual scalar voices.
No time-axis dependency is approximated or removed.

The scalar voice kernel and original Biquad share coefficient rounding/stability
checks and the same sample recurrence. AudioLayout converters share the production
pack/scatter helpers, reject overlap before writing and also support external
frame-major buffers. Packing, validation and small voice counts add overhead;
which processor/backend is fastest must be measured on the intended workload.

SmoothedBiquad owns two prepared banks and two planar scratch buffers, each bounded
to 64 frames per channel. During a transition both banks process the original input
before output writes. Their output mix advances once per frame across all channels.
The destination bank starts with zero history. Its coefficients remain fixed for
the fade, so coefficient interpolation is not needed for this transition method.
After its final frame, that bank becomes active and alone handles subsequent samples.

At most one following target is retained. Requests during a fade replace that queued
target; the current fade finishes first. Returning to its destination cancels the
queue. A request for the already-active target can cancel a deferred next fade.
Repeated identical targets preserve progress/duration. Zero-duration edits apply
immediately and cancel fades/queue. reset snaps to the latest requested coefficients
and clears both histories. prepare retains that target and commits a fresh plan.
These operations are owner-serialized and allocate only during preparation.
Output crossfades can change phase/amplitude and introduce a new-bank transient;
no click-free or gain-limiting guarantee is implied.

## Channel mixing and staged convolution

ChannelMatrix owns an output-major matrix of current and target double gains,
rounded to the sample type at parameter acceptance. It snapshots all planar inputs
in tiles of at most 64 frames. A pairwise full-block alias validator accepts exact
cross-channel aliases and rejects partial overlaps before mutation. This allows
in-place permutations/downmixing without overwriting a later input within a tile
or a following tile. Output channels must remain disjoint. Constant matrix segments
reuse SIMD vector kernels; shared-frame scalar ramps handle parameter trajectories.
Zero gains omit their input contribution. No speaker-layout model is embedded.

SmoothedConvolver owns two hybrid plans with equal P and zero added block latency.
Its prepare/stageImpulse methods allocate outside the callback and commit only on
success. Its internal plan block limit and two output scratch buffers are bounded
to 64 frames; larger host blocks are split internally without altering partition
history. Staging does not advance the pending plan's history. startTransition only
updates scalar state and consumes the staged flag. By default, the destination
receives IR-length-minus-one live input frames while the old output remains audible,
then the fade begins. This bounds work per callback without a historical replay
burst; activation is deliberately deferred by that warmup. Both plans read the
original input before output writes, preserving exact in-place processing.

At completion the active index changes; no plan or buffer is destroyed in process.
The retired plan stays owned until the next serialized non-real-time staging or
re-preparation. Re-staging during a fade is rejected. There is no background-thread
handoff or queued IR update. reset clears histories, snaps an active fade to its
destination and retains an unstarted stage. The same private crossfade-block helper
now implements biquad and convolver frame progression and exact endpoint selection.
Passing prewarm=false explicitly requests a cold destination. Phase cancellation,
deliberately faded old tails and dual-bank CPU cost still require workload acceptance.

## Shared algorithms

Complex FFT stores bit reversal and stage-contiguous twiddles. A linear recurrence
prepares the reversal table. Processing dispatches once per full FFT stage; small
stages batch independent groups in SSE2/AVX2/AVX-512/NEON registers. This avoids
underfilled wide vectors and repeated group dispatch. SVE retains scalable kernels.
The original group dispatcher remains as a private constexpr/numerical/benchmark
reference with the same weights, ordering and normalization. Runtime tests also
compare every stage to independent scalar butterfly arithmetic on unaligned spans.
Real FFT packs even/odd samples into N/2 complex values, uses a half-size complex
plan and recombines with prepared twiddles. Inverse reconstruction recovers packed
even/odd samples with matching 1/N semantics. DC/Nyquist imaginary parts are canonical.
The N=1 case is handled directly.

Convolution reuses real FFT and complex multiply-accumulate kernels. Each channel
has independent input/history/overlap; one immutable IR spectrum is shared. A whole
partition's older-history products are scheduled across its input frames with a
bounded integer-debt recurrence. Only the current partition product and transforms
remain at the boundary, so callback cost still has FFT peaks. Added latency stays P. Direct FIR instead uses duplicated
ring history and vector dot kernels without added block buffering. HybridConvolver
uses a direct P-tap head and a partitioned tail starting at IR offset P. The tail
processor's existing P-frame buffering supplies exactly that offset, so the combined
output has no added block latency. It computes the tail before an in-place head
can overwrite caller input, using prepared planar scratch.

The resampler uses a bounded rational phase remainder, a normalized sinc phase table,
linear interpolation between adjacent table rows, and duplicated per-channel input
history. Coefficients and accumulations are double precision for both sample types.
Float history uses a mixed float/double dot: conversion happens before multiplication.
SSE2, AVX2, AVX-512 and ARM64 NEON/SVE implementations retain double multiplication/accumulation;
addition order varies by backend. Backend selection is explicit during prepare.
No absolute sample counter grows during long streams. This independent streaming
implementation does not inherit the complete upstream r8brain quality/performance.

Biquad validates pole stability after sample-type rounding. The original Biquad
recurrence remains scalar and direct coefficient edits are immediate. BiquadBank
adds explicit voice-parallel processing; SmoothedBiquad adds output transitions.
DelayLine can optionally ramp the read delay once per frame, using the same progression helper as Gain; immediate
delay edits remain available. Delay ramps change pitch and are not band-limited
modulation. IR transitions require explicit staging/activation; SmoothedBiquadCascade provides full multi-section EQ output transitions. No denormal policy, audio sanitization,
clipping or gain limiting is implied. Explicit FP operations avoid fast-math and
FMA contraction; bitwise identity across platforms or reduction backends is not
promised.

## Extended prepared processors

SmoothedBiquadCascade preallocates two full section banks and uses the same bounded
crossfade helper as the single-section and IR variants. Coefficient transactions
validate every section before committing. NonuniformConvolver splits the IR at its
larger partition size, aligning the late tier's natural buffering with that offset.
MatrixConvolver copies each route's IR but shares input histories and forward FFTs
per active input/tier. Route products accumulate per output, with one inverse FFT
per active output/tier. The direct head also shares its input ring across routes.
It snapshots tile inputs before output writes and distributes older-partition MACs.
SIMD dots use independent accumulators and balanced reductions. The smallest FFT
stages inline identical arithmetic; trusted internal kernels avoid repeating public
span checks after the processor has validated its blocks.

AsyncConvolver keeps three plan slots at stable addresses. A producer claims a free
slot with acquire ownership, prepares it, then publishes its index with release.
The audio consumer takes the pending index at a callback boundary and retains both
active/warming/fading slots until completion. Releasing a retired slot makes it reusable by
the worker after the audio thread's last access. Only the worker replaces/destroys
that plan. Publication replacement and full-queue behavior are bounded. Native
concurrent stress and allocation/free checks exercise this lifecycle; it remains a
single-producer/single-consumer contract, not a general multi-writer queue.

Resampler owns bounded prepared zero input for EOS draining and can rebuild its
coefficient table outside real time while retaining history/phase. Presets increase
support for downsampling; the raw default is 128 taps after measured 64-tap limits.
AudioGraph copies and sorts topology, assigns output buffers from their last use,
and processes bounded tiles. Borrowed callbacks provide processor composition;
arbitrary callbacks are not automatically fused or made thread-safe.


Variable-rate resampling prepares a conservative cutoff for maximumRatio (input
frames per output), then setRatio changes only bounded phase/ramp state on audio.
The fixed rational clock remains the default. A common prepared cutoff avoids
filter redesign/allocations during ramps, at the cost of reduced passband at the
higher output rates of that envelope. Reset snaps to the latest ratio target.

Graph preparation fuses uniquely consumed one-input null-callback chains. Their
ordered gains remain separate sample-precision operations in vector registers;
there is no algebraic gain reassociation. Four independent SIMD groups (or four
scalar samples in the ARM32 double fallback) share each gain load, exposing arithmetic
parallelism without extra buffer passes. Zero-gain mute and sum-with-zero semantics
are retained. Shared branches and arbitrary user callbacks remain explicit nodes.
compiledNodeCount exposes the resulting schedule; last-use analysis uses that plan.


## Progressive late convolution

ScheduledConvolver is an optional single-thread alternative to NonuniformConvolver.
For late partition P, a HybridConvolver covers IR offsets [0,2P). A captured P-frame
input block is processed over the following P input frames; its tail contribution
is first due at offset 2P. This supplies a full processing interval without delaying
the audible prefix or adding block latency. Short IRs omit the scheduled tail.

The tail divides packing/bit reversal, packed complex FFT butterflies, real-spectrum
split/merge, partition MACs, inverse butterflies and overlap-add into chunks of at
most 64 numerical items. A division-free integer debt schedule distributes exactly
the prepared job count over P frames. Two input and two ready buffers keep capture,
output consumption and future output writes separate; one FFT workspace is reused
serially across channels. Immutable plan/workspace pointers are cached during
preparation; unique ownership keeps their State and FFT storage stable across moves.
Paired sample emission avoids repeated phase-parity work. State transitions and
vector swaps allocate/free nothing.

maximumWorkUnitsPerFrame bounds scheduled tail jobs, excluding the synchronous
hybrid prefix. Chunks have different arithmetic/memory costs; this is not a CPU
cycle bound or an OS deadline guarantee. Extra prefix coverage and job scheduling
can increase average CPU while reducing sampled callback peaks. Both convolution
modes remain available and are compared on the actual target. The original-code
FFT machinery and pinned MIT overlap-add provenance are reused; no new dependency
or worker thread is introduced.

## Common convolution plans and retained IR engines

Convolver owns a variant of HybridConvolver, NonuniformConvolver and
ScheduledConvolver. Dispatch is once per process/reset/observer call; each engine
still validates its own complete audio blocks. Preparation constructs a candidate
of the selected type, prepares all resources, then commits by noexcept swap. Failed
preparation cannot change the previous history or configuration. No heap-allocated
polymorphic wrapper or runtime code generation is added.

SmoothedConvolver retains two Convolver banks; AsyncConvolver retains three.
Configuration is immutable during live updates, and all worker-prepared IRs use
that configuration and resolved backend. Variant replacement/destruction only
happens during non-real-time prepare/stage, under existing ownership. Audio adoption
changes indices, leaving retired resources retained until the producer reclaims
them. ScheduledConvolver's stable State keeps cached FFT plan pointers valid when
a variant/bank moves, including an unfinished incremental transform. Nonuniform
and progressive scheduling are now available during prewarming and fades; neither
is made the default. MatrixConvolver accepts the same configuration while retaining its shared FFTs.

## Progressive shared MIMO

The scheduled MatrixConvolver prefix covers [0,2P), using the shared direct head
and shared early tier. Its late tier retains an independent uniquely owned State.
All active inputs complete progressive pack/forward/split jobs before route MACs;
all routes contribute before progressive merge/inverse/emit for each active output.
Route partition counts are independent, while the shared history ring uses their
maximum. Every input has one forward transform per interval, and every output one
inverse. Disconnected channels need no late FFT and remain silent when unrouted.

For Q=64, let A=ceil(P/Q), B=ceil((P+1)/Q), F=log2(P)*ceil((P/2)/Q),
I/O be active late input/output counts and R the sum of their route partition counts.
The exact late job budget is (I+O)*(2*A+F)+R*B. P=1 omits butterfly jobs. The integer
debt scheduler distributes those jobs over P frames. Packing/real recombination,
butterflies, route MACs and emission have bounded per-job item counts, but different
CPU costs. Emission also clears consumed per-output spectral sums without a boundary
clear loop. Cached plan pointers belong to State storage and remain valid across
MatrixConvolver moves; processing never changes the State owner or resizes a plan.

The independent-channel and MIMO engines share the same incremental butterfly
helper. Filter construction reuses the original shared tier without allocating its
unused old-partition job list. The temporary filter time buffer is released during
prepare. Configured hybrid/nonuniform/scheduled modes retain their distinct cost
profiles and the previous MatrixConvolver overload still chooses nonuniform.

## Project boundary

CyDSP is the standalone numerical layer, namespace `cydsp`, with no dependency on
CyAudioCore, device discovery, file codecs, MIDI protocols or plugin SDKs. Its DSP
graph composes processors. It retains Wavetable, WavetableOscillator, SampleData,
prepared sample interpolation/mips and the optional Stretch algorithm.

CyAudioCore consumes the installed public API. It owns sources/transport,
MIDI/MPE/UMP/MIDI-CI, polyphonic instrument orchestration, devices, files and
plugin hosting/isolation. Applications choose their GUI independently.

The prepared aliases for AudioBlock/AudioBuffer/Status/ProcessSpec in CyAudioCore
are the same CyDSP types. No conversion layer or second SIMD dispatcher is added.
See [migration](Migration.fr.md) for the breaking package/header changes.

## Expanded numerical modules

The ten-lot extension remains in the static numerical target, with public headers
under control, spectral, spatial, synthesis, processors and analysis. It composes
existing FFT, vector, buffer, convolution, resampling and filter primitives.
Shared HistorySinc interpolation serves live grains and pitch effects; PolyBLEP
math is shared with the original oscillators. Mutable state is per instance.
Applications supply scene geometry, HRIR data and processor contexts. No reverse
dependency on CyAudioCore or consuming applications is introduced. See the
[extension contracts](ExpansionGuide.fr.md).

## Second advanced extension

Shared prepared HRTFs, order-7 HOA, SourceBatch, diffraction, IR analysis, HPSS,
spectral pitch/harmony, physical contacts, AEC and quality graph transitions are
documented in the [advanced API guide](AdvancedExpansionGuide.fr.md).

## Integration hotspot paths

Linked zero-lookahead compression/limiting shares one detector/curve only when
retained per-channel envelopes are equal. Linking previously independent histories
retains the general path. Above the soft knee, the equivalent linear power law
uses square roots for common ratios; the final dB observer is evaluated once per
block. minimumAppliedGain reports the minimum detector gain including makeup in
the last successful nonempty block, before limiter ceiling clipping. It allows
consumers to meter attenuation without dividing output by near-zero program input.

ADAA retains a lazy previous-primitive cache, invalidated on shape edits and after
midpoint/direct segments. The stored driven sample survives drive/bias/AA edits.
Wavefolder reduction uses bounded power-of-two floor arithmetic. Private AVX2 and
AVX-512 translation units supply a double-precision time-parallel kernel with safe
denominators, scalar tails and retained cross-tile input. Other backends use the
scalar cached path. No ISPC runtime/compiler dependency is required by production.

SmoothedBiquadCascade prepares scratch up to the caller's maximum block size and
splits at transition boundaries; BiquadBank still limits larger SIMD packs to 64
frames. MatrixConvolver snapshots a contiguous direct-head history per tile;
four independent FIR tap chains can be accumulated across time in private x64
SIMD kernels. This changes floating-point summation order within a route; route
addition order, scheduled work budgets, routing and zero latency are retained. The zero-capacity DryWetMixer delay copies planar dry samples directly.


The second hotspot pass batches tier input/output copies up to a partition boundary.
The number and order of completed jobs match the sample scheduler at the end of
any call, including single-frame calls. Pending jobs read previous input storage
and write next-ready storage; they never consume the current partial input block.

Linked equal-envelope dynamics can collect 64 detector values and evaluate their
curve through private AVX2/AVX-512 double arithmetic. Detector recurrence, sidechain
validation, lookahead behavior and final dB metering retain their existing order.
Scalar/other backends retain scalar processing. The input is not rescanned when
it is also the identical detector block; distinct sidechains remain validated.

Private audio log/exp helpers use bounded double range reduction and small
polynomials. Normal operating ranges are covered by independent numerical sweeps;
subnormal/domain and exp underflow/overflow boundaries use standard libm. Near-one
log evaluation avoids cancellation. No global fast-math, reciprocal approximation,
FMA contraction or FP-state mutation is enabled. EQ designs share their denominator
reciprocal and use the bounded gain conversion; stability checks remain mandatory.

Hard clipping and tanh ADAA have time-parallel x64 kernels alongside wavefolding.
The tanh integral retains standard libm secants for endpoints within 1% of the
sample scale, and the original midpoint threshold (1e-5) is unchanged. This bounds
amplification of primitive rounding when endpoints almost coincide. Scalar tails,
shape/drive/bias automation, in-place processing and the first active sample follow
the same contracts. Regressions include output gains up to 16 and clamp extremes.

Constant tanh input channels evaluate their first ADAA transition using retained
history, then fill the remaining midpoint output. No silence detector threshold or
state reset is introduced; the shortcut requires exact equality of input samples.

Measured results and precision limits of this pass: [detailed hotspot report](DetailedHotspotOptimization.fr.md).
