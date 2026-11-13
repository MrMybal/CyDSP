# Deuxième extension de CyDSP : dix lots

Travail accepté le 13 novembre 2026. Ce plan est distinct des dix lots de
[la première extension](ExpansionPlan.fr.md), dont les résultats restent historiques.
Code original MIT, sans import de code JUCE, Steam Audio ou IEM.

| Lot | Livraison | État |
| --- | --- | --- |
| 1 | Sources nombreuses : oscillateurs SIMD, grains, routage 3D et données partagées | Implémenté et validé |
| 2 | Préparation HRTF : onset, alignement, conversion de fréquence, interpolation | Implémenté et validé |
| 3 | Ambisonics ordre 7, décodage irrégulier et binaural HOA | Implémenté et validé |
| 4 | Diffraction fréquentielle pilotée par les trajets de l’hôte | Implémenté et validé |
| 5 | Analyse IR : décroissances, réflexions, clarté et correspondance FDN | Implémenté et validé |
| 6 | Séparation harmonique/percussive avec reconstruction | Implémenté et validé |
| 7 | Transposition spectrale : formants, stéréo et transitoires | Implémenté et validé |
| 8 | Excitateurs physiques : contact, frottement, roulement et archet | Implémenté et validé |
| 9 | Annulation d’écho : référence, double parole, adaptation contrôlée | Implémenté et validé |
| 10 | Profils de qualité communs et transitions préparées | Implémenté et validé |

Contrats : préparation transactionnelle ; traitement/reset sans allocation,
libération, verrou, I/O ou modification globale du processeur ; float et double ;
ISA privées. Tests numériques, segmentation des blocs, contrats temps réel,
mesures x64/Quest, compilation ARM64/SVE et installation indépendante réalisés :
voir [les résultats](AdvancedExpansionValidation.fr.md). Linux/macOS et exécution SVE différés.
