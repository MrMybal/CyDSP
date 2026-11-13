# Validation de la deuxième extension : dix lots

État exact des sources compilées : `07393a6935e0ad0e5661193196ed550403432ad3a428c42c6952114d701e2326`.
Synthèse produite le 2026-11-13 01:54 UTC.

Les dix lots sont implémentés : [API, contrats et limites](AdvancedExpansionGuide.fr.md).
Aucun autre projet modifié, aucun commit/push ou workflow GitHub lancé pour ces lots.
Les cinq adaptations MIT et les notices restent inchangées. Aucun jeu de HRIR tiers importé.

## Validation

| Vérification | Résultat |
| --- | --- |
| Windows x64 MSVC + mesures | Passé |
| Windows x64 clang-cl | Passé |
| Windows x64 scalaire | Passé |
| Windows x64 AddressSanitizer | Passé |
| Android ARM64/SVE compilation et liaison | Passé |
| Quest 3 ARM64/NEON exécution + mesures | Passé |
| Windows ARM64 en-têtes et objets | Passé |
| Installation statique + consommateur indépendant | Passé |
| Isolation ISA x64 | Passé |
| Isolation ISA Windows ARM64 | Passé |
| Isolation ISA Android/SVE | Passé |
| Linux/macOS desktop | Exécution différée par le propriétaire |
| Windows ARM64 natif et SVE sur matériel compatible | Exécution non vérifiée |
| Écoute et intégration DAW/jeu/périphériques | À réaliser dans les projets consommateurs |

Régressions float/double : rotation HOA7 covariante et orthogonale, dispositions irrégulières,
projection binaurale isotrope, seuil/onset HRIR, gain DC SRC, fenêtres/positions des grains,
sinus groupés contre formule analytique, complémentarité HPSS, pitch/stéréo/formants,
impulsion mécanique, identité FIR AEC et gel des coefficients en double parole, profils/fondus.
Les contrôles supplémentaires vérifient préparation transactionnelle, entrées invalides
avant mutation, blocs de tailles différentes, blocs vides, déplacements et audits
allocation/libération des chemins temps réel. Les en-têtes et exemples sont compilés séparément.
Le Quest exécute NEON, pas SVE ; les fichiers temporaires sont retirés après les tests.

## Mesures CPU

Float, **128 frames à 48 kHz**, microsecondes par appel : **moyenne / p99**.
Budget d’un bloc : **2 666,67 µs**. 48 appels de préchauffage, puis 128 mesures.
Les cas spectraux travaillent par hops : leur p99 inclut ces pics. Mesures séparées
des compilations et de la préparation, sur données synthétiques. Les coûts de l’hôte,
des périphériques, de l’allocation et des changements de graphe ne sont pas inclus.

| Cas | Ryzen 9 7900X, AVX-512 | Quest 3, NEON |
| --- | ---: | ---: |
| 128 sinus groupés → stéréo | 66.80 / 81.30 | 117.01 / 123.59 |
| 128 oscillateurs individuels + même mixage | 131.19 / 157.50 | 243.38 / 249.69 |
| 64 grains sur une banque de samples partagée | 148.90 / 180.90 | 231.51 / 314.84 |
| HRTF : 32 directions × 128 taps, direction/fondu à chaque bloc | 46.00 / 65.00 | 143.32 / 431.72 |
| Binaural HOA3 : 32 HRIR, 128 taps | 63.03 / 82.60 | 169.21 / 187.24 |
| HOA7 : encodage/rotation/décodage 24 enceintes | 135.24 / 156.40 | 233.61 / 274.69 |
| HPSS stéréo N1024, médianes 17 | 40.76 / 96.60 | 92.93 / 186.30 |
| Pitch stéréo N1024, ratio 1,5, formants | 67.59 / 140.30 | 131.90 / 279.06 |
| AEC 512 taps, 2 microphones | 223.36 / 286.20 | 419.27 / 436.67 |

**Ryzen : 1.96×** signifie que le temps moyen des oscillateurs individuels
est divisé par celui du lot de 128 sinus avec le même routage. Ce facteur concerne
uniquement ce cas, ces paramètres et ce matériel ; ce n’est pas une accélération globale.

**Quest : 2.08×** signifie que le temps moyen des oscillateurs individuels
est divisé par celui du lot de 128 sinus avec le même routage. Ce facteur concerne
uniquement ce cas, ces paramètres et ce matériel ; ce n’est pas une accélération globale.

Les grains partagent une boucle de 32768 samples, 64 fenêtres Hann de 10 s,
ratios alternés −1 et 1,5. Les HRIR sont synthétiques et ne mesurent pas une HRTF humaine.
Le cas HOA7 utilise 64 canaux et 24 enceintes irrégulières régularisées.
L’AEC reçoit deux microphones synthétiques et une référence mono ; sa charge dépend aussi
de l’état d’adaptation. Contact archet (mono, scalaire) :
Ryzen 3.28 / 3.60 µs ; Quest 3.87 / 3.96 µs (moyenne / p99).

## Qualité numérique

| Métrique et unité | Ryzen float | Ryzen double | Quest float | Quest double |
| --- | ---: | ---: | ---: | ---: |
| HOA7 : erreur max de coefficient | 2.88658e-15 | 2.88658e-15 | 1.9984e-15 | 1.9984e-15 |
| HPSS : erreur absolue max en amplitude | 8.3819e-08 | 2.22045e-16 | 8.3819e-08 | 2.22045e-16 |
| RT60 exponentiel connu : erreur en secondes | 2.48249e-10 | 1.66533e-15 | 2.48249e-10 | 1.66533e-15 |
| RT60 : R², 0 à 1 | 1 | 1 | 1 | 1 |
| Voyelle synthétique : erreur RMS normalisée sans conservation | 0.910827 | 0.899888 | 0.915125 | 0.900091 |
| Même voyelle : erreur RMS normalisée avec conservation | 0.162345 | 0.127259 | 0.190089 | 0.128161 |

La voyelle est une somme de 60 harmoniques, fondamentale 62,5 Hz à 16 kHz,
avec deux enveloppes gaussiennes centrées sur 600 et 1600 Hz. Transposition d’une octave ;
comparaison de 24 amplitudes harmoniques à l’enveloppe cible, après normalisation du gain.
Cette amélioration quantifie ce signal, sans promettre de transparence sur toute voix/musique.
Les coefficients HOA sont en double dans les deux API.
La régression AEC exige au moins 60 dB de réduction pour un FIR connu, une référence
synchronisée, puis le gel des coefficients pendant une parole proche synthétique forte.
Elle ne remplace pas une mesure avec haut-parleur, microphones, dérives et non-linéarités réels.

## Reproduction

Exécutions explicites, sans GitHub Actions :

```powershell
python -X utf8 -B scripts/run_native.py --build-dir build/expansion-x64 --measure
python -X utf8 -B scripts/run_native.py --compiler clang --build-dir build/expansion-clang
python -X utf8 -B scripts/run_native.py --arch scalar --build-dir build/expansion-scalar
python -X utf8 -B scripts/run_native.py --sanitize --build-dir build/expansion-asan
python -X utf8 -B scripts/build_android.py --abi arm64-v8a --measure
python -X utf8 -B scripts/run_android_device.py --abi arm64-v8a --measure
python -X utf8 -B scripts/build_arm64.py --build-dir build/expansion-arm64 --compile-checks
python -X utf8 -B scripts/validate_advanced_package.py
```

Les rapports JSON locaux `build/native-*.json`, `build/device-arm64-v8a.json`,
`build/advanced-*.json` et `build/android-arm64-v8a/report.json` portent l’empreinte des
sources. Le script `report_advanced_expansion.py` refuse un rapport absent, périmé ou en échec.
Les rapports d’isolation SIMD sont archivés immédiatement après chaque vérification
`verify_simd.py` car ce vérificateur utilise un nom de sortie commun.
