---
id: analysis
title: Analyse spectrale et musicale
description: Interpréter bandes FFT/PSD, constant-Q, hauteur et estimations musicales.
section: Guides
order: 29
sourceRevision: 1
parent: metering
---

## Spectre et unités des bandes {#spectrum}

SpectrumAnalyzer prépare une fenêtre périodique et une FFT réelle double même pour une entrée float. N est une puissance de deux 16–65536 ; le hop vaut 1–N. Le premier résultat suit une fenêtre complète, puis chaque hop. Les getters gardent la dernière frame complète ; un grand appel peut remplacer les frames intermédiaires. Les spans empruntés changent à l’analyse/reset et sont invalidés par nouvelle préparation réussie/destruction.

complexSpectrum contient les bins fenêtrés bruts. Les amplitudes intérieures valent `2*abs(X)/sum(window)` ; DC/Nyquist utilisent le facteur 1. La PSD unilatérale vaut `abs(X)^2/(Fs*sum(window^2))`, doublée pour les bins intérieurs. Sommez la PSD puis multipliez par Fs/N pour obtenir la moyenne quadratique pondérée par fenêtre. Carré d’amplitude et PSD moyenne sont des unités différentes.

SpectralBands construit des moyennes triangulaires Mel/log normalisées. Fournir une PSD donne une PSD moyenne, pas l’énergie intégrée de bande. Les bandes vides non représentables échouent à la préparation : augmentez la FFT ou réduisez les bandes. Les caractéristiques comprennent centroïde, rolloff, flatness, bin dominant et flux normalisé positif. Le bin dominant n’est pas une estimation affinée de fondamentale.

## Constant-Q et musique {#music}

ConstantQAnalyzer utilise des projections complexes directes à fenêtres Hann de longueurs variables. Les graves exigent des noyaux plus longs ; budgétez longueur totale, canaux et hop. Les résultats partagent une date de fin mais des retards centraux différents. Les défauts sont 55 Hz, 12 bins/octave, 96 bins et un hop de 512 frames.

MusicalAnalyzer utilise YIN accéléré par FFT pour hauteur/confiance monophoniques, puis attaques, tempo/battements, chroma, MFCC et tonalité. Choisissez N assez grand pour la hauteur minimale ; l’analyse commence après N échantillons. Les classes tonales vont de 0=do à 11=si. Tempo, attaques et tonalité sont heuristiques et exigent un contrôle de confiance. Un callback noexcept reçoit les caractéristiques complètes ; il suit le même contrat de propriétaire et d’absence d’allocation que le traitement.

## API publique {#api}

- [cydsp/analysis/ConstantQAnalyzer.h](doc:api-analysis-constantqanalyzer)
- [cydsp/analysis/MusicalAnalyzer.h](doc:api-analysis-musicalanalyzer)
- [cydsp/analysis/SpectrumAnalyzer.h](doc:api-analysis-spectrumanalyzer)
