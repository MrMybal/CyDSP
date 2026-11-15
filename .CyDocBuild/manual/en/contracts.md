---
id: contracts
title: Processing contracts
description: Lifetimes, status values, units, latency and tails.
section: Guides
order: 3
revision: 1
parent: overview
---

## Lifecycle and ownership {#lifecycle}

`prepare` validates and may allocate; failure retains the previous usable configuration. `process` and `reset` retain prepared storage without allocation/free, locks or I/O. Repreparation, destruction, moves and explicit staging such as `stageImpulse` belong outside realtime processing.

Serialize all access to a mutable instance, including setters and getters. Views do not own samples or pointer tables. Shared assets and callback contexts have their API's lifetime. A successful `AudioBuffer::resize` invalidates previous views.

## Units and errors {#status}

A frame contains one sample per channel. Frequencies use Hz; time follows the API suffix (`Seconds`, `Ms`, `Frames`); gains are linear unless labeled dB. Sample formats are float/double; counts normally use `std::size_t`.

| Status | Caller action |
| --- | --- |
| `ok` | Use output; honor counts for variable-rate APIs. |
| `not_prepared` | Prepare first. |
| `invalid_spec`, `invalid_argument` | Correct dimensions, ranges or non-finite values. |
| `channel_mismatch`, `block_too_large` | Match prepared channels and maximum block. |
| `partial_overlap` | Use an allowed exact alias or disjoint storage. |
| `unsupported_backend` | Select a compiled CPU/OS-supported backend. |
| `transition_in_progress`, `update_queue_full`, `would_block` | Retain/retry according to the API; never busy-wait in the callback. |
| `end_of_stream` | Drain is complete; reset before a new stream. |

Ordinary validation errors preserve output/state. Graph callback failures cannot roll back executed nodes: external outputs are silenced. Extreme gains or sums can overflow despite finite inputs; CyDSP does not globally normalize or clip audio.

## Latency and tails {#timing}

Latency delays output; a tail continues after input ends. Read `latencyFrames()`/`tailFrames()` and retain fractional values when returned as double. A convolver with zero added block latency may still use an IR with acoustic predelay. Feed zeros or call `flush`/`finish` until drained. Freeze may create an unbounded tail. Reset erases history instead of rendering it.

## Public API {#api}

- [cydsp/core/Types.h](doc:api-core-types)
