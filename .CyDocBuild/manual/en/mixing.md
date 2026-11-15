---
id: mixing
title: Gain, panning and routing
description: Route and mix planar channels with explicit gain laws.
section: Guides
order: 9
revision: 1
parent: overview
---

## Gain and pan {#gain}

Gain uses linear amplitude; `decibelsToGain` converts dB. Gain ramps advance once per frame across channels and across block boundaries. Reset snaps to the current target. Select a Panner/DryWetMixer law explicitly: equal-power and linear fades have different midpoint behavior, and correlated wet/dry signals can sum above unity.

WaveShaper provides elementary static shapes; AdaaSaturator adds antialiasing/stateful choices described in [advanced dynamics](doc:advanced-dynamics). Bias adds a DC offset; it is not a DC-removal filter.

## Channel matrices {#matrix}

ChannelMatrix coefficients are output-major: `coefficients[out * inputChannels + in]`. Each output is the sum of its weighted inputs. Coefficients round to the sample type. There is no implicit speaker mapping, loudness compensation, clipping or normalization.

`setMatrix` preserves shape and can ramp gains once per frame. Repeated identical targets retain ramp progress. Reset snaps to target. Exact cross-channel aliases support permutations and rectangular in-place mixes through retained input snapshots; partial overlap and output/output overlap fail. Separate read-only input channels may alias.

Use application-specific coefficients for mono/stereo/downmix and document their headroom. Muted coefficients omit their source contribution; unmuted extreme sums can overflow.

## Public API {#api}

- [cydsp/processors/ChannelMatrix.h](doc:api-processors-channelmatrix)
- [cydsp/processors/Gain.h](doc:api-processors-gain)
- [cydsp/processors/Mixing.h](doc:api-processors-mixing)
