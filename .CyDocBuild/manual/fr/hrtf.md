---
id: hrtf
title: Préparer et partager les HRTF
description: Séparer les onsets, convertir les FIR et partager les banques préparées immuables.
section: Guides
order: 37
sourceRevision: 1
parent: binaural
---

## Mesures en mémoire {#prepare}

HrtfSet accepte par défaut 2048 directions et 32768 frames/oreille ; les bornes explicites vont à 100000 points et 65536 frames. L’[importeur SOFA optionnel](doc:sofa) lit les mesures en mémoire. Une petite [sélection SADIE II optionnelle](doc:sofa#bundled) est fournie séparément du code. Par défaut, le premier dépassement d’un seuil relatif au pic (-40 dB) définit un onset entier ; les retards fractionnaires fournis restent séparés.

La préparation retire l’onset et rééchantillonne les FIR par sinc fenêtrée si nécessaire. Les taux vont de 8 à 768 kHz et le ratio de conversion de 1/8 à 8. La conversion conserve le gain DC de l’IR et expose sa latence causale commune via latencyFrames. Les onsets acoustiques directionnels restent distincts de cette latence.

## Capacité et partage {#sharing}

La FIR préparée vaut au maximum 2048 taps par défaut, configurable jusqu’à 65536. Utilisez rejectTruncation ou vérifiez truncated(). Voir le [guide de qualité](doc:sofa#prepare). Les bornes de queue des renderers comprennent FIR, retards acoustiques et latence de conversion.

Les renderers acquièrent un état préparé immuable partagé pendant prepare ; leurs historiques/fondus restent indépendants. Repréparer ou détruire le HrtfSet original n’invalide pas les renderers déjà préparés. Ce contrat diffère de l’emprunt brut de SampleData/Wavetable.

L’interpolation directionnelle utilise des poids Shepard sphériques normalisés à localité gaussienne. Une direction mesurée reproduit son point. angularWidthRadians règle la localité, pas la densité des mesures ou l’adaptation à l’auditeur. La détection d’onset n’est pas une calibration fractionnaire. Évaluez mouvements, couverture clairsemée et indices propres à l’auditeur sur des contenus réels.
## Générer des HRIR analytiques et une grille {#analytic}

sphericalFibonacci remplit des spans de directions et éventuellement de poids d’angle solide fournis par l’appelant, sans allocation. Les directions suivent le repère d’écoute CyDSP ; les poids égaux somment à 4π et donnent seulement une intégration approchée. Ce n’est ni un t-design sphérique exact ni une règle de Lebedev. Utilisez une grille suffisamment dense ou fournissez une autre grille/pondération pour une quadrature précise.

~~~cpp
#include <cydsp/spatial/SpatialRendering.h>
#include <vector>
std::vector<cydsp::Vec3> directions(256);
std::vector<double> weights(256);
const auto grid = cydsp::sphericalFibonacci(directions, weights);
if (grid != cydsp::Status::ok) return grid;
cydsp::AnalyticHrirOptions options;
options.sampleRate = 48000;
options.taps = 256;
cydsp::HrirBank<float> owned;
cydsp::AnalyticHrirReport diagnostic;
const auto generated = cydsp::analyticHrirs(options, directions, owned, &diagnostic);
if (generated != cydsp::Status::ok) return generated;
cydsp::HrtfSet<float> bank;
cydsp::HrtfPreparation preparation;
preparation.sourceSampleRate = owned.sampleRate();
preparation.targetSampleRate = 48000;
const auto ready = bank.prepare(owned.points(), preparation);
~~~

analyticHrirs excite le processeur AnalyticBinaural existant par des impulsions unitaires et copie les FIR finies choisies dans un HrirBank<float/double> propriétaire. Chaque point contient son délai acoustique dans la FIR et un délai séparé nul. HrtfSet peut ensuite séparer les onsets entiers pendant sa préparation habituelle ; n’ajoutez pas deux fois les mêmes délais.

La banque possède taux, taps et vues. Les vues survivent au déplacement de leur propriétaire et restent valides jusqu’à destruction ou remplacement réussi. Un échec préserve la banque précédente. La génération alloue : exécutez-la hors audio. Les maxima 100000 directions/65536 taps restent contraints par le budget de samples/vues, 512 Mio par défaut.

Le modèle est l’approximation générique d’ombre de tête/ITD existante, nommée AnalyticBinaural dans ce dépôt. Il n’y a ici ni classe CuedBinaural, ni géométrie individuelle de pavillon/oreille, ni détails d’élévation mesurés. Ajouter des taps ne crée pas ces indices. maximumTailSample inspecte les 64 frames après la FIR retenue : c’est un diagnostic, pas une borne de toute l’énergie omise. Choisissez les taps selon modèle/taux et vérifiez la queue.

## Interpoler les FIR et choisir le moteur {#interpolation-engine}

HrtfSet::interpolate(direction, left, right, delays) remplit deux spans de l’appelant, chacun de bank.taps() échantillons, et un tableau de deux délais d’oreilles en frames. La direction normalisée et la politique Shepard/voisins sont celles de HrirBinaural, y compris les directions mesurées exactes. Les FIR contiennent la réponse préparée de conversion de taux ; les délais d’onset restent séparés et le noyau optionnel de délai fractionnaire n’est pas incorporé. Ne doublez pas les onsets acoustiques. Le stockage est alloué avant l’audio. L’appel const n’alloue et ne libère rien ; des destinations indépendantes peuvent interroger simultanément une banque partagée. Les chevauchements des spans d’oreilles ou avec le tableau de délais échouent sans modifier les sorties. Les destinations float accumulent avec un arrondi float ; le renderer interpole en stockage double.

~~~cpp
std::vector<float> left(bank.taps()), right(bank.taps()); // allouer hors audio
std::array<double, 2> delays{};
const auto status = bank.interpolate({0, 1, 0}, left, right, delays);
if (status != cydsp::Status::ok) return status;
// Garder les délais séparés ou les incorporer une fois à la FIR de l’hôte.
~~~

Les surcharges HrirBinaural::prepare existantes sélectionnent désormais automatic. La surcharge HrirRenderConfig permet de forcer direct ou fft. maximumFftSize vaut 32768 par défaut (puissance de deux, de 32 à 262144). Un plan FFT forcé trop grand échoue sans remplacer l’existant ; le mode automatique garde le chemin direct. La taille FFT couvre toute la FIR avec délais plus une tuile courante d’au plus 128 frames : ce moteur overlap-save à FIR entière n’est pas un moteur partitionné sans borne pour les longues réponses. Il n’ajoute aucun buffering de bloc. Les deux extrémités du fondu utilisent le même historique complet, avec leurs délais d’oreilles respectifs. Le fondu est commun aux oreilles et indépendant des frontières de blocs. FFT et direct réalisent le même modèle FIR/délai avec des ordres de sommation numérique différents. Le calcul FFT utilise les FFT double et accumulations SIMD CyDSP, sans approximation de qualité réduite ni runtime externe.

~~~cpp
cydsp::HrirRenderConfig config;
config.convolution = cydsp::HrirConvolution::automatic;
cydsp::HrirBinaural<float> voice;
const auto ready = voice.prepare({48000, 128, 1}, bank, config);
if (ready != cydsp::Status::ok) return ready;
~~~

La latence déclarée reste celle de conversion de taux plus le noyau fractionnaire optionnel ; les délais causaux des oreilles restent dans la réponse spatiale. setDirection n’alloue rien mais prépare les spectres FIR lorsqu’un plan FFT existe. Incluez mises à jour et fondus à deux filtres dans le budget CPU, surtout avec beaucoup de voix ou l’interpolation globale, et sérialisez avec process. Coefficients et scratch sont alloués seulement dans prepare. SmoothedConvolver::stageImpulse est une autre opération, pouvant allouer/libérer et interdite simultanément au traitement : l’interpolation HRTF sans allocation ne rend pas cette préparation adaptée au callback audio.
