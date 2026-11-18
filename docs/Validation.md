# CyDSP 0.4.6 — 2026-11-18

Un projet consommateur Windows x64 a résolu le paquet installé avec
`find_package(CyDSP 0.4.6 EXACT CONFIG REQUIRED COMPONENTS CyDSP Stretch)`,
vérifié les macros et constantes de version à la compilation et lié les deux
bibliothèques statiques. Les trois suites Release ont réussi (DSP, import SOFA
et données HRTF). Le manuel installé annonce également la version 0.4.6.

Les [mesures de MusicalAnalyzer](Measurements.md) comparent le code de cette
version à la révision `68a423a` sur Ryzen 9 7900X et Quest 3.

---

# CyDSP 0.4.5 : référence temporelle HRTF — 2026-11-17

Retrait optionnel du minimum des arrivées extraites de toute la banque, deux
oreilles et toutes les directions. Le délai fourni et l’onset détecté participent
avant conversion dans les unités du taux cible. Préparation conservatrice par
défaut ; le rapport removedCommonDelayFrames expose l’avance réellement appliquée.
Les FIR alignées restent identiques ; les délais résiduels conservent ITD et
écarts entre directions. La politique ne supprime pas les latences de conception
SRC/noyau fractionnaire/MagLS. Aucune nouvelle dépendance ni code externe.

PASS Windows x64 Release et AddressSanitizer : float/double, délais explicites
fractionnaires, attaques connues, interpolation, taux 48/96/44,1 kHz, silence,
seuils d’onset, échec transactionnel et remise à zéro du rapport. Les rendus
HRIR direct/FFT et HOA LS correspondent au décalage entier de la référence ;
les noyaux fractionnaires 0/17, blocs variables et audits sans allocation passent.

PASS import/cache/préparation/rendu SOFA et banque D1/D2/H10 sur les lecteurs
compact et complet. Au seuil -40 dB à 48 kHz, les minima sont respectivement
4, 5 et 3 frames. Dans D2, les pics de face sont aux indices 107/110 : le seuil
prudent autorise seulement 0,104 ms d’avance commune, pas un retrait arbitraire
de 2,5 ms. Un seuil plus agressif change l’extraction et peut tronquer des
précurseurs. La banque originale et ses notices restent inchangées.

Archive de production Windows x64 reconstruite avec tests/mesures désactivés.
Manuel CyDoc régénéré : 288 pages, 144 paires EN/FR, 90 en-têtes, 2197
signatures publiques, 4492 liens et 3644 ancres appariées. Contrôles sources,
versions, provenance et diff passés. Pas de nouveau contrôle visuel du lecteur.
ARM/Quest/SVE/Linux/macOS non réexécutés pour cette modification de préparation.

---

# CyDSP 0.4.4 : cascade neuronale, lots SIMD et rejeu des transitions — 2026-11-17

Poids L4 D2 conservés, aucun réentraînement. Le rendu spécialisé réutilise les
noyaux biquad doubles CyDSP. processBatch groupe jusqu’à quatre voix et sort
L0,R0,... sans mixage implicite ; setDesigns valide tout le lot avant installation
et groupe le rejeu de l’entrée brute. Historique 512 frames par défaut, réglable
0–65536. Le rejeu fini ne reconstruit pas un passé IIR infini et coûte du CPU.

7900X, un thread, AVX-512 avec noyau stéréo SSE2, 48 kHz / 128 frames :
une voix fixe 14,326 → 7,297 µs ; 64 voix fixes groupées 905,774 → 192,851 µs
(4,70 fois plus rapide). 64 voix mobiles à 62,5 Hz / fondu 256, prédiction,
conception et rejeu inclus : 1893,297 → 949,402 µs (1,99 fois plus rapide).
Les autres historiques et appels individuels sont mesurés séparément. Médianes
de moyennes de séries, pas des p95/maxima de callbacks. Aucun avantage universel
sur le HRIR FFT ou le HOA partagé n’est revendiqué.

Sur 16 transitions avec les poids réels, erreur RMS relative à une cible ayant
reçu tout le même historique : -32,16 dB sans rejeu, -54,78 dB avec 512 frames,
soit 13,52 fois moins d’erreur RMS sur ce signal/test. Le rejeu des 8192 frames
disponibles correspond à cette cible à 6,94e-18 maximum. Pas d’étude perceptive.

PASS : suite Windows x64 Release et AddressSanitizer ; oracles float/double
jusqu’à 32 sections sur scalaire/SSE2/AVX2/AVX-512 ; partitions/offsets, groupes
incomplets, fondus asynchrones, échecs transactionnels et audit sans allocation.
Suite complète sur Quest 3 : PASS, scalaire et NEON. Windows ARM64/NEON : archive,
en-têtes et objets de régression compilés. CyDoc 288 pages, 144 paires EN/FR,
90 en-têtes et 2195 déclarations publiques. Sources/provenance/version et diff
PASS ; Gitleaks environ 129,92 Mo, aucun secret. Linux/macOS/SVE non réexécutés.

Limite explicite : Windows refuse le nouveau hrtf-rendering.exe du contrôle
complémentaire 881 directions. Aucun changement de protection ni identité sur
ces 881 impulsions n’est revendiqué pour cette passe. Oracles et benchmarks
ci-dessus réellement exécutés. Aucun changement de CyMetaSound/CyDAW/CyAudioCore.

Rapports figés sous build/neural-optimization/20261117-final, dont Rapport.fr.md,
JSON et empreintes ; outil scripts/measure_neural_optimization.py. Les fichiers
restent locaux et ignorés ; aucun commit/push dans cette passe.

---

# Exemples d’écoute en mouvement — 2026-11-16

Nouveaux exemples locaux dans build/hrtf-listening/D2-L4-Scenes-62Hz : parole
française synthétisée localement, pas/impacts procéduraux et boucle musicale
originale. Rendu réellement calculé par les moteurs C++ SOFA/ancien/nouveau avec
les mêmes entrées mono et directions, 48 kHz, blocs 128, mises à jour 768 frames
(62,5 Hz), fondus 256 frames. La référence utilise 8 voisins et sinc17 ; 8 frames
de délai algorithmique sont alignées hors ligne, les délais acoustiques restent.

PASS : exécution C++, sorties finies sur les scènes complètes, séquences unitaires
de direction, formes/dimensions natives, gain commun entre versions, PCM24 encodé
puis décodé identique aux valeurs quantifiées, notices et empreintes conservées.
Trois scènes et trois versions complètes de 24 s, plus les trois aperçus assortis.
Pas de mesure de localisation, d’étude humaine ni de certification de callback.
Aucune modification de l’API/du runtime et aucun entraînement pour cette création.

---

# Modèle HRTF L4, checkpoint 134000 : export natif — 2026-11-16

Archive de résultats vérifiée, meilleur checkpoint importé/exporté sans exécution
de code archivé et sans entraînement local. Ancien modèle conservé. Rendu C++ sur
881 mêmes directions de test : spectre 1,671388 → 1,641189 dB RMS, ILD
2,376328 → 2,334502 dB RMS, ITD 1,978400 → 1,932865 µs RMS. L’oracle théorique sur
32 directions présente un écart maximal de 0,000114441 dB. Contrôle CPU sur les
8802 directions : prédictions finies, rayon maximal des pôles 0,9979886094,
aucun contexte CUDA initialisé.

Topologie et taille inchangées : 26259 paramètres, 24 sections/oreille, 105280 octets.
Même bibliothèque 0.4.3, Ryzen 7900X/AVX-512, 48 kHz/128 frames : rendu neuronal
14,0 µs/bloc contre 2,3 µs pour le HRIR FFT mesuré optimisé. Le neuronal n’est plus
le moteur le plus rapide pour ce cas ; son intérêt est surtout la compacité.

Paquet d’intégration avec exemple C++ compilé, notices et validations :
build/hrtf-models/CyDSP-HRTF-D2-48k-L4-134000.zip (ignoré). Trois WAV d’écoute
PCM24/48 kHz/24 s au même gain et six poses statiques ; aller-retour PCM vérifié.
Génération depuis les impulsions C++ puis convolution hors ligne, aucune étude
perceptive ou validation de mouvements/suivi de tête. Rapport et sources figées
sous build/hrtf-native-rendering et build/hrtf-listening ; Linux/macOS/ARM non
réexécutés pour ces nouveaux poids. Gitleaks du paquet natif : aucun secret.

---

# CyDSP 0.4.3 : HRIR FFT et niveau diffus HOA — 2026-11-16

Rendu HRIR double FFT overlap-save sans latence de bloc ajoutée, historique commun
aux deux filtres pendant les fondus. API HrtfSet::interpolate const sans allocation ;
choix direct/FFT/automatique via HrirRenderConfig. Les deux préparations HOA rapportent
les énergies de champ diffus et un gain commun de compensation. La projection
causale garde sa faible latence ; MagLS spectral conserve sa demi-FFT déclarée.

Ryzen 9 7900X / Windows x64 / MSVC Release / AVX-512 automatique, 48 kHz, 128 frames,
D2 8802 directions et 256 taps : direct 31.64 µs/bloc,
FFT 2.39 µs/bloc, soit 13.22 fois plus rapide
et 0.90 ms CPU/seconde audio/voix. Erreur absolue
maximale du test réel 1.42e-14. Ce temps CPU n’est pas la latence audio.
L’interpolation globale générique coûte environ 3 ms par mise à jour ; le choix
explicite 8 voisins/sinc17 ramène cette mise à jour autour de 3,7 µs et ajoute
8 frames de délai sinc déclaré. Les ratios ne constituent pas une garantie multivoix.

PASS : suite native, AddressSanitizer, oracles de fondu/blocs mixtes et d’énergie
diffuse, absence d’allocation/libération audio, SOFA compact/complet et D1/D2/H10.
Compilation ARM64/NEON/en-têtes PASS, sans nouvelle exécution ARM ; Linux/macOS/SVE
non exécutés. Source/provenance PASS, CyDoc EN/FR et liens PASS, Gitleaks sans secret.
Rapports et sources de mesure figées : build/hrir-optimization (ignoré par Git).
Outil reproductible : scripts/measure_hrir_optimization.py, CPU uniquement.

---

# Profil Colab L4 d’affinage qualité — 2026-11-16

Le nouveau notebook CyDSP_HRTF_L4_Quality.ipynb et son ZIP distinct démarrent
sur le checkpoint D2 48 kHz validé, 24 filtres/oreille, 140000 étapes, batch 512,
taux Adam/cosine 0,0003 à 0,00001. Le rythme L4 précédent donne environ 29,4 min
de calcul estimées, hors installation/validation/Drive ; gain de qualité non garanti.

Validation locale CPU uniquement : nbformat 4.5 et neuf cellules vérifiés,
extraction du paquet avec empreintes, poids de référence chargés sur CPU,
commandes/plan vérifiés avec un substitut sans exécution CUDA, refus d’un plan
modifié et transport de checkpoint Drive par fichiers aller/retour vérifiés.
Ancien notebook/ZIP préservés et poids d’intégration inchangés. Aucun contexte
CUDA initialisé ni entraînement local lancé pour cette préparation.
La lecture de la session Chrome/Colab a échoué : cette passe L4 n’a pas été
lancée depuis cette session. OAuth, installation Linux et entraînement distant
restent à exécuter dans Colab. Rapport : build/colab/l4-quality-validation.json.
Gitleaks du paquet décompressé : aucun secret détecté.

---

# CyDSP 0.4.2 : rendu HRTF neuronal C++ — 2026-11-16

L'API publique NeuralHrtfModel / NeuralBinaural est intégrée au cœur statique MIT.
Aucun runtime PyTorch, Python, ONNX ou CUDA dans le processus audio (dépendances
natives inspectées). Prédiction via les additions SIMD CyDSP sur poids transposés ;
états biquad doubles et délais Thiran à amplitude unitaire. Poids partagés immuables,
buffers préparés, aucune allocation pendant prédiction/conception/updates/process.

Le transfert explicite 96→48 kHz utilise les HRIR 48 kHz originales du même D2,
30 000 étapes en 187,60 s de calcul RTX 3090 (192,21 s au total). Les 881 directions
réservées conservent leur découpage. D2 a déjà été évalué auparavant : cette étude
suit le même test directionnel, sans constituer une nouvelle validation indépendante
ni une généralisation à un autre auditeur.

| Mesure des impulsions C++ réellement rendues | 48 kHz | 96 kHz |
| --- | ---: | ---: |
| Directions de test | 881 | 881 |
| Échantillons par réponse/oreille | 8192 | 16384 |
| Erreur spectrale RMS, grille linéaire 200–20 000 Hz | 1.67139 dB | 1.68539 dB |
| Erreur ILD RMS sur cette grille | 2.37633 dB | 2.39884 dB |
| Erreur ITD RMS, pente de phase 200–1500 Hz | 1.97840 µs | 2.00883 µs |
| Erreur absolue ITD au 95e percentile | 4.10224 µs | 4.14986 µs |
| Écart numérique maximal au modèle théorique, 32 directions | 0.000122 dB | 0.000114 dB |

Mesures CPU : Ryzen 9 7900X, un thread, AVX-512, blocs de 128 échantillons,
160 chronométrages après échauffement. Temps incluant le mixage, exprimés en µs
par bloc ; plus petit = moins de temps CPU. Rendu SOFA direct HrirBinaural avec les
8802 points et 256 taps à 48 kHz (512 taps à 96 kHz), directions exactes et
sans détection d'attaque. Le modèle est une approximation, la fidélité n'est pas identique.
Les médianes ne garantissent pas une échéance audio : p95 et maximum restent dans
les rapports complets. Le rendu en fondu calcule deux branches complètes ; il peut
excéder le budget de 1,333 ms pour 64 sources à 96 kHz, même en médiane.

| Traitement à 48 kHz | Sources | Médiane µs/bloc | p95 µs/bloc |
| --- | ---: | ---: | ---: |
| predict | 1 | 5.60 | 5.70 |
| predictBatch | 64 | 356.50 | 380.00 |
| design | 1 | 45.30 | 75.00 |
| neuralStatic | 1 | 14.40 | 14.50 |
| measuredStatic | 1 | 32.30 | 33.00 |
| neuralContinuousFade | 1 | 29.40 | 30.30 |
| measuredContinuousFade | 1 | 67.20 | 104.10 |
| measuredDirectionUpdate | 1 | 0.40 | 1.10 |
| neuralStatic | 16 | 231.70 | 255.90 |
| measuredStatic | 16 | 532.30 | 957.60 |
| neuralContinuousFade | 16 | 471.60 | 503.70 |
| measuredContinuousFade | 16 | 1417.00 | 2076.60 |
| measuredDirectionUpdate | 16 | 5.50 | 7.80 |
| neuralStatic | 64 | 995.70 | 1136.00 |
| measuredStatic | 64 | 2137.40 | 3701.20 |
| neuralContinuousFade | 64 | 2036.60 | 2425.30 |
| measuredContinuousFade | 64 | 5602.80 | 8023.70 |
| measuredDirectionUpdate | 64 | 22.90 | 26.70 |
| setPreparedDesign | 1 | 0.80 | 1.40 |

La prédiction seule est séparée de la conception des biquads/compensation ITD.
Concevoir sur un worker puis installer un NeuralHrtfDesign entre blocs (~0,8 µs
pour une voix dans ce test). La nouvelle disposition de poids ramène la prédiction
d'environ 10 à 5–6 µs, et les identités de phase ramènent la conception d'environ
114 à 44–45 µs à 48 kHz. Calculs précis conservés ; la tentative de lot par tuiles
à quatre directions a été abandonnée après régression mesurée.

Régressions Windows x64 Release et AddressSanitizer : réussies, dont fichiers
tronqués, poids non finis, échec transactionnel, partage des poids, rejet d'un
mauvais taux/alias, cohérence des blocs, offset absolu, compensation de phase,
délais causaux, fondus et audit sans allocation/libération. Prédictions et impulsions
réelles vérifiées sur scalaire, SSE2, AVX2 et AVX-512. Compilation statique ARM64/NEON
et 90 en-têtes indépendants : réussie, sans exécution ARM de cette nouvelle API.
Exécution SVE, macOS et Linux non effectuée. Aucune certification temps réel OS.

Deux exemples audio synthétiques comparables sont créés sous build/hrtf-listening,
avec le même gain d'écoute et retrait du délai commun SOFA. Leur création n'est
pas un test perceptif. Le modèle garde amplitude et ITD approximatifs, pas toute
la phase mesurée ; aucune étude de localisation/écoute humaine n'a été faite.
Les fondus partent d'une nouvelle branche à historique nul, avec limites explicites
dans le manuel. Aucun remplacement automatique du rendu SOFA existant.

Poids expérimentaux CYHRTF02 : 105280 octets chacun, sous build/hrtf-models/D2-48000
et D2-96000, accompagnés de manifestes/hashes et notices SADIE II Apache-2.0.
Ils ne sont pas installés automatiquement ni ajoutés au code MIT.
Manuel CyDoc : 288 pages, 144 paires EN/FR, 90 en-têtes, 2181 déclarations,
4468 liens internes vérifiés. Gitleaks : aucun secret détecté.

Rapports locaux figés :
- build/hrtf-native-rendering/20261116T205053Z-8e97b81f/report.json
- build/hrtf-native-rendering/20261116T205104Z-592a83e4/report.json

Les hashes de bibliothèque, exécutable, outils et poids figurent dans ces rapports.

---

# Colab HRTF notebook and resumable training — 2026-11-16

The optional [A100 notebook](../notebooks/CyDSP_HRTF_A100.ipynb) passes nbformat
4.5 validation and syntax checks for all nine code cells. A local RTX 3090
execution with a simulated Drive runs source unpacking, verified subset
SOFA extraction, batch measurements, warm-start training, backup/restore,
a second-session completed-run restore, frozen-checkpoint evaluation,
numerical validation and plotting. Checks reject traversal paths, archive
type mismatches and corrupted backups before extraction.

A 100-update hybrid-24/width-96 run paused after update 50 then restored from
the standalone resume.pt produces bit-identical final weights, Adam state
and sampler state versus an uninterrupted run in the same environment.
Warm-up is not repeated; the original cosine schedule is retained.
This is same-GPU equivalence, not a cross-GPU reproducibility claim.

The source upload bundle is under 1 MB and excludes raw SOFA, CUDA binaries,
private Git history and local analysis reports. Matching notebook/bundle
hashes are pinned; complete checkpoints are saved atomically and Drive
snapshot pointers are updated only after read-back hash verification.
Older immutable generations are retained.

Test details are local in build/colab/resume-validation.json and
build/colab/notebook-validation.json. Google OAuth/Drive mounting, Linux wheel
installation and actual A100 40/80 GB execution have not run from this Windows
environment. Training GPU compute is separate from audio CPU rendering;
no A100 speedup, convergence or perceptual-quality result is inferred.
Production C++ code is unchanged.

---

# HRTF field quality study — 2026-11-16

Four registered variants, two validation-selected refinements and two frozen
test evaluations completed on RTX 3090 in 364.46 s total wall time. Screening
uses 8000 updates/case; refinement adds 20000 updates to each selected candidate,
with 50 disclosed warm-up updates per phase. The study freezes the cases,
selection rule and trainer source hash; screening/refinement do not evaluate
test directions. Both final hashes are frozen before separate test evaluation.

The same 881 KEMAR/D2 test directions give the following magnitude RMSE.
All grids span 200 Hz–20 kHz; lower is better, not a speed ratio or listening score.

| Experimental field | Sections/ear | Parameters | 256 log bins | 257 linear bins |
| --- | --- | --- | --- | --- |
| Previous pilot | 12 | 10443 | 1.771 dB | 2.847 dB |
| Hybrid compact field | 12 | 19275 | 1.390 dB | 2.069 dB |
| Hybrid higher-budget field | 24 | 26259 | 1.234 dB | 1.853 dB |

Linear-frequency error drops by 27.3% and 34.9%, mainly in the upper band.
Low-band error (200 Hz–2 kHz) is slightly worse: 0.726 dB originally versus
0.800/0.764 dB. The original model remains preserved. Nearest training
measurements still reach 0.916 dB on the linear grid and remain more faithful.
No neural field is enabled by the C++ library or replaces measured SOFA rendering.

The delay head predicts desired total low-band ITD. The preparation helper
subtracts the cascade's own phase slope before assigning causal additional
ear delays. Against an independent complex128 phase oracle, the implied ITD
RMSE drops from 58.424 to 2.410 us (12 sections), or 57.817 to 2.264 us (24).
This evaluates analytical phase plus ideal external delays, not streamed audio
or complete HRIR phase. Do not apply the raw head delay in addition to the
compensated residual. Current fields are designed for 96 kHz.

All 8802 directions have stable predicted poles. Exact plain-weight export,
safe weight-only load, source/data hashes and CPU/GPU numerical agreement pass.
Maximum magnitude oracle error is below 0.000067 dB; CPU/GPU discrepancy below
0.000149 dB and compensated phase restoration error below 0.000148 us.
The validator executes no archived training source. The reported maximum
additional delay covers the evaluated test directions, not a global buffer bound.

Detailed results, frozen selections, exact sources and numerical checks stay in
ignored build/hrtf-quality/20261116T183828Z-comparison-2d62321b and referenced runs.
New original MIT tools: diagnose_hrtf_field.py, compare_hrtf_quality.py and
check_hrtf_field.py. Optional setup/contracts remain in
[scripts/README.hrtf-training.md](../scripts/README.hrtf-training.md).
No listening, streamed-audio, target-rate conversion, cross-listener or CPU
renderer performance result is claimed; production C++ code is unchanged.

---

# Experimental CUDA HRTF pilot — 2026-11-16

The optional original training tool in scripts/train_hrtf_pilot.py ran on the
RTX 3090 with isolated PyTorch 2.9.1+cu128, NumPy 2.3.5 and h5py 3.15.1.
The complete 30000-update KEMAR/D2 pass (512 directions/update, plus 50 disclosed
warm-up updates) took 108.91 s of synchronized training compute and 111.17 s
including preparation, validation and export. Mean 3.630 ms/update; observed
1000-update blocks span 3.509–3.878 ms/update. Peak PyTorch allocation: 246692864
bytes. This excludes CUDA context/cache and other GPU users.

The MLP has 10443 learned parameters, two 64-unit hidden layers, 12 bounded
peaking sections per ear, per-ear level and right-minus-left ITD. The fixed
within-listener direction split has 7041 training, 880 validation and 881 test
directions. Equal directions/both ears share a partition; angular neighbors can
still cross partitions. Test directions are evaluated only after checkpoint
selection by validation.

Test magnitude RMSE: 1.771 dB on the 256-bin logarithmic 200 Hz–20 kHz grid,
2.847 dB on the independent 257-bin linear grid. Target ITD RMSE: 2.238 us;
mean absolute error: 1.743 us, relative to low-band phase-slope estimates.
On the same logarithmic test grid, nearest training measurements achieve
0.541 dB and the mean training spectrum 6.287 dB. The compact pilot therefore
does not yet match the measured bank; these are not listening scores.

Independent checkpoint checks pass: all 8802 directions have stable poles
(maximum radius 0.999676895); a complex128 RBJ oracle differs by at most
0.0000492 dB on 128 held-out directions. CPU/GPU differ by at most 0.0000839 dB
and 0.0001526 us. Safe weight-only loading, exact JSON parameter export and the
archived training-source hash pass. The self-tests cover gradients, response,
identity, known ITD and partition grouping.

Checkpoints, source snapshot, package freeze, CSV, JSON and French report stay
under ignored build/hrtf-runs/20261116T182038Z-D2-b922446c. The first 3000-step
attempt completed compute but failed its post-training checkpoint reload due
to a TorchVersion metadata object; storing the version as a plain string fixed
it before the complete pass. No C++ API or production renderer was added, and
no listening, full-phase or CPU-renderer performance result is implied.
Setup and contracts: [optional training guide](../scripts/README.hrtf-training.md).

---

# CyDSP 0.4.1 optional measured HRTF bank — 2026-11-16

Windows x64 MSVC Release: all three opt-in CTest suites pass (core DSP, SOFA
formats and measured HRTF integration). Each original SADIE II profile D1/D2/H10
at 48 kHz imports through both compact and complete backends. Float32 samples
and delays agree between readers; complete import retains float64. Each full
direction bank prepares without truncation. Impulse responses at three exact
directions per bank agree with loaded measurements within 8e-7*(1+abs(sample)),
including declared latency; changing-direction output remains finite.
This does not establish perceptual fit, listening quality or a CPU speed gain.

The verified signal files total 81,375,249 bytes. SHA-256, original float64
metadata, notices and the strict whole-folder limit of 100,000,000 bytes pass.
Installed data re-verifies byte-for-byte. A separate install with the data flag
OFF contains no SOFA files. Independent CMake package consumers find version
0.4.1 and the correct data path (or an empty path when disabled), without
requiring the complete SOFA SDK to consume the core.

All 20 original training SOFA files were explicitly downloaded at 96 kHz,
512 taps/ear (498,669,509 bytes), with archive size/MD5 and SHA-256 verification.
An independent h5py inspection confirms rate, shape and Apache-2.0 metadata.
They remain under ignored build/hrtf-training, outside the distributed bank.
At that data-preparation checkpoint no neural model had been trained. CyDoc passes all 142 English/French page pairs,
89 public header hashes and 2134 public declarations. No additional platform,
plugin or subjective listening validation is claimed.

---

# HOA filter design and analytic HRIR validation — 2026-11-16

MSVC x64 Release and AddressSanitizer pass the full regression suite with the
new tests in tests/HoaDesignRegression.cpp. Existing HrirBinaural/HoaBinaural
tests remain in SpatialRenderingRegression.cpp, AdvancedExpansionRegression.cpp
and the optional SofaRegression.cpp.

Original synthetic checks cover owned analytic FIRs versus the streaming model,
mirror/front symmetry, bank moves and transactional failure, Fibonacci unit
vectors/area, weighted LS/MagLS at orders 1–3 in float/double, low-band complex
agreement, held-out directions and frequencies between design bins, max-rE,
rotations, block splitting, failed preparation and realtime allocation/free.

The following independent installed-consumer measurement used real KEMAR
responses: 608 design directions and 102 held out, at 44100 Hz, 12 iterations,
2048-point FFT, no max-rE and equal directional weights. Values are normalized
RMS MAGNITUDE errors over both ears from 3 to 16 kHz; lower is better.
They are not speed ratios, dB, listening scores or a general perceptual guarantee.

| Ambisonic order | Spectral LS error | MagLS error |
| --- | --- | --- |
| 1 | 83.53% | 34.71% |
| 2 | 78.61% | 25.47% |
| 3 | 73.74% | 18.17% |

Both spectral designs add 1024 declared frames (23.22 ms at 44100 Hz); the legacy
overload keeps its previous latency. These are finite frequency-domain designs,
not an assertion of phase-perfect interpolation or individual listener matching.
The real dataset remains ignored because its metadata requests permission.
No listening test, new platform execution or plugin integration is claimed.
The implementation adds no third-party code dependency.
The updated assets-only installed consumer compiles/links without HDF5; its new
runtime launch remained blocked by a Windows sharing-violation file lock. No lock
or antivirus setting was changed. The full installed consumer ran the measurements
above, and native regression/ASan execution succeeded.

---

# CyDSP 0.4.0 SOFA/HRTF validation — 2026-11-16

Windows x64 Release (MSVC): the full DSP regression and optional SOFA regression
pass. The latter covers 16 original MIT synthetic SOFA files: old and modern
HDF5 layouts/indexes, float32/64 and big-endian data, compressed arrays, dense
metadata, empty attributes, listener transforms, receiver order, delays, rejected
formats/rates/budgets, cache integrity, distance selection, common-ear EQ and level
normalization. Unsupported compact filter pipelines fail rather than misdecode.

The compact build also passes these applicable tests and 3000 deterministic
single-byte mutations under AddressSanitizer. This is bounded regression coverage,
not a proof against every malformed HDF5 file. Complete SDK internal allocations
are outside the application buffer budgets.

Nearest-direction lookup agrees with exhaustive search over 3001 directions.
The scalar complex-response error for a half-frame delay at 9.6 kHz / 48 kHz
is 0.190983 (economy), 0.000318535 (realtimeHigh), 0.00000665728 (reference).
These are dimensionless numerical errors, not speed ratios or listening scores.
HRIR and order-zero HOA impulse outputs agree; realtime updates/process/reset
pass the allocation/free audit.

Production assets-only compilation and individual public-header/contract checks
pass without HDF5. Independent installed consumers link/run with assets only
and with the complete importer; exported CyDSP targets contain no SDK build path.
The tested external SDK is static HDF5 2.2.0 with zlib 1.3.1.

A real MIT KEMAR file imports as 710 points × 512 taps at 44100 Hz through the
complete reader. It remains in the ignored validation folder: its metadata requests
the author's permission, and no measured dataset is redistributed.

No new macOS/Linux/ARM execution or CyMetaSound integration is claimed here.
Code notices and dataset rights are documented in the bilingual SOFA guide.

---

# CyDSP 0.3.2 optimization validation — 2026-11-14

Compiler-input SHA-256: `114977a9b190a6dd11a017bddf6c144cbc5d69230976cb0ef419782ec3a73bd7`.

MSVC x64 Release, clang-cl x64 Release, x64 AddressSanitizer and Quest 3
Android ARM64/NEON completed the regression suite. Windows ARM64 compiled its
static library, individual public headers and regression objects. Android ARM64
compiled/linked NEON and SVE; actual SVE execution/vector lengths remain unverified.

New independent checks cover direct SIMD wavefolder tails, exact same-channel
aliases and typed intermediate mix rounding; the precise transient's unchanged
scalar power argument; dynamic-EQ filter/envelope oracles with 1/2/3/7 channels,
mono/full sidechains, in-place processing, split blocks, resets and automation;
and saturation DC-cache transitions including signed zero. Existing convolution,
allocation/free, alias, latency and scheduled-budget regressions still pass.

Object inspection covered 84 x64, 84 Android ARM64 and 83 Windows ARM64 production
objects, with no fused FP instructions or wide-ISA leakage. The installed exact
0.3.2 package compiled, linked and ran a consumer with the updated private
MatrixConvolver layout. Rebuild headers, archive and consumers together.

Detailed timing repetitions, discarded experiments, raw audio comparisons and
integration-oriented notes remain local under the Git-ignored build directory.
These checks do not imply a listening test or a new CyMetaSound integration run.
macOS/Linux desktop execution remains deferred; Windows ARM64 and SVE runtime,
and x86/ARM32 reruns, are still outstanding for this snapshot.

---

# Ten-lot DSP / spatial expansion — 2026-11-13

The latest extension results, exact source digest, CPU costs and numerical errors
are in [ExpansionValidation.fr.md](ExpansionValidation.fr.md). The
[API guide](ExpansionGuide.fr.md) defines the supported configurations and limits.
The older measurements and validation snapshots below are retained as history;
their hashes and coverage must not be attributed to the current extension.

---

# Granulation and analysis validation — 2026-11-12

The feature snapshot has compiler-input SHA-256
`9047361b0fbc9ba1372504c7cfecc9a8e9a06cdd4ea168dc1d7b1643f5262148`.
These results supersede the platform coverage of the earlier optimization snapshot
below. Public contracts and limits are in [GranulationAndAnalysis.md](GranulationAndAnalysis.md).

| Check on this feature snapshot | Result |
| --- | --- |
| MSVC x64 Release regressions and 774 general measurement records | Passed; scalar/SSE2/AVX2/AVX-512 executed |
| clang-cl x64 Release regressions | Passed |
| x64 with explicit SIMD disabled | Passed |
| x64 AddressSanitizer | Passed |
| Process/reset/trigger allocation and free audit | Passed |
| Quest 3 Android ARM64 regressions and 526 general measurement records | Passed; scalar/NEON executed; temporary executables removed |
| Windows ARM64 static libraries, individual public headers, regression objects and integration example | Compiled; runtime unverified |
| Android ARM64 library, individual public headers, regression executable and integration example | Compiled/linked; SVE runtime unverified |
| ISA isolation / no fused floating-point contraction | Passed: 49 x64, 47 Windows ARM64, 48 Android ARM64 production objects |
| Source, build-policy and pinned-notice verification | Passed: 144 C++ sources/headers, same five pinned components |

Independent numerical tests include direct DFT/PSD Parseval checks, variable-window
complex projection references, naive sliding peak/RMS/DC/correlation windows,
analytical grain offsets/reverse/pan/envelopes, block splitting, seeded clouds,
overflow transaction checks, 4x/8x intersample sine peaks, and direct-form-I
BS.1770 weighting/gating and LRA quantiles. 44.1/48/96 kHz tone checks exercise
loudness rate handling. NaN/infinity rejection and failed preparation preserve
prior states. The compiled integration sketch uses only public CyDSP APIs.

The grain octave fixture (0.35 cycles/input sample, ratio 2) measures approximately
93.20 dB less residual alias energy with the existing band-limited reader than
linear reading, for both float/double. This is one synthetic fixture, not a general
stop-band guarantee. LUFS integrated error against the independent 48 kHz oracle
is at most 5.33e-15 LUFS across x64/Quest runs. A 997 Hz, -20 dBFS peak mono tone
reads approximately -23.01 LUFS. These checks do not establish official EBU/ITU
conformance; the complete official vector suite and certification have not run.

macOS and desktop Linux execution remain deferred. Windows ARM64 runtime,
SVE-capable hardware/vector lengths, x86 and ARM32 reruns remain unverified.
No physical audio-device test or listening test is implied. Production presets
still build static libraries with native tests and measurements disabled.

Local raw reports and logs are retained under
`build/granulation-analysis-2026-11-12/`, outside source distributions.
Reproduce with `scripts/run_native.py --measure`, its `--compiler clang`,
`--arch scalar` and `--sanitize` variants, `scripts/build_arm64.py --compile-checks`,
`scripts/build_android.py --abi arm64-v8a --measure`, and an explicitly selected
owner-authorized device via `scripts/run_android_device.py --abi arm64-v8a --measure`.

---

# Optimization validation — 2026-11-12

The preceding optimization reports identify compiler-input source SHA-256
`0f8e550f794a20e11817f56d4e72983898c09ebfc1ff50291d719a8e72b3cb2f`. See [measurements](Measurements.md) for workloads, quality bounds,
before/after data and reproduction instructions.

| Earlier optimization check | Result |
| --- | --- |
| MSVC x64 Release, scalar/SSE2/AVX2/AVX-512 regressions and measurements | Passed |
| Six interleaved before/after pairs, 680 focused cases each | Passed |
| clang-cl x64 Release regressions | Passed |
| x64 without explicit SIMD | Passed |
| x64 AddressSanitizer including offset/tail buffers | Passed |
| Process/reset allocation and free auditing | Passed |
| Quest 3 ARM64 scalar/NEON regressions and quality, 502 general records | Passed; temporary executables removed |
| Windows ARM64 libraries, public headers and regression objects | Compiled; runtime unverified |
| Android ARM64 SVE library and test executable | Compiled and linked; SVE runtime unverified |
| ISA isolation and no fused FP contraction | Passed: 43 x64, 41 Windows ARM64, 42 Android ARM64 production objects |

The actual runtime backends are scalar/SSE2/AVX2/AVX-512 on the Ryzen and
scalar/NEON on the Quest. macOS and desktop Linux runtime remain deferred.
Windows ARM64 runtime, SVE vector lengths, x86 and ARM32 reruns are outstanding.
No physical audio-device measurement or listening test is implied. Numerical
gate changes are bounded by the independent oracle; bitwise identity across all
architectures is not promised. Both static production targets retain tests and
measurements off by default.

---

# Earlier split validation — 2026-11-12

The preceding split was validated as two independent projects. CyAudioCore checks
below were not repeated as part of the subsequent CyDSP optimization work.

| Check | Result |
| --- | --- |
| Windows x64 Release, independent DSP regressions | Passed |
| Windows x64 Release, services/files/sources/MIDI/plugin regressions | Passed |
| VST3 optional controller state, short read/EOF/error/capacity, direct + worker | Passed, original fixtures |
| Windows x64 AddressSanitizer, both regression executables | Passed |
| Independent compilation of all public headers | Passed, x64 and ARM64 |
| Windows ARM64 clang-cl static archives with NEON | Passed; no ARM64 Windows runtime |
| SIMD isolation / no fused FP contraction | 181 x64 + 179 ARM64 objects inspected |
| Installed CMake packages and independent consumer compile/link/run | Passed |
| Installed headers, exact notices and relocatable exports | Passed |
| CyAudioCore -> public CyDSP boundary, no reverse dependency | Passed |
| Source manifests | 5 DSP + 13 service components; hashes/notices verified |

No CI, push, release, application migration, physical audio/MIDI
or CyUI integration was performed. Native editor/worker tests use original fixtures
and hidden Windows parent windows, not installed third-party plugins.

macOS and desktop Linux execution remain deferred by the owner. At that split
checkpoint Android/Quest runtime, x86, ARM32 and SVE execution had not been repeated. The optimization snapshot above now covers Quest ARM64.
x86, ARM32 and SVE runtime remain unverified for the latest snapshot. Older
monolithic measurements and platform results do not establish this snapshot's
coverage. Windows ARM64 compilation does not prove execution on an ARM64 device.

Candidate build logs and the application plan for that checkpoint are retained
in the local build/separation-* directories, outside source packages.
README structure and relative links are verified locally; actual GitHub rendering
is unverified because no publication was requested.

Current second-extension evidence: [advanced validation](AdvancedExpansionValidation.fr.md).
Earlier measurements above retain their original source hashes.
