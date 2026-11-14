<table width="100%">
  <tr>
    <td width="300" align="left" valign="top">
      <a href="docs/Modules.md">Documentation</a><br>
      <a href="docs/Migration.fr.md">Migration</a><br>
      <a href="https://github.com/MrMybal/CyDSP">Dépôt</a><br><br>
      <sub>README</sub><br>
      <a href="README.md">English</a> · <a href="README.fr.md">Français</a>
    </td>
    <td width="400" align="center" valign="middle"><strong>CyDSP</strong><br><sub>C++20 DSP · x64 / ARM64</sub></td>
    <td width="300" align="right" valign="top">
      <p><sub>Licence de la bibliothèque</sub><br><a href="LICENSE"><strong>MIT</strong></a></p>
      <p><sub>Version</sub><br><a href="VERSION.fr.md"><strong>0.3.1</strong></a><br><strong>EN DÉVELOPPEMENT</strong></p>
    </td>
  </tr>
</table>

<h1 align="center">CyDSP</h1>

Bibliothèque C++20 de calcul DSP audio pour les DAW, effets, synthétiseurs et jeux vidéo. Construire des chaînes de traitement, rendre de nombreuses sources et travailler le son spatial avec une API float/double commune, un stockage préparé et des implémentations SIMD pour x64 et ARM64.

> **EN DÉVELOPPEMENT · 0.3.1**
>
> Disponible sous forme de sources ; aucune release binaire publiée. Consulter les [notes de version et limites des plateformes](VERSION.fr.md).

## À propos de CyDSP

CyDSP fournit les briques numériques des applications audio : filtres et dynamique, synthèse et sampling, traitement spectral, analyse, restauration et son 3D. Combiner les vues planaires `AudioBlock`, le stockage aligné `AudioBuffer` et un `ProcessSpec` commun ; préparer les algorithmes et la mémoire avant le callback audio. Le traitement et la remise à zéro conservent le stockage préparé, sans allocation, verrou ou I/O dans ces chemins. Le [guide d’architecture](docs/Architecture.md) décrit la propriété des paramètres et le cycle de vie.

Les tables, samples et HRTF préparés peuvent être partagés entre sources. Les graphes gèrent les sidechains explicites, la compensation de latence entre branches et les transitions entre états préparés. Les profils économie, temps réel haute qualité et hors ligne fournissent une politique de préparation commune.

Les périphériques audio, le MIDI, les entrées/sorties de fichiers audio, le transport de lecture, l’hébergement de plugins et l’interface relèvent du moteur consommateur ou de **CyAudioCore**. CyDSP peut être utilisé indépendamment dans un DAW, un jeu, un effet ou un synthétiseur.

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

Les implémentations scalaire, SSE2, AVX2, AVX-512, NEON et SVE optionnel sont sélectionnées à la préparation selon le code compilé et les capacités CPU/OS. Un backend disponible peut être demandé explicitement. Le code propre aux ISA reste dans des fichiers d’implémentation privés.

L’[index des modules](docs/Modules.md), le [guide DSP/spatial](docs/ExpansionGuide.fr.md) et le [guide des API avancées](docs/AdvancedExpansionGuide.fr.md) précisent les limites, la propriété et les latences. Plusieurs modèles physiques et musicaux sont des approximations ; les choisir et les évaluer selon la source et la scène visées.

---

## Installation

Utiliser CMake 3.24+ et un compilateur C++20 pour produire les bibliothèques statiques de l’ABI cible. La cible principale est `CyDSP::CyDSP` ; `CyDSP::Stretch` ajoute le module d’étirement optionnel. Windows x64 et Android ARM64 disposent d’une couverture en exécution. Les autres limites de cibles et d’exécution SVE figurent dans les [notes de version](VERSION.fr.md).

Après installation du package CMake, le lier à l’application :

```cmake
find_package(CyDSP 0.3.1 CONFIG REQUIRED)

target_link_libraries(MyApplication PRIVATE CyDSP::CyDSP)
# Traitement durée/hauteur optionnel :
# target_link_libraries(MyApplication PRIVATE CyDSP::Stretch)
```

Les en-têtes publics se trouvent sous `cydsp/` ; `cydsp/CyDSP.h` les regroupe. Commencer avec l’[exemple de traitement](examples/Processing.cpp), l’[exemple granulation/analyse](examples/GranulationAndAnalysis.cpp), l’[exemple spatial/DAW](examples/SpatialAndDaw.cpp) ou l’[exemple HRTF partagées/sources groupées](examples/AdvancedAudio.cpp). Le [guide de migration](docs/Migration.fr.md) accompagne les applications existantes.

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

- [Index des modules et contrats d’API](docs/Modules.md) · [Architecture](docs/Architecture.md).
- [Granulation et analyse](docs/GranulationAndAnalysis.md) · [Intégration DSP/spatial](docs/ExpansionGuide.fr.md) · [API avancées](docs/AdvancedExpansionGuide.fr.md).
- [Guide de migration](docs/Migration.fr.md) · [Notes de version](VERSION.fr.md).
- [Dépôt des sources](https://github.com/MrMybal/CyDSP) · [Suivi des problèmes](https://github.com/MrMybal/CyDSP/issues).
