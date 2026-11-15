---
id: buffers
title: Buffers and channel layouts
description: Own storage, borrow views and interleave channels.
section: Guides
order: 4
revision: 1
parent: contracts
---

## Views and storage {#views}

`AudioBlock<T>` borrows a planar pointer table, channels and frames. `channel(i)` applies the sub-block offset; `channelData()` returns the original table. Nested `subBlock` calls accumulate offsets. Do not treat that original table as if it already pointed to a sub-block's start.

`AudioBuffer<T>::resize(channels, frames)` allocates zeroed 64-byte-aligned channel storage with private padding. Its block exposes logical frames only. Allocate outside processing. Keep allocations and pointer tables alive and unchanged while borrowed views are used.

## Overlap and conversion {#overlap}

Ordinary fixed-rate processors accept matching-channel exact in-place views or disjoint views; partial overlap fails. ChannelMatrix and MatrixConvolver additionally permit documented exact cross-channel aliases. FFTs, resamplers and shape-changing APIs have their own rules.

`interleave`/`deinterleave` use exactly `frames * channels` flat samples at `frame * channels + channel`. All planar/flat overlap is rejected; deinterleave also rejects overlapping output channels. No rate, sample-format or speaker-order conversion is implied. Validation cannot prove pointer lifetime or allocated capacity.

## Public API {#api}

- [cydsp/core/AudioBlock.h](doc:api-core-audioblock)
- [cydsp/core/AudioBuffer.h](doc:api-core-audiobuffer)
- [cydsp/core/AudioLayout.h](doc:api-core-audiolayout)
- [cydsp/core/Memory.h](doc:api-core-memory)
