---
id: sampling
title: Prepared samples and loops
description: Render sample blocks with interpolation and harmonic mips.
section: Guides
order: 23
revision: 1
parent: synthesis
---

## Prepare and own sample data {#prepare}

SampleData::prepare copies planar samples, sample rate and root note and builds prepared antialiasing mips. Decode files in the application before calling it. Objects borrowing SampleData, such as Granulator and SourceBatch, need stable source addresses and immutable contents during use.

`value(channel, position, ratio, interpolation)` reads a fractional sample position. SampleInterpolation offers linear and bandLimited. Higher-speed playback can use a lower-bandwidth mip; this reduces aliasing but does not promise perfect reconstruction at every ratio.

## Loops and sequential rendering {#loops}

setLoop uses `[first,end)` with an optional crossfade length; 0/0 disables looping. Prepare loops while readers are stopped and serialize all access. A loop crossfade changes the effective wrapped start to loopStart+loopCrossfade as documented.

`render(position, ratio, mode, output, inside)` selects its mip once per block, writes every channel and advances caller-owned position. Extra output channels repeat the last source channel. Reads beyond the end become zero and still advance. inside counts the leading frames read before the end. Invalid requests retain position/output. Handle end-of-sample and instrument voice scheduling in the application.

## Public API {#api}

- [cydsp/synthesis/SampleData.h](doc:api-synthesis-sampledata)
