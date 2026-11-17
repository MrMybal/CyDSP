---
id: neural-hrtf
title: HRTF neuronale native
description: Préparer un modèle directionnel compact et rendre ses filtres binauraux.
section: Guides
order: 44
sourceRevision: 1
parent: hrtf
---

## Ce qui s’exécute dans l’application {#runtime}

NeuralHrtfModel prédit deux cascades de filtres peaking bornés, les niveaux des oreilles et l’ITD voulu droite-moins-gauche à partir d’une direction relative à l’auditeur (+X avant, +Y gauche, +Z haut). NeuralBinaural<float/double> rend ces paramètres avec les biquads et délais fractionnaires CyDSP. La bibliothèque statique C++ ne demande ni PyTorch, Python, CUDA, ONNX ni téléchargement de modèle. Les outils Python d’entraînement/export sont séparés et optionnels. Les poids expérimentaux ne sont pas installés automatiquement avec CyDSP.

Le modèle approche l’amplitude et l’ITD basse fréquence d’un auditeur connu. Il ne reconstruit pas la phase complète des HRIR mesurées et ne fournit pas une HRTF personnalisée pour un auditeur arbitraire. Conserver HrirBinaural avec les mesures SOFA comme référence mesurée et pour les usages offline exigeants.

## Charger et préparer {#prepare}

L’hôte lit une banque de poids puis transmet ses octets à load. Le chargement vérifie bornes, dimensions, fréquence, coefficients finis et encodage, puis possède une copie partagée immuable des poids. Les copies de NeuralHrtfModel partagent les poids ; les prédictions const peuvent être concurrentes. Chargement, affectation et destruction restent hors callback audio. La sélection scalaire/SSE2/AVX2/AVX-512/NEON/SVE réutilise les noyaux compilés CyDSP. Valider séparément chaque plateforme déployée.

```cpp
#include <cydsp/spatial/NeuralHrtf.h>

cydsp::Status prepareVoice(std::span<const std::byte> bytes,
                           cydsp::NeuralHrtfModel& model,
                           cydsp::NeuralBinaural<float>& voice) noexcept {
    auto status = model.load(bytes);
    if (status != cydsp::Status::ok) return status;
    // L’audio doit réellement fonctionner au taux exact de cette banque.
    status = voice.prepare({48000, 128, 1}, model);
    if (status != cydsp::Status::ok) return status;
    return voice.setDirection({1, 0, 0}, 0);
}
```

Un modèle 96 kHz est refusé par une voix à 48 kHz. Réentraîner/adapter le champ à la nouvelle fréquence puis le valider ; changer seulement le taux RBJ modifie la réponse. La préparation alloue deux banques audio complètes et des historiques de délai bornés. Installer une direction avant process ; entrée mono, sortie stéréo et zones d’échantillons disjointes.

## Prédire, concevoir et publier {#updates}

Appeler model.predict(direction, parameters), puis designNeuralHrtf(parameters, design) sur un worker/thread de contrôle. Publier la valeur terminée dans une file bornée de l’hôte, puis appeler voice.setDesign(design, fadeFrames) entre deux process. predict accepte aussi un span de directions, avec validation de tout le lot avant écriture ; entrées/sorties du lot disjointes. Le lot est séquentiel et réutilise les poids transposés immuables via les noyaux SIMD addScaled CyDSP. Il ne demande ni lot GPU ni BLAS.

setDirection combine prédiction, conception des coefficients/phase et installation sans allocation, mais coûte davantage qu’un design déjà préparé. Sans allocation ne signifie pas un temps callback garanti. Le helper soustrait l’ITD intrinsèque des cascades, mesuré par pente de phase 200–1 500 Hz sur la grille fréquentielle d’une FFT 4 096, avant d’attribuer les délais externes positifs. Les niveaux des oreilles restaurent déjà l’offset d’amplitude d’entraînement. Ne pas réappliquer l’ITD brut ni restaurer deux fois l’offset.

process, reset, predict, designNeuralHrtf et les mises à jour des voix n’allouent aucune mémoire. L’hôte sérialise mises à jour et traitement d’une voix. Il ne doit jamais modifier une voix pendant son process. Le modèle se partage entre voix ; chaque voix possède ses états de filtres/délais.

## Transitions et précision {#quality}

Une transition non nulle fond linéairement les sorties complètes ancien/nouveau filtres, gain et délai. setDesign et setDirection renvoient transition_in_progress tant qu’elle continue ; l’hôte conserve/réessaie sa cible la plus récente. Le premier design ou une transition nulle s’installe immédiatement en réinitialisant les états de filtres/délais. L’historique d’entrée brute continue jusqu’à reset, qui choisit la cible en attente et efface tous les historiques. Avant une mise à jour en fondu, la nouvelle branche rejoue l’entrée brute conservée avec ses propres coefficients, gain et délai. NeuralBinauralOptions::transitionHistoryFrames vaut 512 par défaut et accepte 0–65536. Zéro conserve le démarrage à froid précédent ; un historique plus long coûte davantage de CPU dans setDesign/setDesigns. Un rejeu fini approche le passé infini d’un IIR et ne garantit pas une transparence perceptive pour tous les signaux ou mouvements.

Coefficients et états biquad restent en double précision, pour les API audio float et double. Un passe-tout Thiran d’ordre 1 fournit les retards fractionnaires résiduels à amplitude unitaire, évitant la perte d’aigus d’une interpolation linéaire. Sa phase dépend de la fréquence ; cette conception vise l’ITD basse fréquence et non toute la phase des aigus. Aucun calcul approximatif ni drapeau flottant global n’est activé. latencyFrames indique zéro latence commune de buffering ; délais causaux par oreille et queues IIR subsistent. La capacité de délai externe vaut 5 ms par défaut, préparables jusqu’à 100 ms. Un design hors capacité est refusé en conservant la voix actuelle. Les queues IIR n’ont pas la durée finie d’une FIR.

## Rendre plusieurs voix ensemble {#batch}

Pour plusieurs émetteurs, conserver un tableau contigu d’objets NeuralBinaural préparés. processBatch(voices, sources, stereoPairs) prend N canaux mono en entrée et 2*N canaux en sortie dans l’ordre L0,R0,L1,R1,... . Il rend jusqu’à quatre voix par groupe de noyau, avec prise en charge du dernier groupe incomplet. Il ne mélange pas les sources ; router ou sommer les paires avec les fonctions de mixage CyDSP habituelles. Les voix doivent partager fréquence et backend sélectionné, avec des blocs préparés assez grands pour l’appel. Les zones d’échantillons entrée/sortie doivent être disjointes. L’appel entier est validé avant traitement ou écriture.

Appeler setDesigns(voices, completedDesigns, fadeFrames) sur le propriétaire du traitement, entre deux blocs, pour installer un design préparé sur worker par voix. L’appel vérifie tout le lot avant mutation et groupe le rejeu de l’historique. Chaque voix garde ses états de filtres/délais et son compteur de fondu ; les mises à jour individuelles restent disponibles. Prédiction et conception peuvent tourner sur un worker, mais l’installation d’un design et le rejeu de l’historique vivant ne doivent jamais être concurrents du traitement de ces voix.

La cascade spécialisée conserve coefficients et états en double précision et réutilise les noyaux biquad CyDSP existants. Sur x64, les groupes stéréo emploient SSE2 lorsqu’un backend plus large a été sélectionné ; les groupes complets emploient le backend sélectionné. Scalaire, NEON et SVE suivent leurs noyaux compilés. Cette exécution ne modifie ni poids, ni topologie entraînée, ni nombre de filtres. Un rendu plus rapide ne garantit pas moins de CPU total pour tous les historiques, rythmes de mise à jour ou usages.

## Exporter les poids et vérifier {#tools}

```sh
python scripts/fetch_hrtf_training.py --subjects D2 --rate 48000
python scripts/train_hrtf_pilot.py --subject D2 --sample-rate 48000 --width 96 --filters 24 --feature-bands 4 --center-grid hybrid --frequency-grid mixed --selection-grid linear --steps 30000
python scripts/export_hrtf_native.py --run build/hrtf-runs/YOUR_RUN --output build/hrtf-models/D2-48000/model.cyhrtf
python scripts/validate_hrtf_rendering.py --run build/hrtf-runs/YOUR_RUN
```

L’option explicite --transfer-sample-rate avec --initial-weights autorise le transfert depuis un autre taux du même auditeur/jeu vérifié ; nouvel optimiseur/calendrier et conservation des taux et hashes sources. Une poursuite au même taux reste une nouvelle expérience. Les instantanés sources des checkpoints restent intacts. La validation native exporte les impulsions réellement rendues, compare spectre/ILD/ITD au SOFA, vérifie la réponse théorique et mesure séparément prédiction, conception, mixage et transitions complètes. Le coût dépend du nombre de voix, de la qualité, du taux, du backend et du bloc. Les métriques objectives ne remplacent pas des tests d’écoute/localisation.

Le format little-endian CYHRTF02 contient : huit octets de signature ; six uint32 (entrées, largeur, sorties, sections par oreille, bandes de Fourier, fréquence) ; un float32 d’offset ; puis les float32 W1,b1,W2,b2,W3,b3 en ordre lignes, centres, décalages et constantes de Fourier. Le prédicteur est entrées→largeur→largeur→sorties, avec tanh cachées et décodage borné. Limites : largeur 16–256, 4–32 sections/oreille, 1–6 bandes de Fourier et 44,1–192 kHz. La taille exacte est exigée. Les banques du benchmark CYHRTF01 sont refusées. L’exporteur ajoute SHA-256/provenance et notices des données à côté des poids ; l’hôte vérifie le hash choisi et conserve les droits applicables.

## API publique {#api}

- [cydsp/spatial/NeuralHrtf.h](doc:api-spatial-neuralhrtf)
