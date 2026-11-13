"""Generate second-extension validation from fresh successful local evidence."""
from datetime import datetime, timezone
import json
from pathlib import Path
from reporting import source_digest
ROOT=Path(__file__).resolve().parents[1]
digest=source_digest()
checks={
 'Windows x64 MSVC + mesures':'native-x64.json',
 'Windows x64 clang-cl':'native-x64-clang.json',
 'Windows x64 scalaire':'native-scalar.json',
 'Windows x64 AddressSanitizer':'native-x64-asan.json',
 'Android ARM64/SVE compilation et liaison':'android-arm64-v8a/report.json',
 'Quest 3 ARM64/NEON exécution + mesures':'device-arm64-v8a.json',
 'Windows ARM64 en-têtes et objets':'advanced-arm64.json',
 'Installation statique + consommateur indépendant':'advanced-install-check.json',
 'Isolation ISA x64':'advanced-simd-x64.json',
 'Isolation ISA Windows ARM64':'advanced-simd-winarm64.json',
 'Isolation ISA Android/SVE':'advanced-simd-android.json',
}
reports={}
for label,name in checks.items():
 r=json.loads((ROOT/'build'/name).read_text(encoding='utf-8'))
 if r.get('status')!='passed' or r.get('sourceSha256')!=digest:raise SystemExit('Failed or stale: '+label)
 reports[label]=r
cpu=reports['Windows x64 MSVC + mesures'];arm=reports['Quest 3 ARM64/NEON exécution + mesures']
def entries(report):return [x for x in report['measurements'] if x.get('scope')=='advancedExpansion10']
def perf(report,op,backend):return next(x for x in entries(report) if x.get('operation')==op and x.get('backend')==backend and x.get('sampleType')=='float')
def quality(report,key,type_):return next(x['value'] for x in entries(report) if x.get('metric')==key and x.get('sampleType')==type_)
text=['# Validation de la deuxième extension : dix lots','',
 'État exact des sources compilées : `'+digest+'`.',
 'Synthèse produite le '+datetime.now(timezone.utc).strftime('%Y-%m-%d %H:%M UTC')+'.','',
 'Les dix lots sont implémentés : [API, contrats et limites](AdvancedExpansionGuide.fr.md).',
 'Aucun autre projet modifié, aucun commit/push ou workflow GitHub lancé pour ces lots.',
 'Les cinq adaptations MIT et les notices restent inchangées. Aucun jeu de HRIR tiers importé.','',
 '## Validation','', '| Vérification | Résultat |','| --- | --- |']
for label in checks:text.append('| '+label+' | Passé |')
text+=['| Linux/macOS desktop | Exécution différée par le propriétaire |',
 '| Windows ARM64 natif et SVE sur matériel compatible | Exécution non vérifiée |',
 '| Écoute et intégration DAW/jeu/périphériques | À réaliser dans les projets consommateurs |','',
 'Régressions float/double : rotation HOA7 covariante et orthogonale, dispositions irrégulières,',
 'projection binaurale isotrope, seuil/onset HRIR, gain DC SRC, fenêtres/positions des grains,',
 'sinus groupés contre formule analytique, complémentarité HPSS, pitch/stéréo/formants,',
 'impulsion mécanique, identité FIR AEC et gel des coefficients en double parole, profils/fondus.',
 'Les contrôles supplémentaires vérifient préparation transactionnelle, entrées invalides',
 'avant mutation, blocs de tailles différentes, blocs vides, déplacements et audits',
 'allocation/libération des chemins temps réel. Les en-têtes et exemples sont compilés séparément.',
 'Le Quest exécute NEON, pas SVE ; les fichiers temporaires sont retirés après les tests.','',
 '## Mesures CPU','',
 'Float, **128 frames à 48 kHz**, microsecondes par appel : **moyenne / p99**.',
 'Budget d’un bloc : **2 666,67 µs**. 48 appels de préchauffage, puis 128 mesures.',
 'Les cas spectraux travaillent par hops : leur p99 inclut ces pics. Mesures séparées',
 'des compilations et de la préparation, sur données synthétiques. Les coûts de l’hôte,',
 'des périphériques, de l’allocation et des changements de graphe ne sont pas inclus.','',
 '| Cas | Ryzen 9 7900X, AVX-512 | Quest 3, NEON |','| --- | ---: | ---: |']
cases=[('sineBatch128Stereo','128 sinus groupés → stéréo'),('individualSine128Stereo','128 oscillateurs individuels + même mixage'),
 ('sharedSampleGrains64','64 grains sur une banque de samples partagée'),('movingHrtf32x128','HRTF : 32 directions × 128 taps, direction/fondu à chaque bloc'),
 ('hoa3Binaural32Hrir128','Binaural HOA3 : 32 HRIR, 128 taps'),('hoa7EncodeRotateDecode24','HOA7 : encodage/rotation/décodage 24 enceintes'),
 ('hpss1024x17Stereo','HPSS stéréo N1024, médianes 17'),('spectralPitch1024FormantsStereo','Pitch stéréo N1024, ratio 1,5, formants'),
 ('aec512TwoMicrophones','AEC 512 taps, 2 microphones')]
for key,label in cases:
 values=[perf(r,key,b) for r,b in [(cpu,'avx512'),(arm,'neon')]]
 text.append('| '+label+' | '+' | '.join(f"{v['meanNs']/1000:.2f} / {v['p99Ns']/1000:.2f}" for v in values)+' |')
for r,b,label in [(cpu,'avx512','Ryzen'),(arm,'neon','Quest')]:
 factor=perf(r,'individualSine128Stereo',b)['meanNs']/perf(r,'sineBatch128Stereo',b)['meanNs']
 text+=['',f'**{label} : {factor:.2f}×** signifie que le temps moyen des oscillateurs individuels',
        'est divisé par celui du lot de 128 sinus avec le même routage. Ce facteur concerne',
        'uniquement ce cas, ces paramètres et ce matériel ; ce n’est pas une accélération globale.']
text+=['','Les grains partagent une boucle de 32768 samples, 64 fenêtres Hann de 10 s,',
 'ratios alternés −1 et 1,5. Les HRIR sont synthétiques et ne mesurent pas une HRTF humaine.',
 'Le cas HOA7 utilise 64 canaux et 24 enceintes irrégulières régularisées.',
 'L’AEC reçoit deux microphones synthétiques et une référence mono ; sa charge dépend aussi',
 'de l’état d’adaptation. Contact archet (mono, scalaire) :']
values=[perf(r,'bowedContactMono','scalar') for r in (cpu,arm)]
text+=['Ryzen '+f"{values[0]['meanNs']/1000:.2f} / {values[0]['p99Ns']/1000:.2f}"+' µs ; Quest '+f"{values[1]['meanNs']/1000:.2f} / {values[1]['p99Ns']/1000:.2f}"+' µs (moyenne / p99).','',
 '## Qualité numérique','', '| Métrique et unité | Ryzen float | Ryzen double | Quest float | Quest double |','| --- | ---: | ---: | ---: | ---: |']
metrics=[('hoa7RotationMaxError','HOA7 : erreur max de coefficient'),('hpssReconstructionMaxError','HPSS : erreur absolue max en amplitude'),
 ('analyticRt60ErrorSeconds','RT60 exponentiel connu : erreur en secondes'),('analyticRt60FitRSquared','RT60 : R², 0 à 1'),
 ('vowelEnvelopeErrorWithoutRetention','Voyelle synthétique : erreur RMS normalisée sans conservation'),
 ('vowelEnvelopeErrorWithRetention','Même voyelle : erreur RMS normalisée avec conservation')]
for key,label in metrics:
 values=[quality(r,key,t) for r in (cpu,arm) for t in ('float','double')]
 text.append('| '+label+' | '+' | '.join(f'{v:.6g}' for v in values)+' |')
text+=['','La voyelle est une somme de 60 harmoniques, fondamentale 62,5 Hz à 16 kHz,',
 'avec deux enveloppes gaussiennes centrées sur 600 et 1600 Hz. Transposition d’une octave ;',
 'comparaison de 24 amplitudes harmoniques à l’enveloppe cible, après normalisation du gain.',
 'Cette amélioration quantifie ce signal, sans promettre de transparence sur toute voix/musique.',
 'Les coefficients HOA sont en double dans les deux API.',
 'La régression AEC exige au moins 60 dB de réduction pour un FIR connu, une référence',
 'synchronisée, puis le gel des coefficients pendant une parole proche synthétique forte.',
 'Elle ne remplace pas une mesure avec haut-parleur, microphones, dérives et non-linéarités réels.','',
 '## Reproduction','', 'Exécutions explicites, sans GitHub Actions :','', '```powershell',
 'python -X utf8 -B scripts/run_native.py --build-dir build/expansion-x64 --measure',
 'python -X utf8 -B scripts/run_native.py --compiler clang --build-dir build/expansion-clang',
 'python -X utf8 -B scripts/run_native.py --arch scalar --build-dir build/expansion-scalar',
 'python -X utf8 -B scripts/run_native.py --sanitize --build-dir build/expansion-asan',
 'python -X utf8 -B scripts/build_android.py --abi arm64-v8a --measure',
 'python -X utf8 -B scripts/run_android_device.py --abi arm64-v8a --measure',
 'python -X utf8 -B scripts/build_arm64.py --build-dir build/expansion-arm64 --compile-checks',
 'python -X utf8 -B scripts/validate_advanced_package.py',
 '```','',
 'Les rapports JSON locaux `build/native-*.json`, `build/device-arm64-v8a.json`,',
 '`build/advanced-*.json` et `build/android-arm64-v8a/report.json` portent l’empreinte des',
 'sources. Le script `report_advanced_expansion.py` refuse un rapport absent, périmé ou en échec.',
 'Les rapports d’isolation SIMD sont archivés immédiatement après chaque vérification',
 '`verify_simd.py` car ce vérificateur utilise un nom de sortie commun.']
(ROOT/'docs/AdvancedExpansionValidation.fr.md').write_text('\n'.join(line.rstrip() for line in text)+'\n',encoding='utf-8',newline='\n')
(ROOT/'build/advanced-validation.json').write_text(json.dumps({'sourceSha256':digest,'timeUtc':datetime.now(timezone.utc).isoformat(),'status':'passed','checks':checks},indent=2)+'\n',encoding='utf-8')
p=ROOT/'docs/AdvancedExpansionPlan.fr.md';content=p.read_text(encoding='utf-8').replace('En cours','Implémenté et validé').replace('À réaliser','Implémenté et validé');content=content.replace('mesures x64/Quest, compilation ARM64/SVE et installation indépendante à actualiser\naprès stabilisation des sources.', 'mesures x64/Quest, compilation ARM64/SVE et installation indépendante réalisés :\nvoir [les résultats](AdvancedExpansionValidation.fr.md).');p.write_text(content,encoding='utf-8',newline='\n')
print('PASS: all 11 fresh second-extension validation reports')
