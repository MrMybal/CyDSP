"""Explicit native Linux/macOS validation entry point; run on that target host."""
import argparse
from datetime import datetime,timezone
import json
from pathlib import Path
import platform
import shutil
import subprocess
from reporting import source_digest
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--cmake',default=shutil.which('cmake'))
p.add_argument('--measure',action='store_true')
p.add_argument('--sanitize',action='store_true')
a=p.parse_args()
if platform.system() not in ('Linux','Darwin'):p.error('Run on a Linux/macOS host; this does not emulate another OS')
if not a.cmake:p.error('CMake 3.24+ must be installed on the target host')
folder=ROOT/'build'/('native-'+platform.system().lower()+('-asan' if a.sanitize else ''));folder.mkdir(parents=True,exist_ok=True)
report={'sourceSha256':source_digest(),'system':platform.platform(),'timeUtc':datetime.now(timezone.utc).isoformat(),'status':'running'}
def run(name,command):
    result=subprocess.run(command,cwd=ROOT,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,timeout=600)
    (folder/(name+'.log')).write_text(result.stdout,encoding='utf-8')
    if result.returncode:raise RuntimeError(name+': '+result.stdout[-6000:])
    return result.stdout
try:
    run('configure',[a.cmake,'-S',str(ROOT),'-B',str(folder),'-DCMAKE_BUILD_TYPE=Release','-DCYDSP_BUILD_TESTS=ON',
        '-DCYDSP_COMPILE_CHECKS=ON','-DCYDSP_ENABLE_SIMD=ON','-DCYDSP_BUILD_MEASUREMENTS='+('ON' if a.measure else 'OFF'),
        '-DCYDSP_SANITIZE_ADDRESS='+('ON' if a.sanitize else 'OFF')])
    run('build',[a.cmake,'--build',str(folder),'--parallel','6'])
    report['regressions']=run('regressions',[str(folder/'cydsp_tests')])
    if a.measure:report['measurements']=json.loads(run('measurements',[str(folder/'cydsp_measurements')]))
    report['status']='passed'
except Exception as error:report.update(status='failed',error=str(error))
(folder/'report.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
if report['status']!='passed':raise SystemExit(report['error'])
print('PASS: '+report['system'])
