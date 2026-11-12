# Module contracts

All sample processors support float and double. prepare is non-real-time and
transactional: an error keeps the previous configuration usable. reset clears
signal history without freeing buffers. Methods return Status; numerical sample
values are not automatically clipped or sanitized. Stateful instances require
owner-side serialization of prepare, parameter edits, process and destruction.
AsyncConvolver explicitly allows one producer stageImpulse call concurrently with
one audio consumer; its prepare/reset/destruction still require both to be quiescent.

## Blocks and storage

ProcessSpec contains sampleRate, maxBlockFrames and channels. All must be positive;
sampleRate must be finite. Fixed-rate processors require equal input/output frames
within that limit and the prepared channel count (ChannelMatrix has separate input
and output counts). Corresponding-channel in-place
processing is supported; partial and hazardous cross-channel overlap is rejected.

AudioBlock<T> is a borrowed planar pointer table and frame count. subBlock(offset,
frames) borrows the same table without allocating; nested sub-blocks accumulate
offsets. Mutable-to-const views preserve that offset. channelData() is the original
table, whereas channel(index) applies the offset. Invalid sub-block bounds yield
an empty view. Caller allocations and the pointer table must remain alive and
large enough; validation cannot prove pointer lifetimes.

AudioBuffer<T>::resize allocates zeroed, per-channel 64-byte-aligned storage with
private padding. block() exposes only the requested frames. resize is not an audio
callback operation and invalidates borrowed views on success.

Ordered, internally disjoint input/output channel ranges use linear-time alias
validation. Arbitrary layouts retain the complete pairwise checks. This does not
change the public alias rules or require preparation of additional validation memory.

## Audio layout conversion

interleave(planar, flat) and deinterleave(flat, planar) support float/double without
allocation. The flat span has exactly frames * channels samples, stored as
flat[frame * channels + channel]. Planar sub-block offsets are honored. Default
empty views and zero-frame views with valid pointer tables are no-ops; a zero-channel
view with nonzero frames is invalid. Size arithmetic is checked before copying.

All planar/flat overlap is rejected, including exact aliases. deinterleave also
rejects overlapping destination channels. Read-only input channels may alias.
Errors leave destination data unchanged. The functions borrow all storage, including
the AudioBlock pointer table, and have no implicit format, rate or sample conversion.

## Channel routing and mixing

ChannelMatrix<T>::prepare(inputSpec, outputChannels, coefficients, backend) creates
an outputChannels by inputSpec.channels matrix. Coefficients are a flat double span
in output-major order: coefficients[out * inputChannels + in]. Every frame computes
y[out] = sum(coefficients[out,in] * x[in]). No channel-layout assumptions, automatic
gain normalization, clipping or speaker standard are applied. Stereo/mono downmix,
upmix, channel permutation and arbitrary finite linear mixes use the same API.

Coefficients must be finite and within the finite range of T, then round to T.
setMatrix(coefficients, rampFrames=0) changes the existing shape without allocation.
A zero duration applies immediately. Positive ramps start from current gains and
advance once per frame, first step on the first frame, across all coefficients.
Their internal states are double; multiplication/accumulation uses T. Differences
from current to requested rounded gains must remain finite in double. A new target
retargets from current values; repeated identical rounded targets preserve progress.
reset snaps to the target. Empty blocks do not advance gains. Errors preserve state.

Constant segments use selected SIMD add-scaled/fill kernels; ramp segments use a
scalar frame loop. Zero coefficients omit their source contribution, including a
non-finite sample on a muted input. Summation may still overflow for large signals.
The input scratch stores at most 64 frames per channel, copied before any output
writes in each tile. Exact full-block aliases may map any input channel to any
output channel, including permutations and rectangular in-place mixes. Partial
input/output overlap and all output/output overlap fail before writes. Read-only
input channels may alias each other. This matrix-specific validation is pairwise.
inputChannels(), outputChannels(), backend() and rampRemainingFrames() expose the
prepared shape/backend and pending ramp; moves unprepare the source.

## VectorOps

Create a table with VectorOps::create(CpuBackend, result) during preparation.
Automatic chooses a compiled supported backend; forced unavailable choices return
unsupported_backend. A default VectorOps uses scalar. backend() reports selection.

| Operation | Alias and shape rule |
| --- | --- |
| copy, scale, addScaled | Equal lengths; exact source/destination alias or disjoint |
| multiply | Equal lengths; output may exactly alias either input |
| complexMultiplyAccumulate | d += a*b; equal complex lengths, exact output aliases allowed |
| complexButterfly | e = oldE + oldO*w, o = oldE - oldO*w; all arrays disjoint; optional conjugated w |
| dot | Equal lengths; float/float, double/double or float/double; empty result is zero |
| fill | Fills the destination; any value is accepted |

Partial source/destination overlap is an error for the checked vector operations.
Inputs which are both read-only may alias each other. Scale/addScaled gains must
be finite. Error leaves result/output unchanged. SIMD dot reduction has a different
addition order from scalar, so low-bit rounding differences are expected. Unaligned
spans and lengths below/not divisible by vector widths are supported. Mixed dots
convert float samples to double before multiplication with double coefficients,
then accumulate in double. ARM32 NEON uses the scalar mixed implementation.

## Gain and IIR

A new Gain<T> starts at 1; prepare/reset snaps to its current target.
setGain(linear, rampFrames) accepts finite values,
including negative gain. A zero ramp applies immediately. Ramps advance once per
frame, starting with one step on the first processed frame, and use the same gain
across channels. reset snaps to the target. Constant segments use vector kernels;
ramps currently use a scalar frame loop with a double internal gain state.

Biquad<T> uses independent transposed direct-form-II channel state. Coefficients
normalize a0 to 1 and subtract a1/a2 feedback. They must remain finite and strictly
stable after rounding to the sample type. Design functions leave the supplied
result unchanged on invalid input. Low/high/band-pass, notch and all-pass use
positive Q and a frequency strictly inside (0, Nyquist). Band-pass has constant
peak gain. Peaking/shelving gain is [-120,120] dB; shelf slope is (0,1].

BiquadCascade<T> has 0..64 sections, prepared transactionally. Empty means a checked
bypass. setSection changes an existing index only and preserves its signal state.
IIR processing is currently scalar. Coefficient edits are immediate and unsmoothed.

## Voice filter banks

BiquadBank<T>::prepare(spec, backend) creates spec.channels independent voices,
initially identity filters with zero history. It accepts the same validated planar
blocks, block limits and in-place rules as Biquad. Coefficients are finite and
strictly stable after rounding to T, using the same check as Biquad.

setCoefficients(voice, coefficients) edits one existing voice; setAllCoefficients
edits all voices after a single successful validation. Both preserve signal state
and apply immediately. reset clears state while retaining coefficients. Failed
preparation/edits preserve the previous configuration and state. Moves unprepare
the source. voices() and backend() report the prepared voice count and kernel table.

SIMD runs across independent voices, with scalar residuals; AVX2 also handles
128-bit residual groups. ARM32 double falls back to scalar. At most 64 frames per
voice are packed internally per tile, keeping scratch independent of large host
block limits. Any valid host split preserves recurrence order. The copy/packing
cost means a speed advantage is not implied for small channel counts.

## Filter transitions

SmoothedBiquad<T> applies the same coefficient target to every channel. prepare(spec,
backend) prepares two banks and bounded scratch; a fresh instance starts at identity.
Re-preparation retains the latest target and resets histories/transitions. It has
the standard fixed-rate shape, alias, error and move contracts.

setCoefficients(coefficients, transitionFrames=64) validates/rounds the target, then
crossfades old/new filter outputs over the requested frames. The first processed
frame advances one step; the last uses only the destination output. The destination
starts from zero filter history and warms during the fade. Both filters retain
fixed stable coefficients. Frame progression is shared by channels and independent
of callback splits; empty blocks do not advance or activate a queued transition.

A running fade finishes before a new positive-duration request starts. One queued
target/duration is retained, with the latest different target replacing it.
Repeated identical targets do not restart a fade or alter its duration. Returning
to the current destination cancels the queue; returning to an already-active target
can cancel a next transition that has not begun. A queue at an exact callback
boundary starts on the next nonempty block.

A zero-duration request applies immediately to the active bank, preserves that
bank's signal state, and cancels the fade and queue. reset snaps both banks to the
latest requested target, clears histories and cancels pending transitions.
targetCoefficients reports the latest request, including a queued target.
transitionRemainingFrames reports only the current fade; hasQueuedTransition and
isTransitioning cover the pending next target as well.

All parameter commands require owner-side serialization with processing. No memory
allocation, freeing or locking occurs during edits, reset or process. Fixed bank
coefficients avoid an interpolated pole path, but output phase cancellation,
amplitude changes and a new-bank transient remain possible. This is not a measured
click-free guarantee or a transition for an entire multi-section cascade.

## Full EQ transitions

SmoothedBiquadCascade<T>::prepare(spec, maxSections=16, backend) prepares two banks
of up to 64 sections and starts as a bypass. Each section uses BiquadBank with
independent channel history. Preparation allocates all section state and bounded
64-frame workspaces, so later coefficient commands do not allocate or resize.

setCoefficients(span, transitionFrames=128) accepts zero through maxSections stable
sections, validating and rounding the ENTIRE target before mutation. Empty means
bypass. It fades outputs of the old and new full cascades, with fixed coefficients
and zero destination history. One subsequent target/duration is coalesced during
an active fade; returning to the destination cancels it. Identical targets retain
progress. A queued target starts on the next nonempty frame after completion.

Zero duration installs the entire target immediately and clears its histories.
reset snaps to the latest requested cascade, clears histories and cancels fades.
prepare starts a fresh bypass rather than retaining the old target. Parameter
commands and processing are serialized by the owner. maxSections, targetSections,
backend, isTransitioning and transitionRemainingFrames expose current metadata.
Output phase/amplitude changes and cold-bank transients remain possible.

## Direct FIR

Fir<T>::prepare(spec, coefficients, backend) accepts 1..65536 finite coefficients.
h[0] multiplies the current input; h[k] multiplies the sample k frames earlier.
Coefficients are shared across independent channel histories, initially zero.
The implementation uses duplicated rings and the selected vector dot kernel.

There is no additional block buffering. Filter group delay depends on coefficients;
no universal zero signal delay is implied. Blocks may be split arbitrarily within
prepared limits. reset retains coefficients. Use partitioned convolution for long
IRs only after measuring the intended workload; no fixed crossover is promised.

## Fourier transforms

ComplexFft<T>::prepare(N, backend) accepts powers of two from 1 through 2^24.
Forward sign is negative and unscaled; inverse sign is positive and divides by N.
process requires exactly N complex inputs and outputs. Exact in-place and disjoint
buffers work; partial overlap fails. The plan is move-only. Its const process uses
caller buffers and immutable tables; reset is a no-op.

RealFft<T> has the same size range and normalization. It maps N real samples to
N/2+1 complex bins in increasing frequency order. N=1 has one DC bin. Forward sets
DC and (when present) Nyquist imaginary parts to zero. Inverse requires these
imaginary parts to equal zero. For N>=2, even/odd samples are packed into an N/2
complex transform, with prepared spectral recombination twiddles and matching inverse.
Input/output must be disjoint. Mutable workspace means simultaneous calls on one
instance are unsupported. N=1 copies the single real/DC value directly.
reset is a no-op. The smaller transform size is not a measured speed guarantee.

## Partitioned convolution

PartitionedConvolver<T>::prepare(spec, impulse, P, backend) accepts a nonempty finite
IR and power-of-two P <= 2^23. One IR is applied independently to every prepared
channel. Uniform partitions use transforms of size 2P. No quiet IR samples are
trimmed. Preparation may be expensive; it is outside the callback.

latencyFrames() is exactly P. tailFrames() is P + IR length - 1, the additional
zero-input frames needed after the source ends to expose its finite causal tail.
The first P output frames after prepare/reset are zero. Host blocks can have any
length up to maxBlockFrames; they need not divide or match P. Exact in-place is
supported. reset clears history/overlap and preserves the IR and plan.

Products against older input partitions are distributed across input frames using
an allocation-free, division-free debt scheduler. The current partition product
and forward/inverse FFTs still run at the boundary, so periodic FFT peaks remain. This version has no nonuniform partitioning, per-channel/matrix IR or automatic
IR crossfade within this class. SmoothedConvolver adds explicit hybrid-plan transitions.
A dry path can use DelayLine with P frames to align its buffering
latency; see examples/Processing.cpp.

## Hybrid convolution

HybridConvolver<T>::prepare(spec, impulse, P, backend) accepts a finite nonempty IR
and power-of-two P <= 65536. A direct FIR handles its first min(P, IR length) taps;
when needed, the FFT tail starts at IR offset P and uses P-frame partitions.
The tail processor's P-frame buffering matches that offset without delaying the
head, giving latencyFrames() = 0 and tailFrames() = IR length - 1.

This means no added block buffering; leading zeros or phase/group delay inside the
IR still affect the signal. If the IR fits in the head, only direct FIR runs.
One IR is shared across independent channel states. Arbitrary prepared host block
sizes, exact in-place processing, transactional preparation, reset and safe moves
follow the common contract. Tail output uses preallocated planar scratch so its
input is read before an in-place FIR head overwrites it.

Larger P increases direct-head work while changing FFT-tail cost; choose it from
measurements on the intended machine. Tail computations still have periodic work
peaks. Use SmoothedConvolver for IR transitions, NonuniformConvolver for two-tier
partitioning, and MatrixConvolver for per-route IRs.
See ImmediateConvolution in examples/Processing.cpp.

## Impulse transitions

SmoothedConvolver<T>::prepare(spec, impulse, P=128, backend) prepares one hybrid
convolver and two bounded scratch buffers. P follows HybridConvolver's power-of-two
limit of 65536. The class shares the fixed-rate channel/alias rules and reports zero
added block latency. Each IR is mono and applies independently to all channels.

stageImpulse(impulse) prepares a second hybrid plan using the same spec, P and
selected backend. This is a NON-real-time operation: it may allocate/free memory and
compute FFT plans. It must be serialized with process by the owner; it is not a
concurrent background-update API. A successful stage replaces an earlier unstarted
stage without changing the active convolver's history. Failure preserves both plans
and the staged flag. The caller can release its IR samples after success.

startTransition(frames=256, prewarm=true) consumes the stage without planning,
allocation or freeing. By default the destination receives IR-length-minus-one
live input frames before the fade; the old output remains audible during this
warmup. No past stream snapshot or catch-up burst is required. The destination
then has every finite-IR history sample needed for its first audible frame.
This intentionally delays activation by the warmup duration and runs both plans.
Pass prewarm=false to start with zero history on the next nonempty frame.

The linear output fade advances once per frame across channels: the first fade
frame advances one step, the last uses the destination exactly. Zero fade duration
switches after warmup (immediately in cold mode). Arbitrary callback splits do not
change warmup/fade progression. warmupRemainingFrames reports the hidden phase;
transitionRemainingFrames reports only fade frames. isTransitioning covers both.

Staging or starting another transition during warmup or a fade returns transition_in_progress
and preserves the fade. There is no IR request queue. startTransition without a stage
returns invalid_argument. discardStagedImpulse clears an unstarted request without
freeing its plan. Neither processing, reset nor activation destroys the retired plan;
its memory is retained until later non-real-time staging/re-preparation/destruction.
The double-convolver processing and retained plans increase CPU/memory during fades.

reset clears both histories. A running warmup/fade snaps to its destination; an unstarted
stage stays staged. Empty blocks advance neither phase. Moves transfer stages/warmup/fades
and unprepare the source. hasStagedImpulse(), isTransitioning() and
transitionRemainingFrames() report their respective states. tailFrames() is the
maximum of both IR tails during warmup/fade and the active IR tail otherwise; an unstarted
stage does not change it. Feed zeros after the source to obtain the active tail.
The old tail is deliberately faded out and is no longer emitted after the transition.

Output fades can change amplitude/phase; cold mode can create a startup transient. They do
not imply artifact-free switching, bounded gain, nonuniform partitioning, per-channel
IRs or parallel IR preparation. See ChangingImpulse in examples/Processing.cpp.

## Nonuniform and matrix convolution

NonuniformConvolver<T>::prepare(spec, impulse, earlyPartition=128,
latePartition=1024, backend) uses a hybrid early tier covering IR offsets
[0, latePartition), plus a larger uniform FFT tier starting at latePartition.
Its late-tier buffering equals that offset, so latencyFrames is zero. The early
size is a power of two <=65536; late is a power of two in [early,2^23]. Short IRs
omit the late tier. tailFrames is IR length minus one. Both tiers run synchronously,
so larger late partitions reduce average work but can create larger callback peaks.
Common alias, transactional prepare, reset and safe-move contracts apply.

MatrixConvolver<T>::prepare(inputSpec, outputChannels, routes, early, late, backend)
accepts sparse ConvolutionRoute<T> records {inputChannel, outputChannel, impulse}.
Preparation copies route IRs. The direct head shares input rings, each FFT tier
shares forward spectra/history per active input and combines route products into
one inverse FFT per active output. The legacy nonuniform mode distributes older
products over input frames and retains boundary transforms. Duplicate routes add; missing routes are silent; empty routing is a
valid silence processor. Different IR lengths are allowed. Each route has zero
added block latency and tailFrames reports the longest route tail.

All input channels are snapshotted in tiles of at most 64 frames before output
writes. Exact whole-channel cross aliases, including rectangular in-place routing,
are supported; partial input/output and overlapping output channels fail before
mutation. Route inputs/outputs must be in range and IRs finite/nonempty. Large MIMO
matrices still retain spectra for each route IR; input histories and transforms
are shared, including sparse, duplicate and unequal-length routes.

The config overload `prepare(inputSpec, outputChannels, routes, config, backend)`
accepts the same ConvolutionConfig as the independent-channel convolvers. Hybrid
uses the direct head and one shared tier of earlyPartition; latePartition is ignored.
Nonuniform retains the legacy two tiers. Scheduled extends the shared early tier
to IR offset 2*latePartition, then computes the late tier progressively across the
following input interval. All modes preserve zero added block latency.

Scheduled MIMO shares capture/history/forward FFT per active late input and one
inverse FFT per active late output; route products alone remain per-route. Unequal
route lengths do not add silent trailing partitions. It does not instantiate one
convolver per route. Spectral sums are cleared as outputs are emitted. The shared
packed FFT workspace is reused after all forward transforms have finished, and
ready/capture double buffers retain unread data. Partial jobs survive callback
splits and moves. State storage remains stable under unique ownership.

configuration reports the successful choice. schedulingActive is false for other
modes and for scheduled IRs with no samples beyond 2*latePartition (including empty
routing). workUnitsPerPartition and maximumWorkUnitsPerFrame count late-tier chunks
only; they exclude the direct/early tiers and do not guarantee CPU duration. There
is no new thread, block buffering latency or callback allocation/free. Failed
preparation retains the previous configuration and all signal/scheduler state.
Successful preparation clears history; reset clears signal history and job cursors
without discarding the configuration. Existing route and alias rules still apply.

## Convolution with progressive FFT work

ScheduledConvolver<T>::prepare(spec, impulse, earlyPartition=128,
latePartition=1024, backend) prepares a zero-added-block-latency convolver. The early
size is a power of two <=65536; late is a power of two in [early,2^23]. Impulses must
be finite and nonempty. Each channel has its own history for the same mono IR.
Preparation is non-real-time and transactional; failed requests retain the old state.

For late size P, the prefix covers [0,2P) with HybridConvolver. Later coefficients
use P-frame input captures; their FFT, real recombination, spectrum MACs, inverse
FFT and overlap-add run as bounded jobs over the following P frames. Their natural
2P-frame tail offset matches the IR split, so latencyFrames remains zero and
h[0] still multiplies the current sample. IRs of length <=2P use only the prefix.
schedulingActive reports whether the progressive tail exists; tailFrames is always
IR length minus one. Feed that many zeros after source input to expose the tail.

process accepts arbitrary host blocks within spec, exact corresponding-channel
in-place views and disjoint buffers. Invalid shape/alias requests fail before
mutation. Callback splits do not change scheduling or the result. reset clears all
prefix, capture, FFT/history, overlap and pending output state without allocation.
Moves transfer even a partially computed FFT and leave the source unprepared.
prepare/destruction/moves/commands and process require owner-side serialization.
No worker thread, locks, plan destruction or memory allocation occurs in process/reset.

workUnitsPerPartition reports scheduled tail jobs across all channels.
maximumWorkUnitsPerFrame reports ceil(workUnitsPerPartition/P); both are zero when
the tail is absent. A job handles at most 64 numerical items and its cost depends
on phase/backend. These counts exclude the synchronous hybrid prefix and do not
promise a fixed CPU duration. Scheduling/longer-prefix work can raise mean CPU;
choose ScheduledConvolver or NonuniformConvolver from mean AND tail measurements.
The asynchronous/staged IR processors retain their existing HybridConvolver plans;
this separate processor does not silently alter their transition contracts.

## Concurrent IR preparation

AsyncConvolver<T> provides a bounded single-producer/single-consumer alternative
to serialized SmoothedConvolver. prepare(spec, impulse, P=128, backend) and all
reset/destruction require both producer and consumer to be quiescent. Instances
are non-copyable and non-movable, keeping their atomic publication state at a
stable address. The class creates no thread; the host owns its worker scheduling.

One worker may call stageImpulse(impulse, transitionFrames=256, prewarm=true) concurrently with
one audio thread's process calls. Preparation and destruction of retired plans
occur on that worker. Three retained slots use lock-free atomic publication and
ownership release/acquire. A newer unconsumed publication can replace an older
one if a free slot exists. When every slot is occupied, update_queue_full preserves
the current fade and pending IR; retry from the producer after consumer progress.
Failure preserves the existing publication. The supplied IR can be released after
stageImpulse returns. Multiple concurrent producers/consumers are unsupported.

The audio consumer accepts at most one publication at the start of a nonempty,
valid host callback while idle. By default it keeps the old output audible while
the destination accumulates IR-length-minus-one live input frames, then starts the
fade and eventually releases the old slot for worker reuse without freeing it.
Zero fade duration still honors warmup; prewarm=false requests cold activation.
Publications during warmup/fade wait until a following callback. Pending plans
receive no pre-publication input and require no unsafe concurrent history snapshot. process
has no locks, allocations, plan destruction or worker wait. hasPendingImpulse is
an atomic observer; isTransitioning, warmupRemainingFrames and tailFrames are consumer-side observers.
reset snaps a running fade to its destination and retains an unconsumed publication.

## Prepared audio graph

AudioGraph<T>::prepare(spec, nodes, outputs, backend) copies a fixed-rate DAG of
GraphNode<T> records and GraphInput edges. Node identifiers are their indices in
the supplied array. graphExternalInput identifies the host input. Each edge carries
a finite gain rounded to T; a node consumes the sum of its edges and outputs the
prepared planar channel count. A null node callback means identity. Outputs are
another weighted edge list. The graph validates references/cycles and topologically
sorts up to 4096 nodes, so source definitions need not precede their consumers.

Node callbacks are noexcept function pointers with borrowed context pointers;
contexts and their prepared processors must outlive graph use. Callbacks must obey
the library's real-time contract and accept blocks up to min(maxBlockFrames,64).
They receive disjoint private input/output buffers. Branch sums use selected SIMD
kernels. Last-use analysis reuses output slots after all consumers have read them;
inputs are summed before a reused output slot is overwritten. workBufferCount
reports retained output slots; a simple chain can use one slot. Two additional
bounded buffers hold the external input snapshot and each node's summed input.
Uniquely consumed one-input null-callback chains fuse into one SIMD gain-chain
kernel. Gains remain ordered separate T-precision operations, preserving rounding,
sum-with-zero and zero-gain NaN muting; gains are not multiplied together.
compiledNodeCount reports the resulting schedule. Shared branches and arbitrary
callbacks remain explicit nodes and callbacks are never removed as dead code.

Exact corresponding-channel host in-place processing is supported. Invalid host
blocks fail before any callback. A callback error is propagated; already-processed
nodes/tiles are not rolled back, and previously completed output tiles may remain.
reset clears graph scratch without invoking or resetting borrowed processors.
The owner resets those processors separately. An empty graph with empty outputs
emits silence; an external-input output edge provides a bypass.

## Streaming resampling

ResampleSpec specifies positive uint32_t inputRate/outputRate, channels,
maxInputFrames and maxOutputFrames. taps must be even in [8,512]; phases must be
a power of two in [16,4096]. Defaults are 128 taps and 1024 phases. Fixed rates are
rational. resampleSpecForQuality(spec, quality) selects economy (64/256), balanced
(128/1024) or high (256/4096) taps/phases and increases support for large downsampling
ratios within the 512-tap bound. Invalid quality enums produce an invalid spec.
These are configuration presets; measured frequency grids are finite acceptance
samples and do not establish every possible rate/frequency response.
prepare(spec, backend) selects scalar/SSE2/AVX2/AVX-512/NEON/SVE kernels during preparation;
backend() reports selection, and unsupported requests preserve the old plan.

Coefficients use Blackman-windowed sinc with cutoff 0.94 * min(1, output/input),
DC-normalized phase rows and linear interpolation. Tables, weights and accumulation
are double precision. Double samples use double vector dots; float samples use
mixed dots with double multiplication and accumulation. ARM32 double/mixed work
falls back to scalar. Backend rounding order may differ. Tap/phase counts
are configuration bounds; measured configurations are recorded in Measurements.md.
Unmeasured settings are not a passband/alias-rejection quality guarantee;
particularly large downsampling ratios need independent quality assessment.
latencyInputFrames() reports the nominal (taps-1)/2 input-frame filter delay.

process accepts different input and output frame counts, within their independent
limits, and requires disjoint input/output storage. Honor ResampleResult:

1. Check status before consuming any output.
2. Advance the input by inputConsumed and retain all remaining input.
3. Use only the first outputProduced frames of the output block.
4. Supply further input or capacity as needed; zero output capacity causes no progress.

A zero-input call can emit already-pending upsampled frames. When both counts are
zero, supply more input or signal EOS with finish(output). finish rejects invalid
output before state changes, then drains up to taps internally supplied zero input
frames and any already-pending upsampled outputs. It reports inputConsumed=0.
Repeat with output capacity until isFinished; later finish calls produce zero.
An empty stream finishes without output. Zero-capacity finish is a no-op. Once
finishing begins, process, setRates and setRatio return end_of_stream until reset/prepare.
The retained zero buffer is prepared in advance; finish allocates/frees nothing.

setRates(inputRate, outputRate) is serialized NON-real-time preparation. It rebuilds
the table transactionally, preserving FIR history, ring position, remaining input
consumption before the scheduled next output, and its fractional phase rescaled to
the new denominator. That already scheduled output retains its timing; subsequent
output steps use the new ratio. This operation remains non-real-time.

For real-time changes, append minimumRatio/maximumRatio to ResampleSpec (input
frames per output frame). Both zero retain the rational clock; otherwise the
finite ordered envelope must be within [1/65536,65536] and include inputRate/outputRate.
prepare uses cutoff 0.94*min(1,1/maximumRatio) for the entire envelope. Quality presets
increase tap support for its worst downsampling ratio. A fixed conservative cutoff
reduces passband at the envelope's higher output rates; it does not redesign a
filter on audio. Choose the envelope and quality for the intended signal.

setRatio(inputPerOutput, outputRampFrames=0) is allocation-free and serialized with
process. It rejects values outside that prepared envelope without state changes.
Zero duration changes subsequent spacing immediately, retaining the scheduled next
output. A positive duration linearly changes spacing over produced output frames;
the first produced frame advances the ratio one step for scheduling its successor,
and the last sets the exact target. New commands retarget from the current ratio.
ratio and ratioTransitionRemainingFrames report progress. Fractional phase remains
bounded; every channel shares one clock. Honor consumed/produced counts as usual.

reset clears history, phase, EOS and a running ratio ramp, snapping to the latest
ratio target. prepare starts from the spec's nominal ratio. Callback splits preserve
the same progression; variable mode uses bounded double phase while the default
fixed-rate mode retains exact rational integer state.

## Common convolution engine selection

`ConvolutionConfig{mode, earlyPartition, latePartition}` selects `hybrid`,
`nonuniform` or `scheduled`. `Convolver<T>::prepare(spec, impulse, config, backend)`
is a common interface over the existing engines; default configuration is hybrid
128/1024 (hybrid ignores latePartition). All modes apply the same IR independently
to each channel and have zero added block latency. Tail length is IR length minus
one. Existing engine-specific partition limits still apply. Unknown mode values
are rejected. The requested backend is validated by the selected engine.

`configuration()` returns the successfully prepared configuration. A failed prepare
retains the complete previous engine, configuration and signal history, including
partially completed progressive FFT work. Successful prepare replaces history.
Move transfers the engine and its history; the source becomes unprepared.
`process` validates blocks before changing history. `process` and `reset` allocate
and free nothing. The selection remains fixed between preparations.

`SmoothedConvolver` and `AsyncConvolver` accept the same config prepare overload.
Their existing partition-only overloads still select hybrid. Every staged IR uses
the original choice and resolved backend. Changing engines requires preparation
under the processor's existing synchronization contract. Prewarming, cold mode,
zero-duration activation and fades retain their previous semantics for all modes.
SmoothedConvolver rejects staging during a running warmup/fade and preserves an
unstarted stage if replacement fails. AsyncConvolver retains its single producer,
single consumer, three-slot publication contract. A failed worker preparation
releases its claimed slot and preserves the earlier pending IR. The audio thread
never replaces or destroys a plan or switches a variant; it only adopts its index.

## Delay and windows

DelayLine<T>::prepare(spec, maximumDelayFrames) accepts a maximum from 0 through
2^24. setDelay(frames, rampFrames=0) accepts a finite value in [0, maximum].
Integer delays read the exact history sample. Fractional delays interpolate adjacent samples;
this is not a band-limited fractional-delay design and affects high frequencies.
A zero ramp applies immediately. A nonzero ramp advances the delay once per frame,
starting one step on the first processed frame and using one position across channels.
delayFrames() reports the current position; targetDelayFrames() reports its target.
reset clears history, snaps to the target and cancels the ramp. Empty blocks do not
advance it. Ramps cause pitch/Doppler changes and are not a band-limited modulation
method. Channels remain independent.

makeWindow supports rectangular, Hann, Hamming and Blackman windows. Symmetric uses
N-1 in the angular denominator, periodic uses N. A singleton is 1; an empty span or
invalid enum is an error without writes. Window generation evaluates trigonometric
functions and should be done during preparation. No amplitude/energy normalization
beyond the named window formula is implicit.

## Expanded DSP

`DspMath.h`: dB conversion, polynomial evaluation, immutable lookup interpolation,
linear/multiplicative `SmoothedValue`. `Adsr` has linear segments with exact sample
endpoints; zero-duration stages advance immediately. `Oscillator<T>` offers sine,
triangle, PolyBLEP saw/square and deterministic white noise. Triangle is not
band-limited. No generator promises alias-free output at arbitrary frequencies.

`Dynamics<T>` offers compressor, downward expander/gate and hard-ceiling limiter,
peak/RMS detection, channel linking, knee and makeup. The gate uses a downward
expansion curve; there is no hold/hysteresis or look-ahead. `gainReductionDb()` is the
latest processed channel/frame value, not a block minimum. `BallisticsFilter<T>`
has independent attack/release envelope state per channel.

`TptFilter<T>` implements first-order and state-variable responses; `LinkwitzRiley<T>`
returns separate LR4 low/high blocks and rejects overlapping outputs. `LadderFilter`
is a four-pole low-pass with solved linear feedback and input saturation; it is not
a circuit emulation. `designButterworth` designs 1..32-order low/high-pass SOS;
`designWindowedFir` designs odd symmetric Kaiser-windowed low/high-pass taps.

`Panner<T>` supports mono/stereo to stereo, seven pan laws and matrix ramps.
`DryWetMixer<T>` retains the dry block and integer latency history. Each push must
be paired with a same-sized wet mix before the next push. `WaveShaper<T>` offers
built-in functions or a borrowed noexcept callback. `Bias<T>` adds a constant.

`Oversampler<T>` offers 1/2/4/8/16 factors using prepared FIR filters. `processUp`
returns a borrowed internal high-rate block for the user's nonlinear processing;
`processDown` completes the matching block. Latency is 32 input frames above 1x,
zero at 1x. Reset cancels an unfinished block. This implementation is not a
half-band IIR oversampler. It adds filter latency even around identity processing.

`Chorus<T>` uses a modulated interpolated delay; `Phaser<T>` uses six all-pass
stages; `Reverb<T>` uses an original decorrelated Schroeder network. Parameters
change between blocks without implicit smoothing. Reset clears prepared state and
retains storage. Reverb decaySeconds is nominal low-frequency RT60, affected by
damping/frequency and not a promise of a particular room response.

## Pitch and time

Link `CyDSP::Stretch` for `TimeStretch<T>`. The adapted MIT engine uses CyDSP
ComplexFft/VectorOps and private STFT/half-bin reconstruction. FFT sizes are powers
of two or three times a power of two; inverse scaling is folded into STFT windows.
The optional trailing prepare backend and backend() follow common CPU selection. Prepare independent maximum input/output
sizes. The lengths supplied to each process call define the local duration ratio;
the caller schedules integer lengths to reach the desired long-term ratio. Pitch
is -48..48 semitones; formant multiplier .25..4. All input/output overlap is rejected.
Public block offsets are respected. Input/output latency is reported separately.
Feed inputLatency silence frames at the desired ratio before flushing a finite
stream, then drain outputLatency frames. Flush caches/drains the tail across short
callbacks, zero-pads the last block and returns end_of_stream for a short/empty tail.
Process after flushing returns end_of_stream until reset. Tail generation happens
on the first flush, so budget that call for the complete tail computation.

## Audio files and devices

Link `CyDSP::Files` for the seekable, synchronous file/stream reader and writer.
Open is transactional for object state; the path writer creates/truncates its file.
Readers validate block/channel shape, report exact framesRead and zero-pad EOS.
An I/O failure may leave partial output/file data; retry policy belongs to the host.
Maximum block size is configured at open. Float and double planar blocks, including
nonzero sub-block offsets, are supported. Integer PCM normalization uses 2^(bits-1).
The WAV/FLAC backend limits negative export to within one LSB of -1. Nonfinite
export samples are rejected before writing. File methods are not real-time safe.

| Format | Read | Write | Restrictions |
| --- | --- | --- | --- |
| WAV | Yes | Yes | Backend-supported integer PCM and float32/64 |
| AIFF | Yes | Yes | Original PCM8/16/24/32 codec, signed big-endian, 2 GiB limit; no AIFC/compression/metadata |
| FLAC | Yes | Yes | PCM16/24 export; actual codec limits apply, at most 8 channels |
| Ogg Vorbis | Yes | Yes | Lossy floating input/output; quality level separate from encoding |
| MP3 | Yes | No | Compressed source has no PCM bit-depth; exposed decoded encoding is float32 |

Metadata JSON follows the selected codec's schema. It is not a universal lossless
metadata roundtrip. AIFF rejects metadata on export. `audioFileFormats()` reports
compiled format availability and writable encodings; it does not imply every
rate/channel combination is supported.

Link `CyDSP::Devices` for `AudioDeviceContext`, `AudioDevice`, `MidiInput` and
`MidiOutput`. Device IDs belong to their prepared context, MIDI port indices are
snapshots. Enumerate again after hotplug. Audio selects the system default for empty
IDs and negotiates the requested float stream through miniaudio. `sampleRate()`
reports the actual callback rate. Callbacks receive cleared planar output, are
split to maxCallbackFrames, and errors yield silence plus a counter. Lifecycle is
externally serialized; callbacks and user context must outlive stop/close.
Virtual-device mode is explicit and useful for silent deterministic validation.

MIDI input copies packets <=4096 bytes into a configured bounded SPSC queue. Queue
full/oversized messages increment droppedMessages. Read with too small a destination
reports the required size without consuming the packet. Timestamps accumulate backend
deltas from the first event; they are not audio-sample timestamps. Physical sends
can block and must run on a worker thread. The legacy RtMidi API is unavailable on
Android; native UMP instead uses Midi2Device's explicit API33 host JNI bridge.

## MIDI protocol

`MidiMessageView` validates MIDI1 wire messages. `ShortMidiMessage` factories use
1-based channels and 7-bit note/controller values; invalid arguments yield an empty
message. `MidiBuffer` has prepared event/byte capacities, stable sample ordering
and borrowed event views until reset/reprepare. `MidiStreamParser` handles fragmented
messages, running status and realtime messages inside SysEx; overflow discards the
oversized SysEx until its terminator, with an explicit error. Callback spans are
borrowed only for the callback duration. `MidiKeyboardState` tracks notes/sustain.

`MidiFile` reads/writes SMF0/1/2, bounded VLQs, running status, meta and SysEx events,
PPQN tempo maps and SMPTE timing (including 29.97). Parsing is transactional and
requires an End Of Track marker. Serialization appends one when absent. File parse
and serialization allocate; they are not callback services. `SpscQueue<T>` is one producer/one consumer, fixed capacity,
lock-free after prepare; lifecycle requires both participants idle.

## MPE and MIDI 2

`MidiParameterParser` decodes RPN/NRPN selection/data and bounded increments.
`MpeInstrument` keeps a bounded 16-channel/128-key state, lower/upper zones, member
and manager pitch/pressure, CC74, sustain/sostenuto, RPN0 sensitivity and RPN6 zone
configuration. Defaults are lower 15 members, upper 0; member bend 48 semitones,
manager 2. Explicit setZones requires idle notes; live RPN6 stops old notes and
resets expression. Callbacks carry borrowed note state and the supplied frame.

`UmpPacket` stores 1..4 host-order words and serializes big-endian bytes.
`UmpStreamParser` accepts fragmented words/bytes; use one input representation
until its incomplete packet is finished. `UmpBuffer` is prepared and sample sorted.
Factories cover MIDI1 encapsulation, MIDI2 notes/attributes, CC/pitch/pressure,
per-note controllers, program/bank and SysEx7/8. Scaling preserves min/center/max;
conversion returns unsupported_format for unrepresentable per-note/bank messages.
`UmpSysExAssembler` retains one group/stream at a time in bounded storage. Reserved,
Flex and unrelated Stream packets remain opaque. Native UMP transport, MIDI-CI and
Stream Configuration use the dedicated APIs described in [Native MIDI 2](Midi2.md);
that document records platform requirements, ownership and remaining acceptance.

## Polyphonic synthesis

`Synthesizer<T>::prepare(spec, voices, backend)` prepares 1..1024 voices, shared
buffers and selected vector kernels. Voices reuse Oscillator/Adsr; the sampler
reuses ChannelMatrix and SIMD envelope/mix operations. Note events merge MIDI1
and UMP at sample offsets inside the callback (MIDI1 first on equal offsets).
Out-of-block offsets fail before writes. Output replaces the block unless add=true.
Voice selection is inactive, then released, then oldest. Steals blend the last
per-channel contribution over stealFadeFrames; this avoids an immediate jump,
not a reconstruction of the stolen oscillator's future. Zero fade disables it.

`SampleData<T>` owns a copied planar sample and optional [first,end) loop. setSample
borrows it: keep it immutable/alive while attached. Sampler pitch uses linear
interpolation and rate/root-note mapping, without band-limited conversion or
automatic loop crossfades. Mono samples broadcast, mono output averages sample
channels, multichannel output retains matching channels and zero-fills extras.
`SynthVoiceRenderer<T>` borrows noexcept start/stop/render callbacks and context,
with one mono scratch block and per-voice index/identity/expression. CyDSP applies
ADSR/velocity/pan afterwards. Callbacks must obey the audio memory/locking contract.
Renderer errors may leave earlier voices mixed into output.

SetSample/setRenderer/setMpeZones require idle voices and control serialization.
Parameters are block-boundary edits; envelopes and stealing storage are retained.
MIDI1/MPE supports pitch/pressure/timbre/pedals plus volume/pan. UMP synthesis handles
16 groups, high-resolution velocity, pitch attributes, channel/per-note expression,
selected registered note pitch and sustain. Other instrument/controller policy
belongs to the host/custom voice. Synthesizer is a signal engine, not a preset bank.

## Sources, transport and worker I/O

`AudioSource<T>` exposes immutable rate/channel/frame info and random readAt. Its
callback contract is no I/O, allocation or locks, exact framesRead and zero tail.
`MemoryAudioSource<T>` borrows immutable planar samples; lifetime includes their
channel pointer table. All source/output aliasing is rejected before writes.

`AudioTransport<T>` borrows a source of matching rate/channels. A single control
producer enqueues play/pause/stop/seek/loop; the audio consumer applies commands
at callback boundaries. Loops split inside callbacks at exact frame endpoints.
Stop resets to frame zero, pause retains position; a loop starts playback inside
its configured range. EOF stops. would_block increments underruns; hold retains
the unfilled timeline, silenceAndAdvance skips it. position returns a coherent
bounded-retry atomic snapshot or transition_in_progress. reset requires idle
control publication and retains source/loop/storage. There is no thread join.
`TempoMap` prepares piecewise tempo/signature points, integrates beat/frame and
preserves bar phase across tempo changes; signature changes begin a new bar.
Beat is a quarter note and bar is zero-based. Frame precision is limited to 2^53-1.

Link `CyDSP::Sources` (requires Files/Threads) for `ReadAheadSource<T>` and
`BackgroundRecorder<T>`. Sources owns its dedicated file workers. Prepare/open,
prefill, close, finish, moves and destruction are control-thread operations; stop
audio use before them. readAt/push are each single-consumer/producer audio APIs.
The workers poll bounded SPSC queues; audio calls never wait or notify a condition.
ReadAhead prepares 2..128 pages of 1..65536 frames, reports misses as would_block
and discards stale seek epochs. It converts integer source rates to output rates
1..768000 Hz using shared 64-tap/1024-phase centered SincInterpolator. Rate 0 uses
the source rate. Very large ratios needing over 1048576 input frames per page are
rejected. prefill initiates a seek and may wait up to its timeout while audio is idle.

`SincInterpolator<T>` also provides centered random-access processing directly;
prepare its fixed ratio/cutoff/table, supply documented left/right input context
and zero padding at finite boundaries. It reuses windowed-sinc design and mixed
SIMD dots. This API reports no streaming latency; it needs future input context.
ReadAhead supplies that context and maps absolute seek times without adding delay.

BackgroundRecorder copies whole prepared blocks or returns update_queue_full.
The host chooses drop/retry policy; audio must not retry in a wait loop. finish
after producer idle drains the final publication, joins the worker and finalizes
the header, returning the I/O status. Reprepare while active is rejected. Export
format/encoding limits match AudioFileWriter. The recorder opens no microphone.

The [DAW example](../examples/DawBuildingBlocks.cpp) composes these original APIs
without borrowing code from an existing DAW. The optional Plugins target supplies original CLAP/VST3 host/editor and worker interfaces; see [Plugin hosting](Plugins.md).

## Prepared MIDI-CI property service

CiPropertyStore composes MidiCiDevice with generated ResourceList/DeviceInfo/
ChannelList/JSONSchema, custom fixed resId instances, full/partial SET, paginated
lists and acknowledged notify/full/partial subscriptions. A retained JSON tree/
pointer workspace performs bounded edits and list selection. All resource/reply
payloads and workspaces are allocated during preparation.

CiPropertyClient composes the same core with a bounded TTL cache, per-entry
generations, transaction slots and ACK FIFO. Older GETs cannot replace newer
pushed data. Store/client callbacks stay on the serialized control owner and
communicate with audio through existing prepared queues. Unicode/JSON/header
validation and PE Notify semantics share the portable core.
See [Property Exchange](CiProperty.md) for baseline recovery, routing, capacities,
callback lifetime, transactional mutation and remaining platform acceptance.

The [DAW integration contracts](DawIntegration.md) describe the additional filter designs/windows/interpolators, synthesis/presets, worker playback/assets, managers/clock collector, UMP/controllers/profiles and AIFC/CAF/native codecs.


## Integration remediation APIs

AudioMetadata provides typed versioned markers/regions/loops/tags and preserves
native unknown chunks. EditingPcmProxy provides bounded persistent PCM editing.
AudioDevice capabilities/descriptions/selectors and extended callback contexts
make negotiated/known/unknown/clock fields explicit. MidiInputBank/MidiInputGroup
supply per-port ownership, flexible SysEx and disconnect epochs/voice release.
ReadAheadSource policies/SourceTimeMap/providers expose curve/seek/tail semantics.
See [DAW contracts](DawIntegration.md) and [plugin hosts/editors](Plugins.md).
