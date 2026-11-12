<table width="100%">
  <tr>
    <td width="300" align="left" valign="top">
      <a href="docs/Modules.md">Documentation</a><br>
      <a href="examples/Processing.cpp">Exemples de traitement</a><br>
      <a href="https://github.com/MrMybal/CyDSP">Dépôt · privé</a><br><br>
      <sub>README</sub><br>
      <a href="README.md">English</a> · <a href="README.fr.md">Français</a>
    </td>
    <td width="400" align="center" valign="middle">
      <strong>CyDSP</strong><br>
      <sub>Traitement audio C++20</sub>
    </td>
    <td width="300" align="right" valign="top">
      <p><sub>Licence de la bibliothèque</sub><br><a href="LICENSE"><strong>MIT</strong></a></p>
      <p><sub>Version</sub><br><a href="VERSION.fr.md"><strong>0.1.0</strong></a><br><strong>EN DÉVELOPPEMENT</strong></p>
    </td>
  </tr>
</table>

<h1 align="center">CyDSP</h1>

Une bibliothèque de traitement audio en C++20 pour les développeurs d'applications audio, de plug-ins et de jeux. Elle réunit filtres, transformées, convolution et rééchantillonnage dans une même API, avec des implémentations pour les processeurs x64 et ARM64 récents.

> **EN DÉVELOPPEMENT · 0.1.0**
>
> Disponible sous forme de sources ; aucune version binaire n'est publiée. Compiler pour la cible souhaitée et consulter les [notes de version et limites documentées](VERSION.fr.md).

## À propos de CyDSP

CyDSP se compile en bibliothèque statique et utilise l'espace de noms `cydsp`. Les processeurs partagent des blocs audio planaires, des codes de retour explicites et les types d'échantillons `float` et `double`. Les cibles prioritaires sont **x64 et ARM64** ; x86 et ARM32 restent des cibles de compatibilité secondaires.

Préparer les processeurs et leur mémoire avant d'entrer dans le rappel audio, puis réutiliser ces instances. Le traitement et la remise à zéro conservent cette mémoire sans l'allouer ni la libérer. L'implémentation adaptée au processeur est choisie pendant la préparation ; un choix explicite reste possible. Les modules optionnels fournissent les périphériques audio/MIDI, les fichiers audio et le traitement de durée/hauteur. L'application hôte organise les rappels ; le module Sources fournit ses propres workers de fichiers.

Les [exemples de traitement](examples/Processing.cpp) et les [contrats des modules](docs/Modules.md) précisent la propriété des tampons, les codes de retour et l'utilisation en temps réel. La documentation technique liée est actuellement en anglais.

## Fonctionnalités

- **API audio commune** : blocs planaires, tampons alignés, conversion entrelacée et matrices de canaux.
- **Filtres et gain** : rampes de gain, FIR, biquads, cascades, banques de voix parallèles et transitions d'égalisation.
- **Transformées** : FFT complexes et réelles, calcul vectoriel et fonctions de fenêtrage.
- **Convolution** : moteurs uniforme, hybride, non uniforme et progressif ; MIMO à FFT partagées et transitions préparées de réponses impulsionnelles.
- **Rééchantillonnage et temps** : conversion rationnelle, rampes de rapport préparées, vidage explicite du flux et délais entiers ou fractionnaires.
- **Graphes audio** : routage préparé, mélange des branches et fusion des chaînes de gain compatibles.
- **Générateurs et effets** : ADSR, oscillateurs, dynamique, filtres TPT/crossover/ladder, suréchantillonnage, panoramique, chorus, phaser et réverbération.
- **Synthèse et MIDI** : voix polyphoniques préparées, sampler, générateurs personnalisés, zones MPE/RPN, événements MIDI 1/UMP/MIDI 2 à l’échantillon près, [MIDI-CI/UMP natif](docs/Midi2.md) et [serveur/client de propriétés préparé](docs/CiProperty.md) avec instances, mises à jour partielles, pagination et cache.
- **Lecture et enregistrement** : sources mémoire, transport avec commandes et boucles, carte de tempo, lecture anticipée avec conversion sinc commune et enregistrement en arrière-plan.
- **Implémentations CPU** : scalaire, SSE2, AVX2, AVX-512, NEON et SVE optionnel. Leur disponibilité dépend de l'ABI cible, des noyaux compilés et du processeur/système.
- **Intégration CMake** : cible statique `CyDSP::CyDSP` et paquet installable.

---

Les modules optionnels ajoutent les périphériques audio/MIDI, les fichiers audio et le traitement de durée/hauteur. Voir la [couverture détaillée](docs/Coverage.md) pour les fonctions disponibles et les limites.

Le module optionnel [Plugins](docs/Plugins.md) gère CLAP/VST3/LV2/JSFX, les éditeurs natifs et un worker en processus séparé. Les [corrections d’intégration](docs/IntegrationRemediation.md) couvrent aussi les métadonnées, périphériques, MIDI et sources.

## Installation

Un accès au [dépôt privé des sources](https://github.com/MrMybal/CyDSP) est nécessaire. Compiler une bibliothèque statique pour l'ABI de l'application et utiliser le moteur d'exécution C++ correspondant. Aucun installateur ni téléchargement précompilé n'est disponible.

Après compilation et installation selon les étapes ci-dessous, ajouter le préfixe d'installation au `CMAKE_PREFIX_PATH` de l'application, puis lier sa cible CMake existante :

```cmake
find_package(CyDSP 0.1 CONFIG REQUIRED)
target_link_libraries(AudioApp PRIVATE CyDSP::CyDSP)
```

Ici, `AudioApp` désigne la cible de l'application utilisatrice. Il est aussi possible d'inclure les sources CyDSP avec `add_subdirectory` et de lier la même cible `CyDSP::CyDSP`. Inclure `<cydsp/CyDSP.h>` pour accéder à l'API publique.

Les modules optionnels ont leurs propres cibles : `CyDSP::Files`, `CyDSP::Devices`,
`CyDSP::Stretch`, `CyDSP::Sources`, `CyDSP::Plugins`. Sources dépend de Files. Les demander comme composants du paquet CMake et lier les cibles
utiles. Désactiver les modules inutiles avec `CYDSP_BUILD_FILES=OFF`,
`CYDSP_BUILD_DEVICES=OFF`, `CYDSP_BUILD_STRETCH=OFF`, `CYDSP_BUILD_SOURCES=OFF`, `CYDSP_BUILD_PLUGINS=OFF`.
L’[exemple original de briques pour DAW](examples/DawBuildingBlocks.cpp) est vérifié à la compilation sans ouvrir de périphérique ni de fichier.

## Sources, compilation et bibliothèques nécessaires

### Prérequis

- [CMake](https://cmake.org/cmake/help/v3.25/manual/cmake-presets.7.html) **3.25+** pour les préréglages inclus, ou **3.24+** pour une configuration directe ; un compilateur **C++20** avec sa bibliothèque standard et son moteur d'exécution.
- Visual Studio 2022 avec les outils C++ pour les préréglages Windows.
- Git et un accès autorisé au dépôt pour récupérer les sources.
- Python uniquement pour les scripts optionnels de `scripts/` ; la compilation croisée Android demande aussi le NDK Android.

Les adaptations MIT et les composants permissifs optionnels sont inclus avec leur provenance épinglée. Aucun autre framework DSP n'est nécessaire. Les outils de compilation sont installés séparément et conservent leurs propres conditions.

### Récupérer les sources

```sh
git clone https://github.com/MrMybal/CyDSP.git
cd CyDSP
```

### Compiler et installer

<details>
<summary>Windows x64 et autres chaînes de compilation cibles</summary>

Depuis un terminal où CMake est disponible, compiler la bibliothèque Windows x64 en mode Release :

```sh
cmake --preset windows-x64
cmake --build --preset windows-x64
cmake --install build/windows-x64 --config Release --prefix build/stage
```

Pour une autre chaîne C++20 configurée :

```sh
cmake -S . -B build/local -DCMAKE_BUILD_TYPE=Release -DCYDSP_BUILD_TESTS=OFF
cmake --build build/local --config Release
cmake --install build/local --config Release --prefix build/stage
```

Utiliser des dossiers de compilation et d'installation distincts pour chaque ABI. Les préréglages de production construisent des bibliothèques statiques avec les tests natifs désactivés. `CYDSP_ENABLE_SIMD=OFF` sélectionne l'implémentation scalaire ; `CYDSP_ENABLE_AVX512=OFF` ou `CYDSP_ENABLE_SVE=OFF` retire les noyaux optionnels correspondants. Les autres implémentations compilées restent disponibles.

Le [guide d'architecture](docs/Architecture.md) décrit la sélection CPU et les limites des plateformes. SVE est optionnel sur les cibles ARM64 Linux/Android compatibles ; ARM64 fonctionne sans SVE. Les [notes de version](VERSION.fr.md) précisent les plateformes dont la validation native reste à effectuer.

</details>

---

## Licence

**CyDSP utilise la [licence MIT](LICENSE).** Copyright © 2026 MrMybal CyberAlien.

### Bibliothèques tierces

| Bibliothèque · sources | Licence · texte | Utilisation / périmètre |
| --- | --- | --- |
| [DSPFilters](https://github.com/vinniefalco/DSPFilters) | [Notice MIT](third_party/licenses/DSPFilters.txt) | Équations RBJ adaptées pour les coefficients des filtres ; compilées dans CyDSP. |
| [FFTConvolver](https://github.com/HiFi-LoFi/FFTConvolver) | [Notice MIT](third_party/licenses/FFTConvolver.txt) | Partitionnement et recouvrement-addition adaptés pour la convolution ; utilise la FFT de CyDSP. |
| [r8brain-free-src](https://github.com/avaneev/r8brain-free-src) | [Notice MIT](third_party/licenses/r8brain-free-src.txt) | Équations de sinc fenêtrée et de fenêtres adaptées ; il ne s'agit pas du convertisseur amont complet. |

| [miniaudio](https://github.com/mackron/miniaudio) | [MIT-0 alternative](third_party/licenses/miniaudio.txt) | Accès privé aux périphériques audio. |
| [Déclarations Windows MIDI Services](https://github.com/microsoft/MIDI) | [MIT](third_party/licenses/windows-midi.txt) | Adaptation privée de l’ABI UMP ; aucun pilote/service inclus. |
| [RtMidi](https://github.com/thestk/rtmidi) | [MIT](third_party/licenses/rtmidi.txt) | Accès privé aux périphériques MIDI. |
| [CHOC](https://github.com/Tracktion/choc) | [ISC et notices des codecs intégrés](third_party/licenses/choc.txt) | Codecs WAV/FLAC/Ogg/MP3 ; conditions BSD/domaine public conservées. |
| [Signalsmith Stretch](https://github.com/Signalsmith-Audio/signalsmith-stretch) | [MIT](third_party/licenses/signalsmith-stretch.txt) | Traitement de hauteur/durée/formants. |
| [Signalsmith Linear](https://github.com/Signalsmith-Audio/linear) | [MIT](third_party/licenses/signalsmith-linear.txt) | Équations STFT/FFT adaptées ; moteur FFT CyDSP utilisé pendant le traitement. |

| [CLAP](https://github.com/free-audio/clap) | [MIT](third_party/licenses/clap.txt) | Interfaces privées ; hôte et plugin de test originaux. |
| [Interfaces VST3](https://github.com/steinbergmedia/vst3_pluginterfaces) | [MIT](third_party/licenses/vst3-interfaces.txt) | Hôte, contrôleur et éditeur natif originaux. |
| [LV2/Lilv](https://github.com/lv2/lilv) | ISC | Dépendances privées pour l’hébergement, RDF et Atom ; adaptateur CyDSP original. |
| [ysfx](https://github.com/jpcima/ysfx) | Apache 2.0 et notices intégrées | Moteur JSFX ; hôte/canvas originaux, sans wrapper JUCE ; exception du parseur Bison conservée. |

Conserver le fichier LICENSE de CyDSP et les notices amont complètes. Le [manifeste de provenance](third_party/manifest.json) épingle les révisions, les archives et le périmètre des adaptations ; les [détails de licence](docs/Licensing.md) expliquent leur distribution. Le tableau constitue un index de ces composants et de leurs conditions.

## Références et liens

- [Documentation des modules et de l'API](docs/Modules.md) · [Exemples de traitement](examples/Processing.cpp).
- [Architecture](docs/Architecture.md) · [Conventions audio](docs/Conventions.md).
- [Notes de version et limites](VERSION.fr.md).
- [Dépôt privé des sources](https://github.com/MrMybal/CyDSP) · [Suivi des problèmes](https://github.com/MrMybal/CyDSP/issues) — un accès GitHub autorisé est nécessaire.

<details>
<summary><strong>AI Usage</strong></summary>

- [Commencer ici : guide du projet pour les agents](AGENTS.md) — périmètre, règles de développement et documentation importante.
- [Architecture](docs/Architecture.md) — sélection CPU, durée de vie des processeurs et contrats d'intégration.
- [API publique et contrats des modules](docs/Modules.md) — préparation, traitement et comportement des tampons.

</details>

Les [ajouts pour une intégration DAW](docs/DawIntegration.md) couvrent les filtres supplémentaires, tables d’ondes, samples filtrés, presets, lecture avec étirement, cache de formes d’onde, reconnexions, horloge MIDI, UMP et fichiers AIFC/CAF. Les codecs Windows disponibles permettent aussi l’export MP3/AAC/M4A/WMA.
