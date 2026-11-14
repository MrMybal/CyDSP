# Noyaux par tuiles et ADAA exacte — 14 novembre 2026

Cette passe concerne uniquement CyDSP. La référence est le commit `b973c1b`.
Le rapport d’intégration CyMetaSound du même jour a servi à choisir les points
chauds ; aucune source, intégration ni DLL du plugin n’a été modifiée ou
remesurée par cette passe.

## Ce que l’audit a trouvé

| Constat dans le code de référence | Conséquence mesurée |
| --- | --- |
| Tanh ADAA « précis » : tout échantillon à moins de 1 % du précédent repassait par `exp`/`log1p` de la libm en scalaire. Sur un signal de programme (énergie loin sous Nyquist) c’est presque tous les échantillons. | 7,6 à 9,1 µs par bloc stéréo, quel que soit le noyau vectoriel. |
| Atan ADAA précis entièrement scalaire ; le mode `fast` retombait aussi sur la libm pour les échantillons proches. | 7,5 µs (précis), 6,5 à 7,7 µs (`fast`). |
| `validateAnalysis` : test scalaire avec branche par échantillon dans chaque processeur qui l’appelle (une trentaine de fichiers). | Environ 0,55 µs par bloc stéréo de 256, soit 70 % du coût d’un hard clip direct. |
| `Dynamics` hors chemin lié (canaux indépendants, enveloppes inégales, lookahead, expandeur) : `log10` et `pow` par échantillon et par canal. | 17 à 23 µs en stéréo, 66 µs pour 8 canaux. |
| `DynamicEqualizer` : `log10` et `pow` par échantillon et par bande. | 16,6 µs par bande. |
| `TransientShaper` : `pow` (précis) ou exponentielle scalaire (`fast`). | 9,9 µs / 6,6 µs. |
| Les polynômes des noyaux étaient des chaînes de Horner et chaque groupe vectoriel une seule expression : plus de cent opérations dépendantes. | Le processeur ne peut pas chevaucher deux groupes ; le noyau est limité par la latence, pas par le débit. |
| La table AVX-512 renvoyait vers les noyaux AVX2 pour l’ADAA et la courbe du compresseur. | Aucun noyau de ces familles n’utilisait 512 bits. |

## Ce qui change

- **ADAA tanh et atan exactes** (`SaturationSimd.h`). Le noyau calcule la moyenne
  exacte de la forme entre deux échantillons consécutifs avec des identités sans
  annulation (détail dans [Architecture.md](Architecture.md), section *Tile
  kernels*). Plus de seuil de proximité, plus de repli libm, plus de dépendance
  du temps de calcul au signal. L’erreur absolue mesurée reste sous 1e-15 avant
  gain de sortie, contre environ 1e-11 pour la loi scalaire qu’il remplace.
- **Formes sans ADAA** tanh/atan vectorisées (`antialias=false`).
- **Étages d’entrée et de publication** vectorisés pour float et double : drive,
  biais, borne ±1e6, puis arrondi au type d’échantillon et mix dry/wet. Ils ont
  exactement l’ordre d’opérations du code scalaire.
- **Passes courtes par tuile et arbres d’Estrin** : mêmes produits et mêmes
  sommes, organisés pour que plusieurs groupes vectoriels soient en vol.
- **Vraie largeur AVX-512** : les noyaux sont écrits une fois et instanciés avec
  4 doubles (AVX2) ou 8 doubles (AVX-512).
- **`Dynamics`** : tous les chemins passent par tuiles de 64 frames. Récurrences
  d’enveloppe, file de lookahead et machine d’état du gate restent scalaires et
  dans le même ordre ; courbe de transfert, magnitude du détecteur lié, minimum
  du gain, application du gain, plafond du limiteur et mix sont vectoriels.
- **`DynamicEqualizer`** : poids de bande d’une tuile en une passe vectorielle.
- **`TransientShaper`** en mode `fast` : loi de gain d’une tuile en une passe
  vectorielle double précision. Le mode `precise` garde `std::pow` bit à bit.
- **`validateAnalysis`** : test entier sans branche sur les motifs IEEE, mêmes
  valeurs acceptées et refusées.

## Résultats avant/après

Ryzen 9 7900X, Windows x64, MSVC 19.40, Release et `/fp:strict` des deux côtés.
Le même banc source (`tests/HotspotMeasurements.cpp`, étendu de 76 cas de
programme) est compilé contre l’archive de référence et contre l’archive
optimisée. **214 cas**, float et double, blocs de 256 frames à 48 kHz (deux cas
à 480 frames). Cinq paires de processus alternés, affinité sur le processeur
logique 4 ; 256 blocs de chauffe, médiane de neuf moyennes de 128 appels, puis
médiane des cinq processus. Aucun autre build ou banc ne tournait pendant la
campagne.

Durées en **microsecondes par bloc**. La réduction est
`(1 - temps après / temps avant) × 100`. Les signaux « programme » sont un sinus
à 110 Hz, un sinus à 997 Hz et un multitone à cinq composantes, niveau 0,5,
drive 3. Ces pourcentages ne représentent pas le CPU total d’une application.

| Traitement (stéréo, 256 frames, float) | Avant (µs) | Après (µs) | Réduction | Facteur |
| --- | ---: | ---: | ---: | ---: |
| Tanh ADAA, multitone (programme) | 9.104 | 1.670 | 81.7 % | ×5.45 |
| Tanh ADAA, 110 Hz | 7.657 | 1.662 | 78.3 % | ×4.61 |
| Tanh ADAA + étage d’entrée + mix (`processMixed`) | 9.152 | 1.755 | 80.8 % | ×5.21 |
| Tanh sans ADAA + mix | 5.000 | 1.084 | 78.3 % | ×4.61 |
| Atan ADAA précis, multitone | 7.554 | 2.077 | 72.5 % | ×3.64 |
| Atan ADAA + étage d’entrée + mix | 8.404 | 2.156 | 74.3 % | ×3.90 |
| Atan sans ADAA + mix | 4.284 | 0.823 | 80.8 % | ×5.21 |
| Hard clip ADAA, multitone | 1.816 | 0.423 | 76.7 % | ×4.29 |
| Wavefolder ADAA, multitone | 2.105 | 0.723 | 65.6 % | ×2.91 |
| Cubic ADAA (noyau scalaire, inchangé) | 3.320 | 2.707 | 18.5 % | ×1.23 |
| Tanh ADAA, silence | 0.858 | 0.427 | 50.3 % | ×2.01 |
| Compresseur 4:1 lié | 2.311 | 0.934 | 59.6 % | ×2.47 |
| Compresseur 6:1 lié | 3.834 | 1.288 | 66.4 % | ×2.98 |
| Compresseur 4:1, canaux indépendants | 16.895 | 1.854 | 89.0 % | ×9.11 |
| Compresseur 6:1, canaux indépendants | 16.913 | 2.423 | 85.7 % | ×6.98 |
| Compresseur 4:1 lié, lookahead 3 ms | 18.386 | 3.038 | 83.5 % | ×6.05 |
| Compresseur 6:1, sous le seuil (-40 dB) | 2.127 | 0.805 | 62.2 % | ×2.64 |
| Compresseur 6:1, au-dessus du knee (-8 dB) | 3.735 | 1.266 | 66.1 % | ×2.95 |
| Compresseur 8 canaux indépendants | 66.560 | 7.511 | 88.7 % | ×8.86 |
| Limiteur | 4.039 | 1.297 | 67.9 % | ×3.11 |
| Expandeur | 6.467 | 2.174 | 66.4 % | ×2.97 |
| Noise gate | 6.311 | 2.966 | 53.0 % | ×2.13 |
| Transient, `fast`, audio | 6.607 | 1.697 | 74.3 % | ×3.89 |
| Transient, `fast`, `processGain` | 6.424 | 1.649 | 74.3 % | ×3.90 |
| Transient, `precise` (`pow` de référence) | 9.892 | 9.495 | 4.0 % | ×1.04 |
| Dynamic EQ, 1 bande | 16.607 | 3.453 | 79.2 % | ×4.81 |
| Dynamic EQ, 3 bandes | 43.534 | 9.700 | 77.7 % | ×4.49 |
| EQ lissé 8 bandes (témoin, code inchangé) | 8.947 | 8.941 | 0.1 % | ×1.00 |
| Convolution IR 512, scheduled 128/1024, sans `spectralHead` | 5.106 | 5.095 | 0.2 % | ×1.00 |
| Convolution IR 8192, scheduled 128/1024, sans `spectralHead` | 11.229 | 11.171 | 0.5 % | ×1.01 |

[Données complètes des 214 cas, répétitions, rendus et empreintes](TileKernelOptimization.json).

**164 cas sur 214 gagnent plus de 3 %, 3 perdent plus de 3 %.** Les trois reculs
sont des convolutions à partition de 32 (`matrix.512.p32.scheduled` +8,9 %,
`matrix.8192.p32.nonuniform` +8,4 %, `matrix.512.p32.nonuniform` +5,6 %). Le code
de convolution n’était pas encore modifié lors de cette campagne (la tête
spectrale décrite plus bas a été ajoutée ensuite) ; l’écart est reproductible sur les cinq
paires et sa cause n’a pas été recherchée (les partitions 64, 128 et 256 restent
à ±1 %). Le mode `precise` du transient n’est pas accéléré : son contrat est
l’identité bit à bit avec `std::pow`.

## Qualité

- **Rendus float : identiques bit à bit.** Le banc écrit 20 blocs par cas après
  remise à zéro ; sur les 1 359 360 échantillons float des deux archives, l’écart
  absolu maximal est **0**.
- **Rendus double : écart maximal 2,2e-11**, sur l’atan `fast` (l’ancienne
  approximation était la moins exacte des deux). Atan précis : 1,6e-12 ; tanh :
  5,6e-13 ; dynamiques, transient et Dynamic EQ : 2e-15 au plus. Dans les deux
  premiers cas c’est la loi de référence qui portait l’erreur (différence de
  primitives divisée par un petit pas), pas le nouveau noyau.
- Oracles indépendants (libm, identités bien conditionnées) dans
  `tests/HotspotRegression.cpp` : moyenne exacte tanh/atan sur des extrémités
  proches, lointaines, de signes opposés, zéros signés et ±1e6, erreur maximale
  1,6e-14 avec un gain de sortie de 16 ; courbes expandeur, Dynamic EQ et
  transient à 2,3e-14 relatif ; étages typés comparés par égalité exacte à leur
  expression scalaire ; raccourci « canal constant » égal bit à bit au chemin
  général, y compris entre un bloc de 71 frames et des blocs de 7.
- Aucun test d’écoute n’a été réalisé. Les mesures spectrales d’aliasing du
  rapport précédent n’ont pas été relancées ; la loi ADAA étant la même moyenne,
  évaluée plus exactement, elles ne devraient pas changer, sans que cela soit
  remesuré ici.

## Changements de comportement à connaître

- Sur x64, tanh/atan ADAA n’appliquent plus la substitution du point milieu sous
  un pas relatif de 1e-5 : ils renvoient la moyenne exacte. Les autres backends
  gardent la loi scalaire. L’écart entre les deux lois atteint environ 5e-11 fois
  le gain de sortie en double ; il est invisible en float.
- `MathAccuracy::fast` et `precise` partagent le même noyau atan sur x64.
- Le raccourci des canaux constants s’applique maintenant à tanh, atan, hard
  clip et wavefolder, avec ou sans ADAA, à partir de 8 frames.
- `Dynamics` : le vu-mètre `gainReductionDb` est calculé une fois par bloc à
  partir de la dernière enveloppe (même formule, même valeur). Sur les builds
  sans SIMD, la courbe du chemin général utilise les mêmes noyaux scalaires
  `audioLog`/`audioExp` que le chemin lié au lieu de `log10`/`pow`.
- `validateAnalysis` parcourt tout le bloc au lieu de s’arrêter au premier
  échantillon invalide ; le statut renvoyé et l’absence d’écriture sont
  inchangés.

## Validation exécutée

| Configuration | Résultat |
| --- | --- |
| MSVC 19.40 x64, AVX2 et AVX-512 exécutés, régressions complètes | réussi |
| MSVC x64 sans SIMD (`run_native.py --arch scalar`), régressions complètes | réussi |
| clang-cl x64 (`run_native.py --compiler clang`), régressions complètes | réussi |
| `scripts/verify_sources.py` | réussi |

Non exécuté dans cette passe : x86 32 bits, AddressSanitizer, Windows ARM64,
Android/Quest (NEON), SVE, Linux/macOS, et toute mesure dans le plugin Unreal.
Les backends NEON/SVE/SSE2 n’ont pas de nouveaux noyaux : ils utilisent les lois
scalaires, avec la nouvelle validation et l’organisation par tuiles.

## Tête spectrale de la convolution (option `spectralHead`)

Mesure par désactivation sélective dans le code de référence, IR 512 stéréo,
scheduled 128/1024, bloc de 256 : tête FIR directe ≈ 1,5 µs, niveau FFT précoce
≈ 2,9 µs, copies et historique ≈ 0,6 µs. Pour IR 8192 : niveau précoce ≈ 4,7 µs,
niveau tardif progressif ≈ 4,4 µs.

`ConvolutionConfig::spectralHead` (faux par défaut, `MatrixConvolver` seulement)
supprime la FIR quand c’est possible. Si, à une frontière de partition précoce,
l’appel contient encore une partition entière, sa transformée directe — celle
que le niveau précoce calcule de toute façon — est multipliée par le spectre des
`earlyPartition` premiers taps et ajoutée aux spectres déjà accumulés ; une seule
transformée inverse par sortie restitue la tête et les partitions plus anciennes.
La latence reste nulle, aucune IR n’est tronquée, le nombre de transformées est
inchangé. Les partitions incomplètes gardent la FIR ; le passage FIR → spectral
coûte une FIR sur l’ancien historique. Sans l’option, le code suit le chemin
précédent ; ses temps restent à quelques pour cent de ceux de l’exécutable
précédent dans la même passe (entre -6,6 % et +2,2 %).

Une compilation de shaders Unreal extérieure à cette tâche occupait la machine
pendant cette mesure. Les valeurs sont donc le **minimum** de quatre passes
alternées par exécutable (affinité sur le processeur logique 4, priorité haute),
et non la médiane de la campagne ci-dessus ; le témoin `eq.smoothed.8` y vaut
8,94 µs dans les deux exécutables, comme dans la campagne au repos. Stéréo,
float, µs par bloc.

| IR, partitions, bloc | Sans l’option | `spectralHead` | Réduction |
| --- | ---: | ---: | ---: |
| 512, scheduled 128/1024, 256 | 4.983 | 3.121 | 37.4 % |
| 512, scheduled 256/1024, 256 | 6.380 | 2.673 | 58.1 % |
| 512, scheduled 128/1024, 480 | 9.808 | 7.416 | 24.4 % |
| 8192, scheduled 128/1024, 256 | 11.027 | 8.986 | 18.5 % |
| 8192, scheduled 256/1024, 256 | 11.722 | 7.612 | 35.1 % |
| 8192, nonuniform 128/1024, 256 | 9.060 | 6.988 | 22.9 % |
| 8192, nonuniform 256/1024, 256 | 10.024 | 6.102 | 39.1 % |
| 8192, scheduled 128/1024, 480 | 20.844 | 18.081 | 13.3 % |
| 16384, 4 routes, scheduled 128/1024, 256 | 17.308 | 13.552 | 21.7 % |
| 16384, 4 routes, scheduled 256/1024, 256 | 18.512 | 11.297 | 39.0 % |
| 65536, scheduled 128/1024, 256 | 18.762 | 16.486 | 12.1 % |
| 65536, scheduled 256/1024, 256 | 19.497 | 15.402 | 21.0 % |

Avec l’option, une partition précoce égale au bloc de l’hôte devient le meilleur
réglage : la FIR, qui pénalisait les grandes partitions, disparaît pour les blocs
alignés. Avec des blocs de 480 frames et une partition de 128, environ deux
partitions sur 3,75 par appel sont spectrales ; le gain est plus faible. Le mode
`nonuniform` reste moins coûteux en moyenne que `scheduled`, avec ses pics de
calcul à chaque frontière tardive.

La sortie du chemin spectral diffère de la somme directe par l’arrondi de la FFT
et dépend donc du découpage en blocs de l’hôte, d’où l’option explicite.
`MatrixSchedulingRegression.cpp` compare les trois modes, en float et double, à
une convolution directe en précision étendue pour sept découpages (partitions
entières, incomplètes, blocs de 1, 31, 480 frames, séquences mixtes), en place
et après `reset`, avec les tolérances existantes (3e-4 float, 2e-11 double).

## FFT : mêmes opérations, moins de passes

La FFT reste le radix-2 de référence : mêmes papillons, mêmes facteurs, même
ordre d’opérations par valeur. Trois réorganisations, sur x64 :

- **Permutation fusionnée avec les deux premiers étages.** Le bloc permuté
  `reverse[j]` ne dépend que de `x[j]`, `x[j+n/4]`, `x[j+n/2]` et `x[j+3n/4]` :
  les quatre quarts sont lus de façon contiguë, les étages `half=1` et `half=2`
  sont calculés en registres et chaque bloc est écrit à sa place permutée. Trois
  passes sur le tampon deviennent une. Pour la FFT réelle directe, l’entrée
  réelle est déjà la suite compacte pair + i·impair : plus de copie d’empaquetage.
- **Inverse réelle** : la fusion des bins (`mergeRealBin`) est vectorielle, et
  la mise à l’échelle finale est une seule passe vectorielle vers la sortie.
- **Pipelines progressifs** (`MatrixConvolver` et `ScheduledConvolver` en mode
  `scheduled`) : les travaux de découpe et de fusion du spectre utilisent les
  mêmes noyaux par plage de bins. Le nombre de travaux et leur budget par frame
  sont inchangés.

**Les résultats sont identiques bit à bit.** Un test permanent
(`fftReferenceIdentity`) compare par égalité stricte les FFT réelle et complexe,
directe et inverse, de 2 à 8192 points, en float et en double, sur chaque backend
disponible, à la transformée de référence (permutation, un étage scalaire à la
fois, recombinaison et mise à l’échelle scalaires). Les 21 422 080 octets de
rendus du banc, convolutions float et double comprises, sont identiques avant et
après.

Minimum de cinq passes alternées par exécutable, affinité sur le processeur
logique 4, priorité haute ; µs par transformée.

| FFT réelle | Avant | Après | Réduction |
| --- | ---: | ---: | ---: |
| Directe 64, float | 0.091 | 0.061 | 33.3 % |
| Inverse 64, float | 0.109 | 0.067 | 38.6 % |
| Directe 256, float | 0.205 | 0.130 | 36.5 % |
| Inverse 256, float | 0.297 | 0.140 | 52.9 % |
| Directe 512, float | 0.408 | 0.227 | 44.3 % |
| Inverse 512, float | 0.591 | 0.259 | 56.1 % |
| Directe 2048, float | 1.605 | 0.980 | 38.9 % |
| Inverse 2048, float | 2.455 | 1.078 | 56.1 % |
| Directe 256, double | 0.302 | 0.230 | 24.0 % |
| Inverse 256, double | 0.388 | 0.245 | 36.7 % |
| Directe 2048, double | 2.680 | 1.950 | 27.2 % |
| Inverse 2048, double | 3.485 | 2.379 | 31.7 % |

Effet sur la convolution (stéréo, float, bloc de 256 sauf mention, µs par bloc ;
« avant » est l’état avec la tête spectrale mais sans cette passe FFT) :

| IR, partitions | Avant | Après | Réduction |
| --- | ---: | ---: | ---: |
| 512, scheduled 128/1024 | 5.020 | 3.991 | 20.5 % |
| 512, scheduled 128/1024, `spectralHead` | 3.073 | 2.060 | 33.0 % |
| 512, scheduled 256/1024, `spectralHead` | 2.680 | 1.729 | 35.5 % |
| 512, scheduled 128/1024, bloc 480, `spectralHead` | 7.302 | 5.388 | 26.2 % |
| 8192, scheduled 128/1024 | 10.984 | 9.302 | 15.3 % |
| 8192, scheduled 128/1024, `spectralHead` | 8.927 | 7.039 | 21.2 % |
| 8192, scheduled 256/1024, `spectralHead` | 7.725 | 6.002 | 22.3 % |
| 8192, nonuniform 256/1024, `spectralHead` | 6.204 | 4.177 | 32.7 % |
| 8192, scheduled 128/1024, bloc 480, `spectralHead` | 18.066 | 14.883 | 17.6 % |
| 16384, 4 routes, scheduled 256/1024, `spectralHead` | 11.327 | 9.548 | 15.7 % |
| 65536, scheduled 256/1024, `spectralHead` | 15.499 | 13.535 | 12.7 % |

Cumul depuis la référence `b973c1b`, configuration scheduled 128/1024 : IR 512
de 5,1 à 4,0 µs sans rien changer côté hôte, 1,7 µs avec `spectralHead` et une
partition précoce de 256 ; IR 8192 de 11,2 à 9,3 µs, 6,0 µs avec l’option et 256.
Les témoins sans FFT (`eq.smoothed.8`, saturateur, compresseur) restent à ±0,6 %.

## Ce qui reste

- **FFT sans permutation (non réalisé, mesuré comme peu utile)** : depuis la
  fusion avec les deux premiers étages, la permutation n’est plus une passe à
  part mais une simple dispersion des écritures. En remplaçant temporairement
  ces écritures par des écritures contiguës (résultat faux, temps valide), la
  FFT réelle directe de 64 points ne gagne que 2,6 % ; les tailles supérieures
  n’ont pas pu être mesurées ainsi, le banc rejetant les résultats faux. Passer
  les convolveurs en transformées non ordonnées changerait les arrondis pour
  ce gain-là : abandonné.
- **Recombinaison réelle fusionnée (essayée, retirée)** : la même expérience
  attribue environ 22 % de la FFT directe de 64 points à la passe de découpe du
  spectre. Un noyau fusionnant la recombinaison inverse avec la passe de tête
  (sans tampon intermédiaire) a été écrit : résultats identiques bit à bit, mais
  inverse **plus lente de 10 à 15 %** de 128 à 2048 points (0,142 → 0,158 µs à
  256 points float), +1 à 2 % à 64 points. Une longue expression par groupe
  vectoriel empêche le chevauchement des groupes, comme pour le saturateur ;
  deux passes courtes se recouvrent. Le noyau est retiré, et la fusion directe
  (dernier étage + découpe), qui allongerait encore la chaîne, n’a pas été
  tentée.
- **Niveau tardif progressif** : il garde ses passes d’empaquetage et d’émission
  scalaires et ses étages un par un pour conserver son budget de travaux ; en
  IR 8192 le mode `scheduled` coûte environ 1,8 µs de plus que `nonuniform`.
- **Convolution** : la FIR directe reste nécessaire pour les partitions
  incomplètes.
- **Transient `precise`** : inchangé par contrat. Le mode `fast` est maintenant
  à 2e-14 de la puissance décimale ; c’est le réglage temps réel conseillé.
- **Cubic et hystérésis** : toujours scalaires.
- **NEON** : pas de noyaux équivalents ; le Quest garde les chemins scalaires.
- Une variante float 32 bits des noyaux tanh/atan (deux fois plus de voies)
  pourrait servir de palier économique ; elle n’existe pas.
