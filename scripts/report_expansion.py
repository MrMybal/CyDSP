"""Render the ten-lot validation ledger from fresh, successful local reports."""
import json
from datetime import datetime, timezone
from pathlib import Path
from reporting import source_digest
ROOT=Path(__file__).resolve().parents[1]
digest=source_digest()
checks={
    'Windows x64 MSVC + mesures':'native-x64.json',
    'Windows x64 clang-cl':'native-x64-clang.json',
    'Windows x64 sans SIMD explicite':'native-scalar.json',
    'Windows x64 AddressSanitizer':'native-x64-asan.json',
    'Android ARM64 + SVE, compilation/liaison':'android-arm64-v8a/report.json',
    'Quest 3 ARM64 scalaire/NEON + mesures':'device-arm64-v8a.json',
    'Windows ARM64, compilation/en-têtes/objets':'expansion-arm64.json',
    'Package installé + consommateur indépendant':'expansion-install-check.json',
    'Isolation ISA x64':'expansion-simd-x64.json',
    'Isolation ISA Windows ARM64':'expansion-simd-winarm64.json',
    'Isolation ISA Android ARM64/SVE':'expansion-simd-android.json',
}
reports={}
for title,name in checks.items():
    r=json.loads((ROOT/'build'/name).read_text(encoding='utf-8'))
    if r.get('status')!='passed' or r.get('sourceSha256')!=digest:
        raise SystemExit('Missing, failed or stale check: '+title)
    reports[title]=r
cpu=reports['Windows x64 MSVC + mesures'];arm=reports['Quest 3 ARM64 scalaire/NEON + mesures']
def performance(report,op,backend):
    return next(x for x in report['measurements'] if x.get('scope')=='expansion10' and x.get('operation')==op and x.get('sampleType')=='float' and x.get('backend')==backend)
def quality(report,name,type_):
    return next(x['value'] for x in report['measurements'] if x.get('scope')=='expansion10' and x.get('metric')==name and x.get('sampleType')==type_)
text=['# Validation des dix lots DSP / audio 3D','',
      'État source exact : `'+digest+'`.',
      'Rapports locaux datés ; synthèse produite le '+datetime.now(timezone.utc).strftime('%Y-%m-%d %H:%M UTC')+'.','',
      'Les dix lots sont implémentés et leurs régressions passent sur x64 et Quest ARM64. ',
      'Aucune application consommatrice n’a été modifiée. Aucun commit, push ou workflow ',
      'GitHub n’a été lancé pour cette extension. Les cinq adaptations tierces MIT restent ',
      'inchangées ; aucun nouveau composant ou jeu de HRIR n’est importé.','',
      '## Matrice de validation','',
      '| Vérification | Résultat |','| --- | --- |']
for title in checks:text.append('| '+title+' | Passé |')
text+=['| Linux/macOS desktop | Exécution différée par le propriétaire |',
       '| Windows ARM64 / SVE sur matériel compatible | Exécution non vérifiée |',
       '| Écoute, périphérique physique, intégration DAW/jeu | À faire dans les projets consommateurs |','',
       'Les vérifications couvrent float/double, oracles mathématiques, tailles de blocs, ',
       'préparation transactionnelle, déplacement, entrées invalides avant mutation et audits ',
       'allocation/libération des chemins temps réel. Les en-têtes et exemples sont compilés ',
       'séparément. Le Quest exécute NEON, sans SVE ; SVE est compilé/désassemblé uniquement. ',
       'Les exécutables temporaires du Quest sont retirés après mesure.','',
       '## Coût CPU de configurations représentatives','',
       'Microsecondes par appel de **128 frames à 48 kHz**, float : **moyenne / p99**. ',
       'Budget temporel d’un tel bloc : **2 666,67 µs**. Ryzen 9 7900X en AVX-512 et ',
       'Quest 3 en NEON, sauf les algorithmes explicitement scalaires. 48 appels de ',
       'préchauffage puis 128 mesures par cas. Une STFT effectue davantage de travail ',
       'sur ses blocs de hop : le p99 compte cette distribution. Aucun facteur global ',
       'de rapidité, latence matérielle ou nombre maximal de voix n’est déduit.','',
       '| Charge | Ryzen µs moyen / p99 | Quest µs moyen / p99 |',
       '| --- | ---: | ---: |']
cases=[('multitap16','16 taps Lagrange, stéréo',False),('firBank4x129','4 bandes FIR 129 taps, stéréo',False),
       ('fdn16Stereo','FDN 16 lignes, stéréo',False),('hrir128Stereo','HRIR 128 taps, mono → stéréo',False),
       ('hoa3EncodeRotateDecode8','HOA ordre 3 : encodage/rotation/décodage 8 enceintes',False),
       ('truePeakLimiter4x5ms','Limiteur 4x, anticipation 5 ms, stéréo',False),
       ('liveGrains64Sinc32','64 grains live sinc 32 taps, stéréo',False),
       ('fmPmSyncUnison8','FM/PM/sync, 8 voix unisson, stéréo',False),
       ('musicalAnalyzer2048','Analyse musicale N=2048, hop=256, stéréo',False),
       ('harmonizer4Sinc32','Harmoniseur 4 voix sinc, stéréo',False),
       ('denoiser1024','Denoise STFT 1024/hop256, stéréo',False),
       ('nlms64','NLMS 64 taps, stéréo',False),('declickMedian17','Declick médiane 17, stéréo, scalaire',True)]
for op,label,scalar in cases:
    a=performance(cpu,op,'scalar' if scalar else 'avx512');b=performance(arm,op,'scalar' if scalar else 'neon')
    text.append('| '+label+' | '+f"{a['meanNs']/1000:.2f} / {a['p99Ns']/1000:.2f}"+' | '+f"{b['meanNs']/1000:.2f} / {b['p99Ns']/1000:.2f}"+' |')
text+=['','Le cas live conserve 64 grains de 10 s, âge 0,3 s, ratio 1 et sorties stéréo ; ',
       'la capture est préremplie. Le cas HRIR utilise des FIR de synthèse, aucun jeu ',
       'mesuré tiers. Le cas harmoniseur utilise quatre voix : 0/7/12/0,08 demi-tons, ',
       'gain total 1, retard de doubleur 15 ms sur la dernière voix. Les données audio ',
       'sont synthétiques et les paramètres fixes ; ces coûts ne représentent pas toutes ',
       'les combinaisons, les allocations de préparation ou l’activité du moteur hôte.','',
       '## Qualité numérique','',
       '| Cas / unité | Ryzen float | Ryzen double | Quest float | Quest double |',
       '| --- | ---: | ---: | ---: | ---: |']
metrics=[('stft256ReconstructionMaxError','STFT256 : erreur absolue max (amplitude)'),
         ('yin440AbsoluteErrorHz','YIN440 : erreur absolue (Hz)'),
         ('yin440Confidence','YIN440 : confiance (0–1)'),
         ('hoa3RotationMaxError','HOA3 : erreur max de rotation (coefficient double)'),
         ('knownOnePercentThdError','THD connue 1 % : erreur absolue (rapport RMS)')]
for key,label in metrics:
    values=[quality(r,key,t) for r in (cpu,arm) for t in ('float','double')]
    text.append('| '+label+' | '+' | '.join(f'{v:.6g}' for v in values)+' |')
text+=['','La confiance YIN est celle de ce sinus, pas une probabilité de réussite sur un ',
       'morceau musical. Les coefficients HOA sont calculés en double pour les deux API. ',
       'Les régressions supplémentaires vérifient notamment les IIR BP/BS cinq familles ',
       'et ordres 1/3/4/8/16, la reconstruction N bandes, PDC/bypass/transitions, la ',
       'propagation/Doppler, les chemins/portails, les impulsions HRIR fractionnaires, ',
       'le plafond du limiteur sur un sinus, la durée de persistance des résonances, ',
       'l’identification FIR NLMS, la réduction d’alias d’un décimateur, les harmoniques ',
       'THD non cohérentes et la déconvolution d’une réponse connue.','',
       '## Optimisation de l’historique partagé','',
       'Une première paire locale avant/après a identifié les lectures répétées comme ',
       'goulot commun aux grains, HRIR et harmoniseur. `CircularHistory::readWindow` ',
       'regroupe les lectures entières ; les FIR réutilisent les dots SIMD et la même ',
       'interpolation sinc. Les tests indépendants vérifient wrap, passé manquant, futur ',
       'et convolution HRIR fractionnaire. Les coûts actuels figurent dans le tableau ',
       'ci-dessus ; une paire isolée ne constitue pas un facteur de gain général.','',
       '## Reproduire et intégrer','',
       'Commandes natives explicitement autorisées, depuis le dépôt :','',
       '```powershell',
       'python scripts/run_native.py --build-dir build/expansion-x64 --measure',
       'python scripts/run_native.py --compiler clang --build-dir build/expansion-clang',
       'python scripts/run_native.py --arch scalar --build-dir build/expansion-scalar',
       'python scripts/run_native.py --sanitize --build-dir build/expansion-asan',
       'python scripts/build_android.py --abi arm64-v8a --measure',
       'python scripts/run_android_device.py --abi arm64-v8a --measure',
       'python scripts/verify_sources.py',
       '```','',
       'Rapports bruts dans `build/native-*.json`, `build/device-arm64-v8a.json` et ',
       '`build/expansion-*.json`, avec le SHA-256 des entrées de compilation. Les sources ',
       'ne lancent pas ces programmes automatiquement. Voir le [guide des API et limites](ExpansionGuide.fr.md) ',
       'et les [exemples compilés](../examples/SpatialAndDaw.cpp).']
(ROOT/'docs/ExpansionValidation.fr.md').write_text('\n'.join(line.rstrip() for line in text)+'\n',encoding='utf-8',newline='\n')
(ROOT/'build/expansion-validation.json').write_text(json.dumps({'sourceSha256':digest,'status':'passed','checks':checks},indent=2)+'\n',encoding='utf-8')
print('PASS: fresh ten-lot validation reports; '+str(len(checks))+' checks')
