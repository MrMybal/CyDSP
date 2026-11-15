<table width="100%">
  <tr>
    <td width="300" align="left" valign="top">
      <a href="Documentation/Documentation.html#/fr/overview">Documentation</a><br>
      <a href="https://github.com/MrMybal/CyDSP">Dépôt</a><br><br>
      <sub>README</sub><br>
      <a href="README.md">English</a> · <a href="README.fr.md">Français</a>
    </td>
    <td width="400" align="center" valign="middle"><strong>CyDSP</strong><br><sub>C++20 DSP · x64 / ARM64</sub></td>
    <td width="300" align="right" valign="top">
      <p><sub>Licence de la bibliothèque</sub><br><a href="LICENSE"><strong>MIT</strong></a></p>
      <p><sub>Version</sub><br><a href="VERSION.fr.md"><strong>0.3.2</strong></a><br><strong>EN DÉVELOPPEMENT</strong></p>
    </td>
  </tr>
</table>

<h1 align="center">CyDSP</h1>

**CyDSP est une bibliothèque C++20 de traitement numérique du signal audio.** Elle fournit des algorithmes réutilisables pour les effets, l’égalisation, la synthèse, l’analyse audio et le son 3D, avec une API float/double commune et des implémentations SIMD pour x64 et ARM64.

> **EN DÉVELOPPEMENT · 0.3.2**
>
> Le dépôt des sources est actuellement privé. La bibliothèque se compile depuis une copie obtenue avec un accès autorisé ; consulter les [notes de version et limites des plateformes](VERSION.fr.md).

## À propos de CyDSP

CyDSP permet de filtrer, transformer, générer, analyser et spatialiser l’audio dans un DAW, un jeu vidéo, un plugin audio ou un moteur personnalisé. Ses processeurs partagent les mêmes vues audio planaires, conventions de préparation et briques numériques : FFT, filtres, convolution, rééchantillonnage, interpolation et opérations vectorielles.

Les processeurs et leur stockage se préparent avant le callback audio, puis traitent les blocs au moyen de `AudioBlock<float>` ou `AudioBlock<double>`. `ProcessSpec` décrit la fréquence d’échantillonnage, la taille maximale des blocs et le nombre de canaux ; `AudioBuffer` fournit un stockage propriétaire aligné. Le traitement et la remise à zéro réutilisent la mémoire préparée, sans allocation, verrou ni entrée/sortie. Le [guide des contrats de traitement](Documentation/Documentation.html#/fr/contracts) précise les durées de vie, les changements de paramètres et la propriété entre threads.

Les processeurs peuvent être assemblés directement ou dans des graphes DSP multibus avec sidechains explicites, compensation de latence et transitions entre états préparés. Les samples, tables d’ondes et HRTF préparés peuvent être partagés entre sources ; chaque processeur conserve son propre état de traitement mutable.

## Fonctionnalités

| Domaine | Traitements disponibles |
| --- | --- |
| **Filtres et égalisation** | Biquads/cascades, conceptions Butterworth, Chebyshev, elliptique et Bessel, FIR et EQ à phase linéaire, conversion FIR en phase minimale, filtres TPT/ladder, EQ dynamique, séparation en bandes complémentaires et filtres adaptatifs LMS/NLMS. |
| **Dynamique** | Compresseur, expandeur, gate avec maintien/hystérésis/anticipation, sidechain externe, compression multibande, de-esser, travail des transitoires et limiteur true peak suréchantillonné. |
| **FFT et traitement spectral** | FFT complexe/réelle, STFT publique analyse/synthèse, freeze spectral, vocodeur, morphing, filtrage spectral, décalage de fréquence et séparation harmonique/percussive à reconstruction complémentaire. |
| **Convolution et IR** | Convolution directe, partitionnée, non uniforme et MIMO partagé ; transitions d’IR préparées ; diagnostic des onsets, premières réflexions, clarté et décroissances/RT60 par bandes, avec correspondance nominale FDN. |
| **Fréquences d’échantillonnage** | Rééchantillonnage fixe/variable, interpolation sinc et polynomiale, primitives de délai fractionnaire/passe-tout, suréchantillonnage FIR/FIR demi-bande/IIR et génération de fenêtres. |
| **Délais et modulation** | Délais fractionnaires et multitap, échos ping-pong/filtrés, chorus, phaser, flanger, vibrato, trémolo, autopan et wow/flutter. |
| **Effets non linéaires** | Mise en forme d’onde, écrêtage, rectification, saturation ADAA, wavefolding, asymétrie, saturation multibande et modèle d’hystérésis borné. |
| **Réverbération** | Schroeder, réseaux FDN, plaque de type Dattorro, ressort et réverbérations hybrides IR/FDN, avec diffusion, amortissement, freeze et shimmer selon le moteur. |
| **Synthèse** | Oscillateurs/tables à bandes limitées, FM/PM à fréquence audio, synchronisation avec suréchantillonnage, morphing de tables, unisson, enveloppes ADSR/multisegments, bruit et LFO reproductibles. |
| **Sampling et granulation** | Banques de samples préparées, lecture avec réduction du repliement spectral, fondus de boucles, grains manuels et nuages reproductibles, historique d’entrée live, freeze, grains avant/arrière et routage multicanal. |
| **Synthèse physique** | Karplus–Strong, guides d’ondes de cordes/tubes, résonateurs modaux et banques de modes de membranes/plaques ; excitateurs d’impact, glissement, roulement et archet avec unités physiques explicites. |
| **Hauteur et durée** | Harmonisation/doubleur et correction monophonique de hauteur ; transposition spectrale avec conservation des formants, phases de canaux liées et protection des transitoires ; traitement durée/hauteur/formants optionnel via `CyDSP::Stretch`. |
| **Analyse audio et musicale** | Peak/RMS/DC, corrélation stéréo, facteur de crête, true peak, LUFS/LRA, spectre/caractéristiques, bandes Mel/log, constant-Q, fondamentale/confiance YIN, attaques, tempo/battements, chroma, MFCC et estimation de tonalité. |
| **Restauration et diagnostic** | Réduction de bruit spectrale, traitement du ronflement/clics et atténuation de résonances persistantes ; réponse fréquence/phase/retard de groupe, transfert/cohérence, THD/THD+N, signaux de mesure et déconvolution de sweeps. |
| **Annulation d’écho acoustique** | Référence distante mono, chemins NLMS indépendants par microphone, gel lié en double parole, contrôles d’adaptation et métriques de diagnostic. |
| **Son 3D et spatialisation** | Distance/directivité, atmosphère/milieux, Doppler, chemins acoustiques, pièces/portails, binaural analytique, HRTF partagées avec alignement des onsets/conversion/interpolation, VBAP, Ambisonics ACN/SN3D jusqu’à l’ordre 7, binaural HOA et approximation de diffraction sur écran mince. |
| **Infrastructure temps réel** | Lots de sources avec récurrences sinus SIMD et données partagées, mixage vectoriel, buffers/rampes, événements de paramètres à l’échantillon, graphes multibus, alignement sidechain/PDC, fondus de graphes et profils de qualité communs. |

## Traitement temps réel et architectures CPU

CyDSP se compile en bibliothèque statique pour l’architecture cible. Les noyaux scalaires, SSE2, AVX2 et AVX-512 servent les cibles x64 ; NEON et SVE optionnel servent les cibles ARM64. La sélection se fait à la préparation selon le code compilé et les capacités CPU/OS. Un backend disponible peut aussi être demandé explicitement.

Le SIMD s’applique aux opérations adaptées, tandis que les filtres récursifs et les enveloppes conservent l’ordre de leurs états de traitement. Les réglages de qualité et de précision appartiennent aux processeurs concernés ; la bibliothèque portable conserve une compilation flottante précise, sans fast-math ni option globale propre au CPU local.

Windows x64 et Android ARM64 disposent d’une couverture en exécution. Windows ARM64 dispose d’une couverture de compilation statique ; l’exécution SVE, macOS et Linux desktop demandent encore une validation sur cible. Les [notes de version](VERSION.fr.md) précisent les limites actuelles des plateformes.

## Projets utilisant CyDSP

**CyAudioCore** utilise CyDSP pour ses traitements audio numériques. Les applications et plugins peuvent également utiliser CyDSP directement au moyen de son API C++ publique.

Le [manuel CyDoc](Documentation/Documentation.html#/fr/overview) décrit les contrats des processeurs, les configurations prises en charge et les latences, avec une [référence des API C++ publiques](Documentation/Documentation.html#/fr/api) et des [guides de son spatial](Documentation/Documentation.html#/fr/spatial). Les jeux de données HRTF sont fournis par l’application ; les modèles physiques et musicaux ont les limites précisées dans ces guides.

---

## Installation

Utiliser CMake 3.24+ et un compilateur C++20 pour produire les bibliothèques statiques de l’ABI cible. La cible principale est `CyDSP::CyDSP` ; `CyDSP::Stretch` ajoute le module d’étirement optionnel. Windows x64 et Android ARM64 disposent d’une couverture en exécution. Les autres limites de cibles et d’exécution SVE figurent dans les [notes de version](VERSION.fr.md).

Après installation du package CMake, le lier à l’application :

```cmake
find_package(CyDSP 0.3.2 CONFIG REQUIRED)

target_link_libraries(MyApplication PRIVATE CyDSP::CyDSP)
# Traitement durée/hauteur optionnel :
# target_link_libraries(MyApplication PRIVATE CyDSP::Stretch)
```

Les en-têtes publics se trouvent sous `cydsp/` ; `cydsp/CyDSP.h` les regroupe. Commencer avec l’[exemple de traitement](examples/Processing.cpp), l’[exemple granulation/analyse](examples/GranulationAndAnalysis.cpp), l’[exemple spatial/DAW](examples/SpatialAndDaw.cpp) ou l’[exemple HRTF partagées/sources groupées](examples/AdvancedAudio.cpp).

## Sources, compilation et bibliothèques nécessaires

Le dépôt inclut cinq adaptations numériques MIT figées et leurs notices complètes. La configuration ne télécharge aucune dépendance. C++20 et CMake sont nécessaires à l’utilisation de la bibliothèque ; Python sert uniquement aux scripts de compilation/validation du dépôt.

Depuis un accès authentifié au dépôt, compiler et installer les bibliothèques statiques Windows x64 :

```sh
git clone https://github.com/MrMybal/CyDSP.git
cd CyDSP
cmake -S . -B build/release -G "Visual Studio 17 2022" -A x64
cmake --build build/release --config Release
cmake --install build/release --config Release --prefix build/install
```

Définir `CMAKE_PREFIX_PATH` dans l’application vers le préfixe d’installation choisi. Définir `CYDSP_BUILD_STRETCH=OFF` si seul le socle est nécessaire. Les tests, mesures et vérifications de compilation sont activés explicitement ; les builds de production créent des bibliothèques statiques. La bibliothèque portable ne requiert pas d’option d’architecture globale pour le CPU local.

---

## Licence

CyDSP utilise la [licence MIT](LICENSE), copyright © 2026 MrMybal CyberAlien. Ses conditions permettent un usage commercial dans des produits à code fermé. Conserver les [notices tierces complètes](docs/Licensing.md) ; le tableau ci-dessous sert d’index. L’adaptation Stretch est utilisée uniquement par la cible optionnelle `CyDSP::Stretch`.

| Bibliothèque · sources | Licence · texte | Utilisation / périmètre |
| --- | --- | --- |
| [DSPFilters](https://github.com/vinniefalco/DSPFilters) | [MIT](third_party/licenses/DSPFilters.txt) | Équations de coefficients/prototypes adaptées à la préparation CyDSP ; traitement par ses filtres et cascades. |
| [FFTConvolver](https://github.com/HiFi-LoFi/FFTConvolver) | [MIT](third_party/licenses/FFTConvolver.txt) | Partitionnement et addition avec recouvrement adaptés à la convolution continue et MIMO, avec FFT et noyaux vectoriels CyDSP. |
| [r8brain-free-src](https://github.com/avaneev/r8brain-free-src) | [MIT](third_party/licenses/r8brain-free-src.txt) | Équations sinc fenêtré et fenêtres adaptées au rééchantillonnage CyDSP ; ce n’est pas l’algorithme SRC amont complet. |
| [signalsmith-linear](https://github.com/Signalsmith-Audio/linear) | [MIT](third_party/licenses/signalsmith-linear.txt) | Reconstruction FFT et normalisation STFT/fenêtres adaptées au socle numérique CyDSP. |
| [signalsmith-stretch](https://github.com/Signalsmith-Audio/signalsmith-stretch) | [MIT](third_party/licenses/signalsmith-stretch.txt) | Algorithme durée/hauteur/formants adapté dans la cible optionnelle `CyDSP::Stretch` ; fichiers amont conservés comme provenance. |

## Références et liens

Ouvrir `Documentation/Documentation.html` depuis une copie locale du dépôt pour consulter le manuel CyDoc complet hors ligne. Conserver tout le dossier `Documentation/` ; GitHub affiche le code HTML sans exécuter le lecteur.

- [Manuel CyDoc complet](Documentation/Documentation.html#/fr/overview) — guides et API C++ publique.
- [Contrats de traitement](Documentation/Documentation.html#/fr/contracts) · [API C++ publique](Documentation/Documentation.html#/fr/api).
- [Granulation](Documentation/Documentation.html#/fr/granular) · [Analyse audio](Documentation/Documentation.html#/fr/analysis) · [Son spatial](Documentation/Documentation.html#/fr/spatial).
- [Notes de version](VERSION.fr.md).
- [Dépôt des sources](https://github.com/MrMybal/CyDSP) · [Suivi des problèmes](https://github.com/MrMybal/CyDSP/issues).
