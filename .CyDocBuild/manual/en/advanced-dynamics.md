---
id: advanced-dynamics
title: Dynamic EQ, transient and nonlinear effects
description: Choose the transfer, antialiasing and detector contract.
section: Guides
order: 13
revision: 1
parent: dynamics
---

## Saturation and antialiasing {#saturation}

AdaaSaturator offers tanh, atan, cubic, hardClip, wavefolder and hysteresis. Drive, bias and outputGain are explicit. With antialias enabled, memoryless shapes use first-order antiderivative antialiasing with a midpoint limit for nearly equal samples. This reduces aliasing but does not eliminate it, and it has frequency-dependent smoothing.

Hysteresis is a bounded play operator followed by tanh, not a physical magnetization solver. MathAccuracy fast and antialias are independent. processMixed fuses an optional double input stage, shaping and linear mixing, while rounding wet audio to T before mixing. Preserve this staging when comparing separate chains.

## Dynamic equalizer and transients {#detectors}

DynamicEqualizer uses unity-peak bandpass corrections cascaded per band, with independent detectors driven by the original sidechain. It supports up to 16 bands; rangeDb may cut or boost. This is not interchangeable with every dynamic bell topology. DeEsser reuses one such band. MultibandDynamics uses complementary FIR splitting, up to 16 bands, with the FIR latency and no compressor look-ahead.

TransientShaper compares fast/slow envelopes and applies attack/sustain controls with a maximum gain. processGain emits the corresponding linear gains and advances the same envelopes as process; do not call both for the same input event unless two advancements are intended. Its fast policy approximates the gain exponential; [quality selection](doc:quality#accuracy) remains local.

## True-peak limiting {#limiting}

TruePeakLimiter uses 4x/8x oversampling, linked look-ahead and FIR return filtering. Margin defaults to 0.5 dB. Latency is fractional double. The ceiling depends on reconstruction/filter assumptions: assess intersample peaks on real content. It does not promise broadcast certification or a universal analogue peak bound.

## Public API {#api}

- [cydsp/processors/AdvancedDynamics.h](doc:api-processors-advanceddynamics)
