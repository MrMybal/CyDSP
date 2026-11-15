---
id: restoration
title: Noise, hum and click restoration
description: Use spectral attenuation, notches, repair and adaptive filters.
section: Guides
order: 31
revision: 1
parent: overview
---

## Choose the disturbance model {#model}

SpectralDenoiser applies prepared spectral noise attenuation; Dehum removes configured hum/harmonic components; Declicker detects and repairs bounded impulsive disturbances. Supply parameters matching the disturbance and protect wanted transients. Strong attenuation or inaccurate noise assumptions can produce musical noise, dulling or false repairs.

AdaptiveFilter implements LMS/NLMS adaptation against a desired/reference signal. Choose step/leakage/normalization settings within accepted ranges, align both signals and monitor convergence. Long taps increase storage and per-sample work. A linear adaptive path cannot remove arbitrary nonlinear distortion.

## Processing and evaluation {#workflow}

Prepare before the callback, budget any analysis/reconstruction delay and drain spectral tails. Start with conservative thresholds, compare bypass at matched loudness, then assess desired transients, harmonics and silence. No automatic filesystem scan, recorded noise database or worker is implied. For loudspeaker leakage into microphones, use the explicit [echo-canceller reference contract](doc:echo).

## Public API {#api}

- [cydsp/processors/AdaptiveFilter.h](doc:api-processors-adaptivefilter)
- [cydsp/processors/Restoration.h](doc:api-processors-restoration)
