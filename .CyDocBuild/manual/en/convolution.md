---
id: convolution
title: Convolution and impulse responses
description: Choose latency, routing and impulse transition behavior.
section: Guides
order: 17
revision: 1
parent: overview
---

## Causal impulses and engines {#engines}

Tap h[0] multiplies the current input sample. PartitionedConvolver uses uniform FFT partitions and reports block latency. HybridConvolver combines an immediate direct head and FFT tail. NonuniformConvolver adds a larger late partition. ScheduledConvolver distributes late work across frames. The common Convolver chooses hybrid/nonuniform/scheduled at preparation and adds no block latency.

Latency zero means immediate application of the IR's first tap, not removal of silence/predelay within the IR. tailFrames defines how long zeros must still be processed after the source stops. reset clears that state.

## MIMO and updates {#routes}

MatrixConvolver routes `ConvolutionRoute<T>` input/output channel pairs; duplicate routes add. Impulses are copied during preparation, and input FFT/history is shared. Supply the true route matrix for true stereo rather than assuming two independent IRs implement crossfeed.

SmoothedConvolver separates non-realtime stageImpulse from allocation-free startTransition. Live-input prewarm precedes a fade by default. Both plans remain retained; updates during a fade can return transition_in_progress. reset snaps a running fade to its destination while retaining an unstarted stage. AsyncConvolver adds the explicit [producer/consumer contract](doc:threading#queues).

See [partition tuning](doc:convolution-options) for the spectral-head and late-work tradeoffs. Keep IR sample rate/channel mapping correct before preparation; loading/decoding files is an application task.

## Public API {#api}

- [cydsp/processors/Convolver.h](doc:api-processors-convolver)
- [cydsp/processors/HybridConvolver.h](doc:api-processors-hybridconvolver)
- [cydsp/processors/NonuniformConvolver.h](doc:api-processors-nonuniformconvolver)
- [cydsp/processors/PartitionedConvolver.h](doc:api-processors-partitionedconvolver)
- [cydsp/processors/SmoothedConvolver.h](doc:api-processors-smoothedconvolver)
