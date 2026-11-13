# Validation des dix lots DSP / audio 3D

État source exact : `d1175112d72f127dca622e0156695b40b5997b4d264afe4f2e45a803e0c107e8`.
Rapports locaux datés ; synthèse produite le 2026-11-13 00:46 UTC.

Les dix lots sont implémentés et leurs régressions passent sur x64 et Quest ARM64.
Aucune application consommatrice n’a été modifiée. Aucun commit, push ou workflow
GitHub n’a été lancé pour cette extension. Les cinq adaptations tierces MIT restent
inchangées ; aucun nouveau composant ou jeu de HRIR n’est importé.

## Matrice de validation

| Vérification | Résultat |
| --- | --- |
| Windows x64 MSVC + mesures | Passé |
| Windows x64 clang-cl | Passé |
| Windows x64 sans SIMD explicite | Passé |
| Windows x64 AddressSanitizer | Passé |
| Android ARM64 + SVE, compilation/liaison | Passé |
| Quest 3 ARM64 scalaire/NEON + mesures | Passé |
| Windows ARM64, compilation/en-têtes/objets | Passé |
| Package installé + consommateur indépendant | Passé |
| Isolation ISA x64 | Passé |
| Isolation ISA Windows ARM64 | Passé |
| Isolation ISA Android ARM64/SVE | Passé |
| Linux/macOS desktop | Exécution différée par le propriétaire |
| Windows ARM64 / SVE sur matériel compatible | Exécution non vérifiée |
| Écoute, périphérique physique, intégration DAW/jeu | À faire dans les projets consommateurs |

Les vérifications couvrent float/double, oracles mathématiques, tailles de blocs,
préparation transactionnelle, déplacement, entrées invalides avant mutation et audits
allocation/libération des chemins temps réel. Les en-têtes et exemples sont compilés
séparément. Le Quest exécute NEON, sans SVE ; SVE est compilé/désassemblé uniquement.
Les exécutables temporaires du Quest sont retirés après mesure.

## Coût CPU de configurations représentatives

Microsecondes par appel de **128 frames à 48 kHz**, float : **moyenne / p99**.
Budget temporel d’un tel bloc : **2 666,67 µs**. Ryzen 9 7900X en AVX-512 et
Quest 3 en NEON, sauf les algorithmes explicitement scalaires. 48 appels de
préchauffage puis 128 mesures par cas. Une STFT effectue davantage de travail
sur ses blocs de hop : le p99 compte cette distribution. Aucun facteur global
de rapidité, latence matérielle ou nombre maximal de voix n’est déduit.

| Charge | Ryzen µs moyen / p99 | Quest µs moyen / p99 |
| --- | ---: | ---: |
| 16 taps Lagrange, stéréo | 19.57 / 21.20 | 49.76 / 123.18 |
| 4 bandes FIR 129 taps, stéréo | 7.19 / 7.40 | 19.48 / 19.53 |
| FDN 16 lignes, stéréo | 16.62 / 18.00 | 38.27 / 38.49 |
| HRIR 128 taps, mono → stéréo | 15.94 / 19.00 | 55.07 / 61.25 |
| HOA ordre 3 : encodage/rotation/décodage 8 enceintes | 12.15 / 12.70 | 22.95 / 23.02 |
| Limiteur 4x, anticipation 5 ms, stéréo | 40.48 / 44.50 | 116.29 / 198.80 |
| 64 grains live sinc 32 taps, stéréo | 301.26 / 312.40 | 703.75 / 744.90 |
| FM/PM/sync, 8 voix unisson, stéréo | 44.66 / 55.30 | 86.43 / 90.10 |
| Analyse musicale N=2048, hop=256, stéréo | 30.29 / 60.40 | 78.75 / 214.48 |
| Harmoniseur 4 voix sinc, stéréo | 105.95 / 139.90 | 282.31 / 411.88 |
| Denoise STFT 1024/hop256, stéréo | 14.35 / 31.70 | 30.09 / 59.01 |
| NLMS 64 taps, stéréo | 25.01 / 28.00 | 66.68 / 80.26 |
| Declick médiane 17, stéréo, scalaire | 32.22 / 36.00 | 43.73 / 67.19 |

Le cas live conserve 64 grains de 10 s, âge 0,3 s, ratio 1 et sorties stéréo ;
la capture est préremplie. Le cas HRIR utilise des FIR de synthèse, aucun jeu
mesuré tiers. Le cas harmoniseur utilise quatre voix : 0/7/12/0,08 demi-tons,
gain total 1, retard de doubleur 15 ms sur la dernière voix. Les données audio
sont synthétiques et les paramètres fixes ; ces coûts ne représentent pas toutes
les combinaisons, les allocations de préparation ou l’activité du moteur hôte.

## Qualité numérique

| Cas / unité | Ryzen float | Ryzen double | Quest float | Quest double |
| --- | ---: | ---: | ---: | ---: |
| STFT256 : erreur absolue max (amplitude) | 1.19209e-07 | 2.77556e-16 | 1.19209e-07 | 2.22045e-16 |
| YIN440 : erreur absolue (Hz) | 0.0193532 | 0.0193532 | 0.0193532 | 0.0193532 |
| YIN440 : confiance (0–1) | 0.999986 | 0.999986 | 0.999986 | 0.999986 |
| HOA3 : erreur max de rotation (coefficient double) | 4.996e-16 | 4.996e-16 | 4.44089e-16 | 4.44089e-16 |
| THD connue 1 % : erreur absolue (rapport RMS) | 2.78675e-10 | 1.73472e-18 | 2.78675e-10 | 3.46945e-18 |

La confiance YIN est celle de ce sinus, pas une probabilité de réussite sur un
morceau musical. Les coefficients HOA sont calculés en double pour les deux API.
Les régressions supplémentaires vérifient notamment les IIR BP/BS cinq familles
et ordres 1/3/4/8/16, la reconstruction N bandes, PDC/bypass/transitions, la
propagation/Doppler, les chemins/portails, les impulsions HRIR fractionnaires,
le plafond du limiteur sur un sinus, la durée de persistance des résonances,
l’identification FIR NLMS, la réduction d’alias d’un décimateur, les harmoniques
THD non cohérentes et la déconvolution d’une réponse connue.

## Optimisation de l’historique partagé

Une première paire locale avant/après a identifié les lectures répétées comme
goulot commun aux grains, HRIR et harmoniseur. `CircularHistory::readWindow`
regroupe les lectures entières ; les FIR réutilisent les dots SIMD et la même
interpolation sinc. Les tests indépendants vérifient wrap, passé manquant, futur
et convolution HRIR fractionnaire. Les coûts actuels figurent dans le tableau
ci-dessus ; une paire isolée ne constitue pas un facteur de gain général.

## Reproduire et intégrer

Commandes natives explicitement autorisées, depuis le dépôt :

```powershell
python scripts/run_native.py --build-dir build/expansion-x64 --measure
python scripts/run_native.py --compiler clang --build-dir build/expansion-clang
python scripts/run_native.py --arch scalar --build-dir build/expansion-scalar
python scripts/run_native.py --sanitize --build-dir build/expansion-asan
python scripts/build_android.py --abi arm64-v8a --measure
python scripts/run_android_device.py --abi arm64-v8a --measure
python scripts/verify_sources.py
```

Rapports bruts dans `build/native-*.json`, `build/device-arm64-v8a.json` et
`build/expansion-*.json`, avec le SHA-256 des entrées de compilation. Les sources
ne lancent pas ces programmes automatiquement. Voir le [guide des API et limites](ExpansionGuide.fr.md)
et les [exemples compilés](../examples/SpatialAndDaw.cpp).
