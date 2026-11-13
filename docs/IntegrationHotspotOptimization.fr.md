# Optimisations des rÃ©gressions dâ€™intÃ©gration â€” 13 novembre 2026

Les rÃ©gressions observÃ©es dans CyMetaSound ont conduit Ã  optimiser le traitement
liÃ© du compresseur, les primitives ADAA, les petites banques de filtres, le
mÃ©langeur sans dÃ©lai et lâ€™historique direct du convolueur matriciel. Les
algorithmes restent prÃ©parÃ©s et sans allocation/libÃ©ration, verrou ni I/O dans
les appels de traitement. Aucun changement global dâ€™ISA ou de prÃ©cision flottante.

## Mesures des composants CyDSP

AMD Ryzen 9 7900X, Windows x64, MSVC 14.38, Release, prÃ©cision flottante stricte.
Le tÃ©moin est le code de production du commit `fda6bd3`, compilÃ© avant modification.
Le banc utilise les mÃªmes paramÃ¨tres et donnÃ©es pour les deux exÃ©cutables.
Trois paires de processus indÃ©pendants alternent avant/aprÃ¨s, sur le processeur
logique 4. Chaque processus chauffe 256 blocs puis mesure neuf moyennes de 128
appels par cas. La valeur retenue est la mÃ©diane des mÃ©dianes de ces moyennes.
78 cas, types float/double, blocs de 256 Ã©chantillons Ã  48 kHz.

Les durÃ©es sont en **microsecondes par bloc** ; plus petit est mieux.
Le facteur est **temps avant / temps aprÃ¨s** : 3Ã— signifie trois fois moins de
temps pour ce traitement et ces conditions. Il ne mesure pas le CPU global du jeu.

| Composant | Avant (Âµs) | AprÃ¨s (Âµs) | Facteur avant/aprÃ¨s |
| --- | ---: | ---: | ---: |
| Compresseur stÃ©rÃ©o liÃ©, peak | 14.334 | 4.651 | 3.08Ã— |
| ADAA tanh stÃ©rÃ©o | 15.812 | 8.649 | 1.83Ã— |
| ADAA wavefolder stÃ©rÃ©o | 7.802 | 2.084 | 3.74Ã— |
| EQ lissÃ© 4 bandes | 10.076 | 4.488 | 2.24Ã— |
| EQ lissÃ© 8 bandes | 20.180 | 8.950 | 2.25Ã— |
| Convolution stÃ©rÃ©o 8192, scheduled 128/1024 | 16.359 | 15.092 | 1.08Ã— |

[DonnÃ©es des 78 cas, empreintes des exÃ©cutables et contrÃ´les](IntegrationHotspotOptimization.json).
Les opÃ©rations indÃ©pendantes du compresseur conservent leur chemin gÃ©nÃ©ral ;
les gains annoncÃ©s pour les canaux liÃ©s ne leur sont pas transfÃ©rables.

## Changements et attribution des coÃ»ts

- **Dynamique** : un seul dÃ©tecteur et une seule courbe par Ã©chantillon lorsque
  les canaux sont liÃ©s et leurs enveloppes retenues Ã©gales. Les historiques
  diffÃ©rents aprÃ¨s un passage indÃ©pendant restent sur le chemin gÃ©nÃ©ral. La
  courbe hors du coude est Ã©valuÃ©e dans le domaine linÃ©aire ; les rapports 2:1 et
  4:1 emploient des racines carrÃ©es. Le vu-mÃ¨tre en dB est calculÃ© en fin de bloc.
- **Vu-mÃ¨tre** : `minimumAppliedGain()` expose le minimum du gain dÃ©tecteur avec
  makeup, avant le plafond du limiteur. Lâ€™adaptation peut supprimer ses divisions
  sortie/entrÃ©e par Ã©chantillon. Cette mesure conserve le dernier bloc non vide
  rÃ©ussi ; les accÃ¨s restent sÃ©rialisÃ©s par le propriÃ©taire.
- **ADAA** : la primitive prÃ©cÃ©dente est mÃ©morisÃ©e et invalidÃ©e selon la forme,
  le mode direct et le passage par la limite du point milieu. Les changements de
  drive/biais conservent lâ€™Ã©chantillon dÃ©jÃ  transformÃ©. La primitive nâ€™est pas
  Ã©valuÃ©e inutilement sur les segments constants.
- **Wavefolder** : rÃ©duction de phase bornÃ©e par une puissance de deux ; noyau
  temporel original en double prÃ©cision dans les fichiers privÃ©s AVX2/AVX-512.
  Le traitement garde lâ€™entrÃ©e prÃ©cÃ©dente entre tuiles, traite les fins de blocs
  et conserve le chemin scalaire pour les autres backends.
- **EQ** : les banques mono/stÃ©rÃ©o parcourent directement les plans audio.
  Elles Ã©vitent les conversions et les petits groupes SIMD insuffisamment remplis.
  La cascade lissÃ©e traite le bloc prÃ©parÃ© et coupe aux frontiÃ¨res de transition.
- **Convolution** : historique direct contigu par tuile et parcours par route.
  Les produits, leur ordre, les partitions tardives et la latence restent inchangÃ©s.
- **MÃ©lange parallÃ¨le** : le dÃ©lai de capacitÃ© nulle copie les plans secs ; il
  Ã©vite les modulos dâ€™un anneau de taille 1.

Le balayage des partitions prÃ©coces 32/64/128/256 a Ã©tÃ© conservÃ© dans les mesures.
Changer uniquement cette taille ne rÃ©sout pas le surcoÃ»t des IR courtes ; aucune
latence ou qualitÃ© supplÃ©mentaire nâ€™a Ã©tÃ© substituÃ©e pour amÃ©liorer les chiffres.

## FidÃ©litÃ© et validation

Le rendu de contrÃ´le comporte **1 003 520 valeurs**
float/double. Lâ€™Ã©cart absolu maximal avant/aprÃ¨s vaut
**1.67e-16**. Les rÃ©organisations de courbe du
compresseur changent lâ€™arrondi double ; elles ne changent pas sa loi de transfert.
Ce contrÃ´le ne remplace pas des Ã©coutes sur des programmes musicaux variÃ©s.

Les tests indÃ©pendants couvrent les intÃ©grales ADAA sans cache, les changements de
forme/drive/biais/antialiasing, les Ã©tats liÃ©s/indÃ©pendants, sidechains, blocs vides,
paramÃ¨tres de dynamique, limites et fins SIMD, erreurs transactionnelles, reset,
convolution directe et routage. Les rÃ©gressions gÃ©nÃ©rales auditent aussi les
allocations/libÃ©rations des chemins temps rÃ©el.

Validation exÃ©cutÃ©e : MSVC, clang-cl, scalaire, AddressSanitizer, Quest 3 ARM64/NEON
et consommation du paquet statique installÃ©. Le dÃ©sassemblage contrÃ´le lâ€™isolation
ISA et lâ€™absence de FMA implicite. Les mesures de gain ci-dessus concernent x64 ;
aucun nouveau gain de performances ARM nâ€™en est dÃ©duit. Linux/macOS et exÃ©cution
SVE restent Ã  valider sÃ©parÃ©ment.

## Ã‰valuation ISPC

ISPC 1.24.0, cible `avx2-i32x8`, FMA dÃ©sactivÃ©e, sans tÃ¢ches ni GPU.
Le prototype original de wavefolder est disponible dans
[tests/ispc](../tests/ispc/README.md), sÃ©parÃ© de la production et du paquet installÃ©.
Sur 204800 valeurs de contrÃ´le, lâ€™erreur mesurÃ©e est nulle.
AprÃ¨s optimisation du noyau C++ : **1.986 Âµs** contre
**1.924 Âµs** pour le prototype ISPC, dans ce microbanc.
Lâ€™Ã©cart dâ€™environ 3.2 % provient dâ€™une exÃ©cution ;
il ne constitue pas un gain gÃ©nÃ©ralisable ou statistiquement Ã©tabli.

Le noyau C++ original reste utilisÃ© en production, avec ses chemins de repli et
ses conventions communes. Le compilateur ISPC est un outil externe fourni
explicitement pour lâ€™expÃ©rience. Les cinq adaptations MIT et leurs notices restent
inchangÃ©es ; aucune dÃ©pendance de production nâ€™a Ã©tÃ© ajoutÃ©e.

## Reproduction

Activer explicitement `CYDSP_BUILD_MEASUREMENTS=ON` dans une construction de
validation, puis construire `cydsp_hotspot_measurements`. Lancer lâ€™exÃ©cutable avec
un chemin de fichier binaire pour sauvegarder son rendu dÃ©terministe.

`scripts/compare_hotspots.py` compare deux exÃ©cutables crÃ©Ã©s avant/aprÃ¨s, avec
`--before`, `--after`, `--output`, Ã©ventuellement `--logical-cpu 4`,
`--before-render` et `--after-render`. Les exÃ©cutables et rendus locaux restent
dans `build/hotspots-2026-11-13` et ne sont pas distribuÃ©s avec la bibliothÃ¨que.

## Vérification dans les graphes Unreal

Même banc de 66 scénarios que le rapport CyMetaSound initial, six processus
indépendants alternés. UE 5.5.4 Editor Development Win64, même compilateur,
quantum MetaSound 480 frames, callbacks de 256 frames à 48 kHz. La valeur est
la médiane de trois processus, chacun retenant la médiane de sept moyennes de
512 callbacks. Les 13 tests d’intégration passent sans échec ni avertissement.

Les durées sont des microsecondes par callback amorti. La variation est
`(CyDSP / V2 - 1) × 100` ; négative = moins de temps CPU pour CyDSP.
V2 reste compilée en `/fp:fast`, CyDSP en `/fp:precise` ; les modèles peuvent
aussi différer. Il s’agit d’une comparaison des intégrations livrées.

| Graphe | V2 (µs) | CyDSP optimisé (µs) | Variation de temps / V2 |
| --- | ---: | ---: | ---: |
| EQ4 | 12.553 | 8.203 | -34.7 % |
| EQ8 | 20.598 | 12.286 | -40.4 % |
| Compressor | 6.331 | 12.071 | +90.7 % |
| Saturation | 16.303 | 13.739 | -15.7 % |
| Waveshaper | 4.914 | 12.430 | +153.0 % |
| Wavefolder | 7.171 | 5.644 | -21.3 % |
| Convolution.Stereo.8192 | 8.949 | 18.500 | +106.7 % |
| Convolution.Stereo.65536 | 41.300 | 26.742 | -35.3 % |
| Chain.ChannelStrip | 31.765 | 33.626 | +5.9 % |
| Chain.Master | 70.298 | 40.604 | -42.2 % |

La chaîne EQ4 → compresseur → saturation passe de +76 % dans le rapport initial
à +5.9 % dans la campagne finale. Les EQ, la saturation et le wavefolder passent
devant V2 ; le compresseur à 6:1, le Waveshaper et les IR courtes restent plus
coûteux. Aucun gain général ou identité sonore avec V2 n’est déduit.

Le rapport détaillé, son CSV et les six exécutions sont conservés dans le plugin :
`Tests/PerformanceComparison-V2-CyDSP-Optimized.fr.md` et
`Tests/Performance/CyDSPOptimization-2026-11-13`. Le rapport initial est conservé.
Les résultats consolidés et l’empreinte du manifeste figurent également dans le
JSON joint à ce document. Les phases de préparation, l’écoute, Shipping, le
périphérique réel, FPS et consommation mémoire ne sont pas mesurés ici.
