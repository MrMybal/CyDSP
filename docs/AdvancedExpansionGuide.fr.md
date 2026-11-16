# Deuxième extension : sources nombreuses, audio 3D et traitements avancés

Ces dix lots prolongent [la première extension](ExpansionGuide.fr.md). Les API
sont dans `cydsp`, en C++20, float/double, dans la cible statique `CyDSP::CyDSP`.
[Plan](AdvancedExpansionPlan.fr.md), [validation](AdvancedExpansionValidation.fr.md),
[exemple compilé](../examples/AdvancedAudio.cpp).

## Contrats communs

`prepare` peut allouer ; il construit un nouvel état puis le valide avant de
remplacer l’ancien. Un échec conserve le processeur préparé précédent.
`process`, `render`, `reset` et les contrôles explicitement temps réel conservent
leur stockage : aucune allocation/libération, aucun verrou, aucune I/O ou
modification globale de l’état CPU. Préparation, déplacement, destruction,
contrôles et lectures des métriques sont sérialisés par le propriétaire.
La bibliothèque ne crée pas de thread.

Les blocs sont planaires. Les entrées non finies sont rejetées avant mutation.
Les sorties doivent avoir la taille, les canaux et les recouvrements autorisés.
Le découpage en blocs ne modifie pas le déroulement des algorithmes. Les instances
déplacées sont non préparées. Aucun getter ne constitue une synchronisation
entre threads. Les réglages changent entre appels du propriétaire ; un événement
à l’échantillon peut être appliqué en découpant le bloc à cet endroit.

Les préparations sélectionnent la même table SIMD privée que le cœur :
scalaire, SSE2/AVX2/AVX-512 sur x64, NEON/SVE selon la cible ARM. Les ISA ne
s’étendent pas aux unités portables. Les traitements ne sont pas tous entièrement
vectorisables ; les mesures distinguent chaque cas et chaque backend.

## 1. Sources groupées et données partagées

`SourceBatch<T>` prépare jusqu’à 1024 emplacements fixes, 64 canaux de sortie,
256 banques de tables et 256 banques de samples. La limite de stockage interne
est de 2^24 valeurs pour le rendu des voix. Chaque `BatchSource` choisit sinus,
table d’ondes ou grain, fréquence, gain, phase, départ différé et gains de canaux.
Les grains ont position, ratio signé (0,125 à 8 en valeur absolue) et durée fixe.
Un emplacement actif n’est pas volé : `start` retourne `would_block`.

Les sinus utilisent des récurrences complexes en structure de tableaux, avec
les noyaux SIMD sur les voix et renormalisation périodique. Les tables utilisent
les mips harmoniques de `Wavetable` ; les grains réutilisent l’interpolation
antirepliement et les boucles de `SampleData`, avec une fenêtre Hann. Lecture
inverse possible. Le mixage/routage des blocs utilise les noyaux SIMD communs.
Le calcul des recherches de tables et des positions de grains reste scalaire.

Les pointeurs de banques sont copiés pendant `prepare`, les données ne le sont
pas. L’hôte doit conserver **les objets et leurs données immobiles et immuables**
jusqu’à la fin de tous les lecteurs. Il peut partager une banque entre plusieurs
lots sans recopier les samples/mips. Aucun compteur de références n’est modifié
pendant le traitement.

`setPosition(slot, position, -1)` produit un panoramique stéréo à puissance
constante et une atténuation 1/max(1,distance). Pour un ordre 0..7, il calcule les
gains ACN/SN3D et exige exactement (ordre+1)^2 sorties. Ce routage stéréo ne
simule pas à lui seul les indices d’élévation HRTF ; le champ HOA ou les sources
séparées peuvent alimenter les moteurs binauraux. Les coordonnées sont celles du
cœur spatial : +X devant, +Y à gauche, +Z en haut, mètres.

## 2. Préparation HRTF et interpolation en mouvement

`HrtfSet<T>` reçoit les mesures en mémoire : par défaut 2048 directions distinctes,
32768 frames par oreille (bornes explicites : 100000 points/65536 frames). Il détecte le premier dépassement d’un seuil
relatif au pic (−40 dB par défaut), retire cet onset entier et conserve le délai.
Les délais fractionnaires explicitement fournis restent pris en compte.
La détection automatique n’est pas une estimation sous-échantillon ni un système
de mesure/calibration de casque.

La préparation convertit les FIR par sinc fenêtrée, avec coupure adaptée à la
réduction de fréquence, en réutilisant le générateur de coefficients du cœur.
Le rapport de fréquences doit être entre 1/8 et 8 ; les fréquences entre 8 et
768 kHz. La conversion de coefficients IR préserve le gain DC. Le filtre complet
conserve son pré/post-ringing causal : le retard commun est exposé par
`latencyFrames()` sur la banque, le rendu HRIR et le rendu HOA. À fréquence égale,
il vaut zéro. Les onsets acoustiques par direction restent des délais distincts.

La limite de FIR préparée est configurable, jusqu’à 65536 taps (2048 par défaut). `truncated()`
signale une troncature ; il faut la vérifier, notamment après conversion.
`tailFrames()` des renderers donne une borne conservative comprenant les FIR,
les délais acoustiques et le retard de conversion.

Les renderers préparent leur historique et partagent l’état immuable de la
banque par référence comptée, acquise uniquement en préparation. La banque peut
ensuite être détruite ou préparée à nouveau sans invalider les renderers déjà
préparés. Leurs historiques, filtres interpolés et transitions restent séparés.

`HrirBinaural::setDirection` interpole les FIR alignées et les délais séparément
par Shepard sphérique localisé par une gaussienne, sur l’ensemble des directions.
Les poids sont normalisés de manière stable même hors de la couverture mesurée.
La direction d’un point mesuré reproduit ce point. Cette interpolation continue
évite le changement abrupt d’appartenance des trois voisins de l’ancien mode.
`angularWidthRadians` règle la localité ; elle ne remplace pas des mesures denses.
Les changements sont fondus ; une nouvelle demande pendant un fondu retourne
`transition_in_progress`. Un importeur SOFA optionnel et les profils HRTF sont décrits dans le [guide CyDoc](../Documentation/Documentation.html#/fr/sofa). Aucun jeu HRTF mesuré n’est fourni.

## 3. Ambisonics et binaural HOA

`ambisonicCoefficients`, `ambisonicRotation` et `AmbisonicRenderer` couvrent les
ordres 0..7, soit 1..64 canaux ACN/SN3D. L’ordre FOA est W,Y,Z,X.
La rotation active respecte encoder(d) puis tourner = encoder(R*d).
Une quadrature Gauss-Legendre 8 × 32 calcule les rotations jusqu’à l’ordre 7,
sans allocation. Le calcul des coefficients/matrices est en double.

Les enceintes peuvent former une disposition irrégulière, jusqu’à 64 sorties.
Le décodeur résout D = Yᵀ(YYᵀ+λI)⁻¹ avec pivotage et régularisation configurable.
Une couverture insuffisante ne crée pas de résolution spatiale absente ; la
régularisation est nécessaire pour les matrices sous-déterminées ou mal
conditionnées. Il ne s’agit pas d’un décodeur AllRAD ni d’une correction de salle.

`HoaBinaural<T>` projette les HRIR mesurées sur le champ harmonique par la même
construction de moindres carrés, intègre les délais fractionnaires aux FIR puis
prépare `MatrixConvolver` avec deux oreilles et les canaux HOA comme entrées.
La rotation du champ est disponible avant la convolution. Les filtres sont
préparés hors callback ; leur convolver MIMO conserve ses optimisations existantes.

## 4. Diffraction et ouvertures

`diffractionGain` calcule une approximation scalaire d’écran mince/knife-edge,
pilotée par deux distances, une hauteur signée de dégagement et la célérité.
La dépendance fréquentielle utilise le paramètre de Fresnel et l’ajustement de
perte d’amplitude de [l’ITU-R P.526](https://www.itu.int/dms_pubrec/itu-r/rec/p/R-REC-P.526-8-200304-S%21%21PDF-E.pdf).
La formule est utilisée comme approximation de demi-plan ; cette référence
radio n’est pas une validation de toute géométrie acoustique.

Une largeur d’ouverture et un angle ajoutent le module de la directivité sinc
d’une ouverture rectangulaire. `DiffractionFilter` en donne une coloration peu
coûteuse sur trois bandes complémentaires, avec rampes. Ce rendu trois bandes
n’est pas une reproduction exacte de la courbe fréquentielle complète.
L’hôte fournit arêtes, visibilité, ouvertures et trajets. Il applique propagation,
retards et divergence avec `AcousticPathRenderer`. Pas de ray tracing, modèle
UTD de coin épais, interférences complexes multi-arêtes ou solveur d’ondes caché.

## 5. Diagnostic de réponses impulsionnelles

`analyzeImpulse` est une fonction **hors temps réel**, sans lecture de fichier.
Elle analyse un canal, au plus 2^26 frames : onset/pic, énergie, centre temporel,
C50/C80, D50, rapport direct/réverbéré et jusqu’à 32 réflexions précoces.
Les fenêtres temporelles sont relatives à l’onset détecté ; le seuil et la
fenêtre directe (2,5 ms par défaut) sont configurables. Les ratios d’énergie
sont exprimés en dB, bornés à ±200 pour éviter des infinis.

L’intégration rétrograde de Schroeder et la régression donnent EDT (0 à −10 dB),
T20 (−5 à −25) et T30 (−5 à −35), extrapolés à −60 dB. Les résultats incluent
R² et un indicateur `valid` ; une IR trop courte, un ajustement insuffisant ou
une plage non atteinte ne doivent pas être traités comme une mesure valide.
Neuf bandes d’octave de 63 à 16000 Hz sont disponibles sous Nyquist, par filtre
passe-bande. Ce sont des outils de diagnostic, pas une certification ISO.

La soustraction optionnelle du bruit estime sa puissance dans les derniers
10 % de l’IR ; une queue réverbérée encore audible peut biaiser cette estimation.
Elle est désactivée par défaut. `matchFdnToImpulse` agrège les décroissances
valides en trois cibles FDN, avec fallback large bande, prédélai et dosage direct.
Les cibles restent nominales : l’ajustement ne recrée pas la réponse exacte.

## 6. Séparation harmonique/percussive

`HarmonicPercussive<T>` applique des médianes temporelles et fréquentielles sur
le spectrogramme, puis deux masques complexes complémentaires. Les puissances
des canaux sont liées pour préserver leur image relative. L’approche suit les
équations de [FitzGerald, DAFx 2010](https://www.dafx.de/paper-archive/2010/DAFx10/DerryFitzGerald_DAFx10_P15.pdf), avec un code original.

De 1 à 8 canaux, N jusqu’à 8192, fenêtres médianes impaires de 1 à 63.
Le retard exact est N + (nombreFramesTemporelles/2)*hop. La médiane est centrée
avec anticipation préparée ; l’addition des sorties reconstruit l’entrée
retardée, aux arrondis FFT près. Il faut envoyer des zéros pour drainer la fin
(latence + 2N), puis `reset` pour un nouveau flux. Les sorties et l’entrée sont
disjointes. Ce traitement ne sépare pas des instruments ou des voix individuels.

## 7. Pitch spectral, harmonisation et correction

`SpectralPitchShifter<T>` transpose sans changer la durée, ratio 0,25..4,
de 1 à 8 canaux, N jusqu’à 8192 et hop au plus N/4. Le retard fixe est N.
Le ratio 1 est une reconstruction retardée exacte aux arrondis près.
Les fréquences instantanées déterminent les incréments de phase ; un canal guide
lié et les phases relatives préservent l’image multicanal. Les pics significatifs
servent au verrouillage vertical local ; les pics dus au bruit numérique sont
écartés. Une enveloppe spectrale lissée pilote la compensation de formants.

`envelopeWidthHz` vaut 150 Hz par défaut et reste réglable : une largeur trop
importante lisse les formants étroits ; une largeur trop faible suit les
harmoniques. Le flux spectral positif déclenche une remise en phase aux attaques.
Ce traitement des transitoires est heuristique. Les sources polyphoniques, les
grands intervalles et les formants sous-échantillonnés restent difficiles ;
les mesures de signaux synthétiques ne garantissent pas un rendu vocal transparent.

`PitchEffects::prepareSpectral` raccorde ce moteur à l’API existante des voix et
de la correction YIN (jusqu’à 8 voix, mono/stéréo). Les mêmes intervalles, gains,
panoramiques, retards, cibles et gammes sont disponibles. `latencyFrames()` expose
N ; les retards créatifs propres aux voix s’ajoutent volontairement au son.
La préparation réserve toutes les voix demandées. Les voix inactives ne sont
pas préchauffées automatiquement. L’ancien moteur à deux têtes sinc reste
accessible par `prepare`, avec sa latence variable documentée.

## 8. Contacts, glissement, roulement et archet

`ContactExciter<T>` produit une excitation mono à envoyer dans `ModalResonator`
ou `Waveguide`. Les paramètres ont des unités explicites : force normale en N,
vitesse en m/s, masse en kg, raideur en N/m, impulsion en N.s, espacement en mètres.
Le glissement utilise un état de bristle borné, intégré implicitement ; l’archet
ajoute une friction de Stribeck et accepte la vitesse de surface issue du modèle
résonant. Le roulement déclenche des micro-contacts sur les aspérités parcourues.

Huit impacts simultanés au plus ; saturation de file explicitement signalée.
Les demi-sinus d’impact respectent l’impulsion discrète demandée avant le
bloqueur DC à 20 Hz. Le gain convertit ensuite ces unités vers le niveau audio.
Les textures sont reproductibles avec `reset(seed)` ; aucun générateur aléatoire
global. L’hôte assure le couplage et la stabilité de sa boucle instrumentale.
Il ne s’agit pas d’un solveur de collisions ou de mécanique des solides.

## 9. Annulation d’écho acoustique

`EchoCanceller<T>` reçoit une référence mono du signal envoyé aux haut-parleurs,
et 1 à 8 microphones. Chaque microphone utilise un chemin FIR NLMS indépendant
(jusqu’à 4096 taps) avec prédiction/résidu. Les dots **et les mises à jour des
coefficients** réutilisent maintenant les noyaux SIMD d’`AdaptiveFilter`.

Le délai de référence, jusqu’à 0,5 s, est fourni par l’hôte. Il doit aligner les
horloges et traiter les dérives. Après l’apprentissage initial, la puissance
résiduelle et la cohérence prédiction/microphone commandent un gel lié des
adaptations pendant la double parole, avec hangover configurable. L’adaptation
est aussi gelée quand la référence est silencieuse ou désactivée explicitement.
`forceAdaptation` permet un réapprentissage choisi après changement du chemin.

Les métriques exposent activité de référence, double parole, adaptation, ERLE,
puissance résiduelle et compteurs. Les faibles niveaux de parole, le choix du
seuil et les changements de pièce demandent un réglage réel ; les non-linéarités
de haut-parleur et la dérive ne sont pas résolues par ce FIR. Ce module n’est pas
une chaîne complète de communication WebRTC, ni un contrôle des périphériques.

## 10. Politique de qualité et transitions

`qualitySettings` prépare une politique commune `economy`, `realtimeHigh` ou
`offline`, adaptée à la fréquence : STFT, médianes HPSS, limite de HRIR,
rééchantillonnage, interpolation des samples, lignes FDN, suréchantillonnage,
true peak, budget suggéré de grains et durée de fondu. Ce sont des réglages
explicites pour construire les processeurs, pas une variable globale cachée.
Le profil offline augmente les budgets ; il n’impose pas de thread ou d’I/O.

`QualityGraph` conserve deux `MultiBusGraph` préparés et leur profil. L’hôte
prépare le nouveau graphe hors callback, le passe à `stage`, puis appelle
`startTransition`. `GraphTransition` aligne les latences dans un budget fixe,
préchauffe puis fond les deux états ; le profil actif change après le fondu.
La consommation pendant cette période inclut les deux graphes.
Aucun graphe/contexte n’est libéré à la fin du fondu. Leur remplacement ultérieur
reste une opération hors temps réel ; les contextes empruntés doivent vivre
jusque-là. `reset` annule la transition et conserve le profil/graphe actif.

## Licence et frontières

Les nouveaux algorithmes et API sont du code original sous la licence MIT
CyDSP (MrMybal CyberAlien). Ils réutilisent les cinq adaptations MIT déjà
référencées dans [les licences](Licensing.md) ; aucun composant, code tiers ou
jeu de HRIR supplémentaire n’a été importé. Les notices restent inchangées.
Les appareils, MIDI, fichiers, transport, plugins et UI restent dans les
bibliothèques/applications consommatrices. CyAudioCore, CyDAW, CyUI, CyCode et
CyMetaSound ne sont pas modifiés par cette extension.
