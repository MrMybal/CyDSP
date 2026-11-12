"""Scan a byte-exact snapshot of every nonignored candidate file with Gitleaks defaults."""
import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import uuid

ROOT = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--gitleaks', type=Path, default=ROOT/'build/tools/gitleaks-8.30.1/gitleaks.exe')
a = p.parse_args()
scanner = a.gitleaks.resolve(strict=True)
folder = ROOT/'build/security'/('expansion-'+uuid.uuid4().hex)
candidate = folder/'candidate'
candidate.mkdir(parents=True)
paths = sorted(p.relative_to(ROOT).as_posix() for p in ROOT.rglob('*') if p.is_file() and not any(part in ('build','.git','__pycache__','out','.vs','comparisons') for part in p.relative_to(ROOT).parts))

digest = hashlib.sha256()
count = 0
for name in paths:
    source = (ROOT/name).resolve()
    if not source.is_relative_to(ROOT):
        raise SystemExit('Candidate escapes repository: '+name)
    if not source.is_file():  # Deleted tracked files are absent from the candidate.
        continue
    destination = candidate/name
    destination.parent.mkdir(parents=True,exist_ok=True)
    shutil.copyfile(source,destination)
    data = destination.read_bytes()
    digest.update(name.encode('utf-8')+b'\0'+data+b'\0')
    count += 1
config = folder/'defaults.toml'
config.write_text('[extend]\nuseDefault=true\n',encoding='utf-8',newline='\n')
version = subprocess.check_output([str(scanner),'version'],text=True).strip()
result = subprocess.run([str(scanner),'dir',str(candidate),'--config',str(config),
    '--redact=100','--no-banner','--report-format','json','--report-path',str(folder/'findings.json')],
    stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,cwd=ROOT)
(folder/'scan.log').write_text(result.stdout,encoding='utf-8',newline='\n')
findings = json.loads((folder/'findings.json').read_text(encoding='utf-8')) if (folder/'findings.json').is_file() else None
report = {'timeUtc':datetime.now(timezone.utc).isoformat(),'tool':'Gitleaks','version':version,
    'status':'passed' if result.returncode==0 and findings==[] else 'failed',
    'files':count,'findings':len(findings) if findings is not None else None,
    'candidateSha256':digest.hexdigest(),'candidate':candidate.as_posix(),
    'defaultRules':True,'customAllowlists':False,'historyScanned':False}
(folder/'report.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8',newline='\n')
(ROOT/'build/security-latest.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8',newline='\n')
print(json.dumps({k:v for k,v in report.items() if k!='candidate'},indent=2))
if report['status']!='passed':
    raise SystemExit('Secret scan failed; inspect the redacted findings report.')
