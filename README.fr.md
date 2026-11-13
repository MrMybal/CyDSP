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
      <p><sub>Version</sub><br><a href="VERSION.fr.md"><strong>0.2.0</strong></a><br><strong>EN DÉVELOPPEMENT</strong></p>
    </td>
  </tr>
</table>

<h1 align="center">CyDSP</h1>

Bibliothèque C++20 de calcul DSP audio pour les développeurs de DAW, d’effets, de synthétiseurs et d’applications audio. Elle fournit filtres, FFT, convolution, rééchantillonnage, synthèse granulaire et analyse audio avec une API commune float/double et une sélection du code adapté aux CPU x64 et ARM64.

> **EN DÉVELOPPEMENT · 0.2.0**
>
> Distribution sous forme de sources ; aucune release binaire publiée. Consulter les [notes de version et limites des plateformes](VERSION.fr.md).

La deuxième extension ajoute HRTF partagées, HOA ordre 7, sources groupées, diffraction, diagnostic IR, séparation harmonique/percussive, pitch spectral, contacts physiques, AEC et profils de qualité préparés. Voir le [guide des API](docs/AdvancedExpansionGuide.fr.md) et les [résultats de validation](docs/AdvancedExpansionValidation.fr.md).

## À propos de CyDSP

Construire une chaîne de traitement avec les vues planaires `AudioBlock`, le stockage `AudioBuffer` et un `ProcessSpec` commun. Préparer les buffers et algorithmes avant le callback audio ; `process` et `reset` conservent leur stockage préparé. Les changements de paramètres et de cycle de vie suivent les règles de propriété décrites dans le [guide d’architecture](docs/Architecture.md).

CyDSP fournit la couche de calcul numérique. Les périphériques audio, le MIDI, les entrées/sorties de fichiers audio, le transport de lecture et l’hébergement de plugins relèvent de **CyAudioCore**, qui utilise CyDSP. Une application peut employer CyDSP seul et le relier à son propre moteur audio et à son interface.

## Fonctionnalités

- **Filtres et dynamique** : biquads et cascades, conceptions Butterworth/Chebyshev/elliptique/Bessel, banques de voix en parallèle, compresseur, expandeur, gate avec maintien/hystérésis/anticipation et limiteur.
- **Transformées et convolution** : FFT complexe/réelle, STFT, FIR, convolution partitionnée et non uniforme, traitement MIMO partagé et transitions préparées de réponses impulsionnelles.
- **Fréquences d’échantillonnage** : rééchantillonnage fixe et variable, interpolation préparée, suréchantillonnage FIR/FIR demi-bande/IIR et génération de fenêtres.
- **Effets et routage** : rampes de gain, matrices de canaux, graphes DSP, délai, panoramique, mélange sec/traité, mise en forme d’onde, chorus, phaser et réverbération.
- **Synthèse numérique** : oscillateurs, enveloppes, tables d’ondes à bandes harmoniques, données de samples et primitives de lecture avec réduction du repliement spectral ; grains manuels et nuages granulaires reproductibles à nombre de voix borné.
- **Analyse audio** : crête/RMS/DC/corrélation sur fenêtre glissante, estimation true peak 4x/8x, spectres continus et caractéristiques spectrales, bandes Mel/log, analyse constant-Q et mesure LUFS/LRA à historique borné. Voir le [guide des API](docs/GranulationAndAnalysis.md).
- **Audio spatial et traitements avancés** : chemins/pièces acoustiques, HRIR binaurales, VBAP, Ambisonics, sidechains/PDC, dynamique multibande, grains live, FM/PM, synthèse physique, analyse musicale et restauration. Voir le [guide des dix lots](docs/ExpansionGuide.fr.md) pour les API et limites.
- **Backends CPU** : scalaire, SSE2, AVX2, AVX-512, NEON et SVE optionnel ; sélection à la préparation selon le code compilé et les capacités CPU/OS. Un backend explicite peut être demandé.
- **Module Stretch optionnel** : cible statique distincte pour l’étirement temporel, la transposition et les formants, utilisant le socle numérique CyDSP.

---

## Installation

Utiliser CMake 3.24+ et un compilateur C++20 pour produire les bibliothèques statiques de l’ABI cible. La cible principale est `CyDSP::CyDSP` ; `CyDSP::Stretch` ajoute le module d’étirement optionnel. Windows x64 et Android ARM64 disposent d’une validation en exécution ; les limites des autres plateformes figurent dans les [notes de version](VERSION.fr.md).

Après installation du package CMake, le lier à l’application :

```cmake
find_package(CyDSP 0.2 CONFIG REQUIRED)
target_link_libraries(MyApplication PRIVATE CyDSP::CyDSP)
# Traitement durée/hauteur optionnel :
# target_link_libraries(MyApplication PRIVATE CyDSP::Stretch)
```

Les en-têtes publics se trouvent sous `cydsp/` ; `cydsp/CyDSP.h` les regroupe. Consulter l’[exemple de traitement](examples/Processing.cpp), l’[exemple granulation/analyse](examples/GranulationAndAnalysis.cpp), les [contrats des modules](docs/Modules.md) et le [guide de migration](docs/Migration.fr.md) pour l’intégration.

## Sources, compilation et bibliothèques nécessaires

Le dépôt inclut ses dépendances numériques figées ; la configuration ne télécharge aucun package. Depuis un accès authentifié au dépôt, la compilation Windows x64 se fait ainsi :

```sh
git clone https://github.com/MrMybal/CyDSP.git
cd CyDSP
cmake -S . -B build/release -G "Visual Studio 17 2022" -A x64
cmake --build build/release --config Release
cmake --install build/release --config Release --prefix build/install
```

Définir `CMAKE_PREFIX_PATH` dans l’application vers le préfixe d’installation choisi. Définir `CYDSP_BUILD_STRETCH=OFF` si seul le socle est nécessaire. Les options d’architecture s’appliquent aux fichiers d’implémentation SIMD concernés ; la bibliothèque portable ne dépend pas d’une compilation globale pour le CPU local.

Les tests, mesures et exemples sont activés explicitement. Python sert uniquement aux scripts de validation/compilation du dépôt, pas à l’utilisation de la bibliothèque C++. Les commandes de validation native et leur couverture effective sont documentées séparément dans [Validation.md](docs/Validation.md).

---

## Licence

CyDSP utilise la [licence MIT](LICENSE), copyright © 2026 MrMybal CyberAlien. Les composants adaptés conservent leurs notices et leur provenance figée. Le tableau sert d’index ; conserver les [notices tierces complètes](docs/Licensing.md).

| Bibliothèque · sources | Licence · texte | Utilisation / périmètre |
| --- | --- | --- |
| [DSPFilters](https://github.com/vinniefalco/DSPFilters) | [MIT](third_party/licenses/DSPFilters.txt) | Équations de coefficients/prototypes adaptées à la préparation CyDSP ; traitement par ses filtres et cascades. |
| [FFTConvolver](https://github.com/HiFi-LoFi/FFTConvolver) | [MIT](third_party/licenses/FFTConvolver.txt) | Partitionnement et addition avec recouvrement adaptés à la convolution continue et MIMO, avec FFT et noyaux vectoriels CyDSP. |
| [r8brain-free-src](https://github.com/avaneev/r8brain-free-src) | [MIT](third_party/licenses/r8brain-free-src.txt) | Équations sinc fenêtré et fenêtres adaptées au rééchantillonnage CyDSP ; ce n’est pas l’algorithme SRC amont complet. |
| [signalsmith-linear](https://github.com/Signalsmith-Audio/linear) | [MIT](third_party/licenses/signalsmith-linear.txt) | Reconstruction FFT et normalisation STFT/fenêtres adaptées au socle numérique CyDSP. |
| [signalsmith-stretch](https://github.com/Signalsmith-Audio/signalsmith-stretch) | [MIT](third_party/licenses/signalsmith-stretch.txt) | Algorithme durée/hauteur/formants adapté dans la cible optionnelle `CyDSP::Stretch` ; fichiers amont conservés comme provenance. |

## Références et liens

- [Modules et contrats d’API](docs/Modules.md) · [Architecture](docs/Architecture.md).
- [Migration vers CyDSP 0.2.0](docs/Migration.fr.md) · [Notes de version](VERSION.fr.md).
- [Mesures de performances](docs/Measurements.md) · [Couverture de validation](docs/Validation.md).
- [Dépôt des sources](https://github.com/MrMybal/CyDSP) · [Suivi des problèmes](https://github.com/MrMybal/CyDSP/issues).
