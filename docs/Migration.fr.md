# Migration vers CyDSP + CyAudioCore

CyDSP 0.2.0 est la couche numérique autonome. CyAudioCore 0.1.0 consomme son API
publique pour les services audio. Ce découpage modifie les headers, namespaces et
cibles ; recompiler les deux bibliothèques et les consommateurs avec le même ABI.
Aucun fichier de CyDAW ou CyUI n'est modifié par cette extraction.

| Avant | Après |
| --- | --- |
| CyDSP::CyDSP, CyDSP::Stretch | Inchangés pour les traitements numériques |
| CyDSP::Files / Devices / Sources / Plugins | CyAudioCore::Files / Devices / Sources / Plugins |
| Services réunis dans le cœur CyDSP | CyAudioCore::Core ; CyAudioCore::CyAudioCore agrège les modules activés |
| cydsp/io, midi, sources, plugins | cyaudio/io, midi, sources, plugins |
| cydsp::Synthesizer / Instrument / SynthParameters | cyaudio::Synthesizer / Instrument / SynthParameters |
| cydsp::SampleData / Wavetable / WavetableOscillator | Restent dans CyDSP |
| cydsp::AudioBlock / AudioBuffer / Status / ProcessSpec | Types CyDSP inchangés, également accessibles par cyaudio |
| CYDSP_BUILD_FILES / DEVICES / SOURCES / PLUGINS | CYAUDIOCORE_BUILD_FILES / DEVICES / SOURCES / PLUGINS |
| cydsp/CyDSP.h pour tous les services | cyaudio/CyAudioCore.h pour les services ; cydsp/CyDSP.h pour le DSP |

## Intégration par sources

```cmake
# Dans le CMakeLists de l'application, chemins adaptés à ses checkouts.
add_subdirectory(path/to/CyDSP)
add_subdirectory(path/to/CyAudioCore)
target_link_libraries(MyApplication PRIVATE CyAudioCore::CyAudioCore)
```

Autre possibilité : fournir CYAUDIOCORE_CYDSP_SOURCE_DIR lors de la configuration
de CyAudioCore. Aucun téléchargement implicite ou chemin absolu de développeur
n'est codé dans la bibliothèque. Sans cible préexistante ni chemin explicite,
CyAudioCore recherche un paquet CyDSP >= 0.2.0 installé.

```cmake
find_package(CyAudioCore 0.1 CONFIG REQUIRED COMPONENTS Plugins Sources)
target_link_libraries(MyApplication PRIVATE CyAudioCore::Plugins CyAudioCore::Sources)
```

Installer CyDSP puis CyAudioCore dans un préfixe commun, ou fournir les deux
préfixes à CMAKE_PREFIX_PATH. Le paquet exporté conserve find_dependency(CyDSP).
La cible d'agrégation est INTERFACE ; tous les artefacts de production sont des
archives statiques. Les exécutables, modules de fixture et tests restent opt-in.

## GUI et isolation

CyAudioCore ne dépend d'aucun toolkit graphique. CyUI reste une dépendance de
l'application. PluginWindow reçoit un parent natif ; tailles, DPI, focus,
paramètres/gestes et événements sont gérés par les contrats publics existants.
L'application conserve sa boucle d'événements et ses responsabilités de thread UI.

PluginWorker et runPluginWorker appartiennent à CyAudioCore::Plugins. L'application
fournit son exécutable worker et choisit quelles instances isoler. Le protocole
actuel représente une instance par worker ; il n'implémente pas un rack de plusieurs
plugins dans un seul worker. L'audio utilise une boîte mémoire bornée sans attente,
avec un bloc de latence supplémentaire et silence pour les réponses tardives.
Ce mécanisme isole les plantages ; il n'est pas une sandbox contre du code hostile.

## Rapport CyDAW du 12 novembre 2026

CYDSP-010 : getState du contrôleur VST3 accepte kNotImplemented uniquement si aucun
octet n'a été écrit et aucune écriture n'a échoué. Les autres erreurs restent des
échecs. CYDSP-011 : une lecture IBStream partielle non vide retourne succès avec
son nombre réel d'octets ; EOF vide retourne faux, et une lecture nulle réussit.
Les tests couvrent les états avec/sans contrôleur sauvegardable, les vraies erreurs,
les octets partiels interdits, les limites et le chemin direct/worker.

Les identifiants persistants sont conservés : CyDSP.VST3State1, CyJSFX version1,
CyDSPInstrument version1 et le protocole worker existant. Ils ne sont pas renommés
avec le projet. Cela préserve les états produits par l'ancien CyDSP. Les enveloppes
historiques JUCE/CyDAW restent distinctes et doivent être converties côté application,
sans importer de code JUCE ici. Les index d'automation de l'application doivent
être associés aux IDs stables exposés par le plugin.

Les programmes/unités VST3 ne disposent pas encore d'un contrat public complet.
Leur adaptation ne doit pas supprimer les presets existants. Le protocole IPC CyDAW
v6 n'est pas interchangeable avec le worker de cette bibliothèque. Wine, les
conversions d'anciens projets, la compensation de latence du DAW et sa migration
fenêtres/modèle/source relèvent de son intégration. Aucune migration du DAW ni
compatibilité universelle des plugins n'est revendiquée par ce découpage.

## Licences et vérification

Chaque projet conserve son LICENSE MIT original et son manifeste de provenance.
CyDSP n'embarque plus les codecs/SDK de plugins/périphériques. CyAudioCore conserve
les notices exactes de ces composants et dépend des notices DSP quand il le distribue.
Les anciennes mesures de la bibliothèque monolithique ne valident pas les nouveaux
paquets. Voir Validation.md pour les vérifications réellement effectuées.
