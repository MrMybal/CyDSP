# Extension CyDSP : intégration des dix lots

Ces API appartiennent à `CyDSP::CyDSP`, en C++20, namespace `cydsp`. Elles sont
originales et sous la licence MIT du projet. Aucun code de CyDAW, CyMetaSound,
Unreal ou JUCE n’a été importé. Les cinq adaptations numériques MIT existantes
restent décrites dans [Licensing.md](Licensing.md). Aucun jeu de HRIR tiers n’est fourni.

Les en-têtes individuels permettent de limiter les inclusions ; `cydsp/CyDSP.h`
les rassemble. L’archive statique conserve le même package. L’extension ne crée
ni thread, ni périphérique, ni transport, ni fenêtre, ni interface de plugin.
Ces services restent à l’application/CyAudioCore.

## Contrats communs

- `prepare` peut allouer et conserve le plan précédent en cas d’erreur.
- `process`, `reset`, les événements, les déclenchements et les setters documentés
  conservent leurs réserves : pas d’allocation/libération, de verrou ou d’E/S.
  Le coût d’un setter borné peut toutefois être important : interpolation d’une
  HRIR, rotation HOA ou recalcul de coefficients. Les planifier entre les blocs.
- L’application sérialise tous les appels, getters compris. Les contextes de
  callbacks, tables d’ondes et processeurs empruntés restent vivants et immuables
  pendant leur utilisation, sauf leur état explicitement possédé par le thread audio.
- Les nouveaux traitements acceptent normalement 8–768 kHz, blocs de 1–65536
  échantillons et au plus 64 canaux, avec des bornes plus petites précisées dans
  chaque en-tête. Préparer explicitement les fréquences de coupure sous Nyquist.
- Les entrées non finies sont rejetées avant traitement. Les vues ne peuvent pas
  valider la durée de vie ni la capacité réelle des allocations du programme appelant.
  Un signal ou gain extrême peut dépasser le format de sortie : pas de normalisation
  globale implicite. Les limites numériques ne sont pas des plafonds de niveau audio.
- Blocs de même forme : alias exact par canal ou séparation complète, sauf API
  exigeant des vues disjointes. Les API à nombres de canaux/tailles différents
  exigent des sorties disjointes. Le graphe et les effets spectraux possèdent leurs
  copies d’entrée. Consulter les exceptions dans les en-têtes.
- L’état des objets possédant des ressources est vidé dans la source d’un déplacement.
  Déplacement, destruction, préparation et staging sont hors callback.

La sélection SIMD réutilise les noyaux existants : scalaire, SSE2, AVX2, AVX-512,
NEON et SVE lorsqu’ils sont compilés et disponibles. Les récurrences dépendantes
restent séquentielles ; un backend ne signifie pas que chaque opération est vectorisée.
Aucun drapeau ISA global, fast-math ou changement de l’environnement FP n’est ajouté.

## 1. Historique, lecteurs et paramètres

`CircularHistory<T>` partage une seule écriture entre des lectures indépendantes.
L’âge 0 désigne le dernier échantillon écrit ; le passé indisponible et les lectures
futures donnent zéro. `readWindow` regroupe les âges entiers en double, sans
modulo répété par tap. Interpolations nearest, linéaire, Lagrange cubique et
Catmull–Rom. `ThiranDelay<T>` est un allpass séquentiel du premier ordre, retard
fractionnaire dans `(0,1]`, et ne remplace pas un lecteur aléatoire.

`MultiTapDelay<T>` : table de taps entrée/sortie, gain et retard en frames, jusqu’à
4096 taps et 64 canaux. `setTaps` croise deux configurations complètes ; une édition
pendant la transition retourne `transition_in_progress`. Ce changement de retard
conserve la hauteur par fondu entre lecteurs ; la modulation continue de retard
est proposée séparément dans `DelayEffect`.

`ParameterBank` copie les noms et conserve des ID numériques stables. Les événements
sont triés par offset, limité au bloc courant. Une rampe avance dès l’échantillon de
l’événement et continue sur les blocs suivants. La sortie double est planaire,
un canal par paramètre, dans l’ordre des descripteurs. Les processeurs/applications
consomment ces valeurs : aucune réflexion ni liaison automatique vers leurs setters.

## 2. Fondations de filtrage et spectral

- `Stft<T>` : analyse/synthèse WOLA publique, FFT puissance de deux 16–65536,
  hop diviseur de N, fenêtre à support de reconstruction valide. Latence N, startup
  rembourré de zéros. Callback de spectre par canal, `noexcept`, borné et sans allocation.
  `flush` alimente 2N zéros puis signale la fin ; `reset` avant un nouveau flux.
- `FirFilterBank<T>` : différences de passe-bas FIR complémentaires, jusqu’à 32 bandes,
  somme égale à l’entrée retardée de `(taps-1)/2`. Ordre des sorties :
  `band * inputChannels + channel`. 129 taps par défaut.
- `designBandIir` : prototypes Butterworth, Chebyshev I/II, elliptique et Bessel,
  ordre prototype 1–16 transformé en passe-bande/coupe-bande d’ordre double.
- `designFirEqualizer`, `minimumPhaseFir` : préparation hors audio, réponse demandée
  tronquée/fenêtrée et conversion par cepstre réel. La troncature d’une conversion
  en phase minimale est approximative ; mesurer le résultat, surtout aux creux profonds.
- `HilbertTransform` : quadrature FIR à retard commun, précision réduite près de DC/Nyquist.
- `Diffuser`, `orthogonalMix` : allpasses de Schroeder, Hadamard/Householder orthogonaux.

## 3. Graphe multibus et compensation de latence

`MultiBusGraph<T>` prépare un DAG de `DspNode`, `DspBus` et `DspConnection`.
Le graphe copie sa topologie ; il emprunte les contextes et fonctions des processeurs.
Maximum 256 nœuds, 4096 connexions et 32 bus par disposition. Un ID de bus est local
au nœud. `graphInputNode` et `graphOutputNode` désignent les frontières du graphe.

Chaque nœud déclare ses bus et `DspTiming`. `tailFrames = SIZE_MAX` représente
une queue sans fin (freeze) et se propage sans débordement ; les queues finies
sont bornées à 2^30 frames dans un chemin de nœuds. Les bus secondaires sont des entrées
explicites : un sidechain n’est pas caché dans le canal principal. Tous les chemins
vers un nœud, puis toutes les sorties externes, sont alignés sur la latence maximale.
Le bypass garde le traitement chaud et utilise le bus principal 0 retardé à la
latence déclarée. Une erreur de callback réduit les sorties externes au silence ;
les états des nœuds déjà exécutés ne peuvent pas être annulés.

`GraphTransition<T>` retient deux graphes préparés et un budget de latence fixe.
Stager hors audio, démarrer la transition entre blocs ; le nouveau graphe reçoit
l’entrée pendant le préchauffage, puis le fondu. Les deux graphes doivent avoir
des contextes mutables indépendants. Aucun graphe n’est détruit à la fin du fondu.
Les latences des nœuds sont fixes jusqu’à nouvelle préparation/staging. Une hauteur
variable ou un filtre IIR à retard dépendant de la fréquence ne se résume pas à une
latence entière exacte : l’hôte doit choisir sa politique.

## 4–5. Propagation et acoustique de jeu

Convention unique : repère direct, **+X avant, +Y gauche, +Z haut**, mètres,
mètres/seconde, radians, quaternion `(w,x,y,z)`. Convertir explicitement les axes et
unités d’un moteur consommateur. `interpolatePose` utilise un SLERP sur l’arc court.
`relativeSources` évalue plusieurs sources/auditeurs, sortie auditeur-major.

`SpatialMath` couvre distance inverse/exponentielle/linéaire, directivité par trois
bandes, taille de source, atmosphère, segments de milieux, vitesse/densité acoustiques
et absorption/transmission/réflexion de matériaux. `waterMedium()` est un préréglage
explicite : l’application fournit les propriétés pertinentes de son eau. Les pertes
sont des modèles paramétriques ; aucune simulation géométrique n’est lancée.

`DopplerResampler` réutilise le rééchantillonneur variable et retourne les nombres
réellement consommés/produits. L’hôte possède sa FIFO. Ne pas appliquer une seconde
variation continue du délai produisant déjà le même Doppler.

`AcousticPathRenderer` garde un historique mono partagé et des slots stables par ID,
avec retard, direction d’arrivée, gains trois bandes et transitions. Maximum 64 chemins,
retard préparé au plus 10 s. Sortie : un canal par slot, à spatialiser ensuite.
La suppression s’estompe avant de libérer un slot ; aucun vol de chemin audible.
L’entrée multicanal est moyennée : traiter des émetteurs distincts séparément.

`AcousticPortalGraph` prépare les routes de transfert entre pièces, avec ouverture,
pertes et milieux. `RoomCellGrid` interpole des réponses de pièce sur une grille régulière.
L’hôte fournit les portails, segments, cellules, occlusions et chemins de réflexion.
CyDSP ne fait ni ray tracing, ni navigation dans les acteurs, ni gestion de monde.

`FdnReverb` : réseau orthogonal 4/8/16/32 lignes, 1–8 canaux, RT60 nominales par
bande, prédélai/diffusion, paramètres rampés et freeze sans injection. Des bornes
passives conservatrices peuvent raccourcir les RT60 demandées pour des profils de
bandes extrêmes. `HybridReverb` combine convolution à IR préparée et FDN, avec staging
d’IR hors audio et démarrage de fondu sans allocation.

`RoomReverbBank` prépare jusqu’à 32 pièces : `beginBlock`, plusieurs `send`, `render`,
puis `mix` ou `roomOutput` emprunté. Toutes les queues continuent avec des sends nuls.
L’application décide si le send est pris avant occlusion et applique les gains vers
chaque auditeur. Pas de verrou, registre global, thread ou suppression automatique de pièce.

## 6. Binaural, enceintes et Ambisonics

`AnalyticBinaural` fournit ITD de tête sphérique, ombrage, coloration avant/arrière
et proximité d’oreille. Jusqu’à 32 entrées placées indépendamment ; deux entrées
permettent un émetteur stéréo étendu. Le rayon atténue les détails directionnels.
Ce modèle paramétrique ne remplace pas une HRTF mesurée/personnalisée.

`HrirBinaural` copie jusqu’à 2048 directions, FIR de 1–2048 taps, au taux préparé.
Chaque direction fournit les FIR gauche/droite **sans délai d’apparition**, puis les
deux délais séparés. L’interpolation utilise désormais des poids Shepard sphériques continus ;
les délais et filtres finaux sont croisés entre lecteurs. La nouvelle banque
`HrtfSet` ajoute détection des onsets, conversion de fréquence et partage immuable :
voir [la deuxième extension](AdvancedExpansionGuide.fr.md). Garder les délais intégrés
aux FIR peut créer du filtrage en peigne : l’application prépare correctement ses données.
Un changement pendant un fondu retourne `transition_in_progress`. Pas de SOFA,
fichier, base de mesures, rééchantillonnage implicite ou licence de données fournie.

`VbapLayout` prépare paires 2D ou triplets de l’enveloppe convexe 3D, 2–64 enceintes.
Gains à énergie unitaire. Hors couverture : erreur sans modification ou repli vers
l’enceinte la plus proche, choisi explicitement à la préparation.

`ambisonicCoefficients`, `ambisonicRotation`, `AmbisonicRenderer` : ordres 0–7,
**ACN/SN3D**, jusqu’à 64 canaux (extension de la limite initiale d’ordre 3). Rotation active : encoder `d` puis tourner égale
encoder `R*d`. Pour une orientation d’auditeur, fournir la rotation inverse.
Décodage least-squares régularisé vers la disposition fournie. Pas de conversion
implicite FuMa/N3D ni compensation particulière de salle. La régularisation ne rend
pas un petit nombre d’enceintes équivalent à un champ complet.

## 7. Dynamique et non-linéarités

`Dynamics::process(program, sidechain, output)` accepte un détecteur mono diffusé
ou un canal par canal de programme. Les paramètres et l’anticipation existants sont
conservés. Le détecteur externe doit être disjoint de la sortie, sauf s’il est le
bloc de programme lui-même.

`DynamicEqualizer` utilise des corrections passe-bande à crête unitaire et détection
indépendante ; jusqu’à 16 bandes, atténuation ou amplification dynamique.
`DeEsser` réutilise une telle bande. `MultibandDynamics` compose les banques FIR,
les compresseurs et saturateurs : jusqu’à 16 bandes, reconstruction neutre exacte
à l’arrondi près, latence du FIR. Ses compresseurs n’ajoutent pas d’anticipation.
`TransientShaper` combine enveloppes rapides/lentes et réglages attaque/sustain.

`AdaaSaturator` fournit ADAA du premier ordre pour tanh, atan, cubique, écrêtage et
wavefolder, avec drive, asymétrie et gain. Sa réponse dépend de la fréquence et
ajoute approximativement un demi-échantillon de lissage pour les formes statiques ;
pas de promesse « sans aliasing ». Le mode hystérésis utilise un opérateur de jeu
borné suivi de tanh, avec mémoire ; ce n’est pas une primitive ADAA d’un modèle magnétique.

`TruePeakLimiter` travaille en 4x/8x, détecteur lié, anticipation et FIR de retour,
avec marge de reconstruction réglable. La latence est retournée en double.
Le plafond dépend de la reconstruction et de la marge : valider avec un mesureur
true peak et le contenu cible. Aucune certification de conformité broadcast.

## 8. Effets et synthèse avancée

- `DelayEffect` : multitap, ping-pong, filtrage/saturation du feedback, ducking,
  flanger/vibrato et wow/flutter selon délai, profondeur, vitesse et mix choisis.
  Les modes ne remplacent pas ces réglages par un preset caché.
- `AmplitudeModulator` : trémolo et autopan stéréo à puissance constante.
- `PlateReverb` : réalisation originale de la topologie Dattorro à deux cuves
  croisées, diffusion/damping, freeze et injection shimmer à double lecteur.
  Le réglage CyDSP ne revendique pas une émulation d’un appareil précis.
- `SpringReverb` : guide d’ondes dispersif amorti ; pas une simulation mécanique FEM.
- `SpectralEffects` : STFT synchronisée carrier/modulator, freeze avec incréments
  de phase, vocodeur à enveloppe spectrale lissée, morphing et gains par bin.
  `mix=0` conserve la reconstruction retardée. `FrequencyShifter` réutilise Hilbert.
- `LiveGranulator` : capture circulaire 0,05–20 s, 1–1024 grains, 1–8 sorties,
  avance/recul indépendante par grain, capture gelable, cloud reproductible.
  Sinc 32 taps, 64 intervalles de phase interpolés, ratios signés 0,125–4.
  L’âge demandé exclut une garde causale de 16 frames. Fin de grain si sa lecture
  rejoint une zone future ou écrasée. Gains multicanaux fournis explicitement.
- `ModulatedOscillator` : FM en Hz, PM en cycles, synchro à 4x avec retour FIR,
  PolyBLEP saw/square, morphing de tables harmoniques empruntées et 1–16 voix d’unisson.
  La surmodulation peut encore replier le spectre. La latence du retour FIR est exposée.
- `MultiSegmentEnvelope`, `ModulationLfo` : segments courbés, boucles/release,
  sample-and-hold et aléatoire lissé à graine déterministe.
- `ProceduralSource` : briques vent/moteur/impact paramétriques. L’assemblage de
  véhicules, armes, météo et comportements reste dans les projets consommateurs.

## 9. Analyse musicale, hauteur et modèles physiques

`MusicalAnalyzer` : différence YIN accélérée par FFT, fondamentale/confiance,
flux/attaques, tempo/battement heuristiques, chromagramme, MFCC et tonalité estimée.
L’analyse attend N échantillons ; N doit couvrir la fréquence basse demandée.
MFCC : puissances Mel, logarithme naturel, DCT-II orthonormale, C0 inclus,
plancher `1e-20`. Chroma normalisé L1 ; tonalité C=0…B=11, majeur/mineur.
La dernière trame est disponible par `features()` ; le callback optionnel reçoit
chaque trame pour ne pas perdre une attaque dans un grand bloc. Ni transcription
polyphonique, ni suivi universel de rythme, ni confiance statistiquement calibrée.

`PitchEffects` : 1–8 voix d’harmoniseur/doubleur, lectures sinc à fenêtres
complémentaires, correction monophonique vers une cible ou un masque de gamme.
Fenêtre de 40 ms, garde de 16 frames, délai de voix 0–200 ms ; le retard des lecteurs
varie dans la fenêtre, sans latence entière exacte unique. Pas de verrouillage des
formants dans ce mode. `prepareSpectral` ajoute le nouveau moteur à latence fixe
avec conservation des formants ; le module Stretch reste disponible séparément.

`ModalResonator` : modes amortis préparés ou génération de membrane rectangulaire /
plaque mince simplement appuyée. Jusqu’à 512 modes, mono vers mono/stéréo.
`Waveguide` : Karplus–Strong, corde dispersive et tube, excitation externe ou pluck
reproductible. Réflexion inversée pour un tube fermé et résonances impaires.
La dispersion et les pertes font dépendre l’accord et la décroissance de la fréquence.

## 10. Restauration, adaptation et mesures

`SpectralDenoiser` : profil de puissance canal-major, apprentissage audible,
soustraction/Wiener avec plancher et lissage. Le reset vide le flux mais conserve le
profil appris. Résonances persistantes : seuil relatif aux voisins et durée minimale
au-dessus du seuil, puis atténuation lissée. Ce traitement peut enlever des partiels
musicaux : c’est un outil paramétrique, pas une séparation intelligente des sources.

`Dehum` : cascades de notches aux harmoniques demandées. `Declicker` : médiane/MAD,
contexte futur de 1–32 frames et remplacement interpolé des valeurs atypiques.
Latence égale au contexte ; peut affecter les transitoires selon ses seuils.

`AdaptiveFilter` : LMS/NLMS par canal, références et cibles séparées, sortie prédiction
et résidu. Coefficients dans l’ordre causal, tap 0 = échantillon courant. Le pas LMS
dépend de la puissance d’entrée. Le garde-coefficient et `clippedUpdates()` rendent
visible la saturation d’adaptation ; le nombre de taps est fixé à la préparation.

`FirRateConverter` : décimation FIR et interpolation polyphase, facteurs 2/4/8/16/32,
longueurs exactes via `outputFrames`, état de phase conservé entre blocs.
`DyadicFilterBank` : banque de paquets Haar orthogonaux, reconstruction exacte et
séparation fréquentielle grossière, ordre de séquence croissante. Les longueurs sont
`analysisFrames(n)` puis `bandFrames * bands` à la synthèse. Les paquets incomplets
restent retenus ; l’hôte possède le buffering entre ces deux cadences.

`firResponse`/`iirResponse` calculent gain complexe, phase et retard de groupe.
`TransferAnalyzer` calcule spectres croisés Welch H1, phase déroulée, retard de groupe
et cohérence. Une excitation insuffisante ne donne pas une mesure fiable.
`measureDistortion` ajuste DC et sinusoïdes/harmoniques à f0 connu, sans exiger de
cohérence FFT. THD et THD+N sont des rapports RMS linéaires, pas des décibels.
`MeasurementGenerator` fournit sinus, bruit blanc, impulsion, sweep linéaire/log.
`inverseLogSweep` et `deconvolveSweep` sont des opérations hors audio, avec réserves
allouées et régularisation explicite pour la déconvolution.

## Exemples, vérifications et références

[SpatialAndDaw.cpp](../examples/SpatialAndDaw.cpp) illustre chemins vers binaural et
bus multibande/limiteur. Il est compilé sans être lancé automatiquement. Les rapports
et cas numériques sont dans [ExpansionValidation.fr.md](ExpansionValidation.fr.md),
avec les historiques [Validation.md](Validation.md) et [Measurements.md](Measurements.md). Une régression numérique ne remplace pas
l’écoute et la validation de l’intégration dans un jeu ou un DAW.

Références mathématiques, sans import de leur code :

- [STFT et overlap-add](https://ccrma.stanford.edu/~jos/sasp/Overlap_Add_OLA_STFT_Processing.html).
- [Interpolation Thiran](https://ccrma.stanford.edu/~jos/pasp/Thiran_Allpass_Interpolators.html).
- [Phase minimale](https://ccrma.stanford.edu/~jos/sasp/Minimum_Phase_Filter_Design.html).
- [FDN](https://ccrma.stanford.edu/~jos/pasp/Feedback_Delay_Networks_FDN.html).
- [Dattorro, topologie de réverbération](https://ccrma.stanford.edu/~dattorro/EffectDesignPart1.pdf).
- [ADAA, Bilbao et al.](https://www.research.ed.ac.uk/files/34115216/bilbao_pdf.pdf).
- [Validation NPL de l’absorption atmosphérique](https://resource.npl.co.uk/acoustics/techguides/absorption/validata.html).
