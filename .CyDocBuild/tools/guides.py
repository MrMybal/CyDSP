"""Authored bilingual CyDSP guides; identifiers and explicit anchors are stable."""
PAGES=[]
def add(id,en_title,fr_title,en_desc,fr_desc,parent,en,fr):
    PAGES.append(dict(id=id,title=(en_title,fr_title),description=(en_desc,fr_desc),parent=parent,body=(en.strip(),fr.strip())))

add('overview','CyDSP 0.4.0','CyDSP 0.4.0','Choose and combine numerical audio processors.','Choisir et assembler les traitements audio numériques.',None,r'''
## What you can build {#purpose}

CyDSP is a C++20 audio digital signal processing library. Use its float/double processors in an effect, synthesizer, DAW, game or audio engine. It supplies filters, dynamics, convolution, spectral processing, synthesis, sampling, analysis, restoration and 3D sound through prepared storage and a common planar API.

Start with [installation](doc:installation), then build a [first processing chain](doc:quickstart). Read the [processing contracts](doc:contracts) before connecting a realtime callback. The [API reference](doc:api) covers every public header, including configuration fields, exact signatures and default arguments.

## Find the right domain {#domains}

| Task | Guide |
| --- | --- |
| Filter, equalize and control levels | [Filters](doc:filters), [dynamics](doc:dynamics), [nonlinear processing](doc:advanced-dynamics) |
| Reverb or impulse processing | [Convolution](doc:convolution), [partition choices](doc:convolution-options), [reverb](doc:reverb) |
| Generate or transform a sound | [Synthesis](doc:synthesis), [samples](doc:sampling), [granulation](doc:granular), [pitch/time](doc:pitch) |
| Inspect or repair audio | [Meters](doc:metering), [analysis](doc:analysis), [diagnostics](doc:diagnostics), [restoration](doc:restoration) |
| Render game audio in space | [Coordinates](doc:spatial), [acoustics](doc:acoustics), [binaural](doc:binaural), [Ambisonics](doc:ambisonics) |

## Library integration {#integration}

CyDSP processes in-memory audio. Your application supplies the audio callback, stream ownership and surrounding services. CyAudioCore is one project using its public API. You can link CyDSP directly into another application.

Version 0.4.0 is in development. [Platform and model limits](doc:limits) distinguish available algorithms from targets requiring execution validation. Keep the [MIT and component notices](doc:licensing) with redistributed software.
''',r'''
## Ce que vous pouvez construire {#purpose}

CyDSP est une bibliothèque C++20 de traitement numérique du signal audio. Ses processeurs float/double s’utilisent dans un effet, un synthétiseur, un DAW, un jeu vidéo ou un moteur audio. Elle fournit filtres, dynamique, convolution, traitements spectraux, synthèse, sampling, analyse, restauration et son 3D avec un stockage préparé et une API planaire commune.

Commencez par l’[installation](doc:installation), puis une [première chaîne de traitement](doc:quickstart). Lisez les [contrats de traitement](doc:contracts) avant de raccorder un callback temps réel. La [référence de l’API](doc:api) couvre tous les en-têtes publics, avec champs de configuration, signatures exactes et arguments par défaut.

## Trouver le bon domaine {#domains}

| Objectif | Guide |
| --- | --- |
| Filtrer, égaliser et contrôler les niveaux | [Filtres](doc:filters), [dynamique](doc:dynamics), [traitements non linéaires](doc:advanced-dynamics) |
| Réverbérer ou traiter des impulsions | [Convolution](doc:convolution), [choix des partitions](doc:convolution-options), [réverbération](doc:reverb) |
| Générer ou transformer un son | [Synthèse](doc:synthesis), [samples](doc:sampling), [granulation](doc:granular), [hauteur/durée](doc:pitch) |
| Examiner ou réparer l’audio | [Mesureurs](doc:metering), [analyse](doc:analysis), [diagnostic](doc:diagnostics), [restauration](doc:restoration) |
| Rendre l’audio d’un jeu dans l’espace | [Coordonnées](doc:spatial), [acoustique](doc:acoustics), [binaural](doc:binaural), [Ambisonics](doc:ambisonics) |

## Intégrer la bibliothèque {#integration}

CyDSP traite l’audio en mémoire. L’application fournit le callback audio, possède les flux et gère les services qui les entourent. CyAudioCore est un projet utilisant son API publique. CyDSP peut être lié directement à une autre application.

La version 0.4.0 est en développement. Les [limites des plateformes et modèles](doc:limits) distinguent les algorithmes disponibles des cibles restant à valider en exécution. Conservez les [notices MIT et tierces](doc:licensing) avec les logiciels redistribués.
''')

add('installation','Build and link CyDSP','Compiler et lier CyDSP','Build the static libraries and use their CMake targets.','Compiler les bibliothèques statiques et utiliser leurs cibles CMake.','overview',r'''
## Requirements {#requirements}

Use CMake 3.24 or later and a C++20 compiler. Obtain an authorized checkout of the private repository. Python is needed only for repository helper scripts. Five pinned numerical adaptations and their notices are included; configuration does not download them.

The main target is `CyDSP::CyDSP`. `CyDSP::Stretch` adds optional time/pitch/formant processing and is built by default. Set `CYDSP_BUILD_STRETCH=OFF` when unnecessary.

## Build Windows x64 {#build}

From the checkout with Visual Studio 2022 C++ tools:

```sh
cmake -S . -B build/release -G "Visual Studio 17 2022" -A x64
cmake --build build/release --config Release
cmake --install build/release --config Release --prefix build/install
```

Production builds create static libraries; native tests and measurements are opt-in. Select a separate compiler/toolchain and archive for each target ABI.

## Link an application {#link}

Point `CMAKE_PREFIX_PATH` at the absolute install prefix. After defining the application's target:

```cmake
find_package(CyDSP 0.4.0 CONFIG REQUIRED)
target_link_libraries(MyApplication PRIVATE CyDSP::CyDSP)
# target_link_libraries(MyApplication PRIVATE CyDSP::Stretch)
```

Include an individual `cydsp/...` header or `cydsp/CyDSP.h`. Match headers and archive; rebuild consumers after object-layout changes. Package compatibility is limited to one minor version. Compile-time constants/macros are in `cydsp/core/Version.h`. See [CPU selection](doc:simd) and [target limits](doc:limits).
''',r'''
## Prérequis {#requirements}

Utilisez CMake 3.24 ou ultérieur et un compilateur C++20. Obtenez une copie autorisée du dépôt privé. Python sert uniquement aux scripts utilitaires du dépôt. Cinq adaptations numériques figées et leurs notices sont incluses ; la configuration ne les télécharge pas.

La cible principale est `CyDSP::CyDSP`. `CyDSP::Stretch` ajoute le traitement optionnel de durée/hauteur/formants et se compile par défaut. Définissez `CYDSP_BUILD_STRETCH=OFF` s’il est inutile.

## Compiler Windows x64 {#build}

Depuis le dépôt avec les outils C++ Visual Studio 2022 :

```sh
cmake -S . -B build/release -G "Visual Studio 17 2022" -A x64
cmake --build build/release --config Release
cmake --install build/release --config Release --prefix build/install
```

Les builds de production créent des bibliothèques statiques ; tests natifs et mesures sont activés explicitement. Choisissez un compilateur/toolchain et une archive distincts pour chaque ABI cible.

## Lier une application {#link}

Faites pointer `CMAKE_PREFIX_PATH` vers le préfixe d’installation absolu. Après définition de la cible de l’application :

```cmake
find_package(CyDSP 0.4.0 CONFIG REQUIRED)
target_link_libraries(MyApplication PRIVATE CyDSP::CyDSP)
# target_link_libraries(MyApplication PRIVATE CyDSP::Stretch)
```

Incluez un en-tête individuel `cydsp/...` ou `cydsp/CyDSP.h`. Faites correspondre en-têtes et archive ; recompilez les consommateurs après changement de disposition des objets. La compatibilité du package est limitée à une version mineure. Les constantes/macros de compilation figurent dans `cydsp/core/Version.h`. Consultez la [sélection CPU](doc:simd) et les [limites des cibles](doc:limits).
''')

add('quickstart','Your first processing chain','Votre première chaîne de traitement','Prepare a stereo gain and process borrowed blocks.','Préparer un gain stéréo et traiter des blocs empruntés.','overview',r'''
## Prepare once, process blocks {#chain}

This complete example owns a stereo gain. Call `prepare` outside the callback. Both supplied channels must contain at least `frames` samples; `frames` must not exceed 512.

```cpp
#include <cydsp/CyDSP.h>
struct StereoGain {
    cydsp::Gain<float> gain;
    cydsp::Status prepare() noexcept {
        auto status = gain.prepare({48000.0, 512, 2});
        if (status != cydsp::Status::ok) return status;
        return gain.setGain(0.5f);
    }
    cydsp::Status process(float* left, float* right, std::size_t frames) noexcept {
        float* channels[] = {left, right};
        cydsp::AudioBlock<float> block{channels, 2, frames};
        return gain.process(block, block);
    }
};
```

A gain of 0.5 halves amplitude, about −6.02 dB. There is no automatic output limiter. `setGain(target, rampFrames)` changes gain smoothly; 480 frames last 10 ms at 48 kHz.

## Compose processors {#compose}

Prepare the next filter/effect with the same `ProcessSpec`, then process the preceding stage's output. Check every `Status`. Preparing several objects is not a transaction for the whole chain: publish a new chain only after all its members form a coherent prepared state.

For separate output, allocate an `AudioBuffer` before the callback. For parallel branches, use [graphs](doc:graphs) or explicitly align [latency](doc:contracts#timing).
''',r'''
## Préparer une fois, traiter les blocs {#chain}

Cet exemple complet possède un gain stéréo. Appelez `prepare` hors callback. Les deux canaux fournis doivent contenir au moins `frames` échantillons ; `frames` ne doit pas dépasser 512.

```cpp
#include <cydsp/CyDSP.h>
struct StereoGain {
    cydsp::Gain<float> gain;
    cydsp::Status prepare() noexcept {
        auto status = gain.prepare({48000.0, 512, 2});
        if (status != cydsp::Status::ok) return status;
        return gain.setGain(0.5f);
    }
    cydsp::Status process(float* left, float* right, std::size_t frames) noexcept {
        float* channels[] = {left, right};
        cydsp::AudioBlock<float> block{channels, 2, frames};
        return gain.process(block, block);
    }
};
```

Un gain de 0,5 divise l’amplitude par deux, soit environ −6,02 dB. Aucun limiteur de sortie n’est implicite. `setGain(target, rampFrames)` change progressivement le gain ; 480 frames durent 10 ms à 48 kHz.

## Assembler les processeurs {#compose}

Préparez le filtre/effet suivant avec le même `ProcessSpec`, puis traitez la sortie précédente. Vérifiez chaque `Status`. Préparer plusieurs objets ne constitue pas une transaction pour toute la chaîne : ne publiez une nouvelle chaîne qu’après avoir préparé tous ses membres de façon cohérente.

Pour une sortie distincte, allouez un `AudioBuffer` avant le callback. Pour des branches parallèles, utilisez les [graphes](doc:graphs) ou alignez explicitement les [latences](doc:contracts#timing).
''')

add('contracts','Processing contracts','Contrats de traitement','Lifetimes, status values, units, latency and tails.','Durées de vie, états, unités, latence et queues.','overview',r'''
## Lifecycle and ownership {#lifecycle}

`prepare` validates and may allocate; failure retains the previous usable configuration. `process` and `reset` retain prepared storage without allocation/free, locks or I/O. Repreparation, destruction, moves and explicit staging such as `stageImpulse` belong outside realtime processing.

Serialize all access to a mutable instance, including setters and getters. Views do not own samples or pointer tables. Shared assets and callback contexts have their API's lifetime. A successful `AudioBuffer::resize` invalidates previous views.

## Units and errors {#status}

A frame contains one sample per channel. Frequencies use Hz; time follows the API suffix (`Seconds`, `Ms`, `Frames`); gains are linear unless labeled dB. Sample formats are float/double; counts normally use `std::size_t`.

| Status | Caller action |
| --- | --- |
| `ok` | Use output; honor counts for variable-rate APIs. |
| `not_prepared` | Prepare first. |
| `invalid_spec`, `invalid_argument` | Correct dimensions, ranges or non-finite values. |
| `channel_mismatch`, `block_too_large` | Match prepared channels and maximum block. |
| `partial_overlap` | Use an allowed exact alias or disjoint storage. |
| `unsupported_backend` | Select a compiled CPU/OS-supported backend. |
| `transition_in_progress`, `update_queue_full`, `would_block` | Retain/retry according to the API; never busy-wait in the callback. |
| `end_of_stream` | Drain is complete; reset before a new stream. |

Ordinary validation errors preserve output/state. Graph callback failures cannot roll back executed nodes: external outputs are silenced. Extreme gains or sums can overflow despite finite inputs; CyDSP does not globally normalize or clip audio.

## Latency and tails {#timing}

Latency delays output; a tail continues after input ends. Read `latencyFrames()`/`tailFrames()` and retain fractional values when returned as double. A convolver with zero added block latency may still use an IR with acoustic predelay. Feed zeros or call `flush`/`finish` until drained. Freeze may create an unbounded tail. Reset erases history instead of rendering it.
''',r'''
## Cycle de vie et propriété {#lifecycle}

`prepare` valide et peut allouer ; un échec conserve la configuration précédente utilisable. `process` et `reset` conservent le stockage préparé sans allocation/libération, verrou ni entrée/sortie. Nouvelle préparation, destruction, déplacements et staging explicite comme `stageImpulse` se font hors traitement temps réel.

Sérialisez tous les accès à une instance mutable, setters et getters compris. Les vues ne possèdent ni échantillons ni tableaux de pointeurs. Données partagées et contextes ont la durée de vie précisée par leur API. Un `AudioBuffer::resize` réussi invalide les anciennes vues.

## Unités et erreurs {#status}

Une frame contient un échantillon par canal. Les fréquences sont en Hz ; les durées suivent le suffixe (`Seconds`, `Ms`, `Frames`) ; les gains sont linéaires sauf mention dB. Les formats sont float/double ; les comptes utilisent généralement `std::size_t`.

| État | Action de l’appelant |
| --- | --- |
| `ok` | Utiliser la sortie ; respecter les comptes des API à taux variable. |
| `not_prepared` | Préparer d’abord. |
| `invalid_spec`, `invalid_argument` | Corriger dimensions, plages ou valeurs non finies. |
| `channel_mismatch`, `block_too_large` | Respecter les canaux et le bloc maximal préparés. |
| `partial_overlap` | Utiliser un alias exact autorisé ou du stockage disjoint. |
| `unsupported_backend` | Choisir un backend compilé et disponible pour le CPU/OS. |
| `transition_in_progress`, `update_queue_full`, `would_block` | Conserver/réessayer selon l’API ; ne pas attendre en boucle dans le callback. |
| `end_of_stream` | Vidange terminée ; réinitialiser avant un nouveau flux. |

Les erreurs ordinaires de validation préservent sortie et état. Une erreur de callback de graphe ne peut annuler les nœuds exécutés : les sorties externes sont mises au silence. Des gains ou sommes extrêmes peuvent déborder malgré des entrées finies ; CyDSP ne normalise ni n’écrête globalement l’audio.

## Latence et queues {#timing}

La latence retarde la sortie ; une queue continue après la fin d’entrée. Consultez `latencyFrames()`/`tailFrames()` et conservez les fractions retournées en double. Un convolver sans latence de bloc ajoutée peut utiliser une IR contenant un prédélai acoustique. Fournissez des zéros ou appelez `flush`/`finish` jusqu’à vidange. Un freeze peut créer une queue infinie. Reset efface l’historique au lieu de le rendre.
''')

add('buffers','Buffers and channel layouts','Buffers et dispositions de canaux','Own storage, borrow views and interleave channels.','Posséder le stockage, emprunter des vues et entrelacer les canaux.','contracts',r'''
## Views and storage {#views}

`AudioBlock<T>` borrows a planar pointer table, channels and frames. `channel(i)` applies the sub-block offset; `channelData()` returns the original table. Nested `subBlock` calls accumulate offsets. Do not treat that original table as if it already pointed to a sub-block's start.

`AudioBuffer<T>::resize(channels, frames)` allocates zeroed 64-byte-aligned channel storage with private padding. Its block exposes logical frames only. Allocate outside processing. Keep allocations and pointer tables alive and unchanged while borrowed views are used.

## Overlap and conversion {#overlap}

Ordinary fixed-rate processors accept matching-channel exact in-place views or disjoint views; partial overlap fails. ChannelMatrix and MatrixConvolver additionally permit documented exact cross-channel aliases. FFTs, resamplers and shape-changing APIs have their own rules.

`interleave`/`deinterleave` use exactly `frames * channels` flat samples at `frame * channels + channel`. All planar/flat overlap is rejected; deinterleave also rejects overlapping output channels. No rate, sample-format or speaker-order conversion is implied. Validation cannot prove pointer lifetime or allocated capacity.
''',r'''
## Vues et stockage {#views}

`AudioBlock<T>` emprunte un tableau de pointeurs planaires, des canaux et des frames. `channel(i)` applique l’offset du sous-bloc ; `channelData()` retourne le tableau original. Les appels `subBlock` imbriqués cumulent les offsets. Ne considérez pas ce tableau original comme pointant déjà sur le début du sous-bloc.

`AudioBuffer<T>::resize(channels, frames)` alloue un stockage de canaux initialisé à zéro, aligné sur 64 octets et doté d’un padding privé. Son bloc expose seulement les frames logiques. Allouez hors traitement. Gardez allocations et tableaux de pointeurs vivants et inchangés pendant l’utilisation des vues empruntées.

## Recouvrement et conversion {#overlap}

Les processeurs usuels à taux fixe acceptent les vues exactement en place par canal correspondant ou disjointes ; le recouvrement partiel échoue. ChannelMatrix et MatrixConvolver permettent aussi les alias exacts entre canaux documentés. FFT, rééchantillonneurs et API changeant la forme ont leurs propres règles.

`interleave`/`deinterleave` utilisent exactement `frames * channels` échantillons plats à `frame * channels + channel`. Tout recouvrement planaire/plat est rejeté ; deinterleave refuse aussi les canaux de sortie qui se recouvrent. Aucune conversion de fréquence, format d’échantillon ou ordre d’enceintes n’est implicite. La validation ne prouve ni la durée de vie ni la capacité des allocations.
''')

add('threading','Realtime ownership','Propriété temps réel','Serialize access and publish bounded control data.','Sérialiser les accès et publier des contrôles bornés.','contracts',r'''
## One owner per mutable processor {#owner}

CyDSP creates no processing thread. Give each mutable instance one audio owner and serialize setters, reset, getters, moves and destruction. Copy display metrics to application-owned snapshots instead of reading analyzer state concurrently.

Borrowed samples, contexts and pointer tables must remain at stable addresses until users stop. Shared HRTF prepared states have the distinct lifetime described in [HRTF preparation](doc:hrtf).

## Bounded communication {#queues}

`SpscQueue` requires exactly one producer and one consumer. Prepare capacity outside processing; check push/pop success and define a full-queue policy. Do not put values requiring realtime destruction into a queue without an ownership protocol.

`AsyncConvolver` explicitly supports one producer staging an IR concurrently with one audio consumer. Three retained slots prevent callback waits and destruction of retired plans. Preparation, reset and destruction require both sides quiescent. Treat queue pressure as a normal result.

Allocation-free does not mean cheap: long loudness queries, HRTF interpolation, HOA rotations and coefficient design still require scheduling. `noexcept` is not a CPU-time bound.
''',r'''
## Un propriétaire par processeur mutable {#owner}

CyDSP ne crée pas de thread de traitement. Confiez chaque instance mutable à un propriétaire audio et sérialisez setters, reset, getters, déplacements et destruction. Copiez les métriques d’affichage dans des instantanés possédés par l’application au lieu de lire simultanément l’état d’un analyseur.

Samples, contextes et tableaux empruntés restent à des adresses stables jusqu’à l’arrêt des utilisateurs. Les états HRTF préparés partagés ont la durée de vie particulière décrite dans la [préparation HRTF](doc:hrtf).

## Communication bornée {#queues}

`SpscQueue` exige exactement un producteur et un consommateur. Préparez sa capacité hors traitement ; vérifiez push/pop et définissez une politique de saturation. N’introduisez pas de valeurs exigeant une destruction temps réel sans protocole de propriété.

`AsyncConvolver` accepte explicitement un producteur préparant une IR en parallèle d’un consommateur audio. Trois emplacements conservés évitent attente et destruction de plans retirés dans le callback. Préparation, reset et destruction exigent l’arrêt des deux côtés. Traitez la pression de file comme un résultat normal.

Sans allocation ne signifie pas peu coûteux : grandes requêtes de sonie, interpolation HRTF, rotations HOA et conception de coefficients se planifient. `noexcept` ne constitue pas une borne de temps CPU.
''')

add('parameters','Parameters and automation','Paramètres et automation','Render stable-ID events and sample-offset ramps.','Rendre les événements à ID stables et les rampes à l’échantillon.','contracts',r'''
## Descriptors and events {#events}

`ParameterDescriptor` supplies stable ID, name, unit, bounds, initial value and linear/logarithmic scale. ParameterBank copies strings during preparation. Its double output has one planar channel per descriptor, in descriptor order.

Submit events sorted by `frameOffset` within the current block. A ramp advances on the event's first sample and continues across blocks. ParameterBank produces control values; the application connects them to processing. It does not reflect fields or call setters automatically.

Use `parameterToNormalized`/`parameterFromNormalized` with the same descriptor; logarithmic ranges need a valid positive domain. UI units do not change processor units.

## Choose the smoothing mechanism {#smoothing}

SmoothedValue and gain ramps avoid scalar jumps. Stable-filter fades, delay-reader fades and prepared IR/graph transitions address different discontinuities. Keep contexts alive through completion and honor `transition_in_progress`.

For sample-accurate changes to a block processor, split the audio at the event, apply a supported setter and continue. ParameterBank output does not make a block-rate setter an audio-rate modulation input.
''',r'''
## Descripteurs et événements {#events}

`ParameterDescriptor` fournit ID stable, nom, unité, bornes, valeur initiale et échelle linéaire/logarithmique. ParameterBank copie les noms à la préparation. Sa sortie double a un canal planaire par descripteur, dans leur ordre.

Fournissez les événements triés par `frameOffset` dans le bloc courant. Une rampe avance dès le premier échantillon de l’événement et continue sur les blocs suivants. ParameterBank produit des valeurs de contrôle ; l’application les raccorde au traitement. Il n’inspecte pas les champs et n’appelle pas automatiquement les setters.

Utilisez `parameterToNormalized`/`parameterFromNormalized` avec le même descripteur ; les plages logarithmiques exigent un domaine positif valide. Les unités d’interface ne changent pas celles des processeurs.

## Choisir le mécanisme de lissage {#smoothing}

SmoothedValue et les rampes de gain évitent les sauts scalaires. Fondus entre filtres stables, lecteurs de délais et IR/graphes préparés répondent à des discontinuités différentes. Gardez les contextes jusqu’à la fin et respectez `transition_in_progress`.

Pour modifier un processeur de blocs précisément à l’échantillon, découpez l’audio à l’événement, appliquez un setter autorisé et continuez. La sortie de ParameterBank ne transforme pas un setter par bloc en entrée de modulation audio-rate.
''')

add('simd','CPU architectures and SIMD','Architectures CPU et SIMD','Choose supported kernels during preparation.','Choisir les noyaux disponibles à la préparation.','contracts',r'''
## Dispatch and binaries {#dispatch}

Build an archive for each target ABI. Automatic selection uses compiled CPU/OS support: AVX-512, AVX2 or SSE2 on suitable x64; SVE then NEON on compatible ARM64 Linux/Android; otherwise the available baseline. Windows/macOS ARM64 use NEON. AVX-512 is x64-only; SVE requires compatible ARM64 hardware.

A forced unavailable backend returns `unsupported_backend` while retaining the old plan. VectorOps::create prepares an immutable table; the default VectorOps is scalar. Read `backend()` where exposed.

## Numerical behavior {#numerics}

SIMD handles independent samples, channels or voices where dependencies permit. Recursive filters/envelopes retain signal order. Dot reductions can round differently across backends; universal bit identity is not assumed.

ISA options stay in private implementation files. Portable code uses no global native-CPU flags, fast-math, FMA contraction or floating-point-state mutation. Automatic selection is capability-based, not measured for each buffer. Profile actual processors/block patterns before overriding it. SVE uses the thread's current vector length; hardware execution remains a [target limit](doc:limits).
''',r'''
## Dispatch et binaires {#dispatch}

Compilez une archive par ABI cible. La sélection automatique utilise les capacités compilées CPU/OS : AVX-512, AVX2 ou SSE2 sur x64 compatible ; SVE puis NEON sous ARM64 Linux/Android compatible ; sinon la base disponible. Windows/macOS ARM64 utilisent NEON. AVX-512 est réservé à x64 ; SVE exige un matériel ARM64 compatible.

Forcer un backend indisponible retourne `unsupported_backend` en conservant l’ancien plan. VectorOps::create prépare une table immuable ; VectorOps utilise le scalaire par défaut. Consultez `backend()` lorsqu’il est exposé.

## Comportement numérique {#numerics}

Le SIMD traite échantillons, canaux ou voix indépendants lorsque les dépendances le permettent. Filtres récursifs et enveloppes conservent l’ordre du signal. Les réductions de produits scalaires peuvent s’arrondir différemment selon le backend ; l’identité binaire universelle n’est pas supposée.

Les options ISA restent dans les implémentations privées. Le code portable n’utilise ni drapeau global propre au CPU, fast-math, contraction FMA ni mutation de l’état flottant. La sélection automatique suit les capacités, sans mesurer chaque buffer. Profilez les processeurs et découpages réels avant de la remplacer. SVE utilise la longueur vectorielle courante du thread ; son exécution matérielle reste une [limite de cible](doc:limits).
''')

add('quality','Quality and accuracy','Qualité et précision','Choose local policies and prepared transitions.','Choisir les politiques locales et transitions préparées.','contracts',r'''
## Preparation profiles {#profiles}

`qualitySettings` recommends settings for `economy`, `realtimeHigh` and `offline`: spectral window/hop, resampling/interpolation, HRTF length, FDN size, oversampling, true peak, grain budget and fade duration. Apply them during processor preparation; there is no hidden global switch or automatic worker.

QualityGraph retains two MultiBusGraph plans with a fixed latency budget, warmup and fade. CPU/memory include both during transition. Contexts remain alive until replaced outside processing.

## Precise and fast {#accuracy}

MathAccuracy::precise keeps the processor's reference transfer; fast explicitly selects supported approximations locally. It is independent of SaturationParameters::antialias. Disabling antialiasing changes the algorithm and may increase spectral aliasing.

```cpp
#include <cydsp/CyDSP.h>
cydsp::Status prepareFastTransient(cydsp::TransientShaper<float>& effect) noexcept {
    auto status = effect.prepare({48000.0, 512, 2});
    if (status != cydsp::Status::ok) return status;
    cydsp::TransientParameters p;
    p.attack = 6; p.sustain = -3;
    p.math = cydsp::MathAccuracy::fast;
    return effect.setParameters(p);
}
```

Compare actual content and gain envelopes before selecting defaults. No error on one corpus does not prove identity for every signal. Keep precise selectable when required by the application's quality contract.
''',r'''
## Profils de préparation {#profiles}

`qualitySettings` recommande des réglages pour `economy`, `realtimeHigh` et `offline` : fenêtre/hop spectraux, rééchantillonnage/interpolation, longueur HRTF, taille FDN, suréchantillonnage, true peak, budget de grains et durée de fondu. Appliquez-les à la préparation ; aucun réglage global caché ni worker automatique n’est créé.

QualityGraph conserve deux plans MultiBusGraph avec budget de latence fixe, préchauffage et fondu. CPU/mémoire incluent les deux pendant la transition. Les contextes restent vivants jusqu’à leur remplacement hors traitement.

## Précis et rapide {#accuracy}

MathAccuracy::precise garde la fonction de transfert de référence ; fast choisit explicitement les approximations proposées localement. Ce choix est indépendant de SaturationParameters::antialias. Désactiver l’antialiasing change l’algorithme et peut augmenter le repliement spectral.

```cpp
#include <cydsp/CyDSP.h>
cydsp::Status prepareFastTransient(cydsp::TransientShaper<float>& effect) noexcept {
    auto status = effect.prepare({48000.0, 512, 2});
    if (status != cydsp::Status::ok) return status;
    cydsp::TransientParameters p;
    p.attack = 6; p.sustain = -3;
    p.math = cydsp::MathAccuracy::fast;
    return effect.setParameters(p);
}
```

Comparez les contenus et enveloppes de gain réels avant de choisir les défauts. Une erreur nulle sur un corpus ne prouve pas l’identité pour tous les signaux. Gardez le mode précis sélectionnable si le contrat de qualité de l’application l’exige.
''')

add('mixing','Gain, panning and routing','Gain, panoramique et routage','Route and mix planar channels with explicit gain laws.','Router et mixer les canaux planaires avec des lois explicites.','overview',r'''
## Gain and pan {#gain}

Gain uses linear amplitude; `decibelsToGain` converts dB. Gain ramps advance once per frame across channels and across block boundaries. Reset snaps to the current target. Select a Panner/DryWetMixer law explicitly: equal-power and linear fades have different midpoint behavior, and correlated wet/dry signals can sum above unity.

WaveShaper provides elementary static shapes; AdaaSaturator adds antialiasing/stateful choices described in [advanced dynamics](doc:advanced-dynamics). Bias adds a DC offset; it is not a DC-removal filter.

## Channel matrices {#matrix}

ChannelMatrix coefficients are output-major: `coefficients[out * inputChannels + in]`. Each output is the sum of its weighted inputs. Coefficients round to the sample type. There is no implicit speaker mapping, loudness compensation, clipping or normalization.

`setMatrix` preserves shape and can ramp gains once per frame. Repeated identical targets retain ramp progress. Reset snaps to target. Exact cross-channel aliases support permutations and rectangular in-place mixes through retained input snapshots; partial overlap and output/output overlap fail. Separate read-only input channels may alias.

Use application-specific coefficients for mono/stereo/downmix and document their headroom. Muted coefficients omit their source contribution; unmuted extreme sums can overflow.
''',r'''
## Gain et panoramique {#gain}

Gain utilise l’amplitude linéaire ; `decibelsToGain` convertit les dB. Les rampes avancent une fois par frame pour tous les canaux et continuent entre blocs. Reset rejoint la cible courante. Choisissez explicitement la loi Panner/DryWetMixer : fondus à puissance constante et linéaires ont des milieux différents ; des signaux dry/wet corrélés peuvent dépasser l’unité en se sommant.

WaveShaper fournit les formes statiques élémentaires ; AdaaSaturator ajoute les choix avec antialiasing/mémoire décrits dans la [dynamique avancée](doc:advanced-dynamics). Bias ajoute un décalage continu ; il ne retire pas le DC.

## Matrices de canaux {#matrix}

Les coefficients ChannelMatrix sont indexés par sortie : `coefficients[out * inputChannels + in]`. Chaque sortie somme ses entrées pondérées. Les coefficients sont arrondis au format d’échantillon. Aucun placement d’enceintes, compensation de sonie, écrêtage ou normalisation n’est implicite.

`setMatrix` conserve la forme et peut ramper une fois par frame. Une cible identique répétée conserve la progression. Reset rejoint la cible. Les alias exacts entre canaux autorisent permutations et mixages rectangulaires en place grâce aux copies d’entrée conservées ; recouvrements partiels et entre sorties échouent. Des entrées en lecture seule peuvent s’aliaser.

Utilisez les coefficients mono/stéréo/downmix adaptés à l’application et définissez leur réserve de niveau. Les coefficients nuls omettent leur entrée ; les grandes sommes non muettes peuvent déborder.
''')

add('filters','Filters and equalizers','Filtres et égaliseurs','Choose biquads, cascades, FIR, TPT or ladder processing.','Choisir biquads, cascades, FIR, TPT ou ladder.','overview',r'''
## Coefficients and state {#biquad}

BiquadCoefficients normalize a0 to 1. Feedback is subtracted: `-a1*y[n-1] - a2*y[n-2]`. Use validated design functions; frequency is strictly between zero and Nyquist and Q is positive/finite. Peaking gain uses dB; shelf slope is in (0,1].

```cpp
#include <cydsp/CyDSP.h>
cydsp::Status prepareLowPass(cydsp::Biquad<float>& filter) noexcept {
    auto status = filter.prepare({48000.0, 512, 2});
    if (status != cydsp::Status::ok) return status;
    cydsp::BiquadCoefficients c;
    status = cydsp::designBiquad(cydsp::BiquadType::lowPass, 48000.0, 8000.0, 0.707, c);
    if (status != cydsp::Status::ok) return status;
    return filter.setCoefficients(c);
}
```

Biquad shares coefficients across channels with independent histories. BiquadBank gives each channel/voice its own coefficients and can vectorize independent voices. A cascade applies ordered sections; an empty cascade bypasses. Parameter edits preserve histories unless the specific transition/reset contract says otherwise.

## Automation and alternatives {#alternatives}

SmoothedBiquad and SmoothedBiquadCascade crossfade stable banks. A new nonzero-duration target during a fade coalesces to the latest following target; zero-duration edits cancel transitions. Reset clears state and snaps to the latest target.

FIR uses causal h[0] on the current sample. TptFilter and LadderFilter offer state-variable/ladder responses; LinkwitzRiley splits bands. Use [filter design](doc:filter-design) for high-order families and complementary FIR bands. IIR phase/group delay is frequency dependent; it is not generally one integer delay.
''',r'''
## Coefficients et état {#biquad}

BiquadCoefficients normalise a0 à 1. Le feedback est soustrait : `-a1*y[n-1] - a2*y[n-2]`. Utilisez les fonctions de conception validées ; fréquence strictement entre zéro et Nyquist, Q positif et fini. Le gain paramétrique est en dB ; la pente shelving appartient à (0,1].

```cpp
#include <cydsp/CyDSP.h>
cydsp::Status prepareLowPass(cydsp::Biquad<float>& filter) noexcept {
    auto status = filter.prepare({48000.0, 512, 2});
    if (status != cydsp::Status::ok) return status;
    cydsp::BiquadCoefficients c;
    status = cydsp::designBiquad(cydsp::BiquadType::lowPass, 48000.0, 8000.0, 0.707, c);
    if (status != cydsp::Status::ok) return status;
    return filter.setCoefficients(c);
}
```

Biquad partage les coefficients entre canaux, avec historiques indépendants. BiquadBank donne à chaque canal/voix ses coefficients et peut vectoriser entre voix indépendantes. Une cascade applique les sections dans l’ordre ; vide, elle laisse passer le signal. Les modifications conservent les historiques sauf contrat spécifique de transition/reset.

## Automation et autres filtres {#alternatives}

SmoothedBiquad et SmoothedBiquadCascade fondent des banques stables. Une nouvelle cible de durée non nulle pendant le fondu remplace la cible suivante en attente ; une modification immédiate annule les transitions. Reset efface l’état et rejoint la dernière cible.

FIR applique h[0] causalement à l’échantillon courant. TptFilter et LadderFilter proposent les réponses à variables d’état/ladder ; LinkwitzRiley sépare les bandes. La [conception des filtres](doc:filter-design) couvre les familles d’ordre élevé et les bandes FIR complémentaires. Le retard de phase/groupe d’un IIR dépend de la fréquence ; ce n’est généralement pas un retard entier unique.
''')

add('filter-design','Filter design and multiband banks','Conception des filtres et banques multibandes','Prepare higher-order IIR, linear/minimum-phase FIR and splits.','Préparer IIR d’ordre élevé, FIR linéaires/minimales et séparations.','filters',r'''
## IIR families {#iir}

`designIir` creates Butterworth, Chebyshev I/II, elliptic or Bessel low/high-pass sections. Set the prototype order, sample rate, cutoff and family's ripple/attenuation controls before processing. `designBandIir` transforms a prototype order 1–16 into a bandpass/bandstop of doubled order. Keep both band edges below Nyquist and check returned Status.

Design functions are preparation work and may allocate. Verify the final coefficient response with [diagnostics](doc:diagnostics), particularly near Nyquist or for narrow/high-order bands. PeakingBiquadDesigner caches frequency/Q work for gain automation and offers sample-type stability validation.

## FIR phase and reconstruction {#fir}

`designFirEqualizer` designs a truncated/windowed response from frequency/gain points. Linear-phase FIR delays its output; `minimumPhaseFir` uses a real cepstrum to create an approximation with energy shifted earlier. Truncation/deep notches can alter the magnitude response: measure the result rather than assuming exact equivalence.

FirFilterBank uses differences of low-pass FIRs for up to 32 complementary bands. The neutral sum equals the input delayed by `(taps-1)/2`, within rounding. Output channel order is `band * inputChannels + channel`. Allocate enough outputs and compensate this latency before parallel dry/wet mixing. MultibandDynamics uses this foundation with its own smaller band limit.
''',r'''
## Familles IIR {#iir}

`designIir` produit des sections passe-bas/haut Butterworth, Chebyshev I/II, elliptique ou Bessel. Réglez ordre du prototype, fréquence d’échantillonnage, coupure et paramètres d’ondulation/atténuation avant traitement. `designBandIir` transforme un prototype d’ordre 1–16 en passe-bande/coupe-bande d’ordre double. Gardez les deux limites sous Nyquist et vérifiez Status.

La conception est un travail de préparation et peut allouer. Vérifiez la réponse finale par les [diagnostics](doc:diagnostics), notamment près de Nyquist et pour les bandes étroites/d’ordre élevé. PeakingBiquadDesigner met en cache la partie fréquence/Q pour l’automation du gain et peut valider la stabilité après arrondi au format d’échantillon.

## Phase FIR et reconstruction {#fir}

`designFirEqualizer` conçoit une réponse tronquée/fenêtrée depuis des points fréquence/gain. Un FIR à phase linéaire retarde sa sortie ; `minimumPhaseFir` utilise le cepstre réel pour une approximation dont l’énergie arrive plus tôt. Troncature et creux profonds peuvent modifier la réponse d’amplitude : mesurez le résultat sans supposer l’équivalence exacte.

FirFilterBank utilise des différences de passe-bas FIR pour au plus 32 bandes complémentaires. Leur somme neutre égale l’entrée retardée de `(taps-1)/2`, aux arrondis près. L’ordre des sorties est `band * inputChannels + channel`. Réservez assez de canaux et compensez cette latence avant un mixage dry/wet parallèle. MultibandDynamics réutilise cette base avec sa propre limite de bandes plus faible.
''')

add('dynamics','Compressors, gates and sidechains','Compresseurs, gates et sidechains','Control detector ballistics and prepared look-ahead.','Contrôler les enveloppes du détecteur et l’anticipation préparée.','overview',r'''
## Modes and detector {#detector}

Dynamics supports compressor, expander, noiseGate and limiter modes. Prepare the mode and maximum look-ahead first; set threshold/knee/makeup in dB, ratio, attack/release/hold in ms, gate hysteresis and linked/unlinked channels through DynamicsParameters.

Peak and RMS detectors have different ballistics. BallisticsFilter also exposes envelope detection independently. A linked detector shares gain decisions across channels; it does not sum those channels into mono output.

## External sidechain and mixing {#sidechain}

The three-block process overload accepts program, sidechain and output. A mono sidechain broadcasts to the program channels; otherwise its channels match the program. Sidechain/output must be disjoint unless the sidechain is the program block itself. Keep detector timing aligned with the desired program event.

processMixed accepts finite linear dry/wet gains of magnitude at most 16. Its dry path follows the configured look-ahead delay. gainReductionDb and minimumAppliedGain describe the last nonempty successful block before dry/wet mixing; read them on the processing owner or publish a snapshot.

Read latencyFrames after selecting look-ahead. Limiters with ordinary sample detection and the oversampled TruePeakLimiter have different reconstruction contracts; use the [advanced guide](doc:advanced-dynamics).
''',r'''
## Modes et détecteur {#detector}

Dynamics propose compressor, expander, noiseGate et limiter. Préparez d’abord le mode et l’anticipation maximale ; réglez threshold/knee/makeup en dB, ratio, attaque/release/maintien en ms, hystérésis du gate et liaison des canaux via DynamicsParameters.

Les détecteurs peak et RMS ont des comportements d’enveloppe différents. BallisticsFilter expose aussi la détection indépendamment. Un détecteur lié partage les décisions de gain entre canaux ; il ne somme pas les sorties en mono.

## Sidechain externe et mixage {#sidechain}

La surcharge process à trois blocs accepte programme, sidechain et sortie. Un sidechain mono est diffusé aux canaux du programme ; sinon les nombres de canaux correspondent. Sidechain/sortie doivent être disjoints sauf si le sidechain est le bloc de programme lui-même. Alignez le détecteur sur l’événement visé dans le programme.

processMixed accepte des gains dry/wet linéaires finis de module au plus 16. Sa voie sèche suit le retard d’anticipation configuré. gainReductionDb et minimumAppliedGain décrivent le dernier bloc réussi non vide avant mixage ; lisez-les depuis le propriétaire du traitement ou publiez un instantané.

Consultez latencyFrames après choix de l’anticipation. Limiteur à détection d’échantillons et TruePeakLimiter suréchantillonné ont des contrats de reconstruction différents ; consultez le [guide avancé](doc:advanced-dynamics).
''')

add('advanced-dynamics','Dynamic EQ, transient and nonlinear effects','EQ dynamique, transitoires et effets non linéaires','Choose the transfer, antialiasing and detector contract.','Choisir le transfert, l’antialiasing et le contrat du détecteur.','dynamics',r'''
## Saturation and antialiasing {#saturation}

AdaaSaturator offers tanh, atan, cubic, hardClip, wavefolder and hysteresis. Drive, bias and outputGain are explicit. With antialias enabled, memoryless shapes use first-order antiderivative antialiasing with a midpoint limit for nearly equal samples. This reduces aliasing but does not eliminate it, and it has frequency-dependent smoothing.

Hysteresis is a bounded play operator followed by tanh, not a physical magnetization solver. MathAccuracy fast and antialias are independent. processMixed fuses an optional double input stage, shaping and linear mixing, while rounding wet audio to T before mixing. Preserve this staging when comparing separate chains.

## Dynamic equalizer and transients {#detectors}

DynamicEqualizer uses unity-peak bandpass corrections cascaded per band, with independent detectors driven by the original sidechain. It supports up to 16 bands; rangeDb may cut or boost. This is not interchangeable with every dynamic bell topology. DeEsser reuses one such band. MultibandDynamics uses complementary FIR splitting, up to 16 bands, with the FIR latency and no compressor look-ahead.

TransientShaper compares fast/slow envelopes and applies attack/sustain controls with a maximum gain. processGain emits the corresponding linear gains and advances the same envelopes as process; do not call both for the same input event unless two advancements are intended. Its fast policy approximates the gain exponential; [quality selection](doc:quality#accuracy) remains local.

## True-peak limiting {#limiting}

TruePeakLimiter uses 4x/8x oversampling, linked look-ahead and FIR return filtering. Margin defaults to 0.5 dB. Latency is fractional double. The ceiling depends on reconstruction/filter assumptions: assess intersample peaks on real content. It does not promise broadcast certification or a universal analogue peak bound.
''',r'''
## Saturation et antialiasing {#saturation}

AdaaSaturator fournit tanh, atan, cubic, hardClip, wavefolder et hysteresis. Drive, bias et outputGain sont explicites. Avec antialias actif, les formes sans mémoire utilisent l’antialiasing par primitive du premier ordre, avec limite au point milieu pour les échantillons proches. Il réduit le repliement sans l’éliminer et produit un lissage dépendant de la fréquence.

L’hystérésis est un opérateur de jeu borné suivi de tanh, pas un solveur physique d’aimantation. MathAccuracy fast et antialias sont indépendants. processMixed fusionne un étage d’entrée double optionnel, la forme et le mixage linéaire, tout en arrondissant le signal wet à T avant mélange. Conservez cet ordre pour comparer des chaînes séparées.

## Égaliseur dynamique et transitoires {#detectors}

DynamicEqualizer utilise des corrections passe-bande à crête unitaire, en cascade par bande, et des détecteurs indépendants alimentés par le sidechain original. Il accepte jusqu’à 16 bandes ; rangeDb peut atténuer ou amplifier. Il n’est pas interchangeable avec toute topologie d’égaliseur dynamique en cloche. DeEsser réutilise une bande. MultibandDynamics emploie une séparation FIR complémentaire, jusqu’à 16 bandes, avec la latence du FIR et sans anticipation du compresseur.

TransientShaper compare des enveloppes rapides/lentes et applique les contrôles attaque/sustain avec un gain maximal. processGain émet les gains linéaires correspondants et avance les mêmes enveloppes que process ; n’appelez pas les deux pour le même événement si un double avancement n’est pas voulu. Le mode fast approxime l’exponentielle du gain ; le [choix de précision](doc:quality#accuracy) reste local.

## Limitation true peak {#limiting}

TruePeakLimiter utilise un suréchantillonnage 4x/8x, une anticipation liée et un filtrage FIR de retour. La marge par défaut est 0,5 dB. La latence est un double fractionnaire. Le plafond dépend du modèle de reconstruction/filtrage : évaluez les crêtes interéchantillons sur les contenus réels. Il ne promet ni certification broadcast ni borne universelle de crête analogique.
''')

add('delay','Delay, history and interpolation','Délai, historique et interpolation','Read fractional history and choose delay transitions.','Lire l’historique fractionnaire et choisir les transitions de délai.','overview',r'''
## Shared history {#history}

CircularHistory writes each source sample once and supports independent reads. Age zero means the latest written sample; unavailable past/future gives zero. Integer windows are gathered into double. Available interpolation choices include nearest, linear, cubic Lagrange and Catmull–Rom; sinc readers provide a prepared filtered alternative.

Interpolation quality and the allowed read range are separate. Keep positions inside prepared history and preserve causal ordering. ThiranDelay is a sequential first-order allpass for a fractional delay in (0,1], not random-access history.

## Taps and modulation {#taps}

MultiTapDelay routes up to 4096 input/output taps within prepared channel/delay capacity. Delay is in frames. setTaps crossfades complete configurations; editing during transition returns transition_in_progress. Fading readers avoids the pitch glide of continuously moving a delay read head.

DelayEffect deliberately modulates read position for flanger, vibrato and wow/flutter; choose delay/depth/rate/mix rather than relying on a mode name as a hidden preset. See [effects](doc:effects). A changing physical propagation delay already produces Doppler: avoid applying the same pitch change twice.
''',r'''
## Historique partagé {#history}

CircularHistory écrit chaque échantillon source une fois et accepte des lectures indépendantes. L’âge zéro désigne le dernier échantillon écrit ; passé indisponible/futur donnent zéro. Les fenêtres entières sont rassemblées en double. Les choix comprennent nearest, linéaire, Lagrange cubique et Catmull–Rom ; les lecteurs sinc offrent une alternative filtrée préparée.

Qualité d’interpolation et plage de lecture autorisée sont distinctes. Gardez les positions dans l’historique préparé et respectez l’ordre causal. ThiranDelay est un passe-tout séquentiel du premier ordre pour un retard fractionnaire dans (0,1], pas un historique à accès aléatoire.

## Taps et modulation {#taps}

MultiTapDelay route jusqu’à 4096 taps entrée/sortie dans les capacités de canaux/retards préparées. Le retard est en frames. setTaps fond des configurations complètes ; une édition pendant la transition retourne transition_in_progress. Le fondu entre lecteurs évite le glissement de hauteur d’une tête déplacée en continu.

DelayEffect module volontairement la position pour flanger, vibrato et wow/flutter ; réglez délai/profondeur/vitesse/mix sans considérer le mode comme un preset caché. Consultez les [effets](doc:effects). Un retard de propagation physique variable produit déjà du Doppler : évitez d’appliquer deux fois la même variation de hauteur.
''')

add('effects','Delay and modulation effects','Effets de délai et modulation','Configure chorus, phaser, echoes, flanger and amplitude modulation.','Configurer chorus, phaser, échos, flanger et modulation d’amplitude.','delay',r'''
## Select behavior and capacity {#configure}

Chorus and Phaser expose dedicated parameter sets. DelayEffect combines a circular history, up to 16 feed-forward taps and a filtered/saturated feedback read. Prepare maximumDelayMs first; all tap delays and modulation excursions must fit the prepared capacity.

Its echo, flanger, vibrato and wowFlutter modes interpret delayMs, depthMs, rateHz, feedback, lowPassHz, feedbackDrive, duckDb, wet and dry. Mode changes do not replace these values with hidden presets. Ping-pong routing is explicit. Feedback filtering/saturation changes both decay and tone; leave headroom and keep feedback inside accepted bounds.

## Amplitude and tail handling {#amplitude}

AmplitudeModulator uses rate/depth and an autopan choice for tremolo/panning. Panning does not reproduce HRTF elevation cues. Reset restarts modulation phase according to the processor's contract.

Continue sending silence while echoes/reverb decay; stopping the process call freezes internal time. For sample-accurate parameter events, split blocks or use a dedicated supported modulation input. Keep continuous delay motion when pitch modulation is intended and reader crossfades when it is not.
''',r'''
## Choisir le comportement et la capacité {#configure}

Chorus et Phaser exposent leurs jeux de paramètres. DelayEffect combine un historique circulaire, jusqu’à 16 taps directs et une lecture de feedback filtrée/saturée. Préparez d’abord maximumDelayMs ; tous les retards et excursions de modulation doivent tenir dans cette capacité.

Ses modes echo, flanger, vibrato et wowFlutter interprètent delayMs, depthMs, rateHz, feedback, lowPassHz, feedbackDrive, duckDb, wet et dry. Un changement de mode ne remplace pas ces valeurs par des presets cachés. Le routage ping-pong est explicite. Filtrage/saturation du feedback modifient décroissance et timbre ; gardez de la réserve de niveau et un feedback dans les bornes acceptées.

## Amplitude et gestion des queues {#amplitude}

AmplitudeModulator utilise vitesse/profondeur et le choix autopan pour trémolo/panoramique. Le panoramique ne reproduit pas les indices d’élévation HRTF. Reset redémarre la phase selon le contrat du processeur.

Continuez à fournir du silence pendant la décroissance des échos/réverbérations ; cesser d’appeler process fige le temps interne. Pour des événements précis à l’échantillon, découpez les blocs ou utilisez une entrée de modulation autorisée. Gardez un mouvement continu du délai si la modulation de hauteur est voulue, et des fondus de lecteurs sinon.
''')

add('reverb','Algorithmic and hybrid reverbs','Réverbérations algorithmiques et hybrides','Choose Schroeder, FDN, plate, spring or IR/FDN processing.','Choisir Schroeder, FDN, plaque, ressort ou IR/FDN.','overview',r'''
## Choose a topology {#topology}

Reverb provides a Schroeder-style network. Diffuser combines allpass stages; orthogonalMix provides Hadamard/Householder mixing. FdnReverb uses 4/8/16/32 orthogonal feedback lines and 1–8 channels, with per-band nominal RT60, predelay, diffusion and damping.

PlateReverb is an original realization of a figure-eight plate topology, with input diffusion and cross-fed delay tanks. Its shimmer injects an octave-up dual-head signal into feedback. SpringReverb is a dispersive feedback waveguide, not a finite-element mechanical simulation.

## Decay, freeze and hybrid IR {#decay}

RT60 settings describe a nominal model. Conservative passive bounds can shorten extreme FDN band decays. Freeze disables injection as documented and may create an indefinitely retained tail. Provide zeros to keep time advancing; reset deliberately discards the tail.

HybridReverb combines an early prepared IR with an algorithmic late FDN. Stage a new impulse outside processing, then start its allocated transition between blocks. Account for both states during fades. RoomReverbBank retains several room responses and their tails; see [room acoustics](doc:acoustics).

ImpulseAnalysis can estimate decay and propose nominal FDN settings. This matches selected metrics, not every characteristic of a measured room.
''',r'''
## Choisir une topologie {#topology}

Reverb fournit un réseau de type Schroeder. Diffuser combine des étages passe-tout ; orthogonalMix propose Hadamard/Householder. FdnReverb utilise 4/8/16/32 lignes à rétroaction orthogonale et 1–8 canaux, avec RT60 nominales par bandes, prédélai, diffusion et amortissement.

PlateReverb est une réalisation originale d’une topologie de plaque en huit, avec diffusion d’entrée et réservoirs de retards croisés. Son shimmer injecte dans le feedback un signal à double tête transposé d’une octave. SpringReverb est un guide d’ondes dispersif à rétroaction, pas une simulation mécanique par éléments finis.

## Décroissance, freeze et IR hybride {#decay}

Les RT60 décrivent un modèle nominal. Les bornes passives conservatrices peuvent raccourcir les décroissances extrêmes des bandes FDN. Le freeze désactive l’injection selon le contrat et peut conserver une queue indéfiniment. Fournissez des zéros pour faire avancer le temps ; reset supprime volontairement la queue.

HybridReverb combine une IR précoce préparée avec une queue FDN. Préparez une nouvelle impulsion hors traitement, puis démarrez sa transition déjà réservée entre blocs. Budgétez les deux états pendant les fondus. RoomReverbBank conserve plusieurs réponses de pièces et leurs queues ; voir l’[acoustique des pièces](doc:acoustics).

ImpulseAnalysis estime les décroissances et peut proposer des réglages FDN nominaux. Il rapproche certaines métriques, pas toutes les caractéristiques d’une pièce mesurée.
''')

add('convolution','Convolution and impulse responses','Convolution et réponses impulsionnelles','Choose latency, routing and impulse transition behavior.','Choisir latence, routage et transitions d’impulsions.','overview',r'''
## Causal impulses and engines {#engines}

Tap h[0] multiplies the current input sample. PartitionedConvolver uses uniform FFT partitions and reports block latency. HybridConvolver combines an immediate direct head and FFT tail. NonuniformConvolver adds a larger late partition. ScheduledConvolver distributes late work across frames. The common Convolver chooses hybrid/nonuniform/scheduled at preparation and adds no block latency.

Latency zero means immediate application of the IR's first tap, not removal of silence/predelay within the IR. tailFrames defines how long zeros must still be processed after the source stops. reset clears that state.

## MIMO and updates {#routes}

MatrixConvolver routes `ConvolutionRoute<T>` input/output channel pairs; duplicate routes add. Impulses are copied during preparation, and input FFT/history is shared. Supply the true route matrix for true stereo rather than assuming two independent IRs implement crossfeed.

SmoothedConvolver separates non-realtime stageImpulse from allocation-free startTransition. Live-input prewarm precedes a fade by default. Both plans remain retained; updates during a fade can return transition_in_progress. reset snaps a running fade to its destination while retaining an unstarted stage. AsyncConvolver adds the explicit [producer/consumer contract](doc:threading#queues).

See [partition tuning](doc:convolution-options) for the spectral-head and late-work tradeoffs. Keep IR sample rate/channel mapping correct before preparation; loading/decoding files is an application task.
''',r'''
## Impulsions causales et moteurs {#engines}

Le tap h[0] multiplie l’échantillon d’entrée courant. PartitionedConvolver utilise des partitions FFT uniformes et annonce une latence de bloc. HybridConvolver combine une tête directe immédiate et une queue FFT. NonuniformConvolver ajoute une partition tardive plus grande. ScheduledConvolver répartit le travail tardif sur les frames. Convolver choisit hybrid/nonuniform/scheduled à la préparation, sans latence de bloc ajoutée.

Latence zéro signifie application immédiate du premier tap, pas suppression du silence/prédélai dans l’IR. tailFrames définit combien de zéros traiter après l’arrêt de la source. reset efface cet état.

## MIMO et mises à jour {#routes}

MatrixConvolver route les couples entrée/sortie `ConvolutionRoute<T>` ; les routes dupliquées s’ajoutent. Les impulsions sont copiées à la préparation et les FFT/historiques d’entrée sont partagés. Fournissez la véritable matrice pour du vrai stéréo : deux IR indépendantes n’implémentent pas nécessairement le croisement des canaux.

SmoothedConvolver sépare stageImpulse hors temps réel de startTransition sans allocation. Un préchauffage sur entrée vivante précède le fondu par défaut. Les deux plans sont conservés ; une mise à jour pendant le fondu peut retourner transition_in_progress. reset rejoint la destination d’un fondu en cours tout en conservant un stage non démarré. AsyncConvolver ajoute le [contrat producteur/consommateur](doc:threading#queues) explicite.

Consultez le [réglage des partitions](doc:convolution-options) pour les compromis de tête spectrale et travail tardif. Corrigez taux et routage des IR avant préparation ; chargement/décodage de fichiers appartiennent à l’application.
''')

add('convolution-options','Tune convolution partitions','Régler les partitions de convolution','Balance average CPU, boundary peaks and zero-latency heads.','Équilibrer CPU moyen, pics aux frontières et tête sans latence.','convolution',r'''
## Configuration {#options}

ConvolutionConfig.mode selects hybrid, nonuniform or scheduled. earlyPartition defaults to 128; latePartition defaults to 1024 and is unused by hybrid. Respect the power-of-two and relative-size constraints accepted during preparation.

`ConvolutionConfig::spectralHead` is used by MatrixConvolver only. If a call presents a complete early partition at an aligned boundary, the first earlyPartition taps use the early tier's frequency-domain transform instead of a direct FIR. A partial/unaligned piece still needs its causal direct computation. Latency stays zero; FFT rounding can depend on the host block pattern.

```cpp
#include <cydsp/CyDSP.h>
cydsp::Status prepareStereoIR(cydsp::MatrixConvolver<float>& effect,
    std::span<const float> leftIR, std::span<const float> rightIR) noexcept {
    const cydsp::ConvolutionRoute<float> routes[] = {{0, 0, leftIR}, {1, 1, rightIR}};
    cydsp::ConvolutionConfig config;
    config.mode = cydsp::ConvolutionMode::nonuniform;
    config.earlyPartition = 128;
    config.latePartition = 1024;
    config.spectralHead = true;
    return effect.prepare({48000.0, 512, 2}, 2, routes, config);
}
```

## Decide with the real callback pattern {#cost}

Nonuniform processing can reduce mean cost but concentrates late work at boundaries. Scheduled processing spreads late jobs while retaining head/early work. A 256-frame native probe and a graph processing 480 internal frames can favor different partitions. Test actual channel routes, IR lengths, block sizes and tails.

workUnitsPerPartition/maximumWorkUnitsPerFrame describe scheduled late chunks only. Different jobs have different CPU costs; these counts do not bound total callback time. Likewise an observed maximum includes OS scheduling and is not a hard realtime guarantee. Preserve sound/routing/latency requirements when choosing a faster configuration.
''',r'''
## Configuration {#options}

ConvolutionConfig.mode choisit hybrid, nonuniform ou scheduled. earlyPartition vaut 128 par défaut ; latePartition vaut 1024 et n’est pas utilisé par hybrid. Respectez les contraintes de puissance de deux et de tailles relatives acceptées à la préparation.

`ConvolutionConfig::spectralHead` sert uniquement à MatrixConvolver. Si un appel présente une partition précoce complète sur une frontière alignée, les premiers earlyPartition taps passent par la transformation fréquentielle du niveau précoce au lieu du FIR direct. Une portion partielle/non alignée exige encore son calcul causal direct. La latence reste zéro ; les arrondis FFT peuvent dépendre du découpage des blocs.

```cpp
#include <cydsp/CyDSP.h>
cydsp::Status prepareStereoIR(cydsp::MatrixConvolver<float>& effect,
    std::span<const float> leftIR, std::span<const float> rightIR) noexcept {
    const cydsp::ConvolutionRoute<float> routes[] = {{0, 0, leftIR}, {1, 1, rightIR}};
    cydsp::ConvolutionConfig config;
    config.mode = cydsp::ConvolutionMode::nonuniform;
    config.earlyPartition = 128;
    config.latePartition = 1024;
    config.spectralHead = true;
    return effect.prepare({48000.0, 512, 2}, 2, routes, config);
}
```

## Décider avec les vrais blocs {#cost}

Le nonuniform peut réduire le coût moyen mais concentre le travail tardif aux frontières. Scheduled répartit les jobs tardifs tout en gardant le coût de tête/niveau précoce. Un test natif de 256 frames et un graphe traitant 480 frames internes peuvent favoriser des partitions différentes. Mesurez routes, tailles d’IR, blocs et queues réels.

workUnitsPerPartition/maximumWorkUnitsPerFrame décrivent uniquement les chunks tardifs planifiés. Leurs coûts CPU diffèrent ; ces comptes ne bornent pas le callback complet. Un maximum observé inclut aussi l’ordonnancement de l’OS et n’est pas une garantie temps réel absolue. Conservez les exigences de son, routage et latence en choisissant une configuration plus rapide.
''')

add('fft','FFT, windows and normalization','FFT, fenêtres et normalisation','Use complex/real transforms without confusing amplitude and PSD.','Utiliser les transformations complexes/réelles sans confondre amplitude et PSD.','overview',r'''
## Transform convention {#transform}

The forward transform uses `exp(-i*2*pi*k*n/N)` without scaling. The inverse uses the opposite sign and divides by N. RealFft exposes N/2+1 bins, from DC through Nyquist. Use supported power-of-two lengths and the prepared span dimensions. Input/output overlap follows each transform's API rather than ordinary audio-block rules.

ComplexFft has immutable prepared tables and can be shared by independent callers with distinct buffers if lifecycle edits are serialized. RealFft owns mutable scratch and needs separate instances for simultaneous calls.

## Windows and levels {#windows}

makeWindow exposes window type, periodic/symmetric choice and normalization. A window suitable for spectral display is not automatically suitable for overlap-add reconstruction. Preserve FFT scaling when interpreting bins or multiplying spectra.

A raw magnitude is not dBFS or PSD. SpectrumAnalyzer normalizes amplitude by window sum and PSD by sample rate/window energy; see [analysis](doc:analysis). STFT handles weighted overlap-add and its explicit latency in the [spectral guide](doc:spectral).
''',r'''
## Convention de transformation {#transform}

La transformation directe utilise `exp(-i*2*pi*k*n/N)` sans normalisation. L’inverse emploie le signe opposé et divise par N. RealFft expose N/2+1 bins, de DC à Nyquist. Utilisez des longueurs supportées en puissance de deux et les dimensions de spans préparées. Le recouvrement suit l’API propre à la transformation, pas les règles ordinaires des blocs audio.

ComplexFft possède des tables préparées immuables et peut être partagé entre appels indépendants avec buffers distincts si le cycle de vie est sérialisé. RealFft possède un scratch mutable et exige des instances séparées pour les appels simultanés.

## Fenêtres et niveaux {#windows}

makeWindow expose type de fenêtre, choix périodique/symétrique et normalisation. Une fenêtre adaptée à l’affichage spectral ne convient pas automatiquement à la reconstruction par recouvrement-addition. Respectez la normalisation FFT pour interpréter les bins ou multiplier les spectres.

Une amplitude brute n’est ni un niveau dBFS ni une PSD. SpectrumAnalyzer normalise l’amplitude par la somme de fenêtre et la PSD par fréquence/énergie de fenêtre ; voir l’[analyse](doc:analysis). STFT gère le recouvrement-addition pondéré et sa latence explicite dans le [guide spectral](doc:spectral).
''')

add('spectral','STFT and spectral effects','STFT et effets spectraux','Process spectral callbacks, freeze, vocoding and separation.','Traiter callbacks spectraux, freeze, vocodage et séparation.','fft',r'''
## STFT reconstruction {#stft}

Stft uses causal weighted overlap-add, zero-padded startup and N-frame latency. FFT length is a power of two in [16,65536]; hop divides N and the window must support reconstruction. A null callback reconstructs delayed input within FFT rounding.

A spectral callback receives one channel's bins and must be noexcept, allocation-free, owner-serialized and leave them finite. DC/Nyquist imaginary parts are discarded. `flush` feeds 2N zeros then reports end_of_stream; reset before a new input stream. Context ownership remains with the application.

## Effects and complementary outputs {#effects}

SpectralEffects provides freeze, vocoder, morphing and frequency-bin filtering. Inputs and sidechains must satisfy the selected mode's shapes; stored spectral state needs warmup. FrequencyShifter uses a matched-delay Hilbert quadrature path: frequency shift adds Hz rather than multiplying musical pitch.

HarmonicPercussive separates time/frequency median masks and reconstructs harmonic/percussive outputs with a complementary sum. Median history adds latency and startup dependence. Choose window/hop/median sizes for the content; separation is a signal heuristic rather than source recognition. SpectralPitchShifter handles phase propagation and pitch/formants separately in [pitch/time](doc:pitch).
''',r'''
## Reconstruction STFT {#stft}

Stft utilise un recouvrement-addition pondéré causal, un démarrage rembourré de zéros et une latence de N frames. La FFT est une puissance de deux dans [16,65536] ; le hop divise N et la fenêtre doit permettre la reconstruction. Sans callback, l’entrée retardée est reconstruite aux arrondis FFT près.

Le callback spectral reçoit les bins d’un canal ; il est noexcept, sans allocation, sérialisé par le propriétaire et doit laisser les bins finis. Les parties imaginaires de DC/Nyquist sont retirées. `flush` fournit 2N zéros puis signale end_of_stream ; reset précède un nouveau flux. L’application possède toujours le contexte.

## Effets et sorties complémentaires {#effects}

SpectralEffects fournit freeze, vocodeur, morphing et filtrage de bins. Entrées et sidechains respectent les formes du mode choisi ; l’état spectral conservé exige un préchauffage. FrequencyShifter utilise Hilbert en quadrature à retard commun : il ajoute des Hz plutôt que de multiplier la hauteur musicale.

HarmonicPercussive sépare par masques de médianes temps/fréquence et reconstruit des sorties harmonique/percussive dont la somme est complémentaire. L’historique des médianes ajoute latence et dépendance au démarrage. Choisissez fenêtre/hop/médianes selon le contenu ; c’est une heuristique de signal, pas une reconnaissance des sources. SpectralPitchShifter traite propagation de phase et hauteur/formants dans le guide [hauteur/durée](doc:pitch).
''')

add('resampling','Resampling and oversampling','Rééchantillonnage et suréchantillonnage','Manage variable consumption, ratios, FIR tails and multirate banks.','Gérer consommation variable, ratios, queues FIR et bancs multirates.','overview',r'''
## Consumption is not block length {#counts}

Resampler uses a prepared Blackman-windowed sinc phase table. ResampleSpec declares rational input/output rates, channels and maximum input/output capacities; ResampleResult returns status, inputConsumed and outputProduced. Keep the unconsumed suffix and use only the valid output prefix. Successful processing does not mean every input frame was consumed.

The application owns a FIFO and may call with empty input to drain available output. `finish` supplies bounded zeros for the FIR tail and reports zero caller input consumed; after completion reset before a new stream. `setRates` is non-realtime and retains history and the scheduled next output.

## Variable ratio and quality {#ratio}

To use allocation-free `setRatio`, prepare minimumRatio/maximumRatio enclosing the nominal inputRate/outputRate. Ratios mean input frames per output frame; ramps count produced output frames. Filter cutoff stays safe for the complete prepared envelope, which can cost quality at gentler ratios.

resampleSpecForQuality selects taps/phases and increases support for downsampling within the accepted cap. Oversampler provides FIR, half-band FIR or IIR up/down paths. Match input/output block capacities and account for latency; IIR phase cannot always be represented by one exact integer delay. FirRateConverter and DyadicFilterBank cover specialized integer/dyadic multirate layouts. No engine silently owns the surrounding stream queue.
''',r'''
## La consommation n’est pas la taille du bloc {#counts}

Resampler utilise une table de phases sinc à fenêtre Blackman préparée. ResampleSpec déclare taux rationnels d’entrée/sortie, canaux et capacités maximales ; ResampleResult retourne status, inputConsumed et outputProduced. Gardez le suffixe non consommé et utilisez seulement le préfixe de sortie valide. Un succès ne signifie pas que toute l’entrée est consommée.

L’application possède la FIFO et peut appeler avec une entrée vide pour vider une sortie disponible. `finish` fournit les zéros bornés de la queue FIR et annonce zéro entrée de l’appelant consommée ; après la fin, reset précède un nouveau flux. `setRates` est hors temps réel et conserve historique et prochaine sortie planifiée.

## Ratio variable et qualité {#ratio}

Pour utiliser `setRatio` sans allocation, préparez minimumRatio/maximumRatio autour du ratio nominal inputRate/outputRate. Les ratios comptent les frames d’entrée par frame de sortie ; les rampes comptent les frames produites. La coupure reste sûre sur toute l’enveloppe préparée, ce qui peut coûter de la qualité aux ratios plus modérés.

resampleSpecForQuality choisit taps/phases et augmente le support en réduction de fréquence dans la limite acceptée. Oversampler fournit des voies FIR, FIR demi-bande ou IIR. Faites correspondre les capacités de blocs et tenez compte de la latence ; la phase IIR n’est pas toujours représentable par un retard entier exact. FirRateConverter et DyadicFilterBank couvrent les dispositions entières/dyadiques spécialisées. Aucun moteur ne possède implicitement la file de flux qui l’entoure.
''')

add('synthesis','Oscillators and envelopes','Oscillateurs et enveloppes','Generate waveforms, FM/PM, wavetable morphs and control envelopes.','Générer formes, FM/PM, morphing de tables et enveloppes.','overview',r'''
## Oscillators and band limits {#oscillators}

Oscillator generates sine, triangle, saw, square and white noise with prepared sample rate/channel count. `next` advances one sample and `process` renders a block; do not call both for the same interval unintentionally. Wavetable prepares harmonic-band versions and WavetableOscillator selects a suitable bank once per block at its fixed frequency.

ModulatedOscillator supports audio-rate FM/PM, hard sync, table morphing and unison. These modes have their prepared oversampling/bandlimit contract; increasing modulation, sync discontinuities or pitch can still create aliasing. Do not assume fixed-frequency table selection alone solves every modulated spectrum.

## Envelopes and LFOs {#envelopes}

Adsr parameters use attack/decay/release seconds and linear sustain. noteOn/noteOff change stages. `process(span)` writes the same sequence as successive next calls. The application owns exact completion-event timing; query active/value on the processing owner or split at event boundaries.

MultiSegmentEnvelope supplies a prepared segment sequence. ModulationLfo includes sample-and-hold and seeded behavior; ProceduralSource generates bounded configured textures. Resets use per-instance phase/random state rather than global random generators. The host owns MIDI allocation, note scheduling, gain staging and the final output limiter.
''',r'''
## Oscillateurs et limites de bande {#oscillators}

Oscillator génère sinus, triangle, dent de scie, carré et bruit blanc avec fréquence d’échantillonnage/canaux préparés. `next` avance d’un échantillon et `process` rend un bloc ; n’appelez pas involontairement les deux sur le même intervalle. Wavetable prépare les versions par bandes harmoniques et WavetableOscillator choisit la banque adaptée une fois par bloc à fréquence fixe.

ModulatedOscillator propose FM/PM audio-rate, synchronisation, morphing de tables et unisson. Ces modes suivent leurs contrats de suréchantillonnage/limitation de bande préparés ; forte modulation, discontinuités de sync ou hauteurs élevées peuvent encore replier le spectre. Le choix d’une table à fréquence fixe ne résout pas tous les spectres modulés.

## Enveloppes et LFO {#envelopes}

Les paramètres Adsr utilisent des secondes pour attaque/decay/release et une amplitude linéaire pour sustain. noteOn/noteOff changent d’étage. `process(span)` écrit la même séquence que des appels next successifs. L’application possède la précision des événements de fin ; consultez active/value depuis le propriétaire ou découpez aux événements.

MultiSegmentEnvelope fournit une séquence préparée. ModulationLfo comprend sample-and-hold et comportement à graine ; ProceduralSource génère les textures bornées configurées. Reset utilise phase/aléatoire par instance, sans générateur global. L’hôte gère allocation MIDI, notes, niveaux et limiteur final.
''')

add('sampling','Prepared samples and loops','Samples préparés et boucles','Render sample blocks with interpolation and harmonic mips.','Lire des blocs avec interpolation et mips harmoniques.','synthesis',r'''
## Prepare and own sample data {#prepare}

SampleData::prepare copies planar samples, sample rate and root note and builds prepared antialiasing mips. Decode files in the application before calling it. Objects borrowing SampleData, such as Granulator and SourceBatch, need stable source addresses and immutable contents during use.

`value(channel, position, ratio, interpolation)` reads a fractional sample position. SampleInterpolation offers linear and bandLimited. Higher-speed playback can use a lower-bandwidth mip; this reduces aliasing but does not promise perfect reconstruction at every ratio.

## Loops and sequential rendering {#loops}

setLoop uses `[first,end)` with an optional crossfade length; 0/0 disables looping. Prepare loops while readers are stopped and serialize all access. A loop crossfade changes the effective wrapped start to loopStart+loopCrossfade as documented.

`render(position, ratio, mode, output, inside)` selects its mip once per block, writes every channel and advances caller-owned position. Extra output channels repeat the last source channel. Reads beyond the end become zero and still advance. inside counts the leading frames read before the end. Invalid requests retain position/output. Handle end-of-sample and instrument voice scheduling in the application.
''',r'''
## Préparer et posséder les samples {#prepare}

SampleData::prepare copie les échantillons planaires, leur taux et la note racine, puis prépare les mips antirepliement. Décodez les fichiers dans l’application avant l’appel. Les objets empruntant SampleData, comme Granulator et SourceBatch, exigent des adresses stables et un contenu immuable pendant l’utilisation.

`value(channel, position, ratio, interpolation)` lit une position fractionnaire d’échantillon. SampleInterpolation propose linear et bandLimited. Une lecture accélérée peut utiliser un mip de bande réduite ; cela réduit le repliement sans promettre la reconstruction parfaite à tout ratio.

## Boucles et lecture séquentielle {#loops}

setLoop utilise `[first,end)` avec fondu optionnel ; 0/0 désactive la boucle. Préparez les boucles pendant l’arrêt des lecteurs et sérialisez les accès. Le fondu change le début effectif après bouclage en loopStart+loopCrossfade selon le contrat.

`render(position, ratio, mode, output, inside)` choisit le mip une fois par bloc, écrit tous les canaux et avance la position possédée par l’appelant. Les sorties supplémentaires répètent le dernier canal source. Les lectures au-delà de la fin deviennent nulles et avancent encore. inside compte le préfixe lu avant la fin. Les requêtes invalides préservent position/sortie. L’application gère fin de sample et ordonnancement des voix d’instrument.
''')

add('granular','Sample and live granulation','Granulation de samples et du direct','Schedule bounded grains and reproducible clouds.','Planifier des grains bornés et des nuages reproductibles.','synthesis',r'''
## Sample grains {#sample}

Granulator prepares mono/stereo output from a prepared mono/stereo SampleData source. maximumGrains is 1–4096, default 128. The source is borrowed: keep the object and sample data alive, stable and unchanged until processing stops and the granulator is reset/reprepared.

GrainParameters specify positionSeconds, durationMs, signed pitchRatio, gain, pan, envelope, interpolation and wrap. Negative ratio reads backwards. Source/output sample-rate conversion is applied separately. Hann, triangular and rectangular envelopes have different click/spectrum behavior. Grain contributions sum without automatic limiting.

trigger accepts an offset in output frames below maxBlockFrames. Shorter later calls carry remaining delay forward. A full pool returns update_queue_full rather than stealing voices. activeGrains includes pending grains; droppedGrains counts rejected automatic cloud events.

## Clouds and live history {#live}

setCloud selects density and symmetric position/pitch/pan spreads; density zero stops new automatic events without stopping active grains. reset(seed) restarts reproducible per-instance scheduling. process(add=false) clears output; add=true accumulates into existing audio.

LiveGranulator uses prepared circular input history instead of a decoded sample. Configure history/grain capacities before processing. Freeze retains the capture history; each grain has its own signed speed and channel gains. Keep grain reads inside available history and manage wet/dry levels explicitly. A frozen history is not an infinite new recording buffer.
''',r'''
## Grains sur samples {#sample}

Granulator prépare une sortie mono/stéréo depuis une source SampleData mono/stéréo préparée. maximumGrains vaut 1–4096, 128 par défaut. La source est empruntée : conservez objet et données vivants, stables et inchangés jusqu’à l’arrêt du traitement et au reset/nouvelle préparation du granulateur.

GrainParameters précise positionSeconds, durationMs, pitchRatio signé, gain, pan, enveloppe, interpolation et wrap. Un ratio négatif lit à l’envers. La conversion des taux source/sortie s’applique séparément. Les enveloppes Hann, triangulaire et rectangulaire ont des comportements de clic/spectre différents. Les grains s’ajoutent sans limitation automatique.

trigger accepte un offset en frames de sortie inférieur à maxBlockFrames. Des appels suivants plus courts reportent le délai restant. Une réserve pleine retourne update_queue_full au lieu de voler des voix. activeGrains inclut les grains en attente ; droppedGrains compte les événements automatiques refusés.

## Nuages et historique live {#live}

setCloud choisit densité et dispersions symétriques de position/hauteur/pan ; une densité nulle arrête les nouveaux événements sans arrêter les grains actifs. reset(seed) redémarre une planification reproductible par instance. process(add=false) efface la sortie ; add=true s’ajoute à l’audio présent.

LiveGranulator utilise un historique circulaire d’entrée préparé au lieu d’un sample décodé. Configurez capacités d’historique/grains avant traitement. Freeze conserve l’historique capturé ; chaque grain possède vitesse signée et gains de canaux. Gardez les lectures dans l’historique disponible et réglez explicitement dry/wet. Un historique figé n’est pas un nouveau buffer d’enregistrement infini.
''')

add('sources','Many sources and shared assets','Sources nombreuses et données partagées','Render fixed source slots with shared sample/wavetable banks.','Rendre des emplacements fixes avec banques de samples/tables partagées.','synthesis',r'''
## Capacity and lifetimes {#capacity}

SourceBatch prepares up to 1024 fixed source slots, 64 output channels, 256 sample banks and 256 wavetable banks, with an internal rendered-voice storage cap of 2^24 values. Preparation copies bank pointers, not their data. Keep bank objects and contents stable and immutable until every reader stops.

Each BatchSource chooses sine, wavetable or grain, plus phase/frequency/gain, delayed start and output gains. An occupied slot is not stolen; start returns would_block. The host chooses voice stealing/retry policy and owns any scene/entity mapping.

## Rendering and positioning {#render}

Sine voices use complex recurrences vectorized across voices and periodic renormalization. Wavetables use harmonic mips; grains use SampleData interpolation/loops and a Hann envelope. Lookup/read positions remain partly scalar. Summation requires headroom.

setPosition with order -1 supplies constant-power stereo pan and 1/max(1,distance) attenuation. Orders 0–7 require exactly `(order+1)^2` ACN/SN3D outputs. Stereo panning alone does not supply HRTF elevation cues. Use the [spatial coordinate convention](doc:spatial#coordinates) and [binaural/HOA rendering](doc:binaural) for those tasks.
''',r'''
## Capacités et durées de vie {#capacity}

SourceBatch prépare jusqu’à 1024 emplacements fixes, 64 canaux de sortie, 256 banques de samples et 256 banques de tables, avec une limite interne de 2^24 valeurs rendues de voix. La préparation copie les pointeurs, pas les données. Gardez objets et contenus stables et immuables jusqu’à l’arrêt de tous les lecteurs.

Chaque BatchSource choisit sinus, table ou grain, puis phase/fréquence/gain, départ différé et gains de sortie. Un emplacement occupé n’est pas volé ; start retourne would_block. L’hôte choisit la politique de vol/réessai et possède la correspondance avec les entités de scène.

## Rendu et placement {#render}

Les sinus utilisent des récurrences complexes vectorisées entre voix et une renormalisation périodique. Les tables emploient les mips harmoniques ; les grains réutilisent interpolation/boucles de SampleData avec une enveloppe Hann. Recherche et positions de lecture restent partiellement scalaires. La somme exige de la réserve de niveau.

setPosition à l’ordre -1 fournit un pan stéréo à puissance constante et une atténuation 1/max(1,distance). Les ordres 0–7 exigent exactement `(order+1)^2` sorties ACN/SN3D. Le pan stéréo seul ne donne pas les indices d’élévation HRTF. Utilisez la [convention spatiale](doc:spatial#coordinates) et le [rendu binaural/HOA](doc:binaural) pour ces usages.
''')

add('physical','Physical and contact synthesis','Synthèse physique et contacts','Excite modal banks, strings and waveguides with explicit units.','Exciter banques modales, cordes et guides avec unités explicites.','synthesis',r'''
## Resonators and guides {#models}

ModalResonator accepts frequency/decay/gain modes and offers membrane/plate mode arrangements. Waveguide provides string/tube and Karplus–Strong-style models from prepared delay/filter storage. Choose a sample rate and mode/frequency range that remain representable; these are reduced models, not general mechanical simulations.

The host supplies excitation, feedback coupling and physical parameter mapping. A stable component does not prove a stable coupled instrument. Limit excitation energy and assess resonances, long tails and sample-rate changes.

## Contact excitation {#contact}

ContactExciter creates mono impact/sliding/rolling/bowed excitation for a resonator. Units are explicit: normal force N, velocity m/s, mass kg, stiffness N/m, impulse N·s and spacing metres. Up to eight impacts overlap; overflow is reported.

Impact half-sines preserve the requested discrete impulse before the 20 Hz DC blocker. Sliding uses bounded bristle state; bowing adds Stribeck friction and can receive resonator surface velocity. Rolling triggers micro-contacts over asperities. reset(seed) makes textures reproducible. Output gain converts physical magnitudes into audio level. No collision engine or rigid-body solver is created.
''',r'''
## Résonateurs et guides {#models}

ModalResonator accepte des modes fréquence/décroissance/gain et propose des dispositions de membrane/plaque. Waveguide fournit des modèles de corde/tube et de type Karplus–Strong sur des retards/filtres préparés. Choisissez taux et plage de modes/fréquences représentables ; ce sont des modèles réduits, pas des simulations mécaniques générales.

L’hôte fournit excitation, couplage de rétroaction et conversion des paramètres physiques. Un composant stable ne prouve pas la stabilité de l’instrument couplé. Bornez l’énergie d’excitation et évaluez résonances, longues queues et changements de fréquence d’échantillonnage.

## Excitation de contact {#contact}

ContactExciter produit une excitation mono d’impact/glissement/roulement/archet pour un résonateur. Les unités sont explicites : force normale N, vitesse m/s, masse kg, raideur N/m, impulsion N·s et espacement en mètres. Au plus huit impacts se superposent ; le débordement est signalé.

Les demi-sinus d’impact conservent l’impulsion discrète demandée avant le bloqueur DC à 20 Hz. Le glissement utilise un état de bristle borné ; l’archet ajoute une friction de Stribeck et peut recevoir la vitesse de surface du résonateur. Le roulement déclenche des micro-contacts sur les aspérités. reset(seed) rend les textures reproductibles. Le gain convertit les grandeurs physiques en niveau audio. Aucun moteur de collisions ni solveur de solides n’est créé.
''')

add('pitch','Pitch, duration and formants','Hauteur, durée et formants','Choose delay-head, spectral or optional stretch processing.','Choisir le traitement à têtes de délai, spectral ou Stretch optionnel.','overview',r'''
## Select an engine {#engines}

PitchEffects provides target-pitch correction, intervals and multivoice harmony with gains, pans and creative delays. Its ordinary prepare uses a two-head sinc delay approach. prepareSpectral connects a phase-vocoder engine with a declared N-frame latency, up to eight voices and mono/stereo operation. Account for creative per-voice delays separately.

SpectralPitchShifter uses power-of-two N=64–16384 with hop N/4. It measures instantaneous frequencies, links a guide channel and preserves relative channel phase. Significant spectral peaks guide local phase locking; a smoothed envelope controls formant compensation. Ratio 1 gives delayed reconstruction within rounding.

## Source assumptions {#quality}

Envelope smoothing width defaults to 150 Hz. Too wide loses narrow formants; too narrow follows harmonics. Positive spectral flux triggers transient phase reset heuristically. Large intervals, polyphony and poorly resolved low formants remain difficult. Monophonic correction uses YIN confidence; a pitch detector is not polyphonic note transcription.

TimeStretch is the optional CyDSP::Stretch target, exposing time/pitch/formant controls. Its stream counters and buffer requirements differ from fixed-rate processing; follow its exact API. Prepare voices/assets outside the callback, retain borrowed contexts and drain tails. Inactive spectral harmony voices are not automatically warmed up.
''',r'''
## Choisir un moteur {#engines}

PitchEffects fournit correction vers une hauteur cible, intervalles et harmonisation multivoix avec gains, pans et délais créatifs. Son prepare ordinaire utilise deux têtes de délai sinc. prepareSpectral raccorde un moteur à vocodeur de phase annonçant N frames de latence, jusqu’à huit voix en mono/stéréo. Comptez séparément les délais créatifs de chaque voix.

SpectralPitchShifter utilise N puissance de deux de 64–16384 et un hop N/4. Il estime les fréquences instantanées, lie un canal guide et conserve les phases relatives entre canaux. Les pics significatifs guident le verrouillage local de phase ; une enveloppe lissée contrôle la compensation de formants. Le ratio 1 donne une reconstruction retardée aux arrondis près.

## Hypothèses sur les sources {#quality}

La largeur de lissage d’enveloppe vaut 150 Hz par défaut. Trop large, elle perd les formants étroits ; trop faible, elle suit les harmoniques. Le flux spectral positif déclenche heuristiquement une remise en phase aux attaques. Grands intervalles, polyphonie et formants graves mal résolus restent difficiles. La correction monophonique utilise la confiance YIN ; détecter une hauteur ne transcrit pas les notes polyphoniques.

TimeStretch appartient à la cible optionnelle CyDSP::Stretch et expose durée/hauteur/formants. Ses comptes de flux et besoins en buffers diffèrent du taux fixe ; respectez son API exacte. Préparez voix/données hors callback, conservez les contextes empruntés et videz les queues. Les voix d’harmonie spectrale inactives ne sont pas préchauffées automatiquement.
''')

add('metering','Levels, true peak and loudness','Niveaux, true peak et sonie','Interpret peak/RMS, intersample peaks and bounded LUFS history.','Interpréter peak/RMS, crêtes interéchantillons et historique LUFS borné.','overview',r'''
## Levels and correlation {#levels}

LevelMeter keeps a prepared exact sliding window: peak, RMS, mean/DC and peak held since reset. Startup uses samples actually received. dBFS is 20log10(amplitude); silence gives negative infinity. Crest factor is peak/RMS in dB, zero for silence. There is no display decay.

Stereo correlation uses the first two channels' normalized cross-product including DC, clamped to [-1,1]; missing/silent channels give zero. The host owns display averaging and snapshot publication.

## True peak {#true-peak}

TruePeakMeter uses 4x/8x windowed-sinc reconstruction and 16/32/64/128 taps per phase. Maxima hold until reset. Original sample peaks are immediate; reconstructed peaks have nominal half-tap latency. Call flush at stream end. This estimates intersample maxima within its filter/phase model; it is not a certified universal analogue bound.

## Loudness and session duration {#loudness}

LoudnessMeter applies K weighting, 400 ms momentary and 3 s short-term windows. Integrated loudness uses -70 LUFS absolute and -10 LU relative gates; LRA uses -70/-20 gates and the 10th–95th percentiles. Channels beyond mono/stereo require explicit weights; map LFE/surround roles yourself.

maximumGatingBlocks reserves exact history at preparation (default 288000, about eight hours at 100 ms steps). If a whole block would exceed capacity, process returns would_block before consuming it. It does not silently use a rolling approximation. Prepare enough history or stop/reset deliberately.

levels is constant cost. integratedLufs scans retained history and loudnessRangeLu also sorts retained scratch: query outside a critical callback, serialized with processing. Mathematical definitions are implemented; complete certification/conformance coverage is not claimed.
''',r'''
## Niveaux et corrélation {#levels}

LevelMeter conserve une fenêtre glissante exacte préparée : peak, RMS, moyenne/DC et crête retenue depuis reset. Le démarrage utilise les échantillons réellement reçus. Le dBFS vaut 20log10(amplitude) ; le silence donne moins l’infini. Le facteur de crête est peak/RMS en dB, zéro pour le silence. Aucune décroissance d’affichage n’est ajoutée.

La corrélation stéréo utilise le produit croisé normalisé des deux premiers canaux, DC compris, borné à [-1,1] ; canaux manquants/silencieux donnent zéro. L’hôte gère moyennage d’affichage et publication d’instantanés.

## True peak {#true-peak}

TruePeakMeter utilise une reconstruction sinc fenêtrée 4x/8x et 16/32/64/128 taps par phase. Les maxima restent jusqu’au reset. Les crêtes d’échantillons sont immédiates ; les crêtes reconstruites ont une latence nominale d’un demi-nombre de taps. Appelez flush en fin de flux. Il estime les maxima interéchantillons dans son modèle de filtre/phases ; ce n’est pas une borne analogique universelle certifiée.

## Sonie et durée de session {#loudness}

LoudnessMeter applique la pondération K, des fenêtres de 400 ms momentanées et 3 s à court terme. La sonie intégrée utilise les gates absolu -70 LUFS et relatif -10 LU ; la LRA utilise -70/-20 et les percentiles 10–95. Au-delà du mono/stéréo, fournissez des poids explicites ; faites vous-même la correspondance LFE/surround.

maximumGatingBlocks réserve l’historique exact à la préparation (288000 par défaut, environ huit heures par pas de 100 ms). Si un bloc complet dépasserait la capacité, process retourne would_block sans le consommer. Aucun historique glissant approximatif n’est substitué. Préparez une capacité suffisante ou arrêtez/réinitialisez volontairement.

levels a un coût constant. integratedLufs parcourt l’historique et loudnessRangeLu trie aussi le scratch conservé : interrogez-les hors callback critique, de façon sérialisée. Les définitions mathématiques sont implémentées ; aucune certification/conformité exhaustive n’est revendiquée.
''')

add('analysis','Spectral and musical analysis','Analyse spectrale et musicale','Interpret FFT/PSD bands, constant-Q, pitch and musical estimates.','Interpréter bandes FFT/PSD, constant-Q, hauteur et estimations musicales.','metering',r'''
## Spectrum and band units {#spectrum}

SpectrumAnalyzer prepares a periodic window and a double real FFT even for float input. N is a power of two 16–65536; hop is 1–N. The first result follows a full window, then each hop. Getters retain the latest completed frame; large calls may replace intermediate frames. Borrowed result spans change on analysis/reset and are invalidated by successful reprepare/destruction.

complexSpectrum contains raw windowed bins. Interior magnitudes are `2*abs(X)/sum(window)`; DC/Nyquist use factor 1. PSD is one-sided `abs(X)^2/(Fs*sum(window^2))`, doubled for interior bins. Sum PSD and multiply by Fs/N to obtain window-weighted mean square. Amplitude squared and mean PSD are different units.

SpectralBands builds normalized Mel/log triangular averages. Supplying PSD gives mean PSD, not integrated band energy. Unsupported empty bands fail preparation: increase FFT size or reduce bands. Features include centroid, rolloff, flatness, dominant bin and positive normalized flux. Dominant bin is not a refined fundamental estimate.

## Constant-Q and music {#music}

ConstantQAnalyzer uses direct variable-length Hann-windowed complex projections. Lower frequencies require longer kernels; budget total kernel length, channel count and hop. Results share an end timestamp but have different effective center delays. Defaults are 55 Hz, 12 bins/octave, 96 bins and 512-frame hop.

MusicalAnalyzer uses FFT-accelerated YIN for monophonic pitch/confidence, plus onset, tempo/beat, chroma, MFCC and key estimates. Choose N large enough for the minimum pitch; analysis starts after N samples. Key classes are 0=C through 11=B. Tempo, attacks and tonal key are heuristic and need confidence gating. A noexcept callback can receive completed features; it follows the same allocation-free owner contract as processing.
''',r'''
## Spectre et unités des bandes {#spectrum}

SpectrumAnalyzer prépare une fenêtre périodique et une FFT réelle double même pour une entrée float. N est une puissance de deux 16–65536 ; le hop vaut 1–N. Le premier résultat suit une fenêtre complète, puis chaque hop. Les getters gardent la dernière frame complète ; un grand appel peut remplacer les frames intermédiaires. Les spans empruntés changent à l’analyse/reset et sont invalidés par nouvelle préparation réussie/destruction.

complexSpectrum contient les bins fenêtrés bruts. Les amplitudes intérieures valent `2*abs(X)/sum(window)` ; DC/Nyquist utilisent le facteur 1. La PSD unilatérale vaut `abs(X)^2/(Fs*sum(window^2))`, doublée pour les bins intérieurs. Sommez la PSD puis multipliez par Fs/N pour obtenir la moyenne quadratique pondérée par fenêtre. Carré d’amplitude et PSD moyenne sont des unités différentes.

SpectralBands construit des moyennes triangulaires Mel/log normalisées. Fournir une PSD donne une PSD moyenne, pas l’énergie intégrée de bande. Les bandes vides non représentables échouent à la préparation : augmentez la FFT ou réduisez les bandes. Les caractéristiques comprennent centroïde, rolloff, flatness, bin dominant et flux normalisé positif. Le bin dominant n’est pas une estimation affinée de fondamentale.

## Constant-Q et musique {#music}

ConstantQAnalyzer utilise des projections complexes directes à fenêtres Hann de longueurs variables. Les graves exigent des noyaux plus longs ; budgétez longueur totale, canaux et hop. Les résultats partagent une date de fin mais des retards centraux différents. Les défauts sont 55 Hz, 12 bins/octave, 96 bins et un hop de 512 frames.

MusicalAnalyzer utilise YIN accéléré par FFT pour hauteur/confiance monophoniques, puis attaques, tempo/battements, chroma, MFCC et tonalité. Choisissez N assez grand pour la hauteur minimale ; l’analyse commence après N échantillons. Les classes tonales vont de 0=do à 11=si. Tempo, attaques et tonalité sont heuristiques et exigent un contrôle de confiance. Un callback noexcept reçoit les caractéristiques complètes ; il suit le même contrat de propriétaire et d’absence d’allocation que le traitement.
''')

add('diagnostics','Responses and measurement signals','Réponses et signaux de mesure','Measure phase, coherence, distortion, sweeps and IR decay.','Mesurer phase, cohérence, distorsion, sweeps et décroissance d’IR.','analysis',r'''
## Frequency and distortion measurements {#response}

firResponse/iirResponse evaluate complex response, phase and group delay from supplied filters. TransferAnalyzer accumulates reference/output spectra to estimate transfer and frequency-dependent coherence. Keep the reference aligned and choose window/averaging duration for the measurement; low coherence warns against treating a ratio as a reliable linear response.

MeasurementGenerator supplies configured deterministic measurement signals. measureDistortion estimates THD/THD+N from a periodic window and chosen fundamental. Use coherent frequencies or suitable windows and distinguish harmonic power from residual/noise; finite FFT resolution changes the result.

## Sweeps and impulses {#impulse}

inverseLogSweep/deconvolveSweep support swept-response reconstruction. Record enough pre/post time, align playback/capture and avoid clipping or time-varying processing if measuring a linear system. File/device I/O remains in the application.

analyzeImpulse reports onset, early reflections, energy, clarity and per-band decay/RT60 fits. Treat extrapolated decay fits according to their valid fit range and noise floor. matchFdnToImpulse proposes nominal FDN settings from those summaries, not a perfect room reconstruction. Measurement is preparation/analysis work; do not put a whole sweep deconvolution or large impulse analysis in a realtime callback.
''',r'''
## Réponse et distorsion {#response}

firResponse/iirResponse évaluent réponse complexe, phase et retard de groupe des filtres fournis. TransferAnalyzer accumule les spectres référence/sortie pour estimer transfert et cohérence selon la fréquence. Alignez la référence et choisissez fenêtre/durée de moyennage adaptées ; une faible cohérence avertit qu’un ratio n’est pas une réponse linéaire fiable.

MeasurementGenerator fournit les signaux déterministes configurés. measureDistortion estime THD/THD+N depuis une fenêtre périodique et une fondamentale choisie. Utilisez fréquences cohérentes ou fenêtres adaptées et distinguez puissance harmonique du résidu/bruit ; la résolution FFT finie change le résultat.

## Sweeps et impulsions {#impulse}

inverseLogSweep/deconvolveSweep servent à reconstruire une réponse par balayage. Enregistrez assez de temps avant/après, alignez lecture/capture et évitez écrêtage ou traitement variable pour mesurer un système linéaire. Les entrées/sorties fichiers/périphériques restent dans l’application.

analyzeImpulse rapporte onset, premières réflexions, énergie, clarté et ajustements de décroissance/RT60 par bandes. Interprétez l’extrapolation selon la plage d’ajustement valide et le bruit de fond. matchFdnToImpulse propose des réglages FDN nominaux depuis ces résumés, pas une reconstruction parfaite de pièce. Ces mesures sont des travaux de préparation/analyse ; ne placez pas une déconvolution de sweep complète ni une grande analyse d’IR dans le callback temps réel.
''')

add('restoration','Noise, hum and click restoration','Restauration du bruit, ronflement et clics','Use spectral attenuation, notches, repair and adaptive filters.','Utiliser atténuation spectrale, encoches, réparation et filtres adaptatifs.','overview',r'''
## Choose the disturbance model {#model}

SpectralDenoiser applies prepared spectral noise attenuation; Dehum removes configured hum/harmonic components; Declicker detects and repairs bounded impulsive disturbances. Supply parameters matching the disturbance and protect wanted transients. Strong attenuation or inaccurate noise assumptions can produce musical noise, dulling or false repairs.

AdaptiveFilter implements LMS/NLMS adaptation against a desired/reference signal. Choose step/leakage/normalization settings within accepted ranges, align both signals and monitor convergence. Long taps increase storage and per-sample work. A linear adaptive path cannot remove arbitrary nonlinear distortion.

## Processing and evaluation {#workflow}

Prepare before the callback, budget any analysis/reconstruction delay and drain spectral tails. Start with conservative thresholds, compare bypass at matched loudness, then assess desired transients, harmonics and silence. No automatic filesystem scan, recorded noise database or worker is implied. For loudspeaker leakage into microphones, use the explicit [echo-canceller reference contract](doc:echo).
''',r'''
## Choisir le modèle de perturbation {#model}

SpectralDenoiser applique une atténuation spectrale préparée du bruit ; Dehum retire les composantes de ronflement/harmoniques configurées ; Declicker détecte et répare des perturbations impulsionnelles bornées. Adaptez les paramètres à la perturbation et protégez les transitoires utiles. Une atténuation forte ou un mauvais modèle de bruit peuvent créer bruit musical, perte d’aigus ou fausses réparations.

AdaptiveFilter implémente l’adaptation LMS/NLMS contre un signal souhaité/de référence. Choisissez pas/fuite/normalisation dans les plages acceptées, alignez les signaux et surveillez la convergence. Beaucoup de taps augmentent stockage et travail par échantillon. Un chemin adaptatif linéaire ne retire pas une distorsion non linéaire quelconque.

## Traitement et évaluation {#workflow}

Préparez avant le callback, budgétez les retards d’analyse/reconstruction et videz les queues spectrales. Commencez avec des seuils conservateurs, comparez au bypass à sonie égale, puis évaluez transitoires utiles, harmoniques et silence. Aucun parcours de fichiers, base enregistrée de bruit ou worker n’est implicite. Pour la fuite des haut-parleurs vers les micros, utilisez le [contrat de référence de l’annulation d’écho](doc:echo).
''')

add('echo','Acoustic echo cancellation','Annulation d’écho acoustique','Align loudspeaker reference and microphone paths before NLMS adaptation.','Aligner référence haut-parleur et microphones avant adaptation NLMS.','restoration',r'''
## Reference and channels {#reference}

EchoCanceller receives a mono reference representing audio sent to loudspeakers and 1–8 microphones. Each microphone owns an independent FIR NLMS prediction/residual path, up to 4096 taps. Prepare the maximum reference delay; the host supplies alignment up to 0.5 s and must manage separate device clocks and drift.

Use the actual loudspeaker feed after relevant output processing, not an unrelated pre-mix. Monitor reference activity, adaptation, residual power, ERLE and counters through owner-serialized metrics.

## Double talk and limitations {#adaptation}

After initial learning, residual power and prediction/microphone coherence control linked adaptation freeze with configurable hangover. Silent reference or disabled adaptation also freezes learning. forceAdaptation requests deliberate relearning after a path change.

Tune thresholds on real speech and room changes. The linear FIR does not solve loudspeaker nonlinearity, clock drift or a complete communications stack. Keep microphone/reference blocks aligned and preserve the reference delay model through callback splits.
''',r'''
## Référence et canaux {#reference}

EchoCanceller reçoit une référence mono représentant l’audio envoyé aux haut-parleurs et 1–8 microphones. Chaque micro possède son chemin FIR NLMS indépendant de prédiction/résidu, jusqu’à 4096 taps. Préparez le retard maximal ; l’hôte fournit l’alignement jusqu’à 0,5 s et doit gérer horloges séparées et dérive.

Utilisez le véritable flux haut-parleur après les traitements de sortie pertinents, pas un prémix sans rapport. Surveillez activité de référence, adaptation, puissance résiduelle, ERLE et compteurs via les métriques sérialisées par le propriétaire.

## Double parole et limites {#adaptation}

Après l’apprentissage initial, puissance résiduelle et cohérence prédiction/microphone contrôlent le gel lié de l’adaptation avec maintien configurable. Référence silencieuse ou adaptation désactivée gèlent aussi l’apprentissage. forceAdaptation demande un réapprentissage volontaire après changement de chemin.

Réglez les seuils sur la parole et les changements de pièce réels. Le FIR linéaire ne résout ni non-linéarités du haut-parleur, ni dérive d’horloge, ni une pile complète de communication. Alignez les blocs micro/référence et conservez le modèle de retard lors des découpages.
''')

add('graphs','DSP graphs and latency compensation','Graphes DSP et compensation de latence','Connect prepared processors, explicit sidechains and retained transitions.','Connecter processeurs préparés, sidechains et transitions conservées.','overview',r'''
## Graph preparation {#topology}

AudioGraph supplies a prepared single-bus DAG. MultiBusGraph adds DspNode, DspBus and DspConnection with explicit bus IDs local to each node. graphInputNode/graphOutputNode identify external boundaries. Preparation copies topology but borrows processor contexts/functions; keep them alive and serialized until the graph is replaced.

MultiBusGraph supports at most 256 nodes, 4096 connections and 32 buses per layout. Cycles are rejected. Declare each node's fixed DspTiming and correct input/output shapes. Sidechains are real input buses rather than hidden main-channel conventions.

## Alignment, errors and transitions {#timing}

Branches entering a node and external outputs align to the greatest declared latency. Bypass keeps the processor warm and uses main bus 0 delayed to its declared latency. Unbounded tail uses SIZE_MAX; finite path tails are bounded by the graph's accepted limits. A callback error silences external output but cannot undo states of earlier nodes.

GraphTransition retains two prepared graphs under a fixed latency budget: stage outside processing, prewarm with live input, then crossfade. Use independent mutable contexts for the two graphs. No graph is destroyed at fade completion; later replacement/destruction occurs outside the callback. Latency stays fixed until new preparation/staging. Frequency-dependent IIR delay and creative pitch effects need an application timing policy beyond integer PDC.
''',r'''
## Préparer la topologie {#topology}

AudioGraph fournit un DAG monobus préparé. MultiBusGraph ajoute DspNode, DspBus et DspConnection avec ID de bus explicites locaux à chaque nœud. graphInputNode/graphOutputNode identifient les frontières externes. La préparation copie la topologie mais emprunte contextes/fonctions ; gardez-les vivants et sérialisés jusqu’au remplacement du graphe.

MultiBusGraph accepte au plus 256 nœuds, 4096 connexions et 32 bus par disposition. Les cycles sont rejetés. Déclarez le DspTiming fixe de chaque nœud et ses formes entrée/sortie. Les sidechains sont de vrais bus d’entrée, sans convention cachée sur les canaux principaux.

## Alignement, erreurs et transitions {#timing}

Les branches entrant dans un nœud et les sorties externes s’alignent sur la plus grande latence déclarée. Bypass garde le processeur chaud et utilise le bus principal 0 retardé à sa latence. Une queue infinie utilise SIZE_MAX ; les queues finies respectent les limites de chemin acceptées. Une erreur de callback réduit la sortie externe au silence sans annuler les états des nœuds antérieurs.

GraphTransition conserve deux graphes préparés sous un budget de latence fixe : stage hors traitement, préchauffage par entrée vivante, puis fondu. Utilisez des contextes mutables indépendants entre graphes. Aucun graphe n’est détruit en fin de fondu ; remplacement/destruction ultérieurs se font hors callback. La latence reste fixe jusqu’à nouvelle préparation/staging. Le retard IIR dépendant de la fréquence et les effets créatifs de hauteur exigent une politique de temps au-delà d’une PDC entière.
''')

add('spatial','3D coordinates and propagation','Coordonnées 3D et propagation','Use metres, radians, source/listener poses and acoustic media.','Utiliser mètres, radians, poses source/auditeur et milieux acoustiques.','overview',r'''
## One coordinate convention {#coordinates}

CyDSP uses a right-handed frame: **+X forward, +Y left, +Z up**. Positions are metres, velocities m/s, angles radians and quaternions `(w,x,y,z)`. Convert a game's axis directions and scale explicitly at the boundary. Normalize orientations and use the API's validation instead of feeding engine-specific units directly.

relativeSource gives distance, local/world directions, azimuth/elevation and radial velocities. relativeSources writes listener-major rows at `listener * sourceCount + source`. interpolatePose uses shortest-arc quaternion interpolation.

## Distance, directivity and media {#propagation}

DistanceAttenuation selects inverse, exponential or linear distance gain with reference/maximum distance, rolloff and clamping policy. Directivity uses a pattern, rear band losses, floor and optional polarity preservation. AcousticMedium contains sound speed, density and three-band absorption; atmospheric helpers use temperature, humidity and pressure. waterMedium is a reference preset, not automatic water physics.

acousticTravel accumulates segment delay/loss. DopplerPlan caches sound-speed/intensity/pitch bounds for repeated updates; DopplerResampler applies the resulting input/output ratio with explicit stream counts. Do not apply identical Doppler twice through resampling and a changing propagation delay. The host owns geometry, occlusion detection, updates and audio FIFO scheduling.
''',r'''
## Une convention de coordonnées {#coordinates}

CyDSP utilise un repère direct : **+X devant, +Y à gauche, +Z vers le haut**. Positions en mètres, vitesses en m/s, angles en radians et quaternions `(w,x,y,z)`. Convertissez explicitement axes et échelle du jeu à la frontière. Normalisez les orientations et utilisez la validation au lieu de fournir directement les unités propres au moteur.

relativeSource donne distance, directions locale/monde, azimut/élévation et vitesses radiales. relativeSources écrit les lignes auditeur-major à `listener * sourceCount + source`. interpolatePose interpole les quaternions par l’arc court.

## Distance, directivité et milieux {#propagation}

DistanceAttenuation choisit une atténuation inverse, exponentielle ou linéaire avec référence/distance maximale, rolloff et politique de clamp. Directivity utilise un motif, des pertes arrière par bandes, un plancher et une conservation optionnelle de polarité. AcousticMedium contient vitesse du son, densité et absorption trois bandes ; les helpers atmosphériques utilisent température, humidité et pression. waterMedium est un préréglage de référence, pas une physique automatique de l’eau.

acousticTravel cumule retards/pertes des segments. DopplerPlan met en cache vitesse/intensité/bornes de hauteur pour les mises à jour répétées ; DopplerResampler applique le ratio entrée/sortie obtenu avec comptes explicites. N’appliquez pas le même Doppler par rééchantillonnage puis par retard de propagation variable. L’hôte possède géométrie, détection des occlusions, mises à jour et FIFO audio.
''')

add('acoustics','Paths, materials, rooms and diffraction','Chemins, matériaux, pièces et diffraction','Render prepared scene paths and room buses from host geometry.','Rendre chemins et bus de pièces préparés depuis la géométrie de l’hôte.','spatial',r'''
## Paths and material response {#paths}

AcousticMaterial supplies three-band absorption/transmission loss, reference thickness and scattering. PreparedAcousticMaterial/PreparedMaterialBand cache validated settings for repeated thickness responses. These gains are parametric approximations rather than a geometric solver.

AcousticPathRenderer has up to 64 stable-ID path slots and at most 10 s prepared delay. It shares mono history and exposes one output channel per path slot, with delay, arrival direction and three-band gains. Multichannel input is averaged; keep independent emitters separate. Removing a path fades it before releasing the slot instead of stealing an audible path.

## Rooms, portals and returns {#rooms}

The application supplies rooms, portal edges/opening/losses and response cells. AcousticPortalGraph prepares transfer routes; RoomCellGrid interpolates a regular response grid. CyDSP does not ray trace the scene or inspect game actors.

RoomReverbBank retains up to 32 room responses. Its block workflow is beginBlock, one or more send calls, render, then mix or read roomOutput. Continue rendering zero-send rooms to advance tails. The application chooses pre/post-occlusion sends and listener-return gains.

## Diffraction {#diffraction}

DiffractionPath describes edge detour and aperture conditions from host geometry. diffractionGain evaluates a thin-screen/frequency-dependent approximation; DiffractionFilter applies its band response. It is not a wave-equation or arbitrary obstacle solver. Keep distances, aperture and frequencies in the API's units and assess low/high-frequency behavior in the intended scene.
''',r'''
## Chemins et matériaux {#paths}

AcousticMaterial fournit absorption/perte de transmission trois bandes, épaisseur de référence et diffusion. PreparedAcousticMaterial/PreparedMaterialBand mettent en cache les réglages validés pour les réponses répétées selon l’épaisseur. Ces gains sont des approximations paramétriques, pas un solveur géométrique.

AcousticPathRenderer possède jusqu’à 64 emplacements à ID stable et au plus 10 s de retard préparé. Il partage l’historique mono et expose un canal par emplacement, avec retard, direction d’arrivée et gains trois bandes. L’entrée multicanale est moyennée ; gardez les émetteurs indépendants séparés. Retirer un chemin le fait disparaître progressivement avant libération, sans vol de chemin audible.

## Pièces, portails et retours {#rooms}

L’application fournit pièces, arêtes/ouvertures/pertes de portails et cellules de réponse. AcousticPortalGraph prépare les routes ; RoomCellGrid interpole une grille régulière. CyDSP ne lance pas de ray tracing et ne parcourt pas les acteurs du jeu.

RoomReverbBank conserve jusqu’à 32 réponses. Le cycle de bloc est beginBlock, un ou plusieurs send, render, puis mix ou lecture de roomOutput. Continuez à rendre les pièces sans send pour avancer les queues. L’application choisit les sends avant/après occlusion et les gains de retour par auditeur.

## Diffraction {#diffraction}

DiffractionPath décrit détour de bord et conditions d’ouverture depuis la géométrie de l’hôte. diffractionGain évalue une approximation sur écran mince dépendante de la fréquence ; DiffractionFilter applique sa réponse par bandes. Ce n’est pas un solveur d’ondes ou d’obstacles arbitraires. Conservez distances, ouverture et fréquences dans les unités de l’API et évaluez graves/aigus dans la scène visée.
''')

add('binaural','Binaural and loudspeaker rendering','Rendu binaural et sur enceintes','Select analytic binaural, measured HRIR or VBAP layouts.','Choisir binaural analytique, HRIR mesurées ou dispositions VBAP.','spatial',r'''
## Headphone rendering {#headphones}

AnalyticBinaural models spherical-head ITD, head shadow, front/back coloration and near-ear effects for up to 32 placed inputs. Source radius reduces directional detail. Two inputs can represent an extended stereo emitter. It is a parametric model, not a measured or listener-personalized HRTF.

HrirBinaural renders measured responses. Each raw HrirPoint supplies onset-free left/right FIRs and separate ear delays at the prepared rate. Keeping acoustic onset inside FIRs while also applying separate delays can cause combing or doubled delay. Prefer a [prepared HrtfSet](doc:hrtf) when converting or sharing measurements.

setDirection interpolates aligned filters and delays, then fades readers. A request during a fade returns transition_in_progress. Retain the previous successful direction until the request can be accepted; do not block the audio thread waiting.

## Speaker layouts {#speakers}

VbapLayout prepares 2D pairs or 3D convex-hull triples for 2–64 loudspeakers and returns unit-energy gains. Outside coverage, choose error or nearest-speaker fallback during preparation. A sparse/poorly distributed layout cannot reproduce every requested direction equally.

Choose [Ambisonics](doc:ambisonics) when sources should share an intermediate sound field. Panning and binaural cues are different rendering choices; applying both unintentionally can alter level and localization.
''',r'''
## Rendu au casque {#headphones}

AnalyticBinaural modélise ITD de tête sphérique, ombrage, coloration avant/arrière et proximité d’oreille pour jusqu’à 32 entrées placées. Le rayon de source réduit le détail directionnel. Deux entrées peuvent représenter un émetteur stéréo étendu. C’est un modèle paramétrique, pas une HRTF mesurée ou personnalisée.

HrirBinaural rend les réponses mesurées. Chaque HrirPoint brut fournit des FIR gauche/droite sans onset et des retards d’oreilles séparés au taux préparé. Garder l’onset acoustique dans les FIR et appliquer les retards séparés peut produire filtrage en peigne ou retard doublé. Préférez un [HrtfSet préparé](doc:hrtf) pour convertir/partager les mesures.

setDirection interpole filtres alignés et retards, puis fond les lecteurs. Une demande pendant le fondu retourne transition_in_progress. Conservez la dernière direction acceptée jusqu’à acceptation de la nouvelle ; ne bloquez pas le thread audio en attente.

## Dispositions d’enceintes {#speakers}

VbapLayout prépare paires 2D ou triplets d’enveloppe convexe 3D pour 2–64 enceintes et retourne des gains à énergie unitaire. Hors couverture, choisissez erreur ou repli vers l’enceinte proche à la préparation. Une disposition clairsemée/mal répartie ne reproduit pas toutes les directions également.

Choisissez les [Ambisonics](doc:ambisonics) pour partager un champ intermédiaire entre sources. Panoramique et indices binauraux sont des rendus différents ; appliquer les deux involontairement peut changer niveau et localisation.
''')

add('hrtf','Prepare and share HRTFs','Préparer et partager les HRTF','Separate onsets, convert FIR rates and retain immutable prepared banks.','Séparer les onsets, convertir les FIR et partager les banques préparées immuables.','binaural',r'''
## In-memory measurements {#prepare}

HrtfSet defaults to 2048 directions and 32768 input frames/ear; explicit limits permit 100000 points and 65536 frames. The [optional SOFA importer](doc:sofa) reads measurements from memory. No measured dataset is bundled. By default the first crossing of a threshold relative to the peak (-40 dB) defines an integer onset; supplied fractional delays remain separate.

Preparation removes that onset and resamples FIRs with windowed sinc when needed. Rates are 8–768 kHz and the conversion ratio is 1/8–8. Conversion preserves IR DC gain and exposes a common causal conversion latency through latencyFrames. Acoustic directional onsets remain distinct from that common latency.

## Capacity and sharing {#sharing}

Prepared FIR length defaults to 2048 taps and is configurable up to 65536. Use rejectTruncation or check truncated(). See the [quality guide](doc:sofa#prepare). Renderer tail bounds include FIR, acoustic delays and conversion latency.

Renderers acquire a shared immutable prepared state during prepare; they retain independent mutable histories/filter fades. Repreparing or destroying the original HrtfSet does not invalidate already prepared renderers. This differs from raw SampleData/Wavetable borrowing.

Direction interpolation uses normalized spherical Shepard weights with Gaussian locality. Exact measurement directions reproduce their measured point. angularWidthRadians controls locality, not the measurement density or individual listener fit. Onset detection is not a fractional calibration procedure. Assess moving sources, sparse coverage and listener-specific cues on actual material.
''',r'''
## Mesures en mémoire {#prepare}

HrtfSet accepte par défaut 2048 directions et 32768 frames/oreille ; les bornes explicites vont à 100000 points et 65536 frames. L’[importeur SOFA optionnel](doc:sofa) lit les mesures en mémoire. Aucun jeu mesuré n’est inclus. Par défaut, le premier dépassement d’un seuil relatif au pic (-40 dB) définit un onset entier ; les retards fractionnaires fournis restent séparés.

La préparation retire l’onset et rééchantillonne les FIR par sinc fenêtrée si nécessaire. Les taux vont de 8 à 768 kHz et le ratio de conversion de 1/8 à 8. La conversion conserve le gain DC de l’IR et expose sa latence causale commune via latencyFrames. Les onsets acoustiques directionnels restent distincts de cette latence.

## Capacité et partage {#sharing}

La FIR préparée vaut au maximum 2048 taps par défaut, configurable jusqu’à 65536. Utilisez rejectTruncation ou vérifiez truncated(). Voir le [guide de qualité](doc:sofa#prepare). Les bornes de queue des renderers comprennent FIR, retards acoustiques et latence de conversion.

Les renderers acquièrent un état préparé immuable partagé pendant prepare ; leurs historiques/fondus restent indépendants. Repréparer ou détruire le HrtfSet original n’invalide pas les renderers déjà préparés. Ce contrat diffère de l’emprunt brut de SampleData/Wavetable.

L’interpolation directionnelle utilise des poids Shepard sphériques normalisés à localité gaussienne. Une direction mesurée reproduit son point. angularWidthRadians règle la localité, pas la densité des mesures ou l’adaptation à l’auditeur. La détection d’onset n’est pas une calibration fractionnaire. Évaluez mouvements, couverture clairsemée et indices propres à l’auditeur sur des contenus réels.
''')

add('ambisonics','Ambisonics and HOA binaural','Ambisonics et binaural HOA','Encode, rotate and decode ACN/SN3D orders zero through seven.','Encoder, tourner et décoder les ordres zéro à sept ACN/SN3D.','spatial',r'''
## Format and rotations {#format}

ambisonicCoefficients, ambisonicRotation and AmbisonicRenderer support orders 0–7, `(order+1)^2` channels: 1–64. Channel order is ACN and normalization SN3D. First-order order is W,Y,Z,X. Convert FuMa/N3D explicitly outside this API.

Rotations are active: encoding direction d then rotating equals encoding R*d. For listener orientation, supply the inverse rotation. Do not mix a world-to-listener coordinate transform with a second equivalent field rotation.

## Decode to speakers or ears {#decode}

Speaker decoding uses regularized least squares for the supplied layout. Choose regularization for poorly conditioned/underdetermined layouts. Extra mathematical channels cannot supply spatial resolution missing from sparse speakers; this is not an AllRAD or room-correction decoder.

HoaBinaural projects measured HRIRs into the harmonic field, embeds fractional acoustic delays into FIRs and prepares a two-ear MatrixConvolver with HOA inputs. Field rotation can precede convolution. Prepare all filters outside processing, retain shared HRTFs and include conversion/convolution latency and tail in graph timing.

```diagram
{"type":"flow","title":"HOA signal path","nodes":[{"id":"sources","label":"Sources","x":0,"y":0},{"id":"encode","label":"ACN/SN3D","x":190,"y":0},{"id":"rotate","label":"Rotation","x":390,"y":0},{"id":"ears","label":"HOA binaural","x":590,"y":0}],"edges":[{"from":"sources","to":"encode"},{"from":"encode","to":"rotate"},{"from":"rotate","to":"ears"}]}
```
''',r'''
## Format et rotations {#format}

ambisonicCoefficients, ambisonicRotation et AmbisonicRenderer acceptent les ordres 0–7, soit `(order+1)^2` canaux : 1–64. L’ordre est ACN et la normalisation SN3D. Au premier ordre : W,Y,Z,X. Convertissez explicitement FuMa/N3D hors de cette API.

Les rotations sont actives : encoder la direction d puis tourner égale encoder R*d. Pour l’orientation d’auditeur, fournissez la rotation inverse. Ne combinez pas une transformation monde-vers-auditeur avec une deuxième rotation de champ équivalente.

## Décoder vers enceintes ou oreilles {#decode}

Le décodage d’enceintes utilise des moindres carrés régularisés pour la disposition fournie. Choisissez la régularisation adaptée aux matrices mal conditionnées/sous-déterminées. Des canaux mathématiques supplémentaires ne recréent pas la résolution absente d’enceintes clairsemées ; ce n’est ni un décodeur AllRAD ni une correction de salle.

HoaBinaural projette les HRIR mesurées dans le champ harmonique, incorpore les retards acoustiques fractionnaires aux FIR et prépare un MatrixConvolver à deux oreilles et entrées HOA. La rotation du champ peut précéder la convolution. Préparez les filtres hors traitement, conservez les HRTF partagées et déclarez latences/queues de conversion/convolution dans le graphe.

```diagram
{"type":"flow","title":"Chaîne de signal HOA","nodes":[{"id":"sources","label":"Sources","x":0,"y":0},{"id":"encode","label":"ACN/SN3D","x":190,"y":0},{"id":"rotate","label":"Rotation","x":390,"y":0},{"id":"ears","label":"Binaural HOA","x":590,"y":0}],"edges":[{"from":"sources","to":"encode"},{"from":"encode","to":"rotate"},{"from":"rotate","to":"ears"}]}
```
''')

add('integration','Integrate a DAW, plugin or game','Intégrer un DAW, plugin ou jeu','Connect application-owned callbacks, controls and spatial scenes.','Raccorder callbacks, contrôles et scènes possédés par l’application.','overview',r'''
## Audio callback boundary {#callback}

Match ProcessSpec to the actual stream rate, channel count and largest processing block. If a host callback can exceed that maximum, split it into bounded sub-blocks and preserve event offsets. Keep planar pointer tables alive for every call. Convert interleaved input/output through prepared application buffers when necessary.

Own device/MIDI/file/plugin-format services in the consuming engine. A CyDSP processor does not register a plugin, open an audio device or create a window. CyAudioCore is a consumer of the library; direct application use follows the same public contracts.

## Practical chains {#recipes}

| Use case | Construction and responsibility |
| --- | --- |
| DAW insert | Prepared filter → dynamics → output gain; declare look-ahead and FIR latency to the host. |
| Convolution reverb | Decode/rate-match IR off-thread → prepare MatrixConvolver → send/mix → drain tail. |
| Instrument | Schedule notes → own voice allocation → oscillators/samples → envelope/filter → headroom/limiting. |
| Game emitter | Convert pose/units → propagation/material paths → independent source renderer → binaural/HOA → room return. |
| Meter display | Process analysis → copy owner snapshot → publish to UI through an application queue. |

## Reconfiguration {#reconfigure}

Prepare replacement processors and immutable assets away from audio. Publish an ownership-safe command or stage a supported transition; never swap/destroy live contexts from an unrelated thread. On sample-rate/layout changes, stop or transition under an explicit policy and reprepare every affected component. Avoid double distance loss, double Doppler and uncompensated dry/wet latency.
''',r'''
## Frontière du callback audio {#callback}

Faites correspondre ProcessSpec au taux réel du flux, au nombre de canaux et au plus grand bloc de traitement. Si un callback peut dépasser ce maximum, découpez-le en sous-blocs bornés et conservez les offsets d’événements. Gardez les tableaux de pointeurs planaires vivants pendant chaque appel. Convertissez les entrées/sorties entrelacées via des buffers préparés de l’application si nécessaire.

Les services périphériques/MIDI/fichiers/formats de plugins appartiennent au moteur consommateur. Un processeur CyDSP n’enregistre pas de plugin, n’ouvre pas de périphérique et ne crée pas de fenêtre. CyAudioCore est un consommateur ; l’utilisation directe respecte les mêmes contrats publics.

## Chaînes pratiques {#recipes}

| Usage | Construction et responsabilité |
| --- | --- |
| Insert de DAW | Filtre préparé → dynamique → gain de sortie ; déclarer anticipation et latence FIR à l’hôte. |
| Réverbération convolution | Décoder/adapter le taux de l’IR hors thread → préparer MatrixConvolver → send/mix → vider la queue. |
| Instrument | Planifier les notes → posséder l’allocation des voix → oscillateurs/samples → enveloppe/filtre → réserve/limitation. |
| Émetteur de jeu | Convertir pose/unités → propagation/matériaux/chemins → source indépendante → binaural/HOA → retour de pièce. |
| Affichage de mesure | Traiter l’analyse → copier l’instantané du propriétaire → publier à l’UI par une file de l’application. |

## Reconfiguration {#reconfigure}

Préparez processeurs de remplacement et données immuables hors audio. Publiez une commande à propriété sûre ou préparez une transition autorisée ; ne remplacez/détruisez pas des contextes vivants depuis un autre thread. Lors d’un changement de taux/disposition, arrêtez ou faites une transition explicite puis repréparez tous les composants concernés. Évitez pertes de distance et Doppler doublés, ainsi que latence dry/wet non compensée.
''')

add('troubleshooting','Troubleshooting','Résoudre les problèmes','Diagnose shape, silence, latency, streaming and spatial issues.','Diagnostiquer formes, silence, latence, flux et spatialisation.','overview',r'''
## Common symptoms {#symptoms}

| Symptom | Check |
| --- | --- |
| not_prepared | All members successfully prepared; moved-from objects need preparation. |
| channel_mismatch/block_too_large | Actual views match prepared channels and maximum frames, including sidechains. |
| partial_overlap | Use exact supported in-place views or disjoint buffers; account for sub-block offsets. |
| Silence after a graph error | Check the first failing node Status and callback context lifetimes. |
| Click on parameter changes | Use gain ramps, stable-filter fades or prepared IR/graph transitions. |
| Flanging in dry/wet | Align fixed latency and distinguish frequency-dependent IIR phase. |
| Dropped resampled input | Keep the suffix after inputConsumed; only publish outputProduced. |
| Missing convolution tail | Continue zeros or use the supported drain operation. |
| Grain trigger rejected | Check pool capacity, offset and source/position lifetime. |
| No early analysis result | Accumulate a full window; respect hop and readiness flags. |
| LUFS processing returns would_block | Exact history is full; size the session or stop/reset deliberately. |
| Moving HRTF request rejected | Previous direction fade is active; coalesce/retry without blocking. |
| Wrong 3D direction or speed | Convert axes, metres, radians, quaternion order and radial velocities. |
| Different precision/backend samples | Reduction order, selected approximation, antialias policy and FFT block pattern can differ. |

## Investigate a minimal chain {#minimal}

Record version, target ABI, sample rate, channel layout, block pattern and exact settings. Reduce to one processor and a finite test signal; inspect returned Status before interpreting output. Match numerical contracts when comparing another implementation, including latency, detector topology and antialiasing. A faster algorithm with different behavior is a separate quality choice.
''',r'''
## Symptômes courants {#symptoms}

| Symptôme | Vérification |
| --- | --- |
| not_prepared | Tous les membres ont réussi prepare ; un objet source déplacé exige une préparation. |
| channel_mismatch/block_too_large | Les vues respectent canaux et frames maximales, sidechains compris. |
| partial_overlap | Vues exactement en place autorisées ou buffers disjoints ; compter les offsets de sous-blocs. |
| Silence après erreur de graphe | Chercher le premier Status en échec et vérifier la vie des contextes. |
| Clic lors d’une modification | Utiliser rampes, fondus de filtres stables ou transitions d’IR/graphes préparées. |
| Flanging dry/wet | Aligner la latence fixe et distinguer la phase IIR dépendante de la fréquence. |
| Entrée rééchantillonnée perdue | Garder le suffixe après inputConsumed ; publier seulement outputProduced. |
| Queue de convolution absente | Continuer les zéros ou utiliser l’opération de vidange autorisée. |
| Déclenchement de grain refusé | Vérifier réserve, offset et durée de vie de source/position. |
| Analyse initiale absente | Accumuler une fenêtre entière ; respecter hop et indicateurs de disponibilité. |
| LUFS retourne would_block | Historique exact plein ; dimensionner la session ou arrêter/réinitialiser volontairement. |
| Direction HRTF mobile refusée | Fondu précédent actif ; regrouper/réessayer sans bloquer. |
| Direction ou vitesse 3D incorrecte | Convertir axes, mètres, radians, ordre du quaternion et vitesses radiales. |
| Échantillons différents selon précision/backend | Ordre des réductions, approximation, antialiasing et découpage FFT peuvent différer. |

## Examiner une chaîne minimale {#minimal}

Relevez version, ABI, taux, canaux, découpage de blocs et réglages exacts. Réduisez à un processeur et un signal fini ; vérifiez Status avant d’interpréter la sortie. Faites correspondre les contrats numériques lors d’une comparaison : latence, topologie du détecteur et antialiasing. Un algorithme plus rapide au comportement différent constitue un autre choix de qualité.
''')

add('limits','Platforms, capacities and model limits','Plateformes, capacités et limites des modèles','Separate algorithm availability from target and model assumptions.','Distinguer algorithmes disponibles, cibles et hypothèses des modèles.','overview',r'''
## Targets {#platforms}

| Target | Current scope |
| --- | --- |
| Windows x64 | MSVC/clang-cl runtime coverage; supported compiled SSE2/AVX2/AVX-512 paths depend on CPU/OS. |
| Android ARM64 | Runtime coverage on Quest 3 with NEON; it has no SVE. |
| Windows ARM64 | Static compilation coverage; device execution remains to validate. |
| ARM64 SVE | Android compile/link coverage; hardware vector-length execution remains to validate. |
| macOS / desktop Linux | Source targets; execution remains to validate for this version. |
| x86 / ARM32 | Compatibility paths exist; they were not rerun for this snapshot. ARM32 double kernels can fall back to scalar. |

No ABI is interchangeable with another. Version 0.4.0 is in development; target deployment needs its own content/performance assessment. Performance depends on processor, dimensions, quality, backend and block boundaries.

## Prepared capacities {#capacities}

| Area | Important boundary |
| --- | --- |
| General expanded processors | Typically 8–768 kHz, 1–64 channels, up to 65536 maximum block frames; individual APIs may be smaller. |
| SourceBatch | 1024 sources, 64 outputs and explicit prepared storage/bank budgets. |
| Granulator | Up to 4096 slots; no implicit voice stealing. |
| Spatial paths / rooms | 64 paths and 10 s prepared path delay; up to 32 room responses. |
| Ambisonics | Orders 0–7, 1–64 ACN/SN3D channels. |
| HRTF | Default 2048 directions/32768 input frames/2048 output taps; explicit reference caps 100000/65536/65536. See [SOFA](doc:sofa). |
| Graphs | MultiBusGraph: 256 nodes, 4096 connections and 32 buses/layout. |

## Interpret models correctly {#models}

No HRTF dataset is supplied; its source/license and listener suitability are application concerns. Analytic binaural, thin-screen diffraction, room transfer, contact physics and musical estimates are reduced/heuristic models. True peak/loudness are not presented as certified broadcast products. Echo cancellation is linear and needs aligned clocks/reference. Choose limits from each header's configuration and assess realistic content rather than extrapolating from a single synthetic signal.
''',r'''
## Cibles {#platforms}

| Cible | Périmètre actuel |
| --- | --- |
| Windows x64 | Couverture en exécution MSVC/clang-cl ; chemins SSE2/AVX2/AVX-512 compilés selon CPU/OS. |
| Android ARM64 | Couverture en exécution sur Quest 3 avec NEON ; il ne possède pas SVE. |
| Windows ARM64 | Couverture de compilation statique ; exécution sur appareil à valider. |
| ARM64 SVE | Compilation/liaison Android ; exécution matérielle selon les longueurs vectorielles à valider. |
| macOS / Linux desktop | Cibles de sources ; exécution à valider pour cette version. |
| x86 / ARM32 | Chemins de compatibilité existants ; non réexécutés pour cet état. Les noyaux double ARM32 peuvent revenir au scalaire. |

Les ABI ne sont pas interchangeables. La version 0.4.0 est en développement ; un déploiement exige son évaluation de contenu/performance. Le coût dépend du processeur, des dimensions, de la qualité, du backend et des frontières de blocs.

## Capacités préparées {#capacities}

| Domaine | Limite importante |
| --- | --- |
| Processeurs étendus usuels | Généralement 8–768 kHz, 1–64 canaux, bloc maximal jusqu’à 65536 frames ; certaines API sont plus petites. |
| SourceBatch | 1024 sources, 64 sorties et budgets explicites de stockage/banques. |
| Granulator | Jusqu’à 4096 emplacements ; aucun vol de voix implicite. |
| Chemins / pièces | 64 chemins et 10 s de retard préparé ; jusqu’à 32 réponses de pièces. |
| Ambisonics | Ordres 0–7, 1–64 canaux ACN/SN3D. |
| HRTF | Défaut 2048 directions/32768 frames d’entrée/2048 taps ; bornes de référence explicites 100000/65536/65536. Voir [SOFA](doc:sofa). |
| Graphes | MultiBusGraph : 256 nœuds, 4096 connexions et 32 bus/disposition. |

## Interpréter les modèles {#models}

Aucun jeu HRTF n’est fourni ; sa provenance/licence et son adaptation à l’auditeur relèvent de l’application. Binaural analytique, diffraction sur écran mince, transferts de pièces, physique des contacts et estimations musicales sont des modèles réduits/heuristiques. True peak et sonie ne sont pas présentés comme des produits broadcast certifiés. L’annulation d’écho est linéaire et exige horloges/référence alignées. Choisissez les limites de chaque configuration et évaluez des contenus réalistes sans extrapoler depuis un signal synthétique unique.
''')

add('licensing','License and component notices','Licence et notices des composants','Retain CyDSP MIT and the notices for adapted numerical components.','Conserver la MIT CyDSP et les notices des composants numériques adaptés.','overview',r'''
## CyDSP {#license}

CyDSP uses the MIT license, copyright © 2026 MrMybal CyberAlien. The complete text is supplied with the library. Retain its copyright and permission notice when redistributing covered copies. The license does not require publication of a consuming application's source code.

## Numerical adaptations {#components}

| Component | License | Scope |
| --- | --- | --- |
| DSPFilters | MIT | Filter coefficient/prototype equations adapted to CyDSP preparation. |
| FFTConvolver | MIT | Partition/overlap-add foundations adapted to CyDSP streaming and MIMO. |
| r8brain-free-src | MIT | Window/sinc equations; not the complete upstream SRC algorithm. |
| signalsmith-linear | MIT | FFT reconstruction and STFT/window normalization. |
| signalsmith-stretch | MIT | Optional CyDSP::Stretch time/pitch/formant target. |
| libmysofa reader / miniz | BSD-3-Clause / MIT | Optional compact memory reader; no upstream renderer or dataset. |
| External HDF5 / zlib | BSD-3-Clause / Zlib | Optional complete SDK, not vendored. |

Pinned provenance and complete notices accompany the sources and installed library. This table is an index; it does not replace the texts. Host-provided samples, IRs and HRTF datasets retain their own licenses.

## Documentation reader {#reader}

The generated reader uses CyDoc 0.1.0 under MIT. The export includes its complete notice together with the CyDSP and numerical component notices. Documentation-generation tools are separate from the C++ library and are not a runtime dependency of applications linking CyDSP.
''',r'''
## CyDSP {#license}

CyDSP utilise la licence MIT, copyright © 2026 MrMybal CyberAlien. Son texte complet accompagne la bibliothèque. Conservez copyright et autorisation lors de la redistribution des copies concernées. La licence n’exige pas de publier les sources d’une application consommatrice.

## Adaptations numériques {#components}

| Composant | Licence | Périmètre |
| --- | --- | --- |
| DSPFilters | MIT | Équations de coefficients/prototypes adaptées à la préparation CyDSP. |
| FFTConvolver | MIT | Fondations de partitions/recouvrement-addition adaptées au flux et au MIMO. |
| r8brain-free-src | MIT | Équations de fenêtres/sinc ; pas l’algorithme SRC amont complet. |
| signalsmith-linear | MIT | Reconstruction FFT et normalisation STFT/fenêtres. |
| signalsmith-stretch | MIT | Cible optionnelle CyDSP::Stretch de durée/hauteur/formants. |
| Lecteur libmysofa / miniz | BSD-3-Clause / MIT | Lecteur compact optionnel ; aucun renderer ou jeu amont. |
| HDF5 / zlib externes | BSD-3-Clause / Zlib | SDK complet optionnel, non inclus. |

Provenances figées et notices complètes accompagnent sources et bibliothèque installée. Ce tableau est un index, pas un remplacement des textes. Samples, IR et jeux HRTF apportés par l’hôte gardent leurs propres licences.

## Lecteur de documentation {#reader}

Le lecteur généré utilise CyDoc 0.1.0 sous MIT. L’export contient sa notice complète ainsi que celles de CyDSP et des composants numériques. Les outils de génération documentaire sont distincts de la bibliothèque C++ et ne sont pas une dépendance d’exécution des applications liées à CyDSP.
''')

add('sofa','SOFA import and HRTF assets','Import SOFA et banques HRTF','Import measurements optionally; prepare quality and portable caches.','Importer les mesures en option ; préparer la qualité et les caches portables.','hrtf',r'''
## Optional targets {#targets}

| Build switch | Link target | Dependencies |
| --- | --- | --- |
| Default core | CyDSP::CyDSP | No SOFA/file-format dependency |
| CYDSP_BUILD_HRTF_ASSETS=ON | CyDSP::HrtfAssets | Core only |
| CYDSP_BUILD_SOFA=ON | CyDSP::Sofa | Private compact BSD-3-Clause libmysofa reader + MIT miniz |
| Also CYDSP_SOFA_COMPLETE=ON | CyDSP::Sofa | External HDF5 ≥1.14.6 and zlib SDKs |

All production targets are static. SOFA and assets default OFF; the compact backend defaults ON within an enabled importer. Automatic selects the complete reader if built, otherwise compact. No SDK is downloaded by CMake. The complete SDK must be present at the final link step; declare C and C++ languages and use CMAKE_PREFIX_PATH to locate it.

~~~cmake
# Source build:
set(CYDSP_BUILD_SOFA ON CACHE BOOL "")
add_subdirectory(CyDSP)
target_link_libraries(editor PRIVATE CyDSP::Sofa)
# Installed package:
find_package(CyDSP 0.4.0 CONFIG REQUIRED COMPONENTS Sofa)
~~~

A separately configured game build may enable HrtfAssets alone and consume decoded caches without either file-format reader.

## Memory import and errors {#import}

Include cydsp/spatial/Sofa.h. The host supplies bytes; loadSofa performs no application file reads. Import, preparation, EQ and cache loading allocate and belong in a worker/editor, never the audio callback. Failure preserves the previous output; SofaLoadReport supplies a specific error.

~~~cpp
cydsp::SofaHrtf asset;
cydsp::SofaLoadOptions options;
cydsp::SofaLoadReport diagnostic;
const auto status = cydsp::loadSofa(fileBytes, asset, options, &diagnostic);
~~~

SimpleFreeFieldHRIR requires M×2×N IRs. GeneralFIR requires allowGeneralFir=true and two-ear receiver geometry. Other conventions fail explicitly. Source spherical degrees/radians or Cartesian metres/cm/mm become unit directions relative to ListenerPosition/View/Up (+X front, +Y left, +Z up). Missing listener vectors use convention defaults. ReceiverPosition identifies left/right by local Y. Nonzero EmitterPosition and multi-emitter data are unsupported.

Data.Delay is read as source-rate frames from 1×2 or M×2 and expanded per point. It remains separate from FIR onset detection. SamplingRate must be constant (8–768 kHz). Imported distances and original metadata/license are retained.

The complete SDK backend supports superblocks 0/2/3, object headers 1/2, compact/dense links and attributes, contiguous/chunked data including single/fixed/extensible modern indexes, float32/64 and fixed/variable strings. Deflate, shuffle and Fletcher32 are allowed. External links/storage, virtual datasets and unknown filters are rejected before data reads; CyDSP has no custom filter loading path.

The compact reader covers object header v2, layout v3, legacy chunk B-tree v1 with shuffle then deflate (other filter pipelines fail), contiguous arrays and supported compact/dense metadata. Use complete for old object header v1 or modern layout/index v4. Compact converts arrays to float32. SofaHrtf64 therefore requires complete or explicit allowPrecisionLoss=true. Reports disclose precision reduction; float output deliberately narrows float64 input. A noexcept inflate callback may use host zlib; it must return ok only for an exact output size and checksum-verified zlib stream.

Default limits: 128 MiB file, 512 MiB decoded buffers, 64 KiB metadata, 100000 points, 32768 taps. Samples and output coordinate/delay arrays share a budget. The compact parser also bounds allocations, read iterations and recursion. These are CyDSP buffer bounds, not a process-wide cap on SDK caches, file-image copies and temporary FFT storage. Use the host worker/sandbox's overall resource limits for untrusted imports.

## Preparation and quality {#prepare}

~~~cpp
cydsp::SofaPreparation settings;
const auto quality = cydsp::hrtfPreparation(
    cydsp::HrtfQuality::realtimeHigh, 48000, settings.hrtf);
if (quality != cydsp::Status::ok) return quality;
settings.diffuseFieldEqualization = true; // optional common-ear correction
settings.normalization = cydsp::HrtfLevelNormalization::frontalEnergy;
cydsp::HrtfSet<float> bank;
cydsp::SofaPreparationReport report;
const auto prepared = cydsp::prepareSofaHrtf(asset, bank, settings, &report);
~~~

Preparation selects the measured distance closest to distanceMetres (default 1 m), retaining points within distanceToleranceMetres of that shell. It neither mixes duplicate directions across radii nor invents missing measurements.

Optional diffuse-field EQ averages both ears' power, smooths log power over one-third octave, clamps inverse gain (default ±12 dB), then creates a common minimum-phase FIR through CyDSP FFT/filter primitives. Provide per-original-point solid-angle weights for irregular grids; empty weights mean equal weights, reported explicitly. This is an approximate correction model, not headphone calibration or exact integration of an irregular grid.

FrontalEnergy applies one common gain so that mean left/right impulse energy at the nearest frontal point is one, preserving interaural level differences. It does not flatten the frontal spectrum. EQ grows the FIR; inspect selected distance, gain and truncation in the report.

| HrtfQuality | FIR cap | Direction interpolation | Fractional delay / SRC sinc half-length |
| --- | --- | --- | --- |
| economy | 256 taps | 4 nearest | Linear / 16 |
| realtimeHigh | 2048 taps | 8 nearest | 17-tap sinc / 64 |
| reference | 65536 taps | All points | 65-tap sinc / 128 |

reference rejects truncation, permits up to 100000 points and retains double precision with SofaHrtf64/HrtfSet<double>. Bounds are configurable within header limits and the coefficient budget. Default HrtfPreparation retains the prior all-point/linear behavior; presets are explicit. QualityProfile maps its three levels to these presets. Multiple profiles can coexist in one application.

Nearest modes use a prepared balanced spatial index and normalized Shepard weights. A finite neighbor set can change at boundaries; crossfade direction changes. All-point weighting retains full-bank continuity. HrirBinaural bakes the fractional-delay kernel during setDirection; The legacy HoaBinaural overload applies it during preparation. Its spectral design overload instead evaluates fractional delays by frequency-bin phase and declares its own additional latency. Extra common latency is kernel length divided by two, rounded down, exposed by each renderer.

HrirBinaural uses direct FIR convolution: very long reference FIRs and all-point updates target offline/editor budgets. A quality label does not promise faster CPU execution or perceptually better localization for every dataset. HoaBinaural uses existing matrix convolution. Prepared process/reset/setDirection allocate no memory; serialize updates with audio on the same instance. Shared HrtfSet coefficients are immutable; voice histories and fades are independent.

## Portable cache and rights {#cache}

encodeHrtfAsset stores decoded measurements and original metadata in a versioned little-endian float32/64 cache with CRC32. decodeHrtfAsset validates sizes, checksum and data transactionally, requiring only CyDSP::HrtfAssets. Narrowing a double cache requires allowPrecisionLoss. This serializes decoded measurements, not a renderer or EQ result; call prepareSofaHrtf after loading with the chosen quality/EQ. CRC detects accidental corruption, not malicious replacement.

No measured HRTF dataset is distributed. Test fixtures are original synthetic MIT signals. Original CyDSP code stays MIT; optional reader/SDK notices retain their permissive licenses. Dataset rights remain separate: verify each distributed HRTF bank's terms and retain [component notices](doc:licensing).
''',r'''
## Cibles optionnelles {#targets}

| Option de compilation | Cible liée | Dépendances |
| --- | --- | --- |
| Cœur par défaut | CyDSP::CyDSP | Aucune dépendance SOFA/format de fichier |
| CYDSP_BUILD_HRTF_ASSETS=ON | CyDSP::HrtfAssets | Cœur uniquement |
| CYDSP_BUILD_SOFA=ON | CyDSP::Sofa | Lecteur compact privé libmysofa BSD-3-Clause + miniz MIT |
| Aussi CYDSP_SOFA_COMPLETE=ON | CyDSP::Sofa | SDK externes HDF5 ≥1.14.6 et zlib |

Toutes les cibles de production sont statiques. SOFA et assets valent OFF par défaut ; le lecteur compact vaut ON dans un importeur activé. Automatic choisit le lecteur complet s’il est compilé, sinon le compact. CMake ne télécharge aucun SDK. Le SDK complet doit être présent à l’édition de liens finale ; déclarez C et C++ et utilisez CMAKE_PREFIX_PATH pour le localiser.

~~~cmake
# Depuis les sources :
set(CYDSP_BUILD_SOFA ON CACHE BOOL "")
add_subdirectory(CyDSP)
target_link_libraries(editor PRIVATE CyDSP::Sofa)
# Depuis un paquet installé :
find_package(CyDSP 0.4.0 CONFIG REQUIRED COMPONENTS Sofa)
~~~

Une compilation distincte pour le jeu peut activer seulement HrtfAssets et charger les caches décodés sans aucun lecteur de format.

## Import mémoire et erreurs {#import}

Incluez cydsp/spatial/Sofa.h. L’hôte fournit les octets ; loadSofa ne lit aucun fichier applicatif. Import, préparation, EQ et lecture du cache allouent : utilisez un worker/éditeur, jamais le callback audio. Un échec préserve la sortie précédente ; SofaLoadReport donne une erreur précise.

~~~cpp
cydsp::SofaHrtf asset;
cydsp::SofaLoadOptions options;
cydsp::SofaLoadReport diagnostic;
const auto status = cydsp::loadSofa(fileBytes, asset, options, &diagnostic);
~~~

SimpleFreeFieldHRIR exige des IR M×2×N. GeneralFIR exige allowGeneralFir=true et une géométrie de récepteurs à deux oreilles. Les autres conventions échouent explicitement. Les sources sphériques en degrés/radians ou cartésiennes en mètres/cm/mm deviennent des directions unitaires relatives à ListenerPosition/View/Up (+X avant, +Y gauche, +Z haut). Les vecteurs d’écoute absents prennent les défauts de convention. ReceiverPosition identifie les oreilles par Y local. EmitterPosition non nul et les données multi-émetteurs ne sont pas pris en charge.

Data.Delay est lu en frames au taux source depuis 1×2 ou M×2, puis étendu par point. Il reste distinct de la détection d’onset des FIR. SamplingRate doit être constant (8–768 kHz). Distances et métadonnées/licence d’origine sont conservées.

Le lecteur complet gère les superblocs 0/2/3, objets 1/2, liens/attributs compacts ou denses, données contiguës/en blocs avec index modernes single/fixed/extensible, float32/64 et chaînes fixes/variables. Deflate, shuffle et Fletcher32 sont autorisés. Liens/stockages externes, datasets virtuels et filtres inconnus sont rejetés avant lecture ; CyDSP n’a pas de chargement de filtre personnalisé.

Le lecteur compact couvre les objets v2, disposition v3, index de blocs B-tree v1 avec shuffle puis deflate (les autres pipelines échouent), tableaux contigus et métadonnées compactes/denses prises en charge. Utilisez le complet pour les anciens objets v1 ou disposition/index v4. Le compact convertit les tableaux en float32. SofaHrtf64 exige donc le complet ou allowPrecisionLoss=true explicitement. Les rapports exposent la perte de précision ; une sortie float réduit volontairement une entrée float64. Un callback inflate noexcept peut utiliser zlib de l’hôte ; il doit retourner ok uniquement pour une longueur exacte et un flux zlib dont le checksum a été vérifié.

Limites par défaut : fichier 128 Mio, buffers décodés 512 Mio, métadonnées 64 Kio, 100000 points, 32768 taps. Échantillons et tableaux de coordonnées/délais partagent un budget. Le parseur compact borne aussi allocations, itérations de lecture et récursion. Ce sont des bornes de buffers CyDSP, pas de toute la mémoire du processus, des caches SDK, copies d’image de fichier et temporaires FFT. Utilisez les limites globales du worker/sandbox de l’hôte pour un import non fiable.

## Préparation et qualité {#prepare}

~~~cpp
cydsp::SofaPreparation settings;
const auto quality = cydsp::hrtfPreparation(
    cydsp::HrtfQuality::realtimeHigh, 48000, settings.hrtf);
if (quality != cydsp::Status::ok) return quality;
settings.diffuseFieldEqualization = true; // correction commune optionnelle
settings.normalization = cydsp::HrtfLevelNormalization::frontalEnergy;
cydsp::HrtfSet<float> bank;
cydsp::SofaPreparationReport report;
const auto prepared = cydsp::prepareSofaHrtf(asset, bank, settings, &report);
~~~

La préparation sélectionne la distance mesurée la plus proche de distanceMetres (1 m par défaut), puis les points à distanceToleranceMetres de cette couche. Elle ne mélange pas les directions identiques mesurées à plusieurs rayons et ne crée pas de mesures manquantes.

L’EQ optionnelle en champ diffus moyenne la puissance des deux oreilles, lisse la puissance logarithmique au tiers d’octave, borne le gain inverse (±12 dB par défaut), puis crée une FIR commune à phase minimale avec les primitives FFT/filtres CyDSP. Fournissez les poids d’angle solide par point d’origine pour une grille irrégulière ; une liste vide signifie des poids égaux, signalés dans le rapport. C’est une correction approchée, pas une calibration de casque ni une intégration exacte d’une grille irrégulière.

FrontalEnergy applique un gain commun pour que l’énergie impulsionnelle moyenne gauche/droite du point le plus frontal vaille un, en préservant les différences de niveau entre oreilles. Cela n’aplatit pas le spectre frontal. L’EQ allonge la FIR ; vérifiez distance, gain et troncature dans le rapport.

| HrtfQuality | FIR maximale | Interpolation de direction | Délai fractionnaire / demi-longueur sinc de conversion |
| --- | --- | --- | --- |
| economy | 256 taps | 4 voisins | Linéaire / 16 |
| realtimeHigh | 2048 taps | 8 voisins | Sinc à 17 taps / 64 |
| reference | 65536 taps | Tous les points | Sinc à 65 taps / 128 |

reference rejette la troncature, accepte jusqu’à 100000 points et conserve la double précision avec SofaHrtf64/HrtfSet<double>. Les bornes se règlent dans les limites de l’en-tête et du budget de coefficients. HrtfPreparation par défaut conserve le comportement global/linéaire existant ; les profils sont explicites. QualityProfile associe ses trois niveaux à ces profils. Plusieurs profils peuvent coexister dans une application.

Les modes à voisins utilisent un index spatial équilibré préparé et des poids de Shepard normalisés. Un ensemble fini peut changer aux frontières ; fondez les changements de direction. La pondération globale conserve la continuité sur toute la banque. HrirBinaural incorpore le noyau de délai fractionnaire pendant setDirection ; L’ancienne surcharge HoaBinaural l’applique en préparation. La conception spectrale évalue plutôt les délais fractionnaires par la phase des bins et déclare sa propre latence ajoutée. La latence commune ajoutée vaut la moitié entière de la longueur du noyau et est exposée par chaque renderer.

HrirBinaural utilise une convolution FIR directe : les longues FIR de référence et les mises à jour globales visent l’éditeur/offline. Un profil de qualité ne promet ni un calcul plus rapide ni une meilleure localisation perçue pour toute mesure. HoaBinaural utilise la convolution matricielle existante. Process/reset/setDirection préparés n’allouent pas ; sérialisez paramètres et traitement d’une même instance. Les coefficients HrtfSet partagés sont immuables ; historiques et fondus restent propres aux voix.

## Cache portable et droits {#cache}

encodeHrtfAsset stocke les mesures décodées et leurs métadonnées dans un cache versionné little-endian float32/64 avec CRC32. decodeHrtfAsset vérifie tailles, checksum et données avant de remplacer la sortie, avec seulement CyDSP::HrtfAssets. Réduire un cache double exige allowPrecisionLoss. Ce sont des mesures décodées, pas un renderer ni un résultat d’EQ sérialisé ; appelez prepareSofaHrtf après chargement avec la qualité/EQ choisies. Le CRC détecte une corruption accidentelle, pas un remplacement malveillant.

Aucun jeu HRTF mesuré n’est distribué. Les tests utilisent des signaux synthétiques originaux MIT. Le code CyDSP original reste MIT ; lecteurs/SDK optionnels conservent leurs licences permissives. Les droits des mesures restent distincts : vérifiez les termes de chaque banque distribuée et conservez les [notices des composants](doc:licensing).
''')

# Original HOA/analytic-HRIR additions; paired public anchors.
for page in PAGES:
    if page['id']=='ambisonics':
        page['body']=(page['body'][0]+r'''
## Choose the binaural filter design {#design}

The existing prepare(spec, order, bank, regularization, backend) keeps its time-domain least-squares projection. It is useful for compatibility and low added latency. Low-order projection of direction-dependent HRIR phase can lose high-frequency energy; increasing gain alone does not fix its direction-dependent coloration.

The explicit HoaDecoderDesign overload adds weighted spectral least squares and MagLS. The implementation is original MIT C++ based on the [published objective](https://pub.dega-akustik.de/DAGA_2018/data/articles/000301.pdf), using CyDSP FFTs and MatrixConvolver. It imports no IEM/SPARTA implementation or new dependency.

~~~cpp
#include <cydsp/spatial/SpatialRendering.h>
cydsp::HoaDecoderDesign design;
design.method = cydsp::HoaDecoderMethod::magLS;
design.crossoverHz = 2000;
design.transitionHz = 500;
design.maxRe = false;
cydsp::HoaBinaural<float> renderer;
const auto status = renderer.prepare({48000, 256, 4}, 1, bank, design);
if (status != cydsp::Status::ok) return status;
const auto report = renderer.designReport();
~~~

Below the transition, fitting uses the complex HRTF response, including direction-dependent ear arrival delays. Across the transition it blends complex and magnitude errors; above it, alternating projection fits magnitudes using frequency-continuous phase initialization. Iterations are bounded (default 12, 1–256), and a local solution is obtained, not a guaranteed global optimum. The default band spans 1750–2250 Hz; choose it for the material, order and listener. Relaxing high-band phase preserves the low-band LS fit, not every perceptual cue or exact spatial resolution.

The spectral designer uses every supplied bank point with its design weights; direct-renderer neighbor counts and fractional-delay kernel lengths do not control this projection. Fractional delays are evaluated spectrally at the design bins.

An ITD belongs to a direction. A scene with several directions cannot recover all original ITDs by adding one output-ear delay after mixing. This design retains complex low-band targets and relaxes high-band phase instead; there is no claim of a universal remove/reinsert ITD operation.

maxRe optionally tapers high-band harmonic degrees by the max-rE Legendre weights. It broadens the angular response and may change magnitude fit/levels. It is separate from MagLS, defaults OFF and is not a replacement for high-band magnitude optimization.

Spatial weights are empty for equal weighting, or contain one nonnegative weight per bank point. They are normalized internally. Use appropriate solid-angle weights for irregular measured grids. Regularization is applied to this normalized Gram matrix, so its scale is independent of grid point count; it is not numerically identical to the legacy overload's unnormalized regularization.

## Latency, memory and diagnostics {#design-limits}

The spectral designer runs only in prepare, on a worker/editor. It retains the whole designed FIR, with no silent truncation. Auto FFT size is the next power of two at least twice the full delayed HRIR length (minimum 32, maximum 65536). An explicit FFT must satisfy the same lower bound. The causal circular shift adds FFT-size/2 frames; latencyFrames includes this and prior HRTF sample-rate-conversion latency. For example, 2048 FFT frames add 1024 frames: 23.22 ms at 44.1 kHz. This is a processing-latency tradeoff, not a zero-latency algorithm. Very long delayed banks exceeding the FFT bound fail explicitly; the legacy projection remains a separate choice.

maximumStorageBytes (default 512 MiB) bounds estimated designer-owned matrices, spectra and FIR buffers; it is not a cap on the complete renderer, typed filter copies, convolver storage or the whole process. Dense grids, large FFTs and high orders increase preparation work substantially. Audio rendering reuses the prepared MIMO convolver; process/reset/setRotation allocate no memory. Serialize updates with processing on one instance.

designReport reports FFT size, FIR length, total declared latency and relative weighted RMS errors. lowBandComplexError measures complex error below the transition. highBandMagnitudeError measures magnitude error above it; initialHighBandMagnitudeError is the spectral LS baseline for the same design data. These are dimensionless errors over design directions and non-endpoint FFT bins, before float filter quantization, not speed ratios, listening scores or independent validation. spectralMetricsAvailable is false for the legacy overload: its zero-initialized error fields are not measurements. MagLS is not an individualized HRTF or a guarantee against coloration; evaluate actual content and listener motion.
''',page['body'][1]+r'''
## Choisir la conception des filtres binauraux {#design}

L’appel existant prepare(spec, order, bank, regularization, backend) conserve la projection temporelle par moindres carrés. Il reste utile pour la compatibilité et une faible latence ajoutée. Aux faibles ordres, projeter la phase des HRIR dépendante de la direction peut perdre de l’énergie dans les aigus ; augmenter le gain ne corrige pas toute la coloration directionnelle.

La surcharge explicite HoaDecoderDesign ajoute les moindres carrés spectraux pondérés et MagLS. L’implémentation est du C++ MIT original fondé sur l’[objectif publié](https://pub.dega-akustik.de/DAGA_2018/data/articles/000301.pdf), avec les FFT et MatrixConvolver CyDSP. Aucun code IEM/SPARTA ni nouvelle dépendance n’est importé.

~~~cpp
#include <cydsp/spatial/SpatialRendering.h>
cydsp::HoaDecoderDesign design;
design.method = cydsp::HoaDecoderMethod::magLS;
design.crossoverHz = 2000;
design.transitionHz = 500;
design.maxRe = false;
cydsp::HoaBinaural<float> renderer;
const auto status = renderer.prepare({48000, 256, 4}, 1, bank, design);
if (status != cydsp::Status::ok) return status;
const auto report = renderer.designReport();
~~~

Sous la transition, l’ajustement utilise la réponse HRTF complexe, avec les délais d’arrivée propres aux directions et oreilles. Dans la transition il mélange erreurs complexe et d’amplitude ; au-dessus, des projections alternées ajustent les amplitudes avec une initialisation de phase continue entre fréquences. Les itérations sont bornées (12 par défaut, 1–256) ; la solution est locale, pas un optimum global garanti. La bande par défaut va de 1750 à 2250 Hz ; choisissez-la selon contenu, ordre et auditeur. Relâcher la phase aiguë conserve l’ajustement LS grave, pas tous les indices perceptifs ni la résolution spatiale exacte.

La conception spectrale utilise tous les points fournis avec leurs poids ; le nombre de voisins du renderer direct et la longueur de son noyau de délai ne pilotent pas cette projection. Les délais fractionnaires sont évalués spectralement aux bins de conception.

Une ITD appartient à une direction. Avec plusieurs directions mélangées, un délai unique par oreille en sortie ne restitue pas toutes les ITD d’origine. La conception conserve les cibles complexes graves et relâche la phase aiguë ; elle ne prétend pas fournir une opération universelle de retrait/réinjection d’ITD.

maxRe atténue en option les degrés harmoniques aigus par les poids de Legendre max-rE. Il élargit la réponse angulaire et peut modifier l’ajustement d’amplitude/niveau. Ce réglage distinct de MagLS vaut OFF par défaut et ne remplace pas l’optimisation d’amplitude aiguë.

Les poids spatiaux sont vides pour une pondération égale, ou contiennent un poids non négatif par point de la banque. Ils sont normalisés en interne. Utilisez des poids d’angle solide adaptés aux grilles mesurées irrégulières. La régularisation agit sur cette matrice de Gram normalisée, indépendamment du nombre de points ; sa valeur n’est pas numériquement identique à la régularisation non normalisée de l’ancienne surcharge.

## Latence, mémoire et diagnostics {#design-limits}

La conception spectrale s’exécute seulement dans prepare, sur worker/éditeur. Elle conserve toute la FIR conçue, sans troncature silencieuse. La taille FFT automatique est la puissance de deux suivante au moins égale au double de la HRIR complète avec délais (minimum 32, maximum 65536). Une taille explicite doit respecter la même borne basse. Le décalage circulaire causal ajoute taille-FFT/2 frames ; latencyFrames inclut cela et la latence de conversion de fréquence de la HRTF. Par exemple, une FFT de 2048 ajoute 1024 frames : 23,22 ms à 44,1 kHz. C’est un compromis de latence de traitement, pas un algorithme à latence nulle. Une banque trop longue pour la borne FFT échoue explicitement ; la projection historique reste un choix séparé.

maximumStorageBytes (512 Mio par défaut) borne l’estimation des matrices, spectres et FIR du concepteur ; il ne plafonne pas tout le renderer, les copies de filtres typés, la convolution ou le processus. Grilles denses, grandes FFT et ordres élevés alourdissent nettement la préparation. Le rendu audio réutilise le convolver MIMO préparé ; process/reset/setRotation n’allouent pas. Sérialisez paramètres et traitement d’une même instance.

designReport fournit FFT, longueur FIR, latence totale déclarée et erreurs RMS relatives pondérées. lowBandComplexError mesure l’erreur complexe sous la transition. highBandMagnitudeError mesure l’erreur d’amplitude au-dessus ; initialHighBandMagnitudeError donne la référence LS spectrale sur les mêmes données. Ce sont des erreurs sans unité sur les directions de conception et bins FFT hors extrémités, avant quantification des filtres float : ni rapports de vitesse, ni notes d’écoute, ni validation indépendante. spectralMetricsAvailable vaut false pour l’ancienne surcharge : ses champs d’erreur initialisés à zéro ne sont pas des mesures. MagLS ne personnalise pas la HRTF et ne garantit pas l’absence de coloration ; évaluez contenus et mouvements d’auditeur.
''')
    if page['id']=='hrtf':
        page['body']=(page['body'][0]+r'''
## Generate analytic HRIRs and a grid {#analytic}

sphericalFibonacci fills caller-owned direction and optional solid-angle spans, without allocation. Directions use the CyDSP listener frame; weights are equal, sum to 4π and only approximate spherical integration. This is neither an exact spherical t-design nor a Lebedev rule. Use a sufficiently dense grid or supply a different grid/weights for precise quadrature.

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

analyticHrirs excites the existing AnalyticBinaural processor with unit impulses and copies the selected finite FIRs into an owning HrirBank<float/double>. Each point includes its acoustic delay in the FIR and has zero separate delay. HrtfSet can subsequently separate integer onsets during its usual preparation; do not add the same delays twice.

The owning bank retains the sample rate, taps and views. Views survive moving the owner, and remain valid until it is destroyed or successfully replaced. Failed generation preserves the previous bank. Generation allocates, so run it outside audio. The maximum 100000 directions/65536 taps are also constrained by a default 512 MiB sample/view budget.

The model is the existing generic head-shadow/ITD approximation, named AnalyticBinaural in this repository. There is no CuedBinaural class here, individualized ear/pinna geometry or measured elevation detail. Increasing taps does not create such cues. maximumTailSample inspects the next 64 output frames beyond the retained FIR: it is a diagnostic, not a bound on all omitted energy. Choose taps for the model/rate and check the tail.
''',page['body'][1]+r'''
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
''')
