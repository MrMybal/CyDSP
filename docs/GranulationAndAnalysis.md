# Granulation and audio analysis

These APIs belong to `CyDSP::CyDSP` and are included by `cydsp/CyDSP.h`.
They accept float/double audio with the same planar views, status values and
transactional preparation as the other CyDSP processors. The implementations are
original CyDSP code under the project MIT license, composed from its existing
sample reader, vector kernels, windows, real FFT and biquads.

## Lifecycle and threads

Call `prepare` outside the audio callback. `process`, parameter setters, `reset`
and getters retain prepared storage; moves/repreparation/destruction may free it.
Serialize access to each mutable instance, including getters. Returned spans are
borrowed, change on the next analysis/reset, and are invalidated by successful
repreparation/destruction. Copy and publish display snapshots through the host's
own synchronization. No UI, device, file, worker thread or plugin is created here.

Analysis specs accept 8–768 kHz, 1–64 channels and maximum blocks of 1–65536 frames.
A whole input block is checked before state changes: invalid shape, oversized
blocks, NaN/infinity or absolute sample values above `1e100` return an error.
Empty valid blocks do not advance time. Levels are not clipped to full scale.
Forced unavailable CPU backends return `unsupported_backend` and keep the previous
plan. Backend selection applies to the reusable kernels, not every scalar loop.

See the [compile-checked integration example](../examples/GranulationAndAnalysis.cpp).
It renders a cloud and measures its levels, true peak, FFT, Mel PSD bands and
loudness. A host must handle errors and session limits; a chain of several objects
is not a single transaction. Keep the example's owner at a stable address because
its granulator borrows its `SampleData` member.

## Granulator

`Granulator<T>::prepare(spec, sampleData, maximumGrains=128, backend)` borrows a
prepared mono/stereo sample and prepares a mono/stereo output. Keep the sample alive at a stable address whenever the granulator can process.
Stop/reset grains before changing source contents or loop settings, with all
access serialized. Source replacement is performed while stopped and followed by
granulator preparation. The voice limit is 1–4096.
Memory is bounded by that pool, two maximum-block scratch arrays and three
2049-point envelope tables, in addition to the caller-owned sample and its mips.

`trigger(parameters, frameOffset=0)` schedules a grain in output frames. The offset
must be below `spec.maxBlockFrames`; short subsequent process calls carry the
remaining delay forward. Pool exhaustion returns `update_queue_full`; active or
pending voices are never stolen. `activeGrains()` includes pending grains.

| Parameter | Meaning |
| --- | --- |
| `positionSeconds` | Source position in seconds, finite, absolute value at most 1e9. Without wrapping it must start inside the sample. |
| `durationMs` | Grain length in output time, up to 60000 ms; at least two output frames. Rounded to the nearest frame. |
| `pitchRatio` | Signed speed, absolute value 1/64–64; negative means reverse. Source/output sample-rate conversion is applied separately. |
| `gain` | Linear gain, finite, absolute value at most 16. Grain contributions are summed without automatic limiting. |
| `pan` | -1 left through +1 right. Mono→stereo uses constant-power pan; stereo→stereo uses balance with unity gains at center. Stereo→mono averages its two channels and ignores pan. |
| `envelope` | Hann, triangular or rectangular, read from prepared tables with linear interpolation over the grain's lifetime. |
| `interpolation` | Existing linear or harmonic-mip `SampleData` reader. Band-limited mode reduces aliasing when speeding up. |
| `wrap` | Wraps the playhead through the source's configured loop or otherwise through the whole sample. Without wrapping, out-of-range reads become zero until the grain ends. |

Interpolation, mip padding and loop crossfades retain `SampleData` semantics.
Playhead wrapping alone does not guarantee a seamless waveform: configure a source
loop/crossfade as needed. Without an explicit source loop, the fractional final
sample interpolates toward zero. `SampleData` loop interpolation also applies when
`wrap=false`; that flag controls the grain playhead, not the source reader.

`process(output, add=false)` clears output, then sums grains. `add=true` preserves
and adds to it. Output channels must be disjoint and cannot alias source storage.
Mono grains sample and apply their envelope once, then use the selected vector
accumulation kernels for both outputs. Stereo input keeps independent channel reads.

### Clouds and reproducibility

`setCloud` controls the base grain, density in grains/second (0 disables, maximum
one event per output sample), and symmetric uniform position/pitch/pan spreads.
Pitch spread is in semitones; source position is in seconds; pan spread is linear.
Spread results clamp to valid pitch/pan/start ranges. Positive density must produce
a finite sample interval. A density change starts a new schedule on the next
nonempty process call; other cloud edits preserve the existing schedule phase.
Disabling the cloud does not stop already launched grains.

`reset(seed=1)` clears voices, pending offsets, scheduling phase and drop count,
retaining cloud parameters and source. Seed zero maps to one. The random sequence
is deterministic and independent of how the host splits output into blocks.
`droppedGrains()` counts automatic events rejected by the full pool; manual trigger
errors do not increment it. Clouds do not normalize gain as density changes.

## LevelMeter

`prepare(spec, windowFrames=2048)` chooses an exact sliding window of 1–1048576
samples per channel. Startup uses the number actually received. `levels(channel)`
returns absolute sample peak over that window, RMS, arithmetic mean and held peak
since reset. dBFS helpers use `20*log10(amplitude)`; silence is negative infinity.
Crest factor is peak/RMS in dB, with zero for silence. There is no display decay.

`stereoCorrelation()` is the normalized cross product of the first two channels
over the same window, including DC (no mean subtraction), clamped to [-1,1]; it is
zero for a missing/silent channel. Sliding sums use compensated double accumulation.
Peaks use a preallocated monotonic queue with amortized constant work per sample.
`reset` clears windows and peak holds without allocation; its work scales with the
prepared window. Main storage is O(channels × windowFrames).

## TruePeakMeter

`prepare(spec, oversampling=4, tapsPerPhase=64, backend)` accepts factors 4/8 and
16/32/64/128 taps. Original normalized Blackman-windowed sinc phases reconstruct
intersample values using double SIMD dot products. This is an estimate, not a
certified EBU meter. Oversampling and FIR length affect the error near Nyquist;
4x/8x phase sampling need not find the exact continuous-time maximum.

`samplePeak`, `truePeak` and `truePeakDbtp` hold their maxima until reset. Sample
peaks appear immediately; reconstructed peaks have nominal `latencyFrames()` equal
to half the tap count. `flush()` feeds a full FIR tail of zeros at end of stream
without clearing results. Reset before starting an unrelated stream. Storage is
O(channels × taps + (factor−1) × taps); per-frame work scales with both parameters.

## SpectrumAnalyzer

`prepare(spec, {fftFrames=2048, hopFrames=512, window=Hann}, backend)` prepares a
periodic window and double-precision real FFT, even for float input. FFT size is a
power of two in [16,65536]; hop is 1–FFT size. The first result arrives after a full
window, then every hop. `frameCount()` counts completed analyses; getters retain
only the latest complete result, so one large process call can replace several
intermediate results. No callback or queue is implied.

For N samples, Fs sample rate and unnormalized windowed transform X[k]:

- `complexSpectrum`: raw windowed FFT bins, including DC and Nyquist.
- `magnitudes`: one-sided peak amplitude, `2*abs(X)/sum(window)` for interior bins;
  DC/Nyquist use factor one. A bin-centered isolated sinusoid reads its peak level.
- `powerDensity`: one-sided PSD in sample-units squared/Hz, with interior factor two
  and denominator `Fs*sum(window²)`. Multiply its sum by `Fs/N` for window-weighted
  mean square. This is distinct from squaring the amplitude spectrum.

Features are unwindowed RMS of the current window, power-weighted centroid, the
first bin reaching 85% cumulative power, flatness excluding DC, dominant bin
frequency and positive flux. Flux is the Euclidean norm of positive changes in
L1-normalized amplitude bins; the first frame reports zero. Silence reports zero
features. Spectral resolution is Fs/N; dominant frequency is not sub-bin pitch
estimation. Storage is O(channels × N), with shared FFT/window scratch.

### Mel and logarithmic bands

`SpectralBands::prepare(Fs, N, config, backend)` builds sparse triangular filters
between minimum/maximum Hz, with 40 Mel bands by default. Zero maximum means Nyquist.
Mel centers use `2595*log10(1+Hz/700)`; logarithmic centers require positive minimum
Hz. Up to 512 bands are allowed. Bands lacking FFT-bin support are rejected;
increase N or reduce the band count instead of silently returning empty bands.

`process(inputBins, outputBands)` takes exactly N/2+1 finite nonnegative double bins
(up to 1e200), and exactly the prepared number of output bands, with disjoint
storage. Each output is a normalized weighted mean in the input's units: feeding
PSD produces mean PSD, not integrated band energy. Feeding magnitudes produces
mean amplitude. This API does not compute MFCCs. Centers are available separately.

## ConstantQAnalyzer

This is a direct constant-Q bank of variable-length Hann-windowed complex
projections, not an FFT re-binning approximation. Defaults are 55 Hz minimum,
12 bins/octave, 96 bins, 512-frame hop and 131072 maximum kernel frames. Each center
is `minimumHz*2^(bin/binsPerOctave)` and each window length is
`max(4,ceil(Q*Fs/centerHz))`, where `Q=1/(2^(1/binsPerOctave)-1)`.

All centers must be below Nyquist. Limits are 1–96 bins/octave, 1–512 bins,
1–65536 hop frames, at most 1048576 samples per kernel and 16777216 total kernel
samples. Choose a suitable frequency range for the sample rate. Preparation rejects
excessive kernels before processing. Kernels are double, normalized by two/window
sum; their SIMD dots produce magnitudes of complex projections. Nearby tones and
short low-Q windows can affect apparent amplitude through spectral leakage.

The first result waits for the longest kernel, then results follow the hop. Each
band ends at the same input timestamp but has its own window length and effective
center delay. `windowFrames(bin)` exposes that distinction. Latest-frame delivery
and owner serialization follow SpectrumAnalyzer. Storage includes two coefficient
arrays for all kernels, two longest-window history copies per channel, and output
magnitudes. Cost scales with channels × total kernel length per analysis: reduce
bins, increase minimum Hz or use a larger hop for lighter display workloads.

## LoudnessMeter

The meter applies BS.1770 K weighting with existing double CyDSP biquads, explicit
channel energy weights and integrated two-stage gating. Published 48 kHz
coefficients are used at that rate; other supported rates use a bilinear
parameterization. Momentary and short-term windows are 400 ms and 3 s. Gating
energies are sampled every 100 ms; time dimensions round to the nearest sample.

Empty `channelWeights` selects unit weights for mono/stereo. More channels require
one explicit weight per channel, copied at preparation, each in [0,16]. Typical
channel layouts exclude LFE with zero and use 1.41 for relevant surround channels;
the host is responsible for mapping its actual layout. No speaker order is assumed.
This API does not implement the full BS.1770 immersive/object-audio layout model.

`levels()` is O(1), returns readiness flags and current momentary/short-term LUFS.
Before the corresponding complete window it returns negative infinity. Integrated
LUFS uses 400 ms blocks, an absolute -70 LUFS gate and a relative -10 LU gate.
`loudnessRangeLu()` applies absolute -70 LUFS and relative -20 LU gating to 3 s
energies, then subtracts the 10th from the 95th percentile, using linear R7
interpolation. No eligible history gives integrated negative infinity and LRA zero.
These mathematical behaviors are independently tested; official full conformance
vectors and certification have not been run.

### Exact bounded history

`maximumGatingBlocks` defaults to 288000 (approximately eight hours at the 100 ms
hop), with a maximum of 10000000. Preparation reserves all history and sorting
scratch. `process` predicts capacity first: if either history would overflow, it
returns `would_block` without consuming any part of the supplied block. There is no
silent rolling approximation or automatic reset of integrated measurements. The
host must size the session, stop/reset the meter, or prepare another one while
stopped. Reset retains capacity and clears accumulated results.

Call `integratedLufs()` and `loudnessRangeLu()` outside a critical callback and
serialize them with processing: integrated scans history in O(blocks), LRA also
sorts preallocated scratch in O(blocks log blocks). Allocation-free does not mean
constant cost. Memory is about `24*maximumGatingBlocks + 8*round(3*Fs)` bytes plus
weights, small filter state and 64-frame/channel conversion scratch. At 48 kHz
and default history this is roughly 8.1 MB (decimal).

## Mathematical references and validation

The meter uses the published mathematical definitions in
[ITU-R BS.1770-5](https://www.itu.int/dms_pubrec/itu-r/rec/bs/R-REC-BS.1770-5-202311-I!!PDF-E.pdf),
[EBU Tech 3341](https://tech.ebu.ch/docs/tech/tech3341.pdf) and
[EBU Tech 3342](https://tech.ebu.ch/docs/tech/tech3342.pdf).
No external implementation is imported for these modules.

[Validation](Validation.md) records numerical oracles, allocation/free auditing,
supported backends actually executed and remaining platform limits.
[Measurements](Measurements.md) gives complete per-call workloads and units.
