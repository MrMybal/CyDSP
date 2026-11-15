---
id: effects
title: Delay and modulation effects
description: Configure chorus, phaser, echoes, flanger and amplitude modulation.
section: Guides
order: 15
revision: 1
parent: delay
---

## Select behavior and capacity {#configure}

Chorus and Phaser expose dedicated parameter sets. DelayEffect combines a circular history, up to 16 feed-forward taps and a filtered/saturated feedback read. Prepare maximumDelayMs first; all tap delays and modulation excursions must fit the prepared capacity.

Its echo, flanger, vibrato and wowFlutter modes interpret delayMs, depthMs, rateHz, feedback, lowPassHz, feedbackDrive, duckDb, wet and dry. Mode changes do not replace these values with hidden presets. Ping-pong routing is explicit. Feedback filtering/saturation changes both decay and tone; leave headroom and keep feedback inside accepted bounds.

## Amplitude and tail handling {#amplitude}

AmplitudeModulator uses rate/depth and an autopan choice for tremolo/panning. Panning does not reproduce HRTF elevation cues. Reset restarts modulation phase according to the processor's contract.

Continue sending silence while echoes/reverb decay; stopping the process call freezes internal time. For sample-accurate parameter events, split blocks or use a dedicated supported modulation input. Keep continuous delay motion when pitch modulation is intended and reader crossfades when it is not.

## Public API {#api}

- [cydsp/processors/AdvancedEffects.h](doc:api-processors-advancedeffects)
- [cydsp/processors/Effects.h](doc:api-processors-effects)
