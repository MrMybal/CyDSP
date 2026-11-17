# CyDSP 0.4.5


**EN DÉVELOPPEMENT.** Distribution sous forme de sources ; aucune release binaire publiée.

## Changements

- 0.4.5 : retrait optionnel du délai acoustique commun minimal pendant la préparation HRTF. Un même décalage pour toute la banque conserve les écarts entre oreilles et directions ; `removedCommonDelayFrames()` expose l’avance réelle en frames au taux cible. Le timing de mesure reste conservé par défaut.

- 0.4.4 : Cascades neuronales spécialisées en double précision sur les noyaux SIMD CyDSP existants ; rendu explicite par groupes de quatre voix et mises à jour de designs groupées. Les cibles en fondu rejouent par défaut 512 échantillons d’entrée conservés (réglage 0–65536) ; poids et réponse statique inchangés. Recompiler ensemble bibliothèque statique et consommateurs.

- 0.4.3 : Interpolation HRTF sans allocation ; rendu HRIR direct/FFT/automatique avec historique d’entrée partagé, fondus entre filtres et sans latence de bloc ajoutée. Les deux préparations HOA fournissent les énergies de référence/décodage en champ diffus et un gain commun de compensation. Les moindres carrés causaux conservent leur faible latence ; le MagLS spectral conserve sa demi-FFT de retard déclarée.

- 0.4.2 : API HRTF neuronale native expérimentale : modèle partagé immuable, banques bornées en mémoire, prédiction SIMD, ITD compensé, cascades binaurales en double précision et fondus complets. Outils optionnels d’entraînement/export/validation à 48/96 kHz ; poids expérimentaux séparés.

- 0.4.1 : Sélection SOFA SADIE II originale optionnelle : KU100, KEMAR et un auditeur humain à 48 kHz, 81,38 Mo de mesures. Données/notices Apache-2.0 distinctes du code MIT. Installation explicite, contrôles d’intégrité et téléchargement séparé d’un corpus 96 kHz ignoré pour l’entraînement ; aucun renderer neuronal distribué ni changement de l’API de rendu.

- 0.4.0: Import SOFA optionnel depuis la mémoire, cache HRTF, égalisation en champ diffus, normalisation commune, index des directions voisines et profils de qualité HRTF explicites. Cœur MIT ; notices des lecteurs optionnels fournies.
- 0.4.0: Conception HOA MagLS/pondérée originale, max-rE optionnel, génération de banques HRIR analytiques et grille procédurale de Fibonacci ; latence et diagnostics numériques explicites.

- 0.3.2 : fusion SIMD du drive, du wavefolder direct et du mixage sur x64/ARM64. Compensation DC mise en cache lorsque la forme ou le biais change.
- 0.3.2 : quotients et calcul en dB du transient précis vectorisés, en conservant chaque appel scalaire à `std::pow(10, exposant)` et son exposant d’origine. Le mode précis NEON à deux voies conserve la boucle scalaire d’origine, plus rapide sur le Quest 3. EQ dynamique par paires de canaux indépendants, avec les mêmes récurrences doubles et le cache des coefficients/constantes inchangés.
- 0.3.2 : convolution sans copie d’entrée lorsque les buffers sont disjoints, sans double effacement des sorties réécrites et sans revalidation des vues internes déjà vérifiées. Publication SIMD des recouvrements des partitions ordinaires. Latence nulle, routage, alias et budgets de travail conservés.
- Signatures de traitement et qualité par défaut inchangées. Recompiler ensemble l’archive et ses consommateurs : la disposition privée de `MatrixConvolver` change. La tentative de fusion supplémentaire de la tanh a été écartée après un ralentissement sur les deux CPU mesurés ; le chemin par tuiles est conservé.

- 0.3.1 : partage des courbes de saturation et de dynamique entre AVX2, AVX-512, NEON ARM64 et SVE à longueur vectorielle variable. Ajout du cubic SIMD (direct et ADAA) et de l’évaluation vectorielle de tanh pour l’hystérésis, en conservant sa mémoire séquentielle. API publiques et politiques de qualité par défaut inchangées. NEON conserve la primitive scalaire mise en cache pour le cubic ADAA, plus rapide sur Quest ; son chemin direct utilise une passe SIMD fusionnée.
- 0.3.1 : fusion des travaux FFT progressifs avec une borne de 64 papillons, simplification du remplissage des partitions réelles partielles et publication vectorielle du recouvrement. Latence et budgets de travail conservés.

- Version 0.3.1. Le numéro de paquet était resté à 0.2.0 malgré plusieurs ajouts d’API ; il couvre désormais tout ce qui est listé ci-dessous. `find_package` n’accepte qu’une même version mineure : les consommateurs demandent `CyDSP 0.3.1` et recompilent leurs archives avec ces en-têtes. `cydsp/core/Version.h` donne la version à la compilation.
- Ajout de formes par bloc qui renvoient les mêmes échantillons que leurs appels unitaires : `Adsr::process` et `SampleData::render` (un seul choix de niveau mip par bloc, bouclage et fondu de boucle compris). `SpscQueue` avance ses index sans division entière.

- Saturation et dynamique évaluées par tuiles de 64 frames avec des noyaux x64 génériques en largeur (quatre doubles en AVX2, huit en AVX-512). Les ADAA tanh et atan renvoient la moyenne exacte entre échantillons consécutifs (erreur absolue proche de 1e-15, sans repli scalaire) ; les dynamiques non liées, avec sidechain, lookahead, expandeur et gate, l’égaliseur dynamique et le gain transitoire `fast` utilisent des courbes de transfert vectorielles ; la validation des entrées est sans branche. Résultats : [rapport des noyaux par tuiles](docs/TileKernelOptimization.fr.md).
- Ajout de l’option `ConvolutionConfig::spectralHead` pour MatrixConvolver : les partitions précoces entières convoluent leurs premiers taps avec les transformées du niveau précoce au lieu de la FIR directe, sans latence.
- Permutation de la FFT fusionnée avec ses deux premiers étages, fusion et mise à l’échelle vectorielles de l’inverse réelle, noyaux par plage dans les niveaux de convolution progressifs. Résultats identiques bit à bit à la transformée de référence.

- Ajout des sources groupées SIMD et banques partagées, préparation HRTF (onsets, alignement, conversion et interpolation), Ambisonics ordre 7 et rendu binaural HOA, diffraction, analyse détaillée d’IR, séparation harmonique/percussive, pitch spectral/formants, contacts physiques, annulation d’écho avec double parole et profils de qualité avec transitions préparées. Voir le [guide des API avancées](docs/AdvancedExpansionGuide.fr.md).

- Extension des dix lots DSP/3D : fondations, graphes multibus/PDC, propagation, pièces/réverbérations, binaural/VBAP/HOA, dynamique avancée, effets/synthèse, analyse musicale/physique et restauration/diagnostic. API originales MIT ; voir les [contrats et limites](docs/ExpansionGuide.fr.md).

- Ajout des API originales de synthèse granulaire et d’analyse audio : niveaux/corrélation, true peak, spectres/caractéristiques, bandes Mel/log, constant-Q et LUFS/LRA. Réutilisation des noyaux numériques CyDSP et du stockage préparé borné ; voir les [contrats et limites](docs/GranulationAndAnalysis.md).

- Bibliothèque de calcul DSP autonome, accessible par le namespace `cydsp`, les en-têtes publics `cydsp/` et les cibles CMake exportées.
- Sélection des bandes harmoniques des tables d’ondes une fois par bloc, calcul direct des gains du gate et réduction des calculs répétés de dynamique.
- Optimisation des copies, mises à l’échelle, accumulations pondérées et remplissages AVX-512, avec masquage des échantillons restants.
- Ajout de mesures répétées avant/après et de régressions indépendantes pour les résultats numériques, le cycle de vie et le temps réel. Les résultats et limites des cas mesurés figurent dans [Measurements.md](docs/Measurements.md).

## Plateformes et limites connues

Les régressions ont été exécutées sous Windows x64 MSVC/clang-cl et Android ARM64 sur Quest 3. Windows ARM64 dispose d’une couverture de compilation statique. SVE compile et se lie pour Android ARM64, mais son exécution et les différentes longueurs vectorielles exigent un matériel compatible. L’exécution sur macOS, Linux desktop et Windows ARM64 reste à vérifier ; x86 et ARM32 n’ont pas été retestés pour cet état. Voir [Validation.md](docs/Validation.md).

CyDSP fournit le traitement audio numérique au moyen de son API C++ publique. Les performances dépendent du CPU, du backend, des dimensions des blocs et des réglages ; les gains mesurés ne s’appliquent pas à l’ensemble de la bibliothèque.


Une sélection SADIE II optionnelle est fournie sous Apache-2.0, séparément du code MIT ; son installation est désactivée par défaut. Les autres jeux HRTF sont apportés par l’hôte. La diffraction utilise une approximation d’écran mince, l’annulation d’écho suppose un chemin linéaire et une référence alignée ; les formants, transitoires et analyses musicales demandent une évaluation selon les sources. Les [contrats avancés](docs/AdvancedExpansionGuide.fr.md) détaillent ces limites.

Aucune liste de release détaillée bug par bug n’est publiée. Signaler les problèmes reproductibles dans le [suivi des problèmes](https://github.com/MrMybal/CyDSP/issues).
