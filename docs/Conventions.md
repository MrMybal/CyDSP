# Conventions

- C++20; namespace cydsp; PascalCase types, camelCase methods/fields, .h/.cpp files.
- Supported samples are float and double. Counts are std::size_t. A frame means
  one sample per channel. ProcessSpec uses double Hz; ResampleSpec uses positive
  uint32_t rational input/output rates in Hz.
- prepare validates before committing and may allocate/free. reset clears signal
  history without allocation; gain reset snaps to its target. SmoothedConvolver
  stageImpulse is also non-real-time preparation and may allocate/free. Parameter setters
  preserve the previous value on error and require owner-side serialization.
- process is noexcept with bounded work for supplied blocks; no allocation,
  freeing, locks or I/O. Preparation/destruction/moves are outside this contract.
  Gain and DelayLine ramps advance once per frame; reset snaps them to their target.
- Check Status. Invalid processing requests preserve output and signal state.
  Backpressure in resampling is a successful partial operation; its counts must
  be honored. A successful call does not imply that every supplied frame was used.
- Views own neither samples nor pointer tables. subBlock propagates its offset
  through mutable-to-const conversion. channelData returns the original table.
  AudioLayout flat spans are frame-major and reject source/destination overlap.
- Fixed-rate planar processors support exact corresponding-channel in-place use;
  ChannelMatrix additionally permits exact cross-channel aliases using snapshots.
  FFT and variable-rate processors have their own rules in Modules.md.
- Vector multiply/MAC allow exact output aliases, reject partial overlap. Butterfly
  even/odd/twiddle arrays must be disjoint. Dot reduction order varies by backend;
  mixed float/double dots convert samples before double multiplication.
- Biquad a0=1; a1/a2 feedback is subtracted. Frequencies are strictly between zero
  and Nyquist; Q is finite and positive. EQ gains are dB; shelf slope is (0,1].
- BiquadBank channels are independent voices with independent coefficients.
  Coefficient setters preserve histories. prepare starts identity voices; reset
  clears state while retaining coefficients. SIMD runs across voices.
- SmoothedBiquad shares one target across channels and crossfades stable banks.
  New nonzero-duration targets are queued during a fade; the latest target wins.
  Zero-duration edits cancel transitions. reset clears state and snaps to the
  latest target. Parameter commands remain serialized with processing.
- FIR h[0] multiplies the current sample; history starts at zero. Convolution uses
  the same causal impulse convention plus its announced partition latency.
  HybridConvolver combines a direct head and FFT tail without added block latency.
- ChannelMatrix coefficients are output-major, finite and rounded to T. Ramps
  share one frame progression, preserve progress for an identical target, and snap
  on reset. Channel counts and gains carry no implicit speaker-layout normalization.
- SmoothedConvolver separates non-real-time stageImpulse from allocation-free
  startTransition. Default live-input prewarm precedes the fade. Updates are owner-serialized. A transition retains both plans; new
  staging during a fade returns transition_in_progress. reset snaps a running fade
  to its destination and keeps any unstarted stage.
- FFT forward uses exp(-i*2*pi*k*n/N), unscaled. Inverse uses the conjugate sign
  and divides by N. Real FFT bins run from DC through Nyquist.
- Keep ISA types private. Never assume the build machine matches the destination.
- Adapted sources retain attribution, complete notice, pinned revision/hash and
  original archive. Original-code release licensing is an explicit owner choice.

- SmoothedBiquadCascade preallocates full cascades, coalesces one following target,
  and clears new histories on immediate edits/reset. Empty sections mean bypass.
- NonuniformConvolver has two partition sizes with an aligned late-tier offset.
  MatrixConvolver shares input FFT/history across route IRs and permits exact
  cross-channel aliases through input snapshots.
- AsyncConvolver supports one worker producer and one audio consumer, using three
  retained slots. prepare/reset/destruction require both sides quiescent. Queue
  pressure is reported; audio never destroys plans or waits for the producer.
- Resampler finish supplies bounded zero input, reports no caller input consumed,
  and becomes terminal until reset. setRates preserves history outside real time.
  Prepared ratio envelopes allow allocation-free setRatio ramps counted in output frames.
- AudioGraph borrows callback contexts, copies topology, rejects cycles and reuses
  buffers by last use and fuses linear null-callback gain chains while retaining
  stage rounding. Callback errors do not roll back already processed nodes.

- Complex FFT dispatches full stages; small stages may vectorize independent groups.
  Arithmetic remains strict and reductions retain their documented backend differences.
- ScheduledConvolver uses its late IR offset to spread FFT and MAC work over frames.
  It has zero added block latency; work-unit observers cover only its scheduled tail.
