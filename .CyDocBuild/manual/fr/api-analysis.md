---
id: api-analysis
title: Analyse audio
description: Analyse audio
section: Référence API
order: 1008
sourceRevision: 1
parent: api
---

| En-tête | Rôle |
| --- | --- |
| [ConstantQAnalyzer.h](doc:api-analysis-constantqanalyzer) | Projections complexes directes de longueurs variables, avec budgets de taille des noyaux explicites. |
| [Diagnostics.h](doc:api-analysis-diagnostics) | Réponse fréquence/phase, transfert/cohérence, distorsion et sweeps de mesure. |
| [ImpulseAnalysis.h](doc:api-analysis-impulseanalysis) | Onset, premières réflexions, décroissance/RT60 et correspondance nominale FDN depuis une IR en mémoire. |
| [LevelMeter.h](doc:api-analysis-levelmeter) | Peak échantillon, RMS, DC, facteur de crête et corrélation stéréo sur fenêtre glissante. |
| [LoudnessMeter.h](doc:api-analysis-loudnessmeter) | Sonie pondérée K momentanée, courte et intégrée, avec historique exact borné. |
| [MusicalAnalyzer.h](doc:api-analysis-musicalanalyzer) | Estimations YIN de hauteur/confiance, attaques, tempo/battements, chroma, MFCC et tonalité. |
| [SpectrumAnalyzer.h](doc:api-analysis-spectrumanalyzer) | Spectres d’amplitude et PSD par FFT réelle, caractéristiques et bandes Mel/logarithmiques. |
| [TruePeakMeter.h](doc:api-analysis-truepeakmeter) | Estimation suréchantillonnée des crêtes interéchantillons avec retard de reconstruction explicite. |
