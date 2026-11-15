---
id: echo
title: Acoustic echo cancellation
description: Align loudspeaker reference and microphone paths before NLMS adaptation.
section: Guides
order: 32
revision: 1
parent: restoration
---

## Reference and channels {#reference}

EchoCanceller receives a mono reference representing audio sent to loudspeakers and 1–8 microphones. Each microphone owns an independent FIR NLMS prediction/residual path, up to 4096 taps. Prepare the maximum reference delay; the host supplies alignment up to 0.5 s and must manage separate device clocks and drift.

Use the actual loudspeaker feed after relevant output processing, not an unrelated pre-mix. Monitor reference activity, adaptation, residual power, ERLE and counters through owner-serialized metrics.

## Double talk and limitations {#adaptation}

After initial learning, residual power and prediction/microphone coherence control linked adaptation freeze with configurable hangover. Silent reference or disabled adaptation also freezes learning. forceAdaptation requests deliberate relearning after a path change.

Tune thresholds on real speech and room changes. The linear FIR does not solve loudspeaker nonlinearity, clock drift or a complete communications stack. Keep microphone/reference blocks aligned and preserve the reference delay model through callback splits.

## Public API {#api}

- [cydsp/processors/EchoCanceller.h](doc:api-processors-echocanceller)
