---
id: limits
title: Platforms, capacities and model limits
description: Separate algorithm availability from target and model assumptions.
section: Guides
order: 41
revision: 1
parent: overview
---

## Targets {#platforms}

| Target | Current scope |
| --- | --- |
| Windows x64 | MSVC/clang-cl runtime coverage; supported compiled SSE2/AVX2/AVX-512 paths depend on CPU/OS. |
| Android ARM64 | Runtime coverage on Quest 3 with NEON; it has no SVE. |
| Windows ARM64 | Static compilation coverage; device execution remains to validate. |
| ARM64 SVE | Android compile/link coverage; hardware vector-length execution remains to validate. |
| macOS / desktop Linux | Source targets; execution remains to validate for this version. |
| x86 / ARM32 | Compatibility paths exist; they were not rerun for this snapshot. ARM32 double kernels can fall back to scalar. |

No ABI is interchangeable with another. Version 0.4.5 is in development; target deployment needs its own content/performance assessment. Performance depends on processor, dimensions, quality, backend and block boundaries.

## Prepared capacities {#capacities}

| Area | Important boundary |
| --- | --- |
| General expanded processors | Typically 8–768 kHz, 1–64 channels, up to 65536 maximum block frames; individual APIs may be smaller. |
| SourceBatch | 1024 sources, 64 outputs and explicit prepared storage/bank budgets. |
| Granulator | Up to 4096 slots; no implicit voice stealing. |
| Spatial paths / rooms | 64 paths and 10 s prepared path delay; up to 32 room responses. |
| Ambisonics | Orders 0–7, 1–64 ACN/SN3D channels. |
| HRTF | Default 2048 directions/32768 input frames/2048 output taps; explicit reference caps 100000/65536/65536. See [SOFA](doc:sofa). |
| Graphs | MultiBusGraph: 256 nodes, 4096 connections and 32 buses/layout. |

## Interpret models correctly {#models}

The optional [SADIE II selection](doc:sofa#bundled) carries its own Apache-2.0 data license. Listener suitability and rights for other banks remain application concerns. Analytic binaural, thin-screen diffraction, room transfer, contact physics and musical estimates are reduced/heuristic models. True peak/loudness are not presented as certified broadcast products. Echo cancellation is linear and needs aligned clocks/reference. Choose limits from each header's configuration and assess realistic content rather than extrapolating from a single synthetic signal.
