---
id: spectral
title: STFT et effets spectraux
description: Traiter callbacks spectraux, freeze, vocodage et séparation.
section: Guides
order: 20
sourceRevision: 1
parent: fft
---

## Reconstruction STFT {#stft}

Stft utilise un recouvrement-addition pondéré causal, un démarrage rembourré de zéros et une latence de N frames. La FFT est une puissance de deux dans [16,65536] ; le hop divise N et la fenêtre doit permettre la reconstruction. Sans callback, l’entrée retardée est reconstruite aux arrondis FFT près.

Le callback spectral reçoit les bins d’un canal ; il est noexcept, sans allocation, sérialisé par le propriétaire et doit laisser les bins finis. Les parties imaginaires de DC/Nyquist sont retirées. `flush` fournit 2N zéros puis signale end_of_stream ; reset précède un nouveau flux. L’application possède toujours le contexte.

## Effets et sorties complémentaires {#effects}

SpectralEffects fournit freeze, vocodeur, morphing et filtrage de bins. Entrées et sidechains respectent les formes du mode choisi ; l’état spectral conservé exige un préchauffage. FrequencyShifter utilise Hilbert en quadrature à retard commun : il ajoute des Hz plutôt que de multiplier la hauteur musicale.

HarmonicPercussive sépare par masques de médianes temps/fréquence et reconstruit des sorties harmonique/percussive dont la somme est complémentaire. L’historique des médianes ajoute latence et dépendance au démarrage. Choisissez fenêtre/hop/médianes selon le contenu ; c’est une heuristique de signal, pas une reconnaissance des sources. SpectralPitchShifter traite propagation de phase et hauteur/formants dans le guide [hauteur/durée](doc:pitch).

## API publique {#api}

- [cydsp/processors/Hilbert.h](doc:api-processors-hilbert)
- [cydsp/spectral/HarmonicPercussive.h](doc:api-spectral-harmonicpercussive)
- [cydsp/spectral/SpectralEffects.h](doc:api-spectral-spectraleffects)
- [cydsp/spectral/Stft.h](doc:api-spectral-stft)
