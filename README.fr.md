<table width="100%">
  <tr>
    <td width="300" align="left" valign="top">
      <a href="docs/Modules.md">Documentation</a><br>
      <a href="docs/Migration.fr.md">Migration</a><br><br>
      <sub>README</sub><br>
      <a href="README.md">English</a> · <a href="README.fr.md">Français</a>
    </td>
    <td width="400" align="center" valign="middle"><strong>CyDSP</strong><br><sub>C++20 audio</sub></td>
    <td width="300" align="right" valign="top">
      <p><sub>Licence de la bibliothèque</sub><br><a href="LICENSE"><strong>MIT</strong></a></p>
      <p><sub>Version</sub><br><a href="VERSION.fr.md"><strong>0.2.0</strong></a><br><strong>EN DÉVELOPPEMENT</strong></p>
    </td>
  </tr>
</table>

<h1 align="center">CyDSP</h1>

Bibliothèque C++20 de calcul DSP : filtres, FFT, convolution, rééchantillonnage, effets et synthèse numérique, avec un socle commun optimisé pour x64 et ARM64.

> **EN DÉVELOPPEMENT · 0.2.0**
>
> Sources uniquement ; aucune publication binaire. Consulter les [notes de version](VERSION.fr.md).

## À propos de CyDSP

Les processeurs partagent AudioBlock, AudioBuffer, ProcessSpec, Status et les échantillons float/double. Préparer la mémoire avant le callback ; process/reset conservent leur stockage. CyDSP est utilisable seul. Les services audio se trouvent dans CyAudioCore.

## Fonctionnalités

- Filtres, dynamique, effets, mélange, graphes DSP et convolution MIMO.
- FFT, fenêtres, SIMD, rééchantillonnage et suréchantillonnage.
- Oscillateurs, enveloppes, tables d’ondes, données de samples et interpolation préparée.
- Scalaire, SSE2, AVX2, AVX-512, NEON et SVE optionnel ; sélection selon CPU/OS.
- Module statique Stretch pour durée, hauteur et formants.

---

## Installation

Compiler les archives pour l’ABI cible avec CMake 3.24+ et un compilateur C++20. CyAudioCore exige CyDSP 0.2.0. Voir le [guide de migration](docs/Migration.fr.md).

```cmake
find_package(CyDSP 0.2 CONFIG REQUIRED)
target_link_libraries(MyApplication PRIVATE CyDSP::CyDSP)
```

## Sources, compilation et bibliothèques nécessaires

```sh
cmake -S . -B build/release -G "Visual Studio 17 2022" -A x64
cmake --build build/release --config Release
cmake --install build/release --config Release --prefix build/install
```

Les tests et mesures sont désactivés par défaut. Le script scripts/run_native.py les active explicitement sous Windows ; --sanitize active AddressSanitizer. Les backends de plateforme exigent leurs SDK système. Aucun téléchargement implicite.

---

## Licence

CyDSP utilise la [licence MIT](LICENSE). Les dépendances conservent leurs propres licences ; les notices complètes sont dans [Licensing.md](docs/Licensing.md).

| Bibliothèque · sources | Licence · texte | Utilisation / périmètre |
| --- | --- | --- |
| [DSPFilters](https://github.com/vinniefalco/DSPFilters) | [MIT](third_party/licenses/DSPFilters.txt) | Équations et prototypes de filtres adaptés |
| [FFTConvolver](https://github.com/HiFi-LoFi/FFTConvolver) | [MIT](third_party/licenses/FFTConvolver.txt) | Convolution partitionnée et partage FFT adaptés |
| [r8brain-free-src](https://github.com/avaneev/r8brain-free-src) | [MIT](third_party/licenses/r8brain-free-src.txt) | Équations du rééchantillonnage sinc adaptées |
| [signalsmith-linear](https://github.com/Signalsmith-Audio/linear) | [MIT](third_party/licenses/signalsmith-linear.txt) | STFT et reconstruction FFT adaptées |
| [signalsmith-stretch](https://github.com/Signalsmith-Audio/signalsmith-stretch) | [MIT](third_party/licenses/signalsmith-stretch.txt) | Étirement temporel, hauteur et formants adaptés |

## Références et liens

- [Documentation](docs/Modules.md)
- [Architecture](docs/Architecture.md)
- [Migration](docs/Migration.fr.md)
