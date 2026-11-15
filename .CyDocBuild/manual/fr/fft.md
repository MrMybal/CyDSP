---
id: fft
title: FFT, fenêtres et normalisation
description: Utiliser les transformations complexes/réelles sans confondre amplitude et PSD.
section: Guides
order: 19
sourceRevision: 1
parent: overview
---

## Convention de transformation {#transform}

La transformation directe utilise `exp(-i*2*pi*k*n/N)` sans normalisation. L’inverse emploie le signe opposé et divise par N. RealFft expose N/2+1 bins, de DC à Nyquist. Utilisez des longueurs supportées en puissance de deux et les dimensions de spans préparées. Le recouvrement suit l’API propre à la transformation, pas les règles ordinaires des blocs audio.

ComplexFft possède des tables préparées immuables et peut être partagé entre appels indépendants avec buffers distincts si le cycle de vie est sérialisé. RealFft possède un scratch mutable et exige des instances séparées pour les appels simultanés.

## Fenêtres et niveaux {#windows}

makeWindow expose type de fenêtre, choix périodique/symétrique et normalisation. Une fenêtre adaptée à l’affichage spectral ne convient pas automatiquement à la reconstruction par recouvrement-addition. Respectez la normalisation FFT pour interpréter les bins ou multiplier les spectres.

Une amplitude brute n’est ni un niveau dBFS ni une PSD. SpectrumAnalyzer normalise l’amplitude par la somme de fenêtre et la PSD par fréquence/énergie de fenêtre ; voir l’[analyse](doc:analysis). STFT gère le recouvrement-addition pondéré et sa latence explicite dans le [guide spectral](doc:spectral).

## API publique {#api}

- [cydsp/math/Window.h](doc:api-math-window)
- [cydsp/processors/ComplexFft.h](doc:api-processors-complexfft)
- [cydsp/processors/RealFft.h](doc:api-processors-realfft)
