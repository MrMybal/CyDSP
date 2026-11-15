---
id: simd
title: CPU architectures and SIMD
description: Choose supported kernels during preparation.
section: Guides
order: 7
revision: 1
parent: contracts
---

## Dispatch and binaries {#dispatch}

Build an archive for each target ABI. Automatic selection uses compiled CPU/OS support: AVX-512, AVX2 or SSE2 on suitable x64; SVE then NEON on compatible ARM64 Linux/Android; otherwise the available baseline. Windows/macOS ARM64 use NEON. AVX-512 is x64-only; SVE requires compatible ARM64 hardware.

A forced unavailable backend returns `unsupported_backend` while retaining the old plan. VectorOps::create prepares an immutable table; the default VectorOps is scalar. Read `backend()` where exposed.

## Numerical behavior {#numerics}

SIMD handles independent samples, channels or voices where dependencies permit. Recursive filters/envelopes retain signal order. Dot reductions can round differently across backends; universal bit identity is not assumed.

ISA options stay in private implementation files. Portable code uses no global native-CPU flags, fast-math, FMA contraction or floating-point-state mutation. Automatic selection is capability-based, not measured for each buffer. Profile actual processors/block patterns before overriding it. SVE uses the thread's current vector length; hardware execution remains a [target limit](doc:limits).

## Public API {#api}

- [cydsp/core/Cpu.h](doc:api-core-cpu)
- [cydsp/math/VectorOps.h](doc:api-math-vectorops)
