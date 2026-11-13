# CyDSP 0.2.0

**EN DÉVELOPPEMENT.** Distribution sous forme de sources ; aucune release binaire publiée.

## Changements

- Ajout des sources groupées SIMD et banques partagées, préparation HRTF (onsets, alignement, conversion et interpolation), Ambisonics ordre 7 et rendu binaural HOA, diffraction, analyse détaillée d’IR, séparation harmonique/percussive, pitch spectral/formants, contacts physiques, annulation d’écho avec double parole et profils de qualité avec transitions préparées. Voir le [guide des API avancées](docs/AdvancedExpansionGuide.fr.md).

- Extension des dix lots DSP/3D : fondations, graphes multibus/PDC, propagation, pièces/réverbérations, binaural/VBAP/HOA, dynamique avancée, effets/synthèse, analyse musicale/physique et restauration/diagnostic. API originales MIT ; voir les [contrats et limites](docs/ExpansionGuide.fr.md).

- Ajout des API originales de synthèse granulaire et d’analyse audio : niveaux/corrélation, true peak, spectres/caractéristiques, bandes Mel/log, constant-Q et LUFS/LRA. Réutilisation des noyaux numériques CyDSP et du stockage préparé borné ; voir les [contrats et limites](docs/GranulationAndAnalysis.md).

- Séparation du calcul numérique dans CyDSP et des services audio dans CyAudioCore, avec en-têtes, namespaces et cibles CMake distincts. Voir le [guide de migration](docs/Migration.fr.md).
- Sélection des bandes harmoniques des tables d’ondes une fois par bloc, calcul direct des gains du gate et réduction des calculs répétés de dynamique.
- Optimisation des copies, mises à l’échelle, accumulations pondérées et remplissages AVX-512, avec masquage des échantillons restants.
- Ajout de mesures répétées avant/après et de régressions indépendantes pour les résultats numériques, le cycle de vie et le temps réel. Les résultats et limites des cas mesurés figurent dans [Measurements.md](docs/Measurements.md).

## Plateformes et limites connues

Les régressions ont été exécutées sous Windows x64 MSVC/clang-cl et Android ARM64 sur Quest 3. Windows ARM64 dispose d’une couverture de compilation statique. SVE compile et se lie pour Android ARM64, mais son exécution et les différentes longueurs vectorielles exigent un matériel compatible. L’exécution sur macOS, Linux desktop et Windows ARM64 reste à vérifier ; x86 et ARM32 n’ont pas été retestés pour cet état. Voir [Validation.md](docs/Validation.md).

CyDSP fournit le calcul numérique. Les périphériques audio, le MIDI, les formats de fichiers, le transport et les interfaces de plugins relèvent de CyAudioCore. Les applications existantes doivent s’adapter à la séparation des packages et namespaces de la version 0.2.0. Les performances dépendent du CPU, du backend, des dimensions des blocs et des réglages ; les gains mesurés ne s’appliquent pas à l’ensemble de la bibliothèque.


Les jeux de données HRTF restent fournis par l’hôte. La diffraction utilise une approximation d’écran mince, l’annulation d’écho suppose un chemin linéaire et une référence alignée ; les formants, transitoires et analyses musicales demandent une évaluation selon les sources. Les [contrats avancés](docs/AdvancedExpansionGuide.fr.md) détaillent ces limites.

Aucune liste de release détaillée bug par bug n’est publiée. Signaler les problèmes reproductibles dans le [suivi des problèmes](https://github.com/MrMybal/CyDSP/issues).
