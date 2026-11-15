---
id: api-core
title: Core and storage
description: Core and storage
section: API reference
order: 1001
revision: 1
parent: api
---

| Header | Purpose |
| --- | --- |
| [AudioBlock.h](doc:api-core-audioblock) | Borrowed planar views, sub-blocks and shape/overlap validation. |
| [AudioBuffer.h](doc:api-core-audiobuffer) | Owning zero-initialized, 64-byte-aligned planar storage prepared outside the callback. |
| [AudioLayout.h](doc:api-core-audiolayout) | Convert between planar blocks and frame-major interleaved spans without allocation. |
| [CircularHistory.h](doc:api-core-circularhistory) | One prepared signal history shared by independent integer and fractional readers. |
| [Cpu.h](doc:api-core-cpu) | CPU/OS capability detection, compiled backends and explicit backend names. |
| [Memory.h](doc:api-core-memory) | Byte-range overlap detection for borrowed spans. |
| [SpscQueue.h](doc:api-core-spscqueue) | Fixed-capacity single-producer/single-consumer communication with retained storage. |
| [Types.h](doc:api-core-types) | Sample types, processing dimensions, status values and arithmetic policies. |
| [Version.h](doc:api-core-version) | Compile-time version constants and macros for matching headers to the library. |
