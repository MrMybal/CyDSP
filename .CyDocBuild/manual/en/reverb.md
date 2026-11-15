---
id: reverb
title: Algorithmic and hybrid reverbs
description: Choose Schroeder, FDN, plate, spring or IR/FDN processing.
section: Guides
order: 16
revision: 1
parent: overview
---

## Choose a topology {#topology}

Reverb provides a Schroeder-style network. Diffuser combines allpass stages; orthogonalMix provides Hadamard/Householder mixing. FdnReverb uses 4/8/16/32 orthogonal feedback lines and 1–8 channels, with per-band nominal RT60, predelay, diffusion and damping.

PlateReverb is an original realization of a figure-eight plate topology, with input diffusion and cross-fed delay tanks. Its shimmer injects an octave-up dual-head signal into feedback. SpringReverb is a dispersive feedback waveguide, not a finite-element mechanical simulation.

## Decay, freeze and hybrid IR {#decay}

RT60 settings describe a nominal model. Conservative passive bounds can shorten extreme FDN band decays. Freeze disables injection as documented and may create an indefinitely retained tail. Provide zeros to keep time advancing; reset deliberately discards the tail.

HybridReverb combines an early prepared IR with an algorithmic late FDN. Stage a new impulse outside processing, then start its allocated transition between blocks. Account for both states during fades. RoomReverbBank retains several room responses and their tails; see [room acoustics](doc:acoustics).

ImpulseAnalysis can estimate decay and propose nominal FDN settings. This matches selected metrics, not every characteristic of a measured room.

## Public API {#api}

- [cydsp/processors/Diffusion.h](doc:api-processors-diffusion)
- [cydsp/spatial/FdnReverb.h](doc:api-spatial-fdnreverb)
- [cydsp/spatial/HybridReverb.h](doc:api-spatial-hybridreverb)
