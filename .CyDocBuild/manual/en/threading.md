---
id: threading
title: Realtime ownership
description: Serialize access and publish bounded control data.
section: Guides
order: 5
revision: 1
parent: contracts
---

## One owner per mutable processor {#owner}

CyDSP creates no processing thread. Give each mutable instance one audio owner and serialize setters, reset, getters, moves and destruction. Copy display metrics to application-owned snapshots instead of reading analyzer state concurrently.

Borrowed samples, contexts and pointer tables must remain at stable addresses until users stop. Shared HRTF prepared states have the distinct lifetime described in [HRTF preparation](doc:hrtf).

## Bounded communication {#queues}

`SpscQueue` requires exactly one producer and one consumer. Prepare capacity outside processing; check push/pop success and define a full-queue policy. Do not put values requiring realtime destruction into a queue without an ownership protocol.

`AsyncConvolver` explicitly supports one producer staging an IR concurrently with one audio consumer. Three retained slots prevent callback waits and destruction of retired plans. Preparation, reset and destruction require both sides quiescent. Treat queue pressure as a normal result.

Allocation-free does not mean cheap: long loudness queries, HRTF interpolation, HOA rotations and coefficient design still require scheduling. `noexcept` is not a CPU-time bound.

## Public API {#api}

- [cydsp/core/SpscQueue.h](doc:api-core-spscqueue)
- [cydsp/processors/AsyncConvolver.h](doc:api-processors-asyncconvolver)
