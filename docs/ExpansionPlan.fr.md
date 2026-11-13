# Extension CyDSP — dix lots autorisés

Travail demandé le 12 novembre 2026. Implémentations originales MIT et composants
DSP tiers MIT vérifiés uniquement. CyDAW/CyMetaSound servent de références de
besoins ; leurs dépôts ne sont pas modifiés. Les fonctionnalités ci-dessous ont
été implémentées ; la matrice de validation finale est tenue séparément. Le socle existant, dont la
première granulation et les analyseurs, est conservé.

| Lot | Périmètre accepté | État |
| --- | --- | --- |
| 1 | Historique circulaire, lecteurs indépendants, multitap, interpolation Lagrange/Catmull-Rom/Thiran, événements/rampes de paramètres, descripteurs stables | Implémenté ; régressions x64 passées, validation commune passée ; voir le rapport final |
| 2 | STFT analyse/synthèse, N bandes, IIR BP/BS d’ordre élevé, EQ FIR/phase minimale, Hilbert, diffuseurs et matrices de feedback | Implémenté ; régressions x64 passées, validation commune passée ; voir le rapport final |
| 3 | Graphe multibus/sidechain, latences/queues, compensation, bypass aligné, transitions entre graphes préparés, buffers réutilisés | Implémenté ; régressions x64 passées, validation commune passée ; voir le rapport final |
| 4 | Coordonnées/poses/vitesses, mouvements, distance/étendue, directivité, air/eau/milieux, propagation et Doppler, plusieurs auditeurs | Implémenté ; régressions x64 passées, validation commune passée ; voir le rapport final |
| 5 | Chemins/réflexions bornés avec transitions, transmission/occlusion multibandes, FDN par bandes, hybride IR/FDN, bus de pièce/queues, données pièces/portails | Implémenté ; régressions x64 passées, validation commune passée ; voir le rapport final |
| 6 | Binaural analytique, HRIR fournies par l’hôte/interpolation, sources étendues/stéréo, VBAP, Ambisonics encodage/rotation/décodage | Implémenté ; régressions x64 passées, validation commune passée ; voir le rapport final |
| 7 | Sidechain dynamique, EQ dynamique, compresseur/saturateur multibandes, de-esser, transient shaper, limiteur true peak, ADAA/wavefolder/asymétrie/hystérésis | Implémenté ; régressions x64 passées, validation commune passée ; voir le rapport final |
| 8 | Délais/modulations, Dattorro/ressort/shimmer/freeze, effets spectraux/vocodeur, granulaire live, FM/PM/sync/morphing/unisson, MSEG/S&H, générateurs procéduraux | Implémenté ; régressions x64 passées, validation commune passée ; voir le rapport final |
| 9 | Fondamentale/confiance, attaques/tempo/chroma/MFCC/tonalité, correction mono/harmoniseur/doubleur, Karplus–Strong/modal/guides d’ondes/cordes/tubes/membranes/plaques | Implémenté ; régressions x64 passées, validation commune passée ; voir le rapport final |
| 10 | Denoise/dehum/declick/résonances, LMS/NLMS, bancs multirates, réponse/phase/retard/cohérence, THD/THD+N, sweeps/déconvolution | Implémenté ; régressions x64 passées, validation commune passée ; voir le rapport final |

Chaque lot conserve float/double, préparation transactionnelle, traitement/reset
sans allocation/libération/verrou/E/S, limites explicites et réutilisation des
primitives existantes. Les modules doivent avoir des vérifications numériques,
de cycle de vie, de mémoire et de temps réel. Les mesures correspondent à des
charges documentées, jamais à un gain général supposé. Windows x64/Quest ARM64
peuvent être exécutés ; Linux/macOS restent différés par le propriétaire.

## Journal de réalisation

- État initial : modifications locales de granulation/analyse déjà validées,
  conservées. Aucun commit/push automatique ni workflow GitHub demandé pour cette extension.

- Fondations : références historiques, multitap/Thiran, paramètres, STFT WOLA,
  banques FIR, IIR BP/BS, phase minimale, Hilbert et diffuseurs passent sur x64.

- Lot 3 : graphe multibus, PDC, bypass aligné et transitions avec préchauffage
  passent les tests d’impulsion/rampe x64 et les audits allocation/libération.

- Lots 4/5 : poses, milieux, matériaux, Doppler rendu, chemins acoustiques, portails, cellules, FDN et bus de pièce passent leurs régressions numériques x64.

- Lots 6–10 : rendu binaural/HOA, dynamique, création sonore, analyse musicale/physique et restauration/diagnostic implémentés ; régressions x64 passées. Guide d’intégration : [ExpansionGuide.fr.md](ExpansionGuide.fr.md).

- Validation finale : [ExpansionValidation.fr.md](ExpansionValidation.fr.md). Onze contrôles frais, dont x64, Quest ARM64, compilations ARM64/SVE, désassemblage ISA et consommateur installé.
