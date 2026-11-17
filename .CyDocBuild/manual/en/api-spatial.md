---
id: api-spatial
title: Spatial audio
description: Spatial audio
section: API reference
order: 1007
revision: 1
parent: api
---

| Header | Purpose |
| --- | --- |
| [AcousticPaths.h](doc:api-spatial-acousticpaths) | Stable acoustic-path slots with shared input history, delays and three-band gains. |
| [AcousticRooms.h](doc:api-spatial-acousticrooms) | Room/portal transfer, response-grid interpolation and retained room-reverb buses. |
| [Diffraction.h](doc:api-spatial-diffraction) | Thin-screen edge/aperture diffraction gains and a prepared band filter. |
| [Doppler.h](doc:api-spatial-doppler) | Doppler through prepared variable-rate resampling and explicit stream counts. |
| [FdnReverb.h](doc:api-spatial-fdnreverb) | Orthogonal feedback-delay networks with band-dependent decay, diffusion and freeze. |
| [HybridReverb.h](doc:api-spatial-hybridreverb) | Prepared convolution early response combined with an independently controlled FDN tail. |
| [NeuralHrtf.h](doc:api-spatial-neuralhrtf) | Experimental native directional HRTF field and prepared binaural renderer. |
| [Sofa.h](doc:api-spatial-sofa) | Optional memory-only SOFA import, float64 preparation and portable HRTF cache. |
| [SpatialMath.h](doc:api-spatial-spatialmath) | Metre/radian source/listener geometry, media, distance, Doppler, directivity and material response. |
| [SpatialRendering.h](doc:api-spatial-spatialrendering) | Analytic/measured binaural, shared HRTF preparation, VBAP and order-7 ACN/SN3D Ambisonics. |
