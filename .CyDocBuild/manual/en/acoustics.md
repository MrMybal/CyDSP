---
id: acoustics
title: Paths, materials, rooms and diffraction
description: Render prepared scene paths and room buses from host geometry.
section: Guides
order: 35
revision: 1
parent: spatial
---

## Paths and material response {#paths}

AcousticMaterial supplies three-band absorption/transmission loss, reference thickness and scattering. PreparedAcousticMaterial/PreparedMaterialBand cache validated settings for repeated thickness responses. These gains are parametric approximations rather than a geometric solver.

AcousticPathRenderer has up to 64 stable-ID path slots and at most 10 s prepared delay. It shares mono history and exposes one output channel per path slot, with delay, arrival direction and three-band gains. Multichannel input is averaged; keep independent emitters separate. Removing a path fades it before releasing the slot instead of stealing an audible path.

## Rooms, portals and returns {#rooms}

The application supplies rooms, portal edges/opening/losses and response cells. AcousticPortalGraph prepares transfer routes; RoomCellGrid interpolates a regular response grid. CyDSP does not ray trace the scene or inspect game actors.

RoomReverbBank retains up to 32 room responses. Its block workflow is beginBlock, one or more send calls, render, then mix or read roomOutput. Continue rendering zero-send rooms to advance tails. The application chooses pre/post-occlusion sends and listener-return gains.

## Diffraction {#diffraction}

DiffractionPath describes edge detour and aperture conditions from host geometry. diffractionGain evaluates a thin-screen/frequency-dependent approximation; DiffractionFilter applies its band response. It is not a wave-equation or arbitrary obstacle solver. Keep distances, aperture and frequencies in the API's units and assess low/high-frequency behavior in the intended scene.

## Public API {#api}

- [cydsp/spatial/AcousticPaths.h](doc:api-spatial-acousticpaths)
- [cydsp/spatial/AcousticRooms.h](doc:api-spatial-acousticrooms)
- [cydsp/spatial/Diffraction.h](doc:api-spatial-diffraction)
