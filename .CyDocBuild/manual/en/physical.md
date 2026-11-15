---
id: physical
title: Physical and contact synthesis
description: Excite modal banks, strings and waveguides with explicit units.
section: Guides
order: 26
revision: 1
parent: synthesis
---

## Resonators and guides {#models}

ModalResonator accepts frequency/decay/gain modes and offers membrane/plate mode arrangements. Waveguide provides string/tube and Karplus–Strong-style models from prepared delay/filter storage. Choose a sample rate and mode/frequency range that remain representable; these are reduced models, not general mechanical simulations.

The host supplies excitation, feedback coupling and physical parameter mapping. A stable component does not prove a stable coupled instrument. Limit excitation energy and assess resonances, long tails and sample-rate changes.

## Contact excitation {#contact}

ContactExciter creates mono impact/sliding/rolling/bowed excitation for a resonator. Units are explicit: normal force N, velocity m/s, mass kg, stiffness N/m, impulse N·s and spacing metres. Up to eight impacts overlap; overflow is reported.

Impact half-sines preserve the requested discrete impulse before the 20 Hz DC blocker. Sliding uses bounded bristle state; bowing adds Stribeck friction and can receive resonator surface velocity. Rolling triggers micro-contacts over asperities. reset(seed) makes textures reproducible. Output gain converts physical magnitudes into audio level. No collision engine or rigid-body solver is created.

## Public API {#api}

- [cydsp/synthesis/ContactExciter.h](doc:api-synthesis-contactexciter)
- [cydsp/synthesis/PhysicalModels.h](doc:api-synthesis-physicalmodels)
