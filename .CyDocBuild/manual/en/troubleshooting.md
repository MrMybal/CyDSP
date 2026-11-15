---
id: troubleshooting
title: Troubleshooting
description: Diagnose shape, silence, latency, streaming and spatial issues.
section: Guides
order: 40
revision: 1
parent: overview
---

## Common symptoms {#symptoms}

| Symptom | Check |
| --- | --- |
| not_prepared | All members successfully prepared; moved-from objects need preparation. |
| channel_mismatch/block_too_large | Actual views match prepared channels and maximum frames, including sidechains. |
| partial_overlap | Use exact supported in-place views or disjoint buffers; account for sub-block offsets. |
| Silence after a graph error | Check the first failing node Status and callback context lifetimes. |
| Click on parameter changes | Use gain ramps, stable-filter fades or prepared IR/graph transitions. |
| Flanging in dry/wet | Align fixed latency and distinguish frequency-dependent IIR phase. |
| Dropped resampled input | Keep the suffix after inputConsumed; only publish outputProduced. |
| Missing convolution tail | Continue zeros or use the supported drain operation. |
| Grain trigger rejected | Check pool capacity, offset and source/position lifetime. |
| No early analysis result | Accumulate a full window; respect hop and readiness flags. |
| LUFS processing returns would_block | Exact history is full; size the session or stop/reset deliberately. |
| Moving HRTF request rejected | Previous direction fade is active; coalesce/retry without blocking. |
| Wrong 3D direction or speed | Convert axes, metres, radians, quaternion order and radial velocities. |
| Different precision/backend samples | Reduction order, selected approximation, antialias policy and FFT block pattern can differ. |

## Investigate a minimal chain {#minimal}

Record version, target ABI, sample rate, channel layout, block pattern and exact settings. Reduce to one processor and a finite test signal; inspect returned Status before interpreting output. Match numerical contracts when comparing another implementation, including latency, detector topology and antialiasing. A faster algorithm with different behavior is a separate quality choice.
