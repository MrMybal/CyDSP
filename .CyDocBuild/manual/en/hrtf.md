---
id: hrtf
title: Prepare and share HRTFs
description: Separate onsets, convert FIR rates and retain immutable prepared banks.
section: Guides
order: 37
revision: 1
parent: binaural
---

## In-memory measurements {#prepare}

HrtfSet accepts up to 2048 distinct directions and 16384 input frames per ear. The application loads the data and supplies its license. No HRIR dataset or SOFA decoder is bundled. By default the first crossing of a threshold relative to the peak (-40 dB) defines an integer onset; supplied fractional delays remain separate.

Preparation removes that onset and resamples FIRs with windowed sinc when needed. Rates are 8–768 kHz and the conversion ratio is 1/8–8. Conversion preserves IR DC gain and exposes a common causal conversion latency through latencyFrames. Acoustic directional onsets remain distinct from that common latency.

## Capacity and sharing {#sharing}

Prepared FIR length is configurable up to 2048 taps. Check truncated(), especially after rate conversion. Renderer tail bounds include FIR, acoustic delays and conversion latency.

Renderers acquire a shared immutable prepared state during prepare; they retain independent mutable histories/filter fades. Repreparing or destroying the original HrtfSet does not invalidate already prepared renderers. This differs from raw SampleData/Wavetable borrowing.

Direction interpolation uses normalized spherical Shepard weights with Gaussian locality. Exact measurement directions reproduce their measured point. angularWidthRadians controls locality, not the measurement density or individual listener fit. Onset detection is not a fractional calibration procedure. Assess moving sources, sparse coverage and listener-specific cues on actual material.
