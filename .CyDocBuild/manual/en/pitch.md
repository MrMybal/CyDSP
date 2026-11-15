---
id: pitch
title: Pitch, duration and formants
description: Choose delay-head, spectral or optional stretch processing.
section: Guides
order: 27
revision: 1
parent: overview
---

## Select an engine {#engines}

PitchEffects provides target-pitch correction, intervals and multivoice harmony with gains, pans and creative delays. Its ordinary prepare uses a two-head sinc delay approach. prepareSpectral connects a phase-vocoder engine with a declared N-frame latency, up to eight voices and mono/stereo operation. Account for creative per-voice delays separately.

SpectralPitchShifter uses power-of-two N=64–16384 with hop N/4. It measures instantaneous frequencies, links a guide channel and preserves relative channel phase. Significant spectral peaks guide local phase locking; a smoothed envelope controls formant compensation. Ratio 1 gives delayed reconstruction within rounding.

## Source assumptions {#quality}

Envelope smoothing width defaults to 150 Hz. Too wide loses narrow formants; too narrow follows harmonics. Positive spectral flux triggers transient phase reset heuristically. Large intervals, polyphony and poorly resolved low formants remain difficult. Monophonic correction uses YIN confidence; a pitch detector is not polyphonic note transcription.

TimeStretch is the optional CyDSP::Stretch target, exposing time/pitch/formant controls. Its stream counters and buffer requirements differ from fixed-rate processing; follow its exact API. Prepare voices/assets outside the callback, retain borrowed contexts and drain tails. Inactive spectral harmony voices are not automatically warmed up.

## Public API {#api}

- [cydsp/processors/PitchEffects.h](doc:api-processors-pitcheffects)
- [cydsp/processors/TimeStretch.h](doc:api-processors-timestretch)
- [cydsp/spectral/SpectralPitch.h](doc:api-spectral-spectralpitch)
