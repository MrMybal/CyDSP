---
id: ambisonics
title: Ambisonics and HOA binaural
description: Encode, rotate and decode ACN/SN3D orders zero through seven.
section: Guides
order: 38
revision: 1
parent: spatial
---

## Format and rotations {#format}

ambisonicCoefficients, ambisonicRotation and AmbisonicRenderer support orders 0–7, `(order+1)^2` channels: 1–64. Channel order is ACN and normalization SN3D. First-order order is W,Y,Z,X. Convert FuMa/N3D explicitly outside this API.

Rotations are active: encoding direction d then rotating equals encoding R*d. For listener orientation, supply the inverse rotation. Do not mix a world-to-listener coordinate transform with a second equivalent field rotation.

## Decode to speakers or ears {#decode}

Speaker decoding uses regularized least squares for the supplied layout. Choose regularization for poorly conditioned/underdetermined layouts. Extra mathematical channels cannot supply spatial resolution missing from sparse speakers; this is not an AllRAD or room-correction decoder.

HoaBinaural projects measured HRIRs into the harmonic field, embeds fractional acoustic delays into FIRs and prepares a two-ear MatrixConvolver with HOA inputs. Field rotation can precede convolution. Prepare all filters outside processing, retain shared HRTFs and include conversion/convolution latency and tail in graph timing.

```diagram
{"type":"flow","title":"HOA signal path","nodes":[{"id":"sources","label":"Sources","x":0,"y":0},{"id":"encode","label":"ACN/SN3D","x":190,"y":0},{"id":"rotate","label":"Rotation","x":390,"y":0},{"id":"ears","label":"HOA binaural","x":590,"y":0}],"edges":[{"from":"sources","to":"encode"},{"from":"encode","to":"rotate"},{"from":"rotate","to":"ears"}]}
```
