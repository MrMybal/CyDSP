---
id: granular
title: Sample and live granulation
description: Schedule bounded grains and reproducible clouds.
section: Guides
order: 24
revision: 1
parent: synthesis
---

## Sample grains {#sample}

Granulator prepares mono/stereo output from a prepared mono/stereo SampleData source. maximumGrains is 1–4096, default 128. The source is borrowed: keep the object and sample data alive, stable and unchanged until processing stops and the granulator is reset/reprepared.

GrainParameters specify positionSeconds, durationMs, signed pitchRatio, gain, pan, envelope, interpolation and wrap. Negative ratio reads backwards. Source/output sample-rate conversion is applied separately. Hann, triangular and rectangular envelopes have different click/spectrum behavior. Grain contributions sum without automatic limiting.

trigger accepts an offset in output frames below maxBlockFrames. Shorter later calls carry remaining delay forward. A full pool returns update_queue_full rather than stealing voices. activeGrains includes pending grains; droppedGrains counts rejected automatic cloud events.

## Clouds and live history {#live}

setCloud selects density and symmetric position/pitch/pan spreads; density zero stops new automatic events without stopping active grains. reset(seed) restarts reproducible per-instance scheduling. process(add=false) clears output; add=true accumulates into existing audio.

LiveGranulator uses prepared circular input history instead of a decoded sample. Configure history/grain capacities before processing. Freeze retains the capture history; each grain has its own signed speed and channel gains. Keep grain reads inside available history and manage wet/dry levels explicitly. A frozen history is not an infinite new recording buffer.

## Public API {#api}

- [cydsp/synthesis/Granulator.h](doc:api-synthesis-granulator)
- [cydsp/synthesis/LiveGranulator.h](doc:api-synthesis-livegranulator)
