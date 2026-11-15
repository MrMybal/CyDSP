---
id: api-processors
title: Processeurs audio
description: Processeurs audio
section: Référence API
order: 1004
sourceRevision: 1
parent: api
---

| En-tête | Rôle |
| --- | --- |
| [AdaptiveFilter.h](doc:api-processors-adaptivefilter) | Adaptation LMS/NLMS préparée des coefficients à partir du signal et de la référence souhaitée. |
| [AdvancedDynamics.h](doc:api-processors-advanceddynamics) | Saturation ADAA, EQ dynamique, de-essing, transitoires, dynamique multibande et limitation true peak. |
| [AdvancedEffects.h](doc:api-processors-advancedeffects) | Échos multitap filtrés/modulés, modulation d’amplitude, réverbérations à plaque et ressort. |
| [AdvancedFilterDesign.h](doc:api-processors-advancedfilterdesign) | Conceptions de bandes d’ordre élevé, égalisation FIR et conversion FIR en phase minimale. |
| [AsyncConvolver.h](doc:api-processors-asyncconvolver) | Un producteur prépare les IR pendant qu’un consommateur audio utilise les plans conservés sans attente. |
| [AudioGraph.h](doc:api-processors-audiograph) | Traitement de DAG monobus préparé, scratch conservé et callbacks empruntés. |
| [BandGain.h](doc:api-processors-bandgain) | Gains de propagation trois bandes préparés sur les fondations de filtrage existantes. |
| [Biquad.h](doc:api-processors-biquad) | Coefficients normalisés stables, conceptions paramétrique/shelving et biquad multicanal préparé. |
| [BiquadBank.h](doc:api-processors-biquadbank) | Coefficients et historiques indépendants par voix, avec SIMD entre voix indépendantes. |
| [BiquadCascade.h](doc:api-processors-biquadcascade) | Cascade de sections biquad validées avec historiques indépendants par canal. |
| [ChannelMatrix.h](doc:api-processors-channelmatrix) | Matrices de routage indexées par sortie, rampes de gain et alias exacts entre canaux. |
| [ComplexFft.h](doc:api-processors-complexfft) | Transformations complexes avec sens direct/inverse explicite et sélection du backend. |
| [Convolver.h](doc:api-processors-convolver) | Sélection d’une stratégie de convolution préparée et de sa configuration de partitions. |
| [DelayLine.h](doc:api-processors-delayline) | Délai fractionnaire préparé avec changements de retard contrôlés. |
| [Diffusion.h](doc:api-processors-diffusion) | Diffusion par passe-tout de Schroeder et mixage orthogonal Hadamard/Householder. |
| [Dynamics.h](doc:api-processors-dynamics) | Compresseur, expandeur et gate avec détection peak/RMS, sidechain et anticipation préparée. |
| [EchoCanceller.h](doc:api-processors-echocanceller) | Chemins NLMS indépendants par microphone, référence mono alignée et gel lié en double parole. |
| [Effects.h](doc:api-processors-effects) | Chorus, phaser et réverbération de type Schroeder utilisant l’API préparée commune. |
| [FilterBank.h](doc:api-processors-filterbank) | Bandes FIR complémentaires dont la somme neutre reconstruit l’entrée retardée. |
| [FilterDesign.h](doc:api-processors-filterdesign) | Conception passe-bas/haut Butterworth, Chebyshev I/II, elliptique et Bessel. |
| [Fir.h](doc:api-processors-fir) | Filtrage FIR causal ; le tap zéro multiplie l’échantillon courant. |
| [Gain.h](doc:api-processors-gain) | Gain linéaire avec rampe avançant une fois par frame pour tous les canaux. |
| [Generators.h](doc:api-processors-generators) | Oscillateurs et enveloppes ADSR avec traitement par échantillon et par bloc. |
| [Hilbert.h](doc:api-processors-hilbert) | Signaux en quadrature FIR à retard commun pour le traitement du signal analytique. |
| [HybridConvolver.h](doc:api-processors-hybridconvolver) | Tête directe immédiate et queue FFT sans latence de bloc ajoutée. |
| [MatrixConvolver.h](doc:api-processors-matrixconvolver) | Routes d’impulsions MIMO avec transformations d’entrée et couverture des sorties partagées. |
| [Mixing.h](doc:api-processors-mixing) | Panoramique, lois dry/wet, mise en forme d’onde élémentaire et décalage continu. |
| [MultiBusGraph.h](doc:api-processors-multibusgraph) | Bus explicites, sidechains, alignement de latence et transitions entre DAG préparés. |
| [Multirate.h](doc:api-processors-multirate) | Conversion de fréquence FIR entière et bancs dyadiques d’analyse/synthèse. |
| [MultiTapDelay.h](doc:api-processors-multitapdelay) | Taps entrée/sortie bornés, fondu de configurations et délai fractionnaire Thiran séquentiel. |
| [NonuniformConvolver.h](doc:api-processors-nonuniformconvolver) | Partitions précoces et tardives de tailles différentes pour réduire le coût des longues IR. |
| [Oversampler.h](doc:api-processors-oversampler) | Voies montantes/descendantes FIR, FIR demi-bande ou IIR préparées, avec leur latence. |
| [PartitionedConvolver.h](doc:api-processors-partitionedconvolver) | Partitions FFT uniformes avec latence de bloc explicite et queue finie. |
| [PitchEffects.h](doc:api-processors-pitcheffects) | Correction vers une hauteur cible et harmonisation multivoix par moteurs à délais ou spectraux. |
| [PolynomialInterpolator.h](doc:api-processors-polynomialinterpolator) | Lecteurs d’échantillons polynomiaux Lagrange/Catmull–Rom. |
| [RealFft.h](doc:api-processors-realfft) | Transformations réelles exposant les bins de DC à Nyquist, avec inverse normalisée. |
| [Resampler.h](doc:api-processors-resampler) | Conversion sinc à taux fixe/variable avec comptes de consommation/production obligatoires. |
| [Restoration.h](doc:api-processors-restoration) | Atténuation du bruit spectral, suppression du ronflement et détection/réparation bornée des clics. |
| [ScheduledConvolver.h](doc:api-processors-scheduledconvolver) | Répartition du travail FFT/MAC tardif sur les frames d’entrée avec attaque immédiate. |
| [SincInterpolator.h](doc:api-processors-sincinterpolator) | Interpolation sinc fenêtrée préparée pour les positions fractionnaires. |
| [SmoothedBiquad.h](doc:api-processors-smoothedbiquad) | Fondu entre états de filtres stables, sans interpolation de coefficients potentiellement instables. |
| [SmoothedBiquadCascade.h](doc:api-processors-smoothedbiquadcascade) | Transitions de cascades préparées avec regroupement de la cible suivante. |
| [SmoothedConvolver.h](doc:api-processors-smoothedconvolver) | Préparation d’une nouvelle IR hors thread audio, préchauffage et fondu de plans conservés. |
| [TimeStretch.h](doc:api-processors-timestretch) | Étirement de durée, hauteur et formants optionnel dans la cible CyDSP::Stretch. |
| [TptFilter.h](doc:api-processors-tptfilter) | Filtres TPT à variables d’état et ladder, séparation Linkwitz–Riley et conceptions complémentaires. |
