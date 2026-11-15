---
id: spatial
title: 3D coordinates and propagation
description: Use metres, radians, source/listener poses and acoustic media.
section: Guides
order: 34
revision: 1
parent: overview
---

## One coordinate convention {#coordinates}

CyDSP uses a right-handed frame: **+X forward, +Y left, +Z up**. Positions are metres, velocities m/s, angles radians and quaternions `(w,x,y,z)`. Convert a game's axis directions and scale explicitly at the boundary. Normalize orientations and use the API's validation instead of feeding engine-specific units directly.

relativeSource gives distance, local/world directions, azimuth/elevation and radial velocities. relativeSources writes listener-major rows at `listener * sourceCount + source`. interpolatePose uses shortest-arc quaternion interpolation.

## Distance, directivity and media {#propagation}

DistanceAttenuation selects inverse, exponential or linear distance gain with reference/maximum distance, rolloff and clamping policy. Directivity uses a pattern, rear band losses, floor and optional polarity preservation. AcousticMedium contains sound speed, density and three-band absorption; atmospheric helpers use temperature, humidity and pressure. waterMedium is a reference preset, not automatic water physics.

acousticTravel accumulates segment delay/loss. DopplerPlan caches sound-speed/intensity/pitch bounds for repeated updates; DopplerResampler applies the resulting input/output ratio with explicit stream counts. Do not apply identical Doppler twice through resampling and a changing propagation delay. The host owns geometry, occlusion detection, updates and audio FIFO scheduling.

## Public API {#api}

- [cydsp/processors/BandGain.h](doc:api-processors-bandgain)
- [cydsp/spatial/Doppler.h](doc:api-spatial-doppler)
- [cydsp/spatial/SpatialMath.h](doc:api-spatial-spatialmath)
