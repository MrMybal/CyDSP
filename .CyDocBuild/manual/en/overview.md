---
id: overview
title: CyDSP 0.4.5
description: Choose and combine numerical audio processors.
section: Guides
order: 0
revision: 1
---

## What you can build {#purpose}

CyDSP is a C++20 audio digital signal processing library. Use its float/double processors in an effect, synthesizer, DAW, game or audio engine. It supplies filters, dynamics, convolution, spectral processing, synthesis, sampling, analysis, restoration and 3D sound through prepared storage and a common planar API.

Start with [installation](doc:installation), then build a [first processing chain](doc:quickstart). Read the [processing contracts](doc:contracts) before connecting a realtime callback. The [API reference](doc:api) covers every public header, including configuration fields, exact signatures and default arguments.

## Find the right domain {#domains}

| Task | Guide |
| --- | --- |
| Filter, equalize and control levels | [Filters](doc:filters), [dynamics](doc:dynamics), [nonlinear processing](doc:advanced-dynamics) |
| Reverb or impulse processing | [Convolution](doc:convolution), [partition choices](doc:convolution-options), [reverb](doc:reverb) |
| Generate or transform a sound | [Synthesis](doc:synthesis), [samples](doc:sampling), [granulation](doc:granular), [pitch/time](doc:pitch) |
| Inspect or repair audio | [Meters](doc:metering), [analysis](doc:analysis), [diagnostics](doc:diagnostics), [restoration](doc:restoration) |
| Render game audio in space | [Coordinates](doc:spatial), [acoustics](doc:acoustics), [binaural](doc:binaural), [Ambisonics](doc:ambisonics) |

## Library integration {#integration}

CyDSP processes in-memory audio. Your application supplies the audio callback, stream ownership and surrounding services. CyAudioCore is one project using its public API. You can link CyDSP directly into another application.

Version 0.4.5 is in development. [Platform and model limits](doc:limits) distinguish available algorithms from targets requiring execution validation. Keep the [MIT and component notices](doc:licensing) with redistributed software.

## Public API {#api}

- [cydsp/CyDSP.h](doc:api-cydsp)
