---
id: sources
title: Many sources and shared assets
description: Render fixed source slots with shared sample/wavetable banks.
section: Guides
order: 25
revision: 1
parent: synthesis
---

## Capacity and lifetimes {#capacity}

SourceBatch prepares up to 1024 fixed source slots, 64 output channels, 256 sample banks and 256 wavetable banks, with an internal rendered-voice storage cap of 2^24 values. Preparation copies bank pointers, not their data. Keep bank objects and contents stable and immutable until every reader stops.

Each BatchSource chooses sine, wavetable or grain, plus phase/frequency/gain, delayed start and output gains. An occupied slot is not stolen; start returns would_block. The host chooses voice stealing/retry policy and owns any scene/entity mapping.

## Rendering and positioning {#render}

Sine voices use complex recurrences vectorized across voices and periodic renormalization. Wavetables use harmonic mips; grains use SampleData interpolation/loops and a Hann envelope. Lookup/read positions remain partly scalar. Summation requires headroom.

setPosition with order -1 supplies constant-power stereo pan and 1/max(1,distance) attenuation. Orders 0–7 require exactly `(order+1)^2` ACN/SN3D outputs. Stereo panning alone does not supply HRTF elevation cues. Use the [spatial coordinate convention](doc:spatial#coordinates) and [binaural/HOA rendering](doc:binaural) for those tasks.

## Public API {#api}

- [cydsp/synthesis/SourceBatch.h](doc:api-synthesis-sourcebatch)
