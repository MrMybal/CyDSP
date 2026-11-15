---
id: synthesis
title: Oscillators and envelopes
description: Generate waveforms, FM/PM, wavetable morphs and control envelopes.
section: Guides
order: 22
revision: 1
parent: overview
---

## Oscillators and band limits {#oscillators}

Oscillator generates sine, triangle, saw, square and white noise with prepared sample rate/channel count. `next` advances one sample and `process` renders a block; do not call both for the same interval unintentionally. Wavetable prepares harmonic-band versions and WavetableOscillator selects a suitable bank once per block at its fixed frequency.

ModulatedOscillator supports audio-rate FM/PM, hard sync, table morphing and unison. These modes have their prepared oversampling/bandlimit contract; increasing modulation, sync discontinuities or pitch can still create aliasing. Do not assume fixed-frequency table selection alone solves every modulated spectrum.

## Envelopes and LFOs {#envelopes}

Adsr parameters use attack/decay/release seconds and linear sustain. noteOn/noteOff change stages. `process(span)` writes the same sequence as successive next calls. The application owns exact completion-event timing; query active/value on the processing owner or split at event boundaries.

MultiSegmentEnvelope supplies a prepared segment sequence. ModulationLfo includes sample-and-hold and seeded behavior; ProceduralSource generates bounded configured textures. Resets use per-instance phase/random state rather than global random generators. The host owns MIDI allocation, note scheduling, gain staging and the final output limiter.

## Public API {#api}

- [cydsp/processors/Generators.h](doc:api-processors-generators)
- [cydsp/synthesis/AdvancedSynthesis.h](doc:api-synthesis-advancedsynthesis)
- [cydsp/synthesis/Wavetable.h](doc:api-synthesis-wavetable)
