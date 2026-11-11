"""Explicit native execution on the owner's ADB-connected Android device."""
import argparse
from android_paths import android_adb
from datetime import datetime,timezone
import json
from pathlib import Path
import re
import subprocess
import uuid
from reporting import source_digest
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--abi',choices=['arm64-v8a','armeabi-v7a'],default='arm64-v8a')
p.add_argument('--adb',default=str(android_adb()))
p.add_argument('--serial')
p.add_argument('--measure',action='store_true')
a=p.parse_args()
folder=ROOT/'build'/('android-'+a.abi);digest=source_digest()
compiled=json.loads((folder/'report.json').read_text())
if compiled.get('status')!='passed' or compiled.get('sourceSha256')!=digest:p.error('Rebuild this Android ABI for the current sources first')
def invoke(arguments,timeout=30):
    result=subprocess.run([a.adb,*arguments],cwd=ROOT,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,timeout=timeout)
    if result.returncode:raise RuntimeError('ADB command failed ('+str(result.returncode)+'): '+result.stdout[-7000:])
    return result.stdout
serial=a.serial
if not serial:
    candidates=[line.split()[0] for line in invoke(['devices']).splitlines()[1:] if len(line.split())>1 and line.split()[1]=='device']
    if len(candidates)!=1:p.error('Select one connected device with --serial')
    serial=candidates[0]
base=['-s',serial]
model=invoke([*base,'shell','getprop','ro.product.model']).strip()
abis=invoke([*base,'shell','getprop','ro.product.cpu.abilist']).strip().split(',')
if a.abi not in abis:p.error('Device does not advertise this ABI: '+a.abi)
remote='/data/local/tmp/cydsp-'+digest[:12]+'-'+a.abi+'-'+uuid.uuid4().hex[:8]
if not re.fullmatch(r'/data/local/tmp/cydsp-[a-f0-9]{12}-[a-z0-9-]+-[a-f0-9]{8}',remote):raise RuntimeError('Unsafe temporary device path')
report={'sourceSha256':digest,'model':model,'abi':a.abi,'sdk':invoke([*base,'shell','getprop','ro.build.version.sdk']).strip(),
        'timeUtc':datetime.now(timezone.utc).isoformat(),'status':'running','nativeRuntimeExecuted':True,'installedApplication':False}
logs=ROOT/'build/device-logs';logs.mkdir(parents=True,exist_ok=True)
files=['cydsp_tests']+(['cydsp_measurements'] if a.measure else [])
try:
    invoke([*base,'shell','mkdir',remote])
    for name in files:
        if not (folder/name).is_file():raise RuntimeError('Missing executable '+name)
        invoke([*base,'push',str(folder/name),remote+'/'+name])
        invoke([*base,'shell','chmod','700',remote+'/'+name])
    text=invoke([*base,'shell',remote+'/cydsp_tests'],240)
    (logs/(a.abi+'-regressions.log')).write_text(text,encoding='utf-8');report['regressions']=text
    if a.measure:
        text=invoke([*base,'shell',remote+'/cydsp_measurements'],600)
        (logs/(a.abi+'-measurements.log')).write_text(text,encoding='utf-8');report['measurements']=json.loads(text)
    report['status']='passed'
except Exception as error:report.update(status='failed',error=str(error))
finally:
    try:
        # Only delete the exact filenames placed in this unique directory; never recurse.
        invoke([*base,'shell','rm','-f',*[remote+'/'+name for name in files]])
        invoke([*base,'shell','rmdir',remote]);report['temporaryFilesRemoved']=True
    except Exception as error:report['cleanupError']=str(error)
    destination=ROOT/'build'/('device-'+a.abi+'.json');destination.write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
if report['status']!='passed':raise SystemExit(report['error'])
print('PASS: native '+model+' '+a.abi+' regressions'+(' and measurements' if a.measure else ''))
print('Report: '+str(destination))
