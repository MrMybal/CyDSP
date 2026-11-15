---
id: metering
title: Levels, true peak and loudness
description: Interpret peak/RMS, intersample peaks and bounded LUFS history.
section: Guides
order: 28
revision: 1
parent: overview
---

## Levels and correlation {#levels}

LevelMeter keeps a prepared exact sliding window: peak, RMS, mean/DC and peak held since reset. Startup uses samples actually received. dBFS is 20log10(amplitude); silence gives negative infinity. Crest factor is peak/RMS in dB, zero for silence. There is no display decay.

Stereo correlation uses the first two channels' normalized cross-product including DC, clamped to [-1,1]; missing/silent channels give zero. The host owns display averaging and snapshot publication.

## True peak {#true-peak}

TruePeakMeter uses 4x/8x windowed-sinc reconstruction and 16/32/64/128 taps per phase. Maxima hold until reset. Original sample peaks are immediate; reconstructed peaks have nominal half-tap latency. Call flush at stream end. This estimates intersample maxima within its filter/phase model; it is not a certified universal analogue bound.

## Loudness and session duration {#loudness}

LoudnessMeter applies K weighting, 400 ms momentary and 3 s short-term windows. Integrated loudness uses -70 LUFS absolute and -10 LU relative gates; LRA uses -70/-20 gates and the 10th–95th percentiles. Channels beyond mono/stereo require explicit weights; map LFE/surround roles yourself.

maximumGatingBlocks reserves exact history at preparation (default 288000, about eight hours at 100 ms steps). If a whole block would exceed capacity, process returns would_block before consuming it. It does not silently use a rolling approximation. Prepare enough history or stop/reset deliberately.

levels is constant cost. integratedLufs scans retained history and loudnessRangeLu also sorts retained scratch: query outside a critical callback, serialized with processing. Mathematical definitions are implemented; complete certification/conformance coverage is not claimed.

## Public API {#api}

- [cydsp/analysis/LevelMeter.h](doc:api-analysis-levelmeter)
- [cydsp/analysis/LoudnessMeter.h](doc:api-analysis-loudnessmeter)
- [cydsp/analysis/TruePeakMeter.h](doc:api-analysis-truepeakmeter)
