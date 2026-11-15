---
id: binaural
title: Binaural and loudspeaker rendering
description: Select analytic binaural, measured HRIR or VBAP layouts.
section: Guides
order: 36
revision: 1
parent: spatial
---

## Headphone rendering {#headphones}

AnalyticBinaural models spherical-head ITD, head shadow, front/back coloration and near-ear effects for up to 32 placed inputs. Source radius reduces directional detail. Two inputs can represent an extended stereo emitter. It is a parametric model, not a measured or listener-personalized HRTF.

HrirBinaural renders measured responses. Each raw HrirPoint supplies onset-free left/right FIRs and separate ear delays at the prepared rate. Keeping acoustic onset inside FIRs while also applying separate delays can cause combing or doubled delay. Prefer a [prepared HrtfSet](doc:hrtf) when converting or sharing measurements.

setDirection interpolates aligned filters and delays, then fades readers. A request during a fade returns transition_in_progress. Retain the previous successful direction until the request can be accepted; do not block the audio thread waiting.

## Speaker layouts {#speakers}

VbapLayout prepares 2D pairs or 3D convex-hull triples for 2–64 loudspeakers and returns unit-energy gains. Outside coverage, choose error or nearest-speaker fallback during preparation. A sparse/poorly distributed layout cannot reproduce every requested direction equally.

Choose [Ambisonics](doc:ambisonics) when sources should share an intermediate sound field. Panning and binaural cues are different rendering choices; applying both unintentionally can alter level and localization.

## Public API {#api}

- [cydsp/spatial/SpatialRendering.h](doc:api-spatial-spatialrendering)
