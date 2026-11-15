---
id: dynamics
title: Compressors, gates and sidechains
description: Control detector ballistics and prepared look-ahead.
section: Guides
order: 12
revision: 1
parent: overview
---

## Modes and detector {#detector}

Dynamics supports compressor, expander, noiseGate and limiter modes. Prepare the mode and maximum look-ahead first; set threshold/knee/makeup in dB, ratio, attack/release/hold in ms, gate hysteresis and linked/unlinked channels through DynamicsParameters.

Peak and RMS detectors have different ballistics. BallisticsFilter also exposes envelope detection independently. A linked detector shares gain decisions across channels; it does not sum those channels into mono output.

## External sidechain and mixing {#sidechain}

The three-block process overload accepts program, sidechain and output. A mono sidechain broadcasts to the program channels; otherwise its channels match the program. Sidechain/output must be disjoint unless the sidechain is the program block itself. Keep detector timing aligned with the desired program event.

processMixed accepts finite linear dry/wet gains of magnitude at most 16. Its dry path follows the configured look-ahead delay. gainReductionDb and minimumAppliedGain describe the last nonempty successful block before dry/wet mixing; read them on the processing owner or publish a snapshot.

Read latencyFrames after selecting look-ahead. Limiters with ordinary sample detection and the oversampled TruePeakLimiter have different reconstruction contracts; use the [advanced guide](doc:advanced-dynamics).

## Public API {#api}

- [cydsp/processors/Dynamics.h](doc:api-processors-dynamics)
