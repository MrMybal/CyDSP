# Optimisations DSP après diagnostic détaillé — 14 novembre 2026

Cette passe concerne uniquement CyDSP. La référence est le commit `6b1fb80`, qui
contient déjà la première campagne d’optimisations. Les rapports CyMetaSound ont
servi à choisir les points chauds ; aucune intégration ni DLL du plugin n’a été
mise à jour ou remesurée par cette passe.

## Résultats avant/après

Ryzen 9 7900X, Windows x64, MSVC 19.40, Release et `/fp:strict` dans les deux
archives. Le même banc source est compilé avec les mêmes options contre
l’archive de référence figée et l’archive optimisée. **138 cas**, float/double,
blocs de 256 échantillons à 48 kHz. Cinq paires de processus alternés, affinité
du processus sur le processeur logique 4 ; 256 blocs de chauffe, puis médiane
de neuf moyennes de 128 appels, enfin médiane des cinq processus. Aucun autre
benchmark ou build de cette tâche ne tourne pendant cette campagne.

Durées en **microsecondes par bloc**, sauf les deux dernières lignes : elles
mesurent respectivement un lot de **34 conceptions** et de **64 appels métier**.
Les lignes audio ci-dessous utilisent float. La réduction est
`(1 - temps après / temps avant) × 100` : un nombre positif signifie moins de
temps CPU. Ce pourcentage ne représente pas le CPU total d’une application.

| Traitement | Avant (µs) | Après (µs) | Réduction du temps |
| --- | ---: | ---: | ---: |
| Compresseur 6:1, centre du knee, -24 dB | 10.296 | 4.074 | 60.4 % |
| Compresseur 6:1, au-dessus du knee, -8 dB | 8.488 | 4.069 | 52.1 % |
| Tanh ADAA, signal variable | 8.459 | 7.410 | 12.4 % |
| Tanh ADAA, silence | 3.567 | 0.899 | 74.8 % |
| Hard clip ADAA | 3.895 | 1.843 | 52.7 % |
| Convolution stéréo, IR 512, scheduled 128/1024 | 8.206 | 6.022 | 26.6 % |
| Convolution stéréo, IR 8192, scheduled 128/1024 | 15.275 | 12.227 | 20.0 % |
| Conception de 34 bells, paramètres changeants | 1.380 | 0.958 | 30.6 % |
| Matériau, 64 appels, épaisseur ×2 | 5.274 | 3.140 | 40.5 % |

[Données complètes des 138 cas, répétitions, rendus et empreintes](DetailedHotspotOptimization.json).

**Les gains ne sont pas uniformes.** Tanh sur signal lent prend ici +4,8 % en
float et +7,1 % en double ; certains chemins wavefolder restent autour de
+3 à +6 %. Ce sont les résultats conservés, avec les cinq répétitions dans
le JSON. Le repli de précision tanh et les frais de routage ont aussi un coût.
Aucun gain face à V2/Unreal n’est déduit de cette comparaison entre deux CyDSP.

## Changements retenus

- **Dynamique liée sans lookahead** : la récurrence du détecteur garde son ordre,
  puis les courbes sont évaluées par tuiles de 64 en double, avec noyaux privés
  AVX2/AVX-512 et repli scalaire. Les historiques différents après fonctionnement
  indépendant restent sur le chemin général. Un programme utilisé comme son
  propre sidechain n’est plus rescanné deux fois ; les entrées distinctes restent
  validées. Le vu-mètre conserve la formule d’origine en fin de bloc.
- **Convolution** : copies d’entrée/sortie regroupées jusqu’à la frontière de
  partition, avec les mêmes nombres et ordre de travaux dus au retour de chaque
  appel. La tête FIR traite plusieurs sorties temporelles en SIMD. La somme des
  taps est réorganisée ; le routage, les budgets des partitions, la réponse complète
  et la latence algorithmique nulle sont conservés.
- **ADAA** : choix de courbe hors des boucles, hard clip et intégrale tanh en SIMD
  double. Les paires tanh presque identiques reprennent l’intégrale libm ; le seuil
  de passage au point milieu reste `1e-5`. Un canal tanh exactement constant traite
  sa première transition, puis remplit le reste sans changer son historique.
- **EQ et acoustique** : conversion de gain par réduction de domaine
  double, dénominateur commun des coefficients normalisés, et opérations exactes
  pour les épaisseurs matériau usuelles ×0/×0,5/×1/×2. Les contrôles de stabilité
  des filtres et la validation transactionnelle des matériaux restent actifs.

Les noyaux log/exp originaux ont leurs tables et polynômes privés. Les valeurs
hors domaine normal utilisent libm ; le logarithme près de 1 évite la soustraction
de deux nombres proches. Aucun fast-math global, FMA implicite, dépendance de
production supplémentaire ou changement d’environnement flottant n’est ajouté.
Les cinq adaptations et notices MIT restent inchangées.

## Qualité et validation

**1 576 960 valeurs de rendu** sont comparées avant/après : 942 080 en float et
634 880 en double. Écart absolu maximal : **1,1921e-7 en float**,
**4,5908e-14 en double**. Le test tanh séparé, à gain de sortie 16 et avec des
entrées voisines des seuils, observe **7,1055e-13** au maximum. Les tolérances
existantes des régressions n’ont pas été élargies.

Les oracles indépendants couvrent les courbes de compression (rapports 1 à 1000,
knee 0/6/60 dB, seuils -160/-24/+24 dB et limiteur), les intégrales ADAA, les
coefficients RBJ, l’énergie des matériaux, les limites et canaris FIR/SIMD et
les changements de forme/drive/biais/AA. Les noyaux log/exp sont balayés sur
200 001 points, plus les frontières de tables et les valeurs particulières.

Validations exécutées et réussies sur ces sources : MSVC x64, clang-cl x64,
AddressSanitizer, chemin scalaire, Quest 3 ARM64/NEON, installation statique et
consommateur indépendant. 85 objets x64 et 84 objets ARM64 sont désassemblés :
ISA étendues isolées et aucune instruction FMA. Les régressions générales
conservent le contrôle des allocations/libérations dans les chemins temps réel.

Linux/macOS et exécution SVE restent à valider séparément. Les mesures de gain
publiées sont x64 ; le passage des tests ARM64 ne constitue pas une mesure de
gain ARM. Ces contrôles numériques ne constituent pas une écoute musicale.

## Reproduction

Figer une archive de production au commit `6b1fb80` avant de modifier les sources.
Compiler ensuite le même `tests/HotspotMeasurements.cpp` contre chacune des deux
archives, avec les mêmes en-têtes publics, compilateur et options strictes. Les
en-têtes publics n’ont pas changé pendant cette passe. Le banc ne dépend pas des
structures privées dont la taille a changé. On peut aussi utiliser la cible
`cydsp_hotspot_measurements` avec `CYDSP_BUILD_MEASUREMENTS=ON` dans deux checkouts
et la même version du banc. Les exécutables ne sont jamais lancés implicitement.

Chaque exécutable accepte un chemin de rendu binaire. Comparer les exécutables
avec `scripts/compare_hotspots.py --before <avant.exe> --after <apres.exe>` et
`--baseline-commit 6b1fb80bea89f78ffb93a260918b54c432ed77bd --trials 5
--logical-cpu 4 --before-render <avant.bin> --after-render <apres.bin>
--output <comparaison.json>`. Les exécutables, archives et rendus locaux de cette
campagne restent sous `build/hotspots-2026-11-14`, hors du paquet installé.
